#include "ClangVisitors_Structure.h"
#include "CodeGenerators/CodeGenerator_Utils.h"
#include "Core/Utils.h"
#include "Database/ReflectionDatabase.h"

//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    // -------------------------------------------------------------------------
    // Binding extraction helpers
    // -------------------------------------------------------------------------

    struct FieldTypeInfo
    {
        void GetFlattenedTemplateArgs(std::string &flattenedArgs) const
        {
            if (!templateArgs.empty())
            {
                for (auto arg : templateArgs)
                {
                    flattenedArgs.append(arg.name);

                    if (!arg.templateArgs.empty())
                    {
                        flattenedArgs.append("<");
                        arg.GetFlattenedTemplateArgs(flattenedArgs);
                        flattenedArgs.append(">");
                    }

                    flattenedArgs.append(", ");
                }

                flattenedArgs = flattenedArgs.substr(0, flattenedArgs.length() - 2);
            }
        }

        bool AllowsTemplateArguments() const
        {
            if (name == "SE::String")
            {
                return false;
            }

            return true;
        }

		FieldTypeInfo() : name(), templateArgs(), isConstantArray(false)
		{

		}

		std::string name;
        std::vector<FieldTypeInfo> templateArgs;
        bool isConstantArray;
    };

    static void GetFieldTypeInfo(ClangParserContext *pContext, TypeData *pType, CXType type, FieldTypeInfo &info)
    {
        clang::QualType const fieldQualType = ClangUtils::GetQualType(type);

        // Get typename
        if (!ClangUtils::GetQualifiedNameForType(fieldQualType, info.name))
        {
			std::string typeSpelling = ClangUtils::GetString(clang_getTypeSpelling(type));
            return pContext->LogError("Failed to qualify typename for member: {0} in class: {1} and of type: {3}", info.name, pType->name, typeSpelling);
        }

        // Is this a constant array
        info.isConstantArray = (type.kind == CXType_ConstantArray);

        // Get info for template types
        if (info.AllowsTemplateArguments())
        {
            auto const numTemplateArguments = clang_Type_getNumTemplateArguments(type);
            if (numTemplateArguments > 0)
            {
                // We only support one template arg for now
                CXType const argType = clang_Type_getTemplateArgumentAsType(type, 0);

                FieldTypeInfo &templateFieldInfo = Utils::Vector::AddOne(info.templateArgs);
                templateFieldInfo.isConstantArray = (argType.kind == CXType_ConstantArray);
                GetFieldTypeInfo(pContext, pType, argType, templateFieldInfo);
            }
        }
    }

    static void GetAllDerivedProperties(ReflectionDatabase const *pDatabase, StringID parentTypeID, std::vector<PropertyData> &results)
    {
        TypeData const *pParentDesc = pDatabase->GetType(parentTypeID);
        if (pParentDesc != nullptr)
        {
            GetAllDerivedProperties(pDatabase, pParentDesc->parentTypeID, results);
            for (auto &parentProperty : pParentDesc->properties)
            {
                results.push_back(parentProperty);
            }
        }
    }

    static std::string GetCursorComment(CXCursor cr)
    {
        std::string result;
        CXString const commentString = clang_Cursor_getBriefCommentText(cr);
        if (commentString.data != nullptr)
        {
            result = clang_getCString(commentString);
            Utils::String::ReplaceAll(result, "\r", " ");
            Utils::String::TrimStart(result);
            Utils::String::TrimEnd(result);
        }
        clang_disposeString(commentString);
        return result;
    }

    static AccessLevel GetAccessLevel(CXCursor cr, AccessLevel defaultAccess = AccessLevel::Public)
    {
        switch (clang_getCXXAccessSpecifier(cr))
        {
        case CX_CXXPrivate:
            return AccessLevel::Private;
        case CX_CXXProtected:
            return AccessLevel::Protected;
        case CX_CXXPublic:
            return AccessLevel::Public;
        case CX_CXXInvalidAccessSpecifier:
        default:
            return defaultAccess;
        }
    }

    static bool HasMacroFlag(const MarkMacro& macro, const Char* flag)
    {
        for (auto const& value : macro.macroContents)
        {
            if (value == flag)
                return true;
        }
        return false;
    }

    static void SplitMacroContent(std::string const& str, std::vector<std::string>& outParts)
    {
        int32 depth = 0;
        bool inQuote = false;
        int32 start = 0;

        for (int32 i = 0; i < str.length(); i++)
        {
            char const c = str[(size_t)i];
            if (c == '"')
            {
                inQuote = !inQuote;
            }
            else if (!inQuote)
            {
                if (c == '(' || c == '[' || c == '{')
                {
                    depth++;
                }
                else if (c == ')' || c == ']' || c == '}')
                {
                    depth--;
                }
                else if (c == ',' && depth == 0)
                {
                    outParts.push_back(str.substr((size_t)start, (size_t)(i - start)));
                    start = i + 1;
                }
            }
        }

        if (start < str.length())
        {
            outParts.push_back(str.substr((size_t)start));
        }
    }

    static bool TryParseStructureMacroFromLine(HeaderInfo const* pHeaderInfo,
                                               uint32 lineNumber,
                                               ReflectionMacroType macroType,
                                               MarkMacro& macro)
    {
        if (pHeaderInfo == nullptr || lineNumber == 0 || lineNumber > pHeaderInfo->fileContents.size())
        {
            return false;
        }

        std::string const& line = pHeaderInfo->fileContents[(size_t)lineNumber - 1];
        char const* macroName = GetMarkMacroText(macroType);
        int32 const macroIdx = Utils::String::Find(line, macroName);
        if (macroIdx == INVALID_INDEX)
        {
            return false;
        }

        int32 const commentIdx = Utils::String::Find(line, "//");
        if (commentIdx != INVALID_INDEX && commentIdx < macroIdx)
        {
            return false;
        }

        int32 const openIdx = Utils::String::Find(line, '(', macroIdx);
        int32 const closeIdx = Utils::String::FindLast(line, ')');
        if (openIdx == INVALID_INDEX || closeIdx == INVALID_INDEX || closeIdx <= openIdx)
        {
            return false;
        }

        macro = MarkMacro();
        macro.headerID = pHeaderInfo->headerId;
        macro.fileLine = lineNumber;
        macro.fileColumn = (uint32)macroIdx + 1;
        macro.type = macroType;

        std::string macroContent = line.substr((size_t)openIdx + 1, (size_t)(closeIdx - openIdx - 1));
        Utils::String::TrimStart(macroContent);
        Utils::String::TrimEnd(macroContent);
        if (!macroContent.empty())
        {
            std::vector<std::string> parts;
            SplitMacroContent(macroContent, parts);
            for (auto& part : parts)
            {
                Utils::String::TrimStart(part);
                Utils::String::TrimEnd(part);
                if (part == "Reflect")
                {
                    macro.hasReflect = true;
                }
                else if (part == "API")
                {
                    macro.hasAPI = true;
                }
                else if (!part.empty())
                {
                    macro.macroContents.push_back(part);
                }
            }
        }

        return true;
    }

    static bool TryGetMacroValue(const MarkMacro& macro, const Char* key, std::string& outValue)
    {
        std::string prefix = Utils::String::Format("{0}=", key);
        for (auto const& value : macro.macroContents)
        {
            if (Utils::String::StartsWith(value, prefix))
            {
                std::string parsed = value.substr(prefix.length());
                Utils::String::TrimStart(parsed);
                Utils::String::TrimEnd(parsed);
                if (Utils::String::StartsWith(parsed, "\"") && Utils::String::EndsWith(parsed, "\"") && parsed.length() >= 2)
                {
                    parsed = parsed.substr(1, parsed.length() - 2);
                }
                outValue = parsed;
                return true;
            }
        }
        return false;
    }

    static void ApplyMemberMacroMetadata(const MarkMacro& macro, std::string& attributes, std::string& tag,
                                         std::string& marshalAs)
    {
        TryGetMacroValue(macro, "Attributes", attributes);
        TryGetMacroValue(macro, "Tag", tag);
        TryGetMacroValue(macro, "MarshalAs", marshalAs);
    }

    static std::string GetParameterDefaultValue(CXCursor argCr)
    {
        CXSourceRange range = clang_getCursorExtent(argCr);
        CXTranslationUnit translationUnit = clang_Cursor_getTranslationUnit(argCr);
        CXToken* tokens = nullptr;
        uint32 numTokens = 0;
        clang_tokenize(translationUnit, range, &tokens, &numTokens);

        std::string result;
        bool foundEquals = false;
        for (uint32 i = 0; i < numTokens; i++)
        {
            std::string token = ClangUtils::GetString(clang_getTokenSpelling(translationUnit, tokens[i]));
            if (token == "=")
            {
                foundEquals = true;
                continue;
            }
            if (!foundEquals)
                continue;
            if (!result.empty())
                result += " ";
            result += token;
        }

        clang_disposeTokens(translationUnit, tokens, numTokens);
        Utils::String::TrimStart(result);
        Utils::String::TrimEnd(result);
        return result;
    }

    static void FillApiParam(CXCursor argCr, ApiParam& param)
    {
        CXType argType = clang_getCursorType(argCr);
        param.name = ClangUtils::GetCursorSpellingAnsi(argCr);
        if (param.name.empty())
        {
            param.name = "arg";
        }
        param.cppType = ClangUtils::GetTypeSpellingAnsi(argType);

        CXType canonical = clang_getCanonicalType(argType);
        param.isPointer = (canonical.kind == CXType_Pointer);
        param.isRef = (canonical.kind == CXType_LValueReference ||
                       canonical.kind == CXType_RValueReference);
        std::string typeSpelling = param.cppType;
        Utils::String::TrimStart(typeSpelling);
        param.isConst = (clang_isConstQualifiedType(canonical) != 0) || Utils::String::StartsWith(typeSpelling, "const ");
        param.isOut = param.isRef && !param.isConst;
        param.defaultValue = GetParameterDefaultValue(argCr);
        param.comment = GetCursorComment(argCr);
    }

    static void VisitConstructor(CXCursor cr, TypeData *pType)
    {
        /*CXType funcType = clang_getCursorType(cr);
        int numArgs = clang_getNumArgTypes(funcType);

        ReflectedConstructor constructor;

        // 3. 遍历所有参数
        for (int i = 0; i < numArgs; i++)
        {
            // 获取第i个参数的类型
            CXType argType = clang_getArgType(funcType, i);

            ReflectedArgument argument;

            // 参数类型名称
            CXString typeSpelling = clang_getTypeSpelling(argType);
            argument.typeName = clang_getCString(typeSpelling);
            clang_disposeString(typeSpelling);

            // 4. 获取参数名（如果有的话）
            argument.name = "unnamed";

            // 注意：CXCursor API 获取参数名比较麻烦，通常需要额外的遍历
            // 这里提供一个获取参数名的示例方法
            clang_visitChildren(cr,
                [](CXCursor cursor, CXCursor parent, CXClientData clientData) {
                    if (clang_getCursorKind(cursor) == CXCursor_ParmDecl) {
                        CXString paramNameStr = clang_getCursorSpelling(cursor);
                        std::string* pResult = static_cast<std::string*>(clientData);
                        *pResult = clang_getCString(paramNameStr);
                        clang_disposeString(paramNameStr);
                        return CXChildVisit_Break; // 找到后停止
                    }
                    return CXChildVisit_Continue;
                },
                &argument.name
            );

            CXType canonicalType = clang_getCanonicalType(argType);
            argument.isConst = clang_isConstQualifiedType(canonicalType);
            argument.isPoint = canonicalType.kind == CXType_Pointer;
            bool isLReference = canonicalType.kind == CXType_LValueReference;
            bool isRReference = canonicalType.kind == CXType_RValueReference;

            std::type_info t1;
            t1.
            constructor.arguments.emplace_back(argument);
        }

        // 是否为默认/拷贝/移动构造函数
        constructor.isDefaultConstructor = clang_CXXConstructor_isDefaultConstructor(cr);
        constructor.isCopyConstructor = clang_CXXConstructor_isCopyConstructor(cr);
        constructor.isMoveConstructor = clang_CXXConstructor_isMoveConstructor(cr);

        pType->m_constructors.emplace_back(constructor);*/
    }

    static bool VisitField(CXCursor cr, ClangParserContext *pContext, TypeData *pClass, uint32_t const declStartPosition, int const lineNumber)
    {
        MarkMacro propertyMarkMacro;
        bool foundPropertyMacro = pContext->FindMarkMacro(pClass->headerID, cr, propertyMarkMacro, ReflectionMacroType::SEProperty);
        if (foundPropertyMacro && propertyMarkMacro.hasReflect)
        {
            pClass->properties.push_back(PropertyData(ClangUtils::GetCursorDisplayName(cr), lineNumber));
            PropertyData &propertyDesc = pClass->properties.back();

            // Try read any user comments for this field
            propertyDesc.description = GetCursorComment(cr);

            // If we dont have an explicit comment for the property, try to get it from the macro declaration
            if (propertyDesc.description.empty())
            {
                propertyDesc.description = propertyMarkMacro.macroComment;
            }

            auto type = clang_getCursorType(cr);
            auto const pFieldQualType = ClangUtils::GetQualType(type);
            auto typeSpelling = ClangUtils::GetString(clang_getTypeSpelling(type));

            // Check if template parameter
            if (pFieldQualType->isTemplateTypeParmType())
            {
                pContext->LogError("Cannot expose template argument member ({0}) in class ({1})!", propertyDesc.name, pClass->name);
                return false;
            }

            // Check if this field is a c-style array
            //-------------------------------------------------------------------------
            if (pFieldQualType->isArrayType())
            {
                if (pFieldQualType->isVariableArrayType() || pFieldQualType->isIncompleteArrayType())
                {
                    pContext->LogError("Variable size array properties are not supported! Please change to List or fixed size!");
                    return false;
                }

                auto const pArrayType = (clang::ConstantArrayType *)pFieldQualType.getTypePtr();
                propertyDesc.flags.SetFlag(TypeProperty::Flags::IsArray);
                propertyDesc.arraySize = (int32_t)pArrayType->getSize().getSExtValue();

                // Set property type to array type
                type = clang_getElementType(type);
            }

            // Get field typename
            //-------------------------------------------------------------------------

            FieldTypeInfo fieldTypeInfo;
            GetFieldTypeInfo(pContext, pClass, type, fieldTypeInfo);
            ENGINE_ASSERT(!fieldTypeInfo.name.empty());
            TypeID fieldTypeID(fieldTypeInfo.name);

            // Additional processing for special types
            //-------------------------------------------------------------------------

            if (Utils::GetCoreTypeID(Utils::TypeIDCore::List) == fieldTypeID)
            {
                // We need to flag this in advance as we are about to change the field type ID
                propertyDesc.flags.SetFlag(TypeProperty::Flags::IsDynamicArray);

                // We need to remove the List type from the property info as we allow for templated types to be contained within arrays
                FieldTypeInfo const &templateTypeInfo = fieldTypeInfo.templateArgs.front();
                fieldTypeInfo = FieldTypeInfo(templateTypeInfo);
                fieldTypeID = TypeID(fieldTypeInfo.name);

                if (fieldTypeInfo.isConstantArray)
                {
                    pContext->LogError("We dont support arrays of arrays. Property: {0} in class: {1}", propertyDesc.name, pClass->name);
                    return false;
                }
            }
            else if (StringID("::SE::String") == fieldTypeID)
            {
                // We need to clear the template args since we have a type alias and clang is detected the template args for eastl::basic_string
                fieldTypeInfo.templateArgs.clear();
            }

            //-------------------------------------------------------------------------
            // Set property typename and validate
            // If it is a templated type, we only support one level of specialization for exposed properties, so flatten the type
            propertyDesc.typeName = fieldTypeInfo.name;
            propertyDesc.typeID = fieldTypeID;
            propertyDesc.metaData = propertyMarkMacro.macroMetadata;

            if (!fieldTypeInfo.templateArgs.empty())
            {
                std::string flattenedArgs;
                fieldTypeInfo.GetFlattenedTemplateArgs(flattenedArgs);
                propertyDesc.templateArgTypeName = flattenedArgs;
            }

            // Check for unsupported types
            //-------------------------------------------------------------------------
            // Core Types
            if (Utils::IsCoreType(propertyDesc.typeID))
            {
                // Check if this field is a generic resource ptr
                /*                    if (propertyDesc.m_typeID == TypeIDCore::ResourcePtr)
                                    {
                                        pContext->LogError("Generic resource pointers are not allowed to be exposed, please use a TResourcePtr instead! ( property: {0} in class: {0} )", propertyDesc.name, pClass->name);
                                        return CXChildVisit_Break;
                                    }

                                    if (propertyDesc.m_typeID == TypeIDCore::TResourcePtr && propertyDesc.m_templateArgTypeName == "SE::Resource::IResource")
                                    {
                                        pContext->LogError("Generic resource pointers ( TResourcePtr<IResource> ) are not allowed to be exposed, please use a specific resource type instead! ( property: {0} in class: {0} )", propertyDesc.name, pClass->name);
                                        return CXChildVisit_Break;
                                    }*/

                // Bit flags
                /*if (propertyDesc.typeID == TypeIDCore::BitFlags)
                {
                    propertyDesc.flags.SetFlag(PropertyInfo::Flags::IsBitFlags);
                }
                else if (propertyDesc.typeID == TypeIDCore::TBitFlags)
                {
                    propertyDesc.flags.SetFlag(PropertyInfo::Flags::IsBitFlags);

                    // Perform validation on the enum type for the bit-flags
                    DataType const *pFlagTypeDesc = pContext->m_pDatabase->GetType(propertyDesc.templateArgTypeName.ToString());
                    if (pFlagTypeDesc == nullptr || !pFlagTypeDesc->IsEnum())
                    {
                        pContext->LogError("Unsupported type encountered: {0} for bitflags property: {1} in class: {2}", propertyDesc.typeName, propertyDesc.name, pClass->name);
                        return CXChildVisit_Break;
                    }
                }*/

                // Arrays
                /*if (propertyDesc.typeID == TypeIDCore::List)
                {
                    pContext->LogError("We dont support arrays of arrays. Property: {0} in class: {1}", propertyDesc.name, pClass->name);
                    return CXChildVisit_Break;
                }*/
            }
            else // Non-Core Types
            {
                // Non-core types must have a valid type descriptor
                TypeData const *pPropertyTypeDesc = pContext->pDatabase->GetType(propertyDesc.typeID);
                if (pPropertyTypeDesc == nullptr)
                {
                    pContext->LogError("Unsupported type encountered: {0} for property: {1} in class: {2}", propertyDesc.typeName, propertyDesc.name, pClass->name);
                    return false;
                }

                // Check for enum types - bitflags are a special case and are not an enum
                if (pPropertyTypeDesc->IsFlag(TypeData::Flags::IsEnum))
                {
                    propertyDesc.flags.SetFlag(TypeProperty::Flags::IsEnum);
                }
                else
                {
                    propertyDesc.flags.SetFlag(TypeProperty::Flags::IsStructure);
                }
            }
        }

        // --- Bindings: SE_PROPERTY(API) extraction ---
        if (pClass->isAPI)
        {
            // First check if the already-found reflection macro also carries API
            if (foundPropertyMacro && propertyMarkMacro.hasAPI)
            {
                CXType fieldType = clang_getCursorType(cr);
                ApiField field;
                field.name       = ClangUtils::GetCursorSpellingAnsi(cr);
                field.cppType    = ClangUtils::GetTypeSpellingAnsi(fieldType);
                field.isReadOnly = HasMacroFlag(propertyMarkMacro, "ReadOnly");
                field.isStatic   = (clang_Cursor_getStorageClass(cr) == CX_SC_Static);
                field.isHidden   = HasMacroFlag(propertyMarkMacro, "Hidden");
                field.isDeprecated = HasMacroFlag(propertyMarkMacro, "Deprecated");
                clang::QualType fieldQualType = ClangUtils::GetQualType(fieldType);
                if (fieldQualType->isConstantArrayType())
                {
                    auto const* pArrayType = (clang::ConstantArrayType*)fieldQualType.getTypePtr();
                    field.arraySize = (int)pArrayType->getSize().getSExtValue();
                }
                field.lineNumber = (int)lineNumber;
                field.comment    = GetCursorComment(cr);
                if (field.comment.empty())
                    field.comment = propertyMarkMacro.macroComment;
                std::string tag;
                ApplyMemberMacroMetadata(propertyMarkMacro, field.attributes, tag, field.marshalAs);
                pClass->bindingInfo.fields.push_back(field);
            }
        }

        MarkMacro eventMarkMacro;
        if (pClass->isAPI && pContext->FindMarkMacro(pClass->headerID, cr, eventMarkMacro, ReflectionMacroType::SEEvent))
        {
            ApiEvent evt;
            evt.name = ClangUtils::GetCursorSpellingAnsi(cr);
            evt.cppType = ClangUtils::GetTypeSpellingAnsi(clang_getCursorType(cr));
            evt.namespaceNameList = pClass->namespaceScopeList;
            evt.isStatic = (clang_Cursor_getStorageClass(cr) == CX_SC_Static);
            evt.access = GetAccessLevel(cr);
            evt.comment = GetCursorComment(cr);
            evt.lineNumber = (int)lineNumber;
            std::string tag, marshalAs;
            ApplyMemberMacroMetadata(eventMarkMacro, evt.attributes, tag, marshalAs);
            pClass->bindingInfo.events.push_back(evt);
        }

        return true;
    }

    static bool VisitMethod(CXCursor cr, ClangParserContext *pContext, TypeData *pClass, uint32_t const declStartPosition, int const lineNumber)
    {
        MarkMacro bindingMacro;
        if (pContext->FindMarkMacro(pClass->headerID, cr, bindingMacro, ReflectionMacroType::SEFunction))
        {
            if (pClass->isAPI)
            {
                ApiFunction fn;
                fn.name       = ClangUtils::GetCursorSpellingAnsi(cr);
                fn.returnType = ClangUtils::GetTypeSpellingAnsi(clang_getResultType(clang_getCursorType(cr)));
                fn.isStatic   = (clang_CXXMethod_isStatic(cr) != 0);
                fn.isVirtual  = (clang_CXXMethod_isVirtual(cr) != 0);
                fn.isConst    = (clang_CXXMethod_isConst(cr) != 0);
                fn.noProxy    = HasMacroFlag(bindingMacro, "NoProxy");
                fn.isHidden   = HasMacroFlag(bindingMacro, "Hidden");
                fn.isSealed   = HasMacroFlag(bindingMacro, "Sealed");
                fn.isDeprecated = HasMacroFlag(bindingMacro, "Deprecated");
                fn.access     = GetAccessLevel(cr);
                fn.comment    = GetCursorComment(cr);
                fn.uniqueName = fn.name;
                fn.lineNumber = (int)lineNumber;
                fn.entryPoint = Utils::String::Format("{0}::{1}::{2}", CodeGeneratorUtils::GetFullCNameSpaceName(pClass->namespaceScopeList),
                    CodeGeneratorUtils::GetFullCNameSpaceName(pClass->structScopeList), fn.name);
                ApplyMemberMacroMetadata(bindingMacro, fn.attributes, fn.tag, fn.marshalAs);

                int numArgs = clang_Cursor_getNumArguments(cr);
                for (int i = 0; i < numArgs; ++i)
                {
                    CXCursor argCr   = clang_Cursor_getArgument(cr, i);

                    ApiParam param;
                    FillApiParam(argCr, param);
                    fn.params.push_back(param);
                }

                pClass->bindingInfo.functions.push_back(fn);
            }
        }

        if (pContext->FindMarkMacro(pClass->headerID, cr, bindingMacro, ReflectionMacroType::SEProperty))
        {
            if (pClass->isAPI)
            {
                CXType fnType  = clang_getCursorType(cr);
                CXType retType = clang_getResultType(fnType);
                std::string retTypeName = ClangUtils::GetTypeSpellingAnsi(retType);
                std::string fnName = ClangUtils::GetCursorSpellingAnsi(cr);

                bool isGetter = (retTypeName != "void") && (clang_Cursor_getNumArguments(cr) == 0);
                bool isSetter = (retTypeName == "void") && (clang_Cursor_getNumArguments(cr) == 1);

                if (isGetter)
                {
                    std::string propName = fnName;
                    if (Utils::String::StartsWith(propName, "Get"))
                        propName = propName.substr(3);

                    ApiProperty* existing = nullptr;
                    for (int i = 0; i < pClass->bindingInfo.bindingProperties.size(); ++i)
                    {
                        if (pClass->bindingInfo.bindingProperties[i].name == propName)
                        {
                            existing = &pClass->bindingInfo.bindingProperties[i];
                            break;
                        }
                    }

                    if (existing)
                    {
                        existing->hasGetter  = true;
                        existing->getterName = fnName;
                        existing->cppType    = retTypeName;
                        existing->getterUniqueName = fnName;
                        existing->getterEntryPoint = Utils::String::Format("{0}_{1}", pClass->name, fnName);
                        existing->getterAccess = GetAccessLevel(cr);
                        if (existing->comment.empty())
                            existing->comment = GetCursorComment(cr);
                    }
                    else
                    {
                        ApiProperty prop;
                        prop.name             = propName;
                        prop.cppType          = retTypeName;
                        prop.getterName       = fnName;
                        prop.getterUniqueName = fnName;
                        prop.getterEntryPoint = Utils::String::Format("{0}_{1}", pClass->name, fnName);
                        prop.hasGetter  = true;
                        prop.hasSetter  = false;
                        prop.getterAccess = GetAccessLevel(cr);
                        prop.setterAccess = AccessLevel::Public;
                        prop.lineNumber = (int)lineNumber;
                        prop.comment = GetCursorComment(cr);
                        std::string tag;
                        ApplyMemberMacroMetadata(bindingMacro, prop.attributes, tag, prop.marshalAs);
                        pClass->bindingInfo.bindingProperties.push_back(prop);
                    }
                }
                else if (isSetter)
                {
                    std::string propName = fnName;
                    if (Utils::String::StartsWith(propName, "Set"))
                        propName = propName.substr(3);

                    ApiProperty* existing = nullptr;
                    for (int i = 0; i < pClass->bindingInfo.bindingProperties.size(); ++i)
                    {
                        if (pClass->bindingInfo.bindingProperties[i].name == propName)
                        {
                            existing = &pClass->bindingInfo.bindingProperties[i];
                            break;
                        }
                    }

                    if (existing)
                    {
                        existing->hasSetter  = true;
                        existing->setterName = fnName;
                        existing->setterUniqueName = fnName;
                        existing->setterEntryPoint = Utils::String::Format("{0}_{1}", pClass->name, fnName);
                        existing->setterAccess = GetAccessLevel(cr);
                        if (existing->comment.empty())
                            existing->comment = GetCursorComment(cr);
                    }
                    else
                    {
                        CXCursor argCr   = clang_Cursor_getArgument(cr, 0);
                        CXType   argType = clang_getCursorType(argCr);

                        ApiProperty prop;
                        prop.name             = propName;
                        prop.cppType          = ClangUtils::GetTypeSpellingAnsi(argType);
                        prop.setterName       = fnName;
                        prop.setterUniqueName = fnName;
                        prop.setterEntryPoint = Utils::String::Format("{0}_{1}", pClass->name, fnName);
                        prop.hasGetter  = false;
                        prop.hasSetter  = true;
                        prop.getterAccess = AccessLevel::Public;
                        prop.setterAccess = GetAccessLevel(cr);
                        prop.lineNumber = (int)lineNumber;
                        prop.comment = GetCursorComment(cr);
                        std::string tag;
                        ApplyMemberMacroMetadata(bindingMacro, prop.attributes, tag, prop.marshalAs);
                        pClass->bindingInfo.bindingProperties.push_back(prop);
                    }
                }
            }
        }

        return true;
    }

    //-------------------------------------------------------------------------

    CXChildVisitResult VisitStructureContents(CXCursor cr, CXCursor parent, CXClientData pClientData)
    {
        ClangParserContext *pContext = static_cast<ClangParserContext *>(pClientData);
        TypeData *pClass = static_cast<TypeData *>(pContext->pParentReflectedType);

        int const lineNumber = ClangUtils::GetLineNumberForCursor(cr);
        uint32 const declStartPosition = ClangUtils::GetStartPositionForCursor(cr);
        CXCursorKind kind = clang_getCursorKind(cr);

        if (kind == CXCursor_CXXBaseSpecifier)
        {
            if (pClass->parentTypeID != StringID::Invalid)
            {
                // 不支持多继承
                // pContext->LogError("Multiple inheritance detected for class: {0}", pClass->name);
                return CXChildVisit_Continue;
            }

            // Get qualified base type
            clang::CXXBaseSpecifier *pBaseSpecifier = (clang::CXXBaseSpecifier *)cr.data[0];
            std::string fullyQualifiedName;
            if (!ClangUtils::GetQualifiedNameForType(pBaseSpecifier->getType(), fullyQualifiedName))
            {
                pContext->LogError("Failed to qualify typename for base class: {0}, base class = {1}", pClass->name, ClangUtils::GetCursorDisplayName(cr));
                return CXChildVisit_Break;
            }

            pClass->parentTypeID = StringID(fullyQualifiedName);
            GetAllDerivedProperties(pContext->pDatabase, pClass->parentTypeID, pClass->properties);

            // Remove duplicate properties added via the parent property traversal - do not change the order of the array
            for (int i = 0; i < pClass->properties.size(); i++)
            {
                for (int j = i + 1; j < pClass->properties.size(); j++)
                {
                    if (pClass->properties[i].propertyID == pClass->properties[j].propertyID)
                    {
                        Utils::Vector::RemoveAt(pClass->properties, (size_t)j);
                        j--;
                    }
                }
            }

            // Populate binding info fields for base class
            if (pClass->isAPI)
            {
                pClass->bindingInfo.baseClassName = fullyQualifiedName;

                // Detect if the base class is a ScriptingObject
                static const char* ScriptingObjectBases[] = {
                    "SE::ScriptingObject",
                    "SE::ManagedScriptingObject",
                    "SE::PersistentScriptingObject",
                    "SE::BinaryAsset",
                    "SE::SceneObject",
                    "SE::Asset",
                    "SE::Script",
                    "SE::Actor",
                };

                std::string baseSimpleName = ClangUtils::GetTypeSpellingAnsi(clang_getCursorType(cr));
                for (const char* name : ScriptingObjectBases)
                {
                    if (baseSimpleName == name)
                    {
                        pClass->bindingInfo.isScriptingObject = true;
                        break;
                    }
                }

                // If base class itself is already known as ScriptingObject, propagate
                if (!pClass->bindingInfo.isScriptingObject)
                {
                    TypeData const* pBaseType = pContext->pDatabase->GetType(pClass->parentTypeID);
                    if (pBaseType && pBaseType->isAPI && pBaseType->bindingInfo.isScriptingObject)
                    {
                        pClass->bindingInfo.isScriptingObject = true;
                    }
                }
            }
        }
        else if (kind == CXCursor_Constructor)
        {
            // VisitConstructor(cr);
        }
        else if (kind == CXCursor_FieldDecl)
        {
            if (!VisitField(cr, pContext, pClass, declStartPosition, lineNumber))
            {
                return CXChildVisit_Break;
            }
        }
        else if (kind == CXCursor_CXXMethod)
        {
            VisitMethod(cr, pContext, pClass, declStartPosition, lineNumber);
        }
        else if (kind == CXCursor_UnionDecl)
        {
            clang_visitChildren(cr, VisitStructureContents, pContext);
        }
        else if (kind == CXCursor_StructDecl)
        {
            clang_visitChildren(cr, VisitStructureContents, pContext);
        }

        return CXChildVisit_Continue;
    }

    CXChildVisitResult VisitTemplateParameters(CXCursor cr, CXCursor parent, CXClientData pClientData)
    {
        auto pParameters = static_cast<std::vector<std::string>*>(pClientData);
        CXCursorKind kind = clang_getCursorKind(cr);
        if (kind == CXCursor_TemplateTypeParameter || kind == CXCursor_NonTypeTemplateParameter || kind == CXCursor_TemplateTemplateParameter)
        {
            pParameters->push_back(ClangUtils::GetCursorSpellingAnsi(cr));
            return CXChildVisit_Continue;
        }
        return CXChildVisit_Break;
    }

    CXChildVisitResult VisitTemplateStructure(ClangParserContext *pContext, CXCursor &cr, std::string_view const &headerFilePath, HeaderID const headerID)
    {
        MarkMacro macro;
        ReflectionMacroType macroType = ReflectionMacroType::SEClass;

        if (pContext->FindMarkMacro(headerID, cr, macro, ReflectionMacroType::SEClass))
        {
            macroType = ReflectionMacroType::SEClass;
        }
        else if (pContext->FindMarkMacro(headerID, cr, macro, ReflectionMacroType::SEStruct))
        {
            macroType = ReflectionMacroType::SEStruct;
        }
        else if (pContext->FindMarkMacro(headerID, cr, macro, ReflectionMacroType::SEInterface))
        {
            macroType = ReflectionMacroType::SEInterface;
        }
        else
        {
            return CXChildVisit_Continue;
        }

        if (!HasMacroFlag(macro, "Template"))
        {
            // pContext->LogError("Cannot register template type ({0}) without Template flag", cursorName);
            return CXChildVisit_Continue;
        }

        auto cursorName = ClangUtils::GetCursorDisplayName(cr);
        size_t index = cursorName.find_first_of("<");
        cursorName = cursorName.substr(0, index);

        std::string fullyQualifiedCursorName;
        /*CXType cursorType = clang_getCursorType(cr);
        if (!ClangUtils::GetQualifiedNameForType(cursorType, fullyQualifiedCursorName))
        {
            pContext->LogError("Failed to get qualified type for cursor: {0}", fullyQualifiedCursorName);
            return CXChildVisit_Break;
        }*/

        if (pContext->GetCurrentNamespace().empty())
        {
            fullyQualifiedCursorName = cursorName;
        }
        else
        {
            fullyQualifiedCursorName = Utils::String::Format("{0}::{1}", pContext->GetCurrentNamespace(), cursorName);
        }

        std::vector<std::string> templateParameters;
        clang_visitChildren(cr, VisitTemplateParameters, &templateParameters);
        if (templateParameters.empty())
        {
            pContext->LogError("Template type ({0}) does not expose any template parameters", cursorName);
            return CXChildVisit_Break;
        }

        TypeData classDescriptor(pContext->GenerateTypeID(fullyQualifiedCursorName), cursorName);
        classDescriptor.headerID = headerID;
        classDescriptor.namespaceScopeList = pContext->GetNamespaces();
        classDescriptor.structScopeList = pContext->GetStructScopes();
        classDescriptor.flags.Flag(TypeData::Flags::IsAbstract, clang_CXXRecord_isAbstract(cr));
        classDescriptor.flags.Flag(TypeData::Flags::IsStruct, macroType == ReflectionMacroType::SEStruct);
        classDescriptor.isReflect = false;
        classDescriptor.isAPI = macro.hasAPI || HasMacroFlag(macro, "Template");
        classDescriptor.bindingInfo.IsAbstract = HasMacroFlag(macro, "Abstract");
        classDescriptor.bindingInfo.isSealed = HasMacroFlag(macro, "Sealed");
        classDescriptor.bindingInfo.isStatic = HasMacroFlag(macro, "Static");
        classDescriptor.bindingInfo.isInterface = macroType == ReflectionMacroType::SEInterface;
        classDescriptor.bindingInfo.isDeprecated = HasMacroFlag(macro, "Deprecated");
        classDescriptor.bindingInfo.comment = GetCursorComment(cr);
        ApplyMemberMacroMetadata(macro, classDescriptor.bindingInfo.attributes,
                                 classDescriptor.bindingInfo.tag,
                                 classDescriptor.bindingInfo.marshalAs);

        void *pPreviousParentReflectedType = pContext->pParentReflectedType;
        pContext->pParentReflectedType = &classDescriptor;
        {
            clang_visitChildren(cr, VisitStructureContents, pContext);
        }
        pContext->pParentReflectedType = pPreviousParentReflectedType;

        pContext->AddTemplateType(classDescriptor, templateParameters);

        if (pContext->HasErrorOccured())
        {
            return CXChildVisit_Break;
        }

        return CXChildVisit_Continue;
    }

    CXChildVisitResult VisitStructure(ClangParserContext *pContext, CXCursor &cr, std::string_view const &headerFilePath, HeaderID const headerID, bool isStruct)
    {
        auto cursorName = ClangUtils::GetCursorDisplayName(cr);

        std::string fullyQualifiedCursorName;
        if (!ClangUtils::GetQualifiedNameForType(clang_getCursorType(cr), fullyQualifiedCursorName))
        {
            pContext->LogError("Failed to get qualified type for cursor: {0}", fullyQualifiedCursorName);
            return CXChildVisit_Break;
        }

        MarkMacro macro;
        //-------------------------------------------------------------------------
        if (pContext->FindReflectionMacroForMeta(headerID, cr, macro))
        {
            TypeID typeID = pContext->GenerateTypeID(fullyQualifiedCursorName);
            TypeData classDescriptor(typeID, cursorName);
            classDescriptor.headerID = headerID;
            classDescriptor.namespaceScopeList = pContext->GetNamespaces();
            classDescriptor.structScopeList = pContext->GetStructScopes();
            classDescriptor.flags.Flag(TypeData::Flags::IsMeta, true);
            pContext->pDatabase->RegisterType(&classDescriptor, false);
        }

        //-------------------------------------------------------------------------
        ReflectionMacroType structureMacroType = ReflectionMacroType::SEClass;
        bool foundStructureMacro = pContext->FindMarkMacro(headerID, cr, macro, ReflectionMacroType::SEClass);
        if (!foundStructureMacro && isStruct)
        {
            foundStructureMacro = pContext->FindMarkMacro(headerID, cr, macro, ReflectionMacroType::SEStruct);
            structureMacroType = ReflectionMacroType::SEStruct;
        }
        if (!foundStructureMacro)
        {
            foundStructureMacro = pContext->FindMarkMacro(headerID, cr, macro, ReflectionMacroType::SEInterface);
            structureMacroType = ReflectionMacroType::SEInterface;
        }

        if (foundStructureMacro)
        {
            ENGINE_ASSERT(macro.IsValid());

            // Modules
            if (macro.IsModuleMacro())
            {
                std::string const moduleName = Utils::String::Format("{0}{1}", pContext->GetCurrentNamespace(), cursorName);

                if (!pContext->SetModuleClassName(headerFilePath, moduleName))
                {
                    // Could not find originating project for detected registered module class
                    pContext->LogError("Cant find the source project for this module class: {0}", headerFilePath);
                    return CXChildVisit_Break;
                }
            }

            //-------------------------------------------------------------------------
            StringID typeID = pContext->GenerateTypeID(fullyQualifiedCursorName);
            TypeData classDescriptor(typeID, cursorName);
            classDescriptor.headerID = headerID;
            classDescriptor.namespaceScopeList = pContext->GetNamespaces();
            classDescriptor.structScopeList = pContext->GetStructScopes();
            classDescriptor.flags.Flag(TypeData::Flags::IsAbstract, clang_CXXRecord_isAbstract(cr));
            classDescriptor.flags.Flag(TypeData::Flags::IsStruct, isStruct);
            classDescriptor.isReflect = macro.hasReflect;


            // Check for SE_CLASS/SE_STRUCT/SE_INTERFACE binding macro with API flag
            if (macro.hasAPI)
            {
                classDescriptor.isAPI = macro.hasAPI;
                std::string assemblyName, assemblyDir;
                pContext->GetAssemblyInfoForHeader(headerID, assemblyName, assemblyDir);

                classDescriptor.bindingInfo.assemblyName = assemblyName;
                classDescriptor.bindingInfo.assemblyDir = assemblyDir;

                classDescriptor.bindingInfo.noSpawn = HasMacroFlag(macro, "NoSpawn");
                classDescriptor.bindingInfo.noConstructor = HasMacroFlag(macro, "NoConstructor");
                classDescriptor.bindingInfo.IsAbstract = HasMacroFlag(macro, "Abstract");
                classDescriptor.bindingInfo.isSealed = HasMacroFlag(macro, "Sealed");
                classDescriptor.bindingInfo.isStatic = HasMacroFlag(macro, "Static");
                classDescriptor.bindingInfo.isInterface = structureMacroType == ReflectionMacroType::SEInterface;
                classDescriptor.bindingInfo.isDeprecated = HasMacroFlag(macro, "Deprecated");
                classDescriptor.bindingInfo.comment = GetCursorComment(cr);
                TryGetMacroValue(macro, "Name", classDescriptor.bindingInfo.name);
                ApplyMemberMacroMetadata(macro, classDescriptor.bindingInfo.attributes,
                                         classDescriptor.bindingInfo.tag,
                                         classDescriptor.bindingInfo.marshalAs);
            }

            // Record current parent type, and update it to the new type
            void *pPreviousParentReflectedType = pContext->pParentReflectedType;
            pContext->pParentReflectedType = &classDescriptor;
            {
                clang_visitChildren(cr, VisitStructureContents, pContext);
            }
            // Reset parent type back to original parent
            pContext->pParentReflectedType = pPreviousParentReflectedType;

            pContext->pDatabase->RegisterType(&classDescriptor, false);

            if (pContext->HasErrorOccured())
            {
                return CXChildVisit_Break;
            }
        }

        /*//-------------------------------------------------------------------------
        // Handle SE_CLASS/SE_STRUCT/SE_INTERFACE without DEFINE_CLASS
        // (API-only types that have no reflection macro)
        if (!pContext->detectDevOnlyTypesAndProperties)
        {
            ReflectionMacro bindingMacro;
            if (pContext->FindBindingMacroForType(headerID, cr, bindingMacro))
            {
                // Only create a new DataType if we didn't already register one above
                // (i.e. no DEFINE_CLASS was found, but a SE_CLASS/SE_STRUCT/SE_INTERFACE with API exists)
                if (!macro.hasReflect)
                {
                    StringID typeID = pContext->GenerateTypeID(fullyQualifiedCursorName);
                    DataType classDescriptor(typeID, cursorName);
                    classDescriptor.headerID = headerID;
                    classDescriptor.namespaceName = pContext->GetCurrentNamespace();
                    classDescriptor.flags.Flag(DataType::Flags::IsAbstract, clang_CXXRecord_isAbstract(cr));
                    classDescriptor.flags.Flag(DataType::Flags::HasBinding, true);
                    classDescriptor.bindingInfo.hasBinding = true;

                    StringAnsi assemblyName, assemblyDir;
                    pContext->GetAssemblyInfoForHeader(headerID, assemblyName, assemblyDir);
                    classDescriptor.bindingInfo.assemblyName = assemblyName;
                    classDescriptor.bindingInfo.assemblyDir = assemblyDir;

                    // Record current parent type, and update it to the new type
                    void *pPreviousParentReflectedType = pContext->pParentReflectedType;
                    pContext->pParentReflectedType = &classDescriptor;
                    {
                        clang_visitChildren(cr, VisitStructureContents, pContext);
                    }
                    pContext->pParentReflectedType = pPreviousParentReflectedType;

                    if (pContext->HasErrorOccured())
                    {
                        return CXChildVisit_Break;
                    }

                    pContext->pDatabase->RegisterType(&classDescriptor, pContext->detectDevOnlyTypesAndProperties);
                }
            }
        }*/

        return CXChildVisit_Continue;
    }


}

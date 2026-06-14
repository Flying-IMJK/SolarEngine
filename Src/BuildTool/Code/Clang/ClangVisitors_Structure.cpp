#include "ClangVisitors_Structure.h"
#include "BuildTool/Code/ReflectorSettingsAndUtils.h"
#include "BuildTool/Code/Database/ReflectionDatabase.h"
#include "Core/TypeSystem/Types.h"

//-------------------------------------------------------------------------

#include <string>

//-------------------------------------------------------------------------

namespace SE::ReflectTool
{
    // -------------------------------------------------------------------------
    // Binding extraction helpers
    // -------------------------------------------------------------------------

    struct FieldTypeInfo
    {
        void GetFlattenedTemplateArgs(String &flattenedArgs) const
        {
            if (!templateArgs.IsEmpty())
            {
                for (auto arg : templateArgs)
                {
                    flattenedArgs.Append(arg.name);

                    if (!arg.templateArgs.IsEmpty())
                    {
                        flattenedArgs.Append(SE_TEXT("<"));
                        arg.GetFlattenedTemplateArgs(flattenedArgs);
                        flattenedArgs.Append(SE_TEXT(">"));
                    }

                    flattenedArgs.Append(SE_TEXT(", "));
                }

                flattenedArgs = flattenedArgs.Substring(0, flattenedArgs.Length() - 2);
            }
        }

        bool AllowsTemplateArguments() const
        {
            if (name == SE_TEXT("SE::String"))
            {
                return false;
            }

            return true;
        }

		FieldTypeInfo() : name(), templateArgs(), isConstantArray(false)
		{

		}

		String name;
        List<FieldTypeInfo> templateArgs;
        bool isConstantArray;
    };

    static void GetFieldTypeInfo(ClangParserContext *pContext, DataType *pType, CXType type, FieldTypeInfo &info)
    {
        clang::QualType const fieldQualType = ClangUtils::GetQualType(type);

        // Get typename
        if (!ClangUtils::GetQualifiedNameForType(fieldQualType, info.name))
        {
			StringView typeSpelling = ClangUtils::GetString(clang_getTypeSpelling(type));
            return pContext->LogError(SE_TEXT("Failed to qualify typename for member: {0} in class: {1} and of type: {3}"), info.name, pType->name, typeSpelling);
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

                FieldTypeInfo &templateFieldInfo = info.templateArgs.AddOne();
                templateFieldInfo.isConstantArray = (argType.kind == CXType_ConstantArray);
                GetFieldTypeInfo(pContext, pType, argType, templateFieldInfo);
            }
        }
    }

    static void GetAllDerivedProperties(ReflectionDatabase const *pDatabase, StringID parentTypeID, List<DataProperty> &results)
    {
        DataType const *pParentDesc = pDatabase->GetType(parentTypeID);
        if (pParentDesc != nullptr)
        {
            GetAllDerivedProperties(pDatabase, pParentDesc->parentTypeID, results);
            for (auto &parentProperty : pParentDesc->properties)
            {
                results.Add(parentProperty);
            }
        }
    }

    static void VisitConstructor(CXCursor cr, DataType *pType)
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

    static bool VisitField(CXCursor cr, ClangParserContext *pContext, DataType *pClass, uint32_t const declStartPosition, int const lineNumber)
    {
        MarkMacro propertyMarkMacro;
        bool foundPropertyMacro = pContext->FindMarkMacro(pClass->headerID, cr, propertyMarkMacro, ReflectionMacroType::SEProperty);
        if (foundPropertyMacro)
        {
            pClass->properties.Add(DataProperty(ClangUtils::GetCursorDisplayName(cr), lineNumber));
            DataProperty &propertyDesc = pClass->properties.Last();

            // Try read any user comments for this field
            CXString const commentString = clang_Cursor_getBriefCommentText(cr);
            if (commentString.data != nullptr)
            {
                propertyDesc.description = clang_getCString(commentString);
                propertyDesc.description.Replace("\r", " ");
                propertyDesc.description.FirstTrim();
                propertyDesc.description.LastTrim();
            }
            clang_disposeString(commentString);

            // If we dont have an explicit comment for the property, try to get it from the macro declaration
            if (propertyDesc.description.IsEmpty())
            {
                propertyDesc.description = propertyMarkMacro.macroComment.ToStringAnsi();
            }

            auto type = clang_getCursorType(cr);
            auto const pFieldQualType = ClangUtils::GetQualType(type);
            auto typeSpelling = ClangUtils::GetString(clang_getTypeSpelling(type));

            // Check if template parameter
            if (pFieldQualType->isTemplateTypeParmType())
            {
                pContext->LogError(SE_TEXT("Cannot expose template argument member ({0}) in class ({1})!"), propertyDesc.name, pClass->name);
                return false;
            }

            // Check if this field is a c-style array
            //-------------------------------------------------------------------------
            if (pFieldQualType->isArrayType())
            {
                if (pFieldQualType->isVariableArrayType() || pFieldQualType->isIncompleteArrayType())
                {
                    pContext->LogError(SE_TEXT("Variable size array properties are not supported! Please change to List or fixed size!"));
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
            ENGINE_ASSERT(!fieldTypeInfo.name.IsEmpty());
            TypeID fieldTypeID(fieldTypeInfo.name);

            // Additional processing for special types
            //-------------------------------------------------------------------------

            if (GetCoreTypeID(TypeIDCore::List) == fieldTypeID)
            {
                // We need to flag this in advance as we are about to change the field type ID
                propertyDesc.flags.SetFlag(TypeProperty::Flags::IsDynamicArray);

                // We need to remove the List type from the property info as we allow for templated types to be contained within arrays
                FieldTypeInfo const &templateTypeInfo = fieldTypeInfo.templateArgs.First();
                fieldTypeInfo = FieldTypeInfo(templateTypeInfo);
                fieldTypeID = TypeID(fieldTypeInfo.name);

                if (fieldTypeInfo.isConstantArray)
                {
                    pContext->LogError(SE_TEXT("We dont support arrays of arrays. Property: {0} in class: {1}"), propertyDesc.name, pClass->name);
                    return false;
                }
            }
            else if (StringID(SE_TEXT("::SE::String")) == fieldTypeID)
            {
                // We need to clear the template args since we have a type alias and clang is detected the template args for eastl::basic_string
                fieldTypeInfo.templateArgs.Clear();
            }

            //-------------------------------------------------------------------------
            // Set property typename and validate
            // If it is a templated type, we only support one level of specialization for exposed properties, so flatten the type
            propertyDesc.typeName = fieldTypeInfo.name.ToStringAnsi();
            propertyDesc.typeID = fieldTypeID;
            propertyDesc.metaData = propertyMarkMacro.macroMetadata.ToStringAnsi();

            if (!fieldTypeInfo.templateArgs.IsEmpty())
            {
                String flattenedArgs;
                fieldTypeInfo.GetFlattenedTemplateArgs(flattenedArgs);
                propertyDesc.templateArgTypeName = flattenedArgs.ToStringAnsi();
            }

            // Check for unsupported types
            //-------------------------------------------------------------------------
            // Core Types
            if (IsCoreType(propertyDesc.typeID))
            {
                // Check if this field is a generic resource ptr
                /*                    if (propertyDesc.m_typeID == TypeIDCore::ResourcePtr)
                                    {
                                        pContext->LogError(SE_TEXT("Generic resource pointers are not allowed to be exposed, please use a TResourcePtr instead! ( property: {0} in class: {0} )"), propertyDesc.name, pClass->name);
                                        return CXChildVisit_Break;
                                    }

                                    if (propertyDesc.m_typeID == TypeIDCore::TResourcePtr && propertyDesc.m_templateArgTypeName == "SE::Resource::IResource")
                                    {
                                        pContext->LogError(SE_TEXT("Generic resource pointers ( TResourcePtr<IResource> ) are not allowed to be exposed, please use a specific resource type instead! ( property: {0} in class: {0} )"), propertyDesc.name, pClass->name);
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
                        pContext->LogError(SE_TEXT("Unsupported type encountered: {0} for bitflags property: {1} in class: {2}"), propertyDesc.typeName, propertyDesc.name, pClass->name);
                        return CXChildVisit_Break;
                    }
                }*/

                // Arrays
                /*if (propertyDesc.typeID == TypeIDCore::List)
                {
                    pContext->LogError(SE_TEXT("We dont support arrays of arrays. Property: {0} in class: {1}"), propertyDesc.name, pClass->name);
                    return CXChildVisit_Break;
                }*/
            }
            else // Non-Core Types
            {
                // Non-core types must have a valid type descriptor
                DataType const *pPropertyTypeDesc = pContext->pDatabase->GetType(propertyDesc.typeID);
                if (pPropertyTypeDesc == nullptr)
                {
                    pContext->LogError(SE_TEXT("Unsupported type encountered: {0} for property: {1} in class: {2}"), propertyDesc.typeName, propertyDesc.name, pClass->name);
                    return false;
                }

                // Check for enum types - bitflags are a special case and are not an enum
                if (pPropertyTypeDesc->IsEnum())
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
                field.isReadOnly = false;
                field.isStatic   = (clang_Cursor_getStorageClass(cr) == CX_SC_Static);
                field.arraySize  = 0;
                field.lineNumber = (int)lineNumber;
                pClass->bindingInfo.fields.Add(field);
            }
        }

        return true;
    }

    static bool VisitMethod(CXCursor cr, ClangParserContext *pContext, DataType *pClass, uint32_t const declStartPosition, int const lineNumber)
    {
        MarkMacro bindingMacro;
        if (pContext->FindMarkMacro(pClass->headerID, cr, bindingMacro, ReflectionMacroType::SEFunction))
        {
            if (pClass->isAPI)
            {
                ApiFunction fn;
                fn.name       = ClangUtils::GetCursorSpellingAnsi(cr);
                CXType fnType = clang_getCursorType(cr);
                CXType retType = clang_getResultType(fnType);
                fn.returnType = ClangUtils::GetTypeSpellingAnsi(retType);
                fn.isStatic   = (clang_CXXMethod_isStatic(cr) != 0);
                fn.isVirtual  = (clang_CXXMethod_isVirtual(cr) != 0);
                fn.isConst    = (clang_CXXMethod_isConst(cr) != 0);
                fn.uniqueName = fn.name;
                fn.lineNumber = (int)lineNumber;
                fn.entryPoint = StringAnsi::Format("{0}_{1}", pClass->name.Get(), fn.name.Get());

                int numArgs = clang_Cursor_getNumArguments(cr);
                for (int i = 0; i < numArgs; ++i)
                {
                    CXCursor argCr   = clang_Cursor_getArgument(cr, i);
                    CXType   argType = clang_getCursorType(argCr);

                    ApiParam param;
                    param.name    = ClangUtils::GetCursorSpellingAnsi(argCr);
                    param.cppType = ClangUtils::GetTypeSpellingAnsi(argType);

                    CXType canonical = clang_getCanonicalType(argType);
                    param.isPointer = (canonical.kind == CXType_Pointer);
                    param.isRef     = (canonical.kind == CXType_LValueReference ||
                                       canonical.kind == CXType_RValueReference);
                    param.isConst   = (clang_isConstQualifiedType(canonical) != 0);
                    param.isOut     = param.isRef && !param.isConst;

                    fn.params.Add(param);
                }

                pClass->bindingInfo.functions.Add(fn);
            }
        }

        /*
        if (pContext->FindBindingMacroForMember(pClass->headerID, declStartPosition, ReflectionMacroType::SEProperty, bindingMacro))
        {
            if (pClass->isAPI)
            {
                CXType fnType  = clang_getCursorType(cr);
                CXType retType = clang_getResultType(fnType);
                StringAnsi retTypeName = ClangUtils::GetTypeSpellingAnsi(retType);
                StringAnsi fnName = ClangUtils::GetCursorSpellingAnsi(cr);

                bool isGetter = (retTypeName != "void") && (clang_Cursor_getNumArguments(cr) == 0);
                bool isSetter = (retTypeName == "void") && (clang_Cursor_getNumArguments(cr) == 1);

                if (isGetter)
                {
                    StringAnsi propName = fnName;
                    if (propName.StartsWith("Get"))
                        propName = propName.Substring(3);

                    ApiProperty* existing = nullptr;
                    for (int i = 0; i < pClass->bindingInfo.bindingProperties.Count(); ++i)
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
                        existing->getterEntryPoint = StringAnsi::Format("{0}_{1}", pClass->name.Get(), fnName.Get());
                    }
                    else
                    {
                        ApiProperty prop;
                        prop.name             = propName;
                        prop.cppType          = retTypeName;
                        prop.getterName       = fnName;
                        prop.getterUniqueName = fnName;
                        prop.getterEntryPoint = StringAnsi::Format("{0}_{1}", pClass->name.Get(), fnName.Get());
                        prop.hasGetter  = true;
                        prop.hasSetter  = false;
                        prop.getterAccess = AccessLevel::Public;
                        prop.setterAccess = AccessLevel::Public;
                        prop.lineNumber = (int)lineNumber;
                        pClass->bindingInfo.bindingProperties.Add(prop);
                    }
                }
                else if (isSetter)
                {
                    StringAnsi propName = fnName;
                    if (propName.StartsWith("Set"))
                        propName = propName.Substring(3);

                    ApiProperty* existing = nullptr;
                    for (int i = 0; i < pClass->bindingInfo.bindingProperties.Count(); ++i)
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
                        existing->setterEntryPoint = StringAnsi::Format("{0}_{1}", pClass->name.Get(), fnName.Get());
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
                        prop.setterEntryPoint = StringAnsi::Format("{0}_{1}", pClass->name.Get(), fnName.Get());
                        prop.hasGetter  = false;
                        prop.hasSetter  = true;
                        prop.getterAccess = AccessLevel::Public;
                        prop.setterAccess = AccessLevel::Public;
                        prop.lineNumber = (int)lineNumber;
                        pClass->bindingInfo.bindingProperties.Add(prop);
                    }
                }
            }
        }
        */

        return true;
    }

    //-------------------------------------------------------------------------

    CXChildVisitResult VisitStructureContents(CXCursor cr, CXCursor parent, CXClientData pClientData)
    {
        ClangParserContext *pContext = static_cast<ClangParserContext *>(pClientData);
        DataType *pClass = static_cast<DataType *>(pContext->pParentReflectedType);

        int const lineNumber = ClangUtils::GetLineNumberForCursor(cr);
        uint32 const declStartPosition = ClangUtils::GetStartPositionForCursor(cr);
        CXCursorKind kind = clang_getCursorKind(cr);

        if (kind == CXCursor_CXXBaseSpecifier)
        {
            if (pClass->parentTypeID != StringID::Invalid)
            {
                // 不支持多继承
                // pContext->LogError(SE_TEXT("Multiple inheritance detected for class: {0}"), pClass->name);
                return CXChildVisit_Continue;
            }

            // Get qualified base type
            clang::CXXBaseSpecifier *pBaseSpecifier = (clang::CXXBaseSpecifier *)cr.data[0];
            String fullyQualifiedName;
            if (!ClangUtils::GetQualifiedNameForType(pBaseSpecifier->getType(), fullyQualifiedName))
            {
                pContext->LogError(SE_TEXT("Failed to qualify typename for base class: {0}, base class = {1}"), pClass->name, ClangUtils::GetCursorDisplayName(cr));
                return CXChildVisit_Break;
            }

            pClass->parentTypeID = StringID(fullyQualifiedName);
            GetAllDerivedProperties(pContext->pDatabase, pClass->parentTypeID, pClass->properties);

            // Remove duplicate properties added via the parent property traversal - do not change the order of the array
            for (int i = 0; i < pClass->properties.Count(); i++)
            {
                for (int j = i + 1; j < pClass->properties.Count(); j++)
                {
                    if (pClass->properties[i].propertyID == pClass->properties[j].propertyID)
                    {
                        pClass->properties.RemoveAt(j);
                        j--;
                    }
                }
            }

            // Populate binding info fields for base class
            if (pClass->isAPI)
            {
                pClass->bindingInfo.baseClassName = fullyQualifiedName.ToStringAnsi();

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

                StringAnsi baseSimpleName = ClangUtils::GetTypeSpellingAnsi(clang_getCursorType(cr));
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
                    DataType const* pBaseType = pContext->pDatabase->GetType(pClass->parentTypeID);
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

        return CXChildVisit_Continue;
    }

    CXChildVisitResult VisitResourceStructureContents(CXCursor cr, CXCursor parent, CXClientData pClientData)
    {
        auto pContext = static_cast<ClangParserContext *>(pClientData);
        auto pResource = static_cast<ReflectedResourceType *>(pContext->pParentReflectedType);

        CXCursorKind kind = clang_getCursorKind(cr);
        switch (kind)
        {
        case CXCursor_CXXBaseSpecifier:
        {
            clang::CXXBaseSpecifier *pBaseSpecifier = (clang::CXXBaseSpecifier *)cr.data[0];
            if (!ClangUtils::GetAllBaseClasses(pResource->parents, *pBaseSpecifier))
            {
                pContext->LogError(SE_TEXT("Failed to get all base classes type for resource type: {0}"), pResource->className);
                return CXChildVisit_Break;
            }
        }
        break;

        default:
            break;
        }

        return CXChildVisit_Continue;
    }

    CXChildVisitResult VisitStructure(ClangParserContext *pContext, CXCursor &cr, StringView const &headerFilePath, HeaderID const headerID, bool isStruct)
    {
        auto cursorName = ClangUtils::GetCursorDisplayName(cr);

        String fullyQualifiedCursorName;
        if (!ClangUtils::GetQualifiedNameForType(clang_getCursorType(cr), fullyQualifiedCursorName))
        {
            pContext->LogError(SE_TEXT("Failed to get qualified type for cursor: {0}"), fullyQualifiedCursorName);
            return CXChildVisit_Break;
        }

        MarkMacro macro;
        //-------------------------------------------------------------------------
        if (pContext->FindReflectionMacroForMeta(headerID, cr, macro))
        {
            TypeID typeID = pContext->GenerateTypeID(fullyQualifiedCursorName);
            DataType classDescriptor(typeID, cursorName);
            classDescriptor.headerID = headerID;
            classDescriptor.namespaceName = pContext->GetCurrentNamespace().ToStringAnsi();
            classDescriptor.flags.Flag(DataType::Flags::IsMeta, true);
            pContext->pDatabase->RegisterType(&classDescriptor, false);
        }

        //-------------------------------------------------------------------------
        if (pContext->FindMarkMacro(headerID, cr, macro, ReflectionMacroType::SEClass))
        {
            if (!pContext->IsInEngineNamespace())
            {
                pContext->LogError(SE_TEXT("You cannot register types for reflection that are outside the engine namespace ({0}). Type: {1}, File: {2}"), Settings::g_engineNamespace, fullyQualifiedCursorName, headerFilePath);
                return CXChildVisit_Break;
            }

            ENGINE_ASSERT(macro.IsValid());

            // Modules
            if (macro.IsModuleMacro())
            {
                String const moduleName = String::Format(SE_TEXT("{0}{1}"), pContext->GetCurrentNamespace(), cursorName);

                if (!pContext->SetModuleClassName(headerFilePath, moduleName))
                {
                    // Could not find originating project for detected registered module class
                    pContext->LogError(SE_TEXT("Cant find the source project for this module class: {0}"), headerFilePath);
                    return CXChildVisit_Break;
                }
            }

            //-------------------------------------------------------------------------
            StringID typeID = pContext->GenerateTypeID(fullyQualifiedCursorName);
            DataType classDescriptor(typeID, cursorName);
            classDescriptor.headerID = headerID;
            classDescriptor.namespaceName = pContext->GetCurrentNamespace().ToStringAnsi();
            classDescriptor.flags.Flag(DataType::Flags::IsAbstract, clang_CXXRecord_isAbstract(cr));
            classDescriptor.flags.Flag(DataType::Flags::IsStruct, isStruct);
            classDescriptor.isReflect = macro.hasReflect;

            if (classDescriptor.namespaceName.Contains("::"))
            {
                classDescriptor.isReflect = macro.hasReflect;
            }

            // Check for SE_CLASS/SE_STRUCT/SE_INTERFACE binding macro with API flag
            if (macro.hasAPI)
            {
                classDescriptor.isAPI = macro.hasAPI;
                StringAnsi assemblyName, assemblyDir;
                pContext->GetAssemblyInfoForHeader(headerID, assemblyName, assemblyDir);

                classDescriptor.bindingInfo.assemblyName = assemblyName;
                classDescriptor.bindingInfo.assemblyDir = assemblyDir;

                classDescriptor.bindingInfo.noSpawn = macro.macroContents.Contains(SE_TEXT("NoSpawn"));
                classDescriptor.bindingInfo.IsAbstract = macro.macroContents.Contains(SE_TEXT("Abstract"));
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
                    DataType classDescriptor(typeID, cursorName.ToStringAnsi());
                    classDescriptor.headerID = headerID;
                    classDescriptor.namespaceName = pContext->GetCurrentNamespace().ToStringAnsi();
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
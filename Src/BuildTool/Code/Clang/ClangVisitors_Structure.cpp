#include "ClangVisitors_Structure.h"
#include "ClangTemplateTypes.h"
#include "CodeGenerators/CodeGenerator_BindingsTypeMap.h"
#include "CodeGenerators/CodeGenerator_Utils.h"
#include "Core/Utils.h"
#include "Database/TypeDatabase.h"

#include <memory>
#include <utility>

//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    // -------------------------------------------------------------------------
    // Binding extraction helpers
    // -------------------------------------------------------------------------

    struct FieldTypeInfo
    {
        std::string ToCppTypeString() const
        {
            std::string result = name;
            if (!templateArgs.empty())
            {
                result.append("<");
                for (int i = 0; i < templateArgs.size(); i++)
                {
                    if (i > 0)
                        result.append(", ");
                    result.append(templateArgs[i].ToCppTypeString());
                }
                result.append(">");
            }
            return result;
        }

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

    struct ReflectedFieldTypeInfo
    {
        FieldTypeInfo typeInfo;
        TypeID typeID;
        bool isFixedArray = false;
        bool isDynamicArray = false;
        int32 arraySize = 0;
        std::string bindingCppType;
    };

    static void GetFieldTypeInfo(ClangParserContext *pContext, TypeInfoBase *pType, CXType type, FieldTypeInfo &info)
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

    static bool ResolveFieldType(ClangParserContext *pContext, TypeInfoBase *pClass, CXCursor cr,
                                          const std::string& propertyName, ReflectedFieldTypeInfo& outInfo)
    {
        CXType type = clang_getCursorType(cr);
        std::string typeTest = ClangUtils::GetTypeSpellingAnsi(type);
        clang::QualType const fieldQualType = ClangUtils::GetQualType(type);

        if (fieldQualType->isTemplateTypeParmType())
        {
            pContext->LogError("Cannot expose template argument member ({0}) in class ({1})!", propertyName, pClass->name);
            return false;
        }

        if (fieldQualType->isArrayType())
        {
            if (fieldQualType->isVariableArrayType() || fieldQualType->isIncompleteArrayType())
            {
                pContext->LogError("Variable size array properties are not supported! Please change to List or fixed size!");
                return false;
            }

            auto const pArrayType = (clang::ConstantArrayType*)fieldQualType.getTypePtr();
            outInfo.isFixedArray = true;
            outInfo.arraySize = (int32)pArrayType->getSize().getSExtValue();
            type = clang_getElementType(type);
        }

        FieldTypeInfo fieldTypeInfo;
        GetFieldTypeInfo(pContext, pClass, type, fieldTypeInfo);
        ENGINE_ASSERT(!fieldTypeInfo.name.empty());

        outInfo.bindingCppType = fieldTypeInfo.ToCppTypeString();
        TypeID fieldTypeID(fieldTypeInfo.name);

        if (Utils::GetCoreTypeID(Utils::TypeIDCore::List) == fieldTypeID)
        {
            outInfo.isDynamicArray = true;

            if (fieldTypeInfo.templateArgs.empty())
            {
                pContext->LogError("List property ({0}) in class ({1}) is missing an element type", propertyName, pClass->name);
                return false;
            }

            FieldTypeInfo const& templateTypeInfo = fieldTypeInfo.templateArgs.front();
            fieldTypeInfo = FieldTypeInfo(templateTypeInfo);
            fieldTypeID = TypeID(fieldTypeInfo.name);

            if (fieldTypeInfo.isConstantArray)
            {
                pContext->LogError("We dont support arrays of arrays. Property: {0} in class: {1}", propertyName, pClass->name);
                return false;
            }
        }
        else if (StringID("SE::String") == fieldTypeID)
        {
            // We need to clear the template args since we have a type alias and clang is detected the template args for eastl::basic_string
            fieldTypeInfo.templateArgs.clear();
            outInfo.bindingCppType = fieldTypeInfo.ToCppTypeString();
        }

        outInfo.typeInfo = fieldTypeInfo;
        outInfo.typeID = fieldTypeID;
        return true;
    }

    static void GetAllDerivedProperties(TypeDatabase const *pDatabase, StringID parentTypeID, std::vector<PropertyData> &results)
    {
        //TypeInfoBase const *pParentDesc = pDatabase->GetType(parentTypeID);
        //if (pParentDesc != nullptr)
        //{
        //    GetAllDerivedProperties(pDatabase, pParentDesc->parentTypeID, results);
        //    for (auto &parentProperty : pParentDesc->properties)
        //    {
        //        results.push_back(parentProperty);
        //    }
        //}
    }


    static void ApplyStructOptions(const MarkMacro& macro, TypeInfoStruct& type)
    {
        type.APIIsInterface   = macro.type == MacroTypeEnum::SEInterface;
        if (!macro.HasApi())
        {
            return;
        }

        MarkAPI const& api = macro.GetApi();
        type.APINoSpawn       = api.IsNoSpawn;
        type.APINoConstructor = api.IsNoConstructor;
        type.APIIsAbstract    = api.IsAbstract;
        type.APIIsSealed      = api.IsSealed;
        type.APIIsStatic      = api.IsStatic;
        type.APIIsNativeInvokeUseName = api.IsNativeInvokeUseName;
        type.APIName          = api.name;
        type.APIAttributes    = api.attributes;
        type.APIMarshalAs     = api.marshalAs;
        type.APIInBuildMapType = api.inBuildMapType;
    }

    static void ApplyStructOptions(const MarkMacro& macro, TypeInfoStructTemplate& type)
    {
        type.APIIsInterface   = macro.type == MacroTypeEnum::SEInterface;
        if (!macro.HasApi())
        {
            return;
        }

        MarkAPI const& api = macro.GetApi();
        type.APINoSpawn       = api.IsNoSpawn;
        type.APINoConstructor = api.IsNoConstructor;
        type.APIIsAbstract    = api.IsAbstract;
        type.APIIsSealed      = api.IsSealed;
        type.APIIsStatic      = api.IsStatic;
        type.APIIsNativeInvokeUseName = api.IsNativeInvokeUseName;
        type.APIName          = api.name;
        type.APIAttributes    = api.attributes;
        type.APIMarshalAs     = api.marshalAs;
    }

    static void ApplyFieldOptions(const MarkMacro& macro, TypeInfoField& field)
    {
        if (!macro.HasApi())
        {
            return;
        }

        MarkAPI const& api = macro.GetApi();
        field.APIIsReadOnly = api.IsReadOnly;
        field.attributes     = api.attributes;
        field.marshalAs      = api.marshalAs;
    }

    static void ApplyFieldOptions(const MarkMacro& macro, TypeInfoFieldTemplate& field)
    {
        if (!macro.HasApi())
        {
            return;
        }

        MarkAPI const& api = macro.GetApi();
        field.APIIsReadOnly = api.IsReadOnly;
        field.attributes     = api.attributes;
        field.marshalAs      = api.marshalAs;
    }

    static void ApplyEventOptions(const MarkMacro& macro, TypeInfoEvent& evt)
    {
        if (!macro.HasApi())
        {
            return;
        }

        evt.attributes = macro.GetApi().attributes;
    }

    static void ApplyEventOptions(const MarkMacro& macro, TypeInfoEventTemplate& evt)
    {
        if (!macro.HasApi())
        {
            return;
        }

        evt.attributes = macro.GetApi().attributes;
    }

    static void FillTypeInfoParam(CXCursor argCr, TypeInfoParam& param)
    {
        CXType argType = clang_getCursorType(argCr);
        param.name = ClangUtils::GetCursorSpellingAnsi(argCr);
        if (param.name.empty())
        {
            param.name = "arg";
        }
        param.type = TypeID(ClangUtils::GetTypeSpellingAnsi(argType));

        CXType canonical = clang_getCanonicalType(argType);
        param.isPointer = (canonical.kind == CXType_Pointer);
        param.isRef = (canonical.kind == CXType_LValueReference ||
                       canonical.kind == CXType_RValueReference);
        // C++ T& is an input/output reference by default. Treating every
        // non-const reference as C# out loses its input value. Explicit out
        // semantics require a dedicated API annotation; until then ref is the
        // only lossless representation.
        param.isOut   = false;
        param.isConst = clang_isConstQualifiedType(canonical) != 0;
        param.defaultValue = ClangUtils::GetParameterDefaultValue(argCr);
        param.comment      = ClangUtils::GetCursorComment(argCr);
    }

    static void FillTypeInfoParamTemplate(CXCursor argCr,
                                          TypeInfoParamTemplate& param,
                                          std::vector<std::string> const& templateParameters)
    {
        CXType argType = clang_getCursorType(argCr);
        param.name = ClangUtils::GetCursorSpellingAnsi(argCr);
        if (param.name.empty())
        {
            param.name = "arg";
        }
        param.type = ParseTemplateTypeRef(nullptr, argType, templateParameters);
        param.isOut = false;
        param.defaultValue = ClangUtils::GetParameterDefaultValue(argCr);
        param.comment = ClangUtils::GetCursorComment(argCr);
    }

    static void VisitConstructor(CXCursor cr, TypeInfoStruct* pType)
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

    static bool VisitField(CXCursor            cr,
                           ClangParserContext* pContext,
                           TypeInfoStruct*     pClass,
                           uint32_t const      declStartPosition,
                           int const           lineNumber)
    {
        MarkMacro filedMacro;
        if (pContext->FindMarkMacro(pClass->headerID, cr, filedMacro, MacroTypeEnum::SEField))
        {
            TypeInfoField field;
            field.name = ClangUtils::GetCursorSpellingAnsi(cr);
            ReflectedFieldTypeInfo resolvedFieldType;
            if (!ResolveFieldType(pContext, pClass, cr, field.name, resolvedFieldType))
            {
                return false;
            }

            field.isReflect = filedMacro.hasReflect;
            field.isAPI = filedMacro.HasApi();

            field.type    = TypeID(resolvedFieldType.bindingCppType);
            field.isStatic   = ClangUtils::IsStatic(cr);
            field.arraySize  = resolvedFieldType.isFixedArray ? resolvedFieldType.arraySize : 0;
            field.lineNumber = (int)lineNumber;
            field.comment    = ClangUtils::GetCursorComment(cr);
            if (field.comment.empty())
            {
                field.comment = filedMacro.macroComment;
            }
            ApplyFieldOptions(filedMacro, field);
            pClass->fields.push_back(field);


            //pClass->properties.push_back(PropertyData(ClangUtils::GetCursorDisplayName(cr), lineNumber));
            //PropertyData &propertyDesc = pClass->properties.back();

            //// Try read any user comments for this field
            //propertyDesc.description = ClangUtils::GetCursorComment(cr);

            //// If we dont have an explicit comment for the property, try to get it from the macro declaration
            //if (propertyDesc.description.empty())
            //{
            //    propertyDesc.description = propertyMarkMacro.macroComment;
            //}

            //ReflectedFieldTypeInfo resolvedFieldType;
            //if (!ResolveFieldType(pContext, pClass, cr, propertyDesc.name, resolvedFieldType))
            //{
            //    return false;
            //}

            //if (resolvedFieldType.isFixedArray)
            //{
            //    propertyDesc.flags.SetFlag(PropertyFlags::IsArray);
            //    propertyDesc.arraySize = resolvedFieldType.arraySize;
            //}
            //if (resolvedFieldType.isDynamicArray)
            //{
            //    propertyDesc.flags.SetFlag(PropertyFlags::IsDynamicArray);
            //}

            ////-------------------------------------------------------------------------
            //// Set property typename and validate
            //// If it is a templated type, we only support one level of specialization for exposed properties, so flatten the type
            //propertyDesc.typeName = resolvedFieldType.typeInfo.name;
            //propertyDesc.typeID = resolvedFieldType.typeID;
            //if (!resolvedFieldType.typeInfo.templateArgs.empty())
            //{
            //    std::string flattenedArgs;
            //    resolvedFieldType.typeInfo.GetFlattenedTemplateArgs(flattenedArgs);
            //    propertyDesc.templateArgTypeName = flattenedArgs;
            //}

            //// Check for unsupported types
            ////-------------------------------------------------------------------------
            //// Core Types
            //if (Utils::IsCoreType(propertyDesc.typeID))
            //{
            //    // Check if this field is a generic resource ptr
            //    /*                    if (propertyDesc.m_typeID == TypeIDCore::ResourcePtr)
            //                        {
            //                            pContext->LogError("Generic resource pointers are not allowed to be exposed, please use a TResourcePtr instead! ( property: {0} in class: {0} )", propertyDesc.name, pClass->name);
            //                            return CXChildVisit_Break;
            //                        }

            //                        if (propertyDesc.m_typeID == TypeIDCore::TResourcePtr && propertyDesc.m_templateArgTypeName == "SE::Resource::IResource")
            //                        {
            //                            pContext->LogError("Generic resource pointers ( TResourcePtr<IResource> ) are not allowed to be exposed, please use a specific resource type instead! ( property: {0} in class: {0} )", propertyDesc.name, pClass->name);
            //                            return CXChildVisit_Break;
            //                        }*/

            //    // Bit flags
            //    /*if (propertyDesc.typeID == TypeIDCore::BitFlags)
            //    {
            //        propertyDesc.flags.SetFlag(PropertyInfo::Flags::IsBitFlags);
            //    }
            //    else if (propertyDesc.typeID == TypeIDCore::TBitFlags)
            //    {
            //        propertyDesc.flags.SetFlag(PropertyInfo::Flags::IsBitFlags);

            //        // Perform validation on the enum type for the bit-flags
            //        DataType const *pFlagTypeDesc = pContext->m_pDatabase->GetType(propertyDesc.templateArgTypeName.ToString());
            //        if (pFlagTypeDesc == nullptr || !pFlagTypeDesc->IsEnum())
            //        {
            //            pContext->LogError("Unsupported type encountered: {0} for bitflags property: {1} in class: {2}", propertyDesc.typeName, propertyDesc.name, pClass->name);
            //            return CXChildVisit_Break;
            //        }
            //    }*/

            //    // Arrays
            //    /*if (propertyDesc.typeID == TypeIDCore::List)
            //    {
            //        pContext->LogError("We dont support arrays of arrays. Property: {0} in class: {1}", propertyDesc.name, pClass->name);
            //        return CXChildVisit_Break;
            //    }*/
            //}
            //else // Non-Core Types
            //{
            //    // Non-core types must have a valid type descriptor
            //    TypeInfoBase const *pPropertyTypeDesc = pContext->pDatabase->GetType(propertyDesc.typeID);
            //    if (pPropertyTypeDesc == nullptr)
            //    {
            //        pContext->LogError("Unsupported type encountered: {0} for property: {1} in class: {2}", propertyDesc.typeName, propertyDesc.name, pClass->name);
            //        return false;
            //    }

            //    // Check for enum types - bitflags are a special case and are not an enum
            //    if (pPropertyTypeDesc->IsFlag(TypeInfoBase::Flags::IsEnum))
            //    {
            //        propertyDesc.flags.SetFlag(PropertyFlags::IsEnum);
            //    }
            //    else
            //    {
            //        propertyDesc.flags.SetFlag(PropertyFlags::IsStructure);
            //    }
            //}

        }

        MarkMacro eventMacro;
        if (pContext->FindMarkMacro(pClass->headerID, cr, eventMacro, MacroTypeEnum::SEEvent))
        {
            TypeInfoEvent evt;
            CXType        eventType = clang_getCursorType(cr);

            evt.isReflect = eventMacro.hasReflect;
            evt.isAPI   = eventMacro.HasApi();

            evt.name                = ClangUtils::GetCursorSpellingAnsi(cr);
            evt.cppType             = TypeID(ClangUtils::GetTypeSpellingAnsi(eventType));
            evt.isStatic            = ClangUtils::IsStatic(cr);
            evt.access              = ClangUtils::GetAccessLevel(cr);
            evt.comment             = ClangUtils::GetCursorComment(cr);
            evt.lineNumber          = (int)lineNumber;

            // SE_EVENT marks a Delegate<...> field. Unlike a function cursor,
            // the Delegate arguments do not have declaration cursors, so unpack
            // the template argument types and synthesize stable argument names.
            const int numTemplateArguments = clang_Type_getNumTemplateArguments(eventType);
            for (int i = 0; i < numTemplateArguments; i++)
            {
                CXType argumentType = clang_Type_getTemplateArgumentAsType(eventType, i);
                if (argumentType.kind == CXType_Invalid)
                    continue;

                TypeInfoParam param;
                param.name = Utils::String::Format("arg{0}", i);
                param.type = TypeID(ClangUtils::GetTypeSpellingAnsi(argumentType));

                CXType canonical = clang_getCanonicalType(argumentType);
                param.isPointer  = canonical.kind == CXType_Pointer;
                param.isRef      = canonical.kind == CXType_LValueReference || canonical.kind == CXType_RValueReference;
                param.isConst    = clang_isConstQualifiedType(canonical) != 0;
                param.isOut      = false;
                evt.params.push_back(param);
            }

            ApplyEventOptions(eventMacro, evt);
            pClass->events.push_back(evt);
        }

        return true;
    }

    static bool VisitMethod(CXCursor cr, ClangParserContext *pContext, TypeInfoStruct *pClass, uint32_t const declStartPosition, int const lineNumber)
    {
        MarkMacro macro;
        if (pContext->FindMarkMacro(pClass->headerID, cr, macro, MacroTypeEnum::SEFunction))
        {
            TypeInfoFunc func;

            CXType cursorType = clang_getCursorType(cr);

            func.isReflect = macro.hasReflect;
            func.isAPI = macro.HasApi();

            func.name       = ClangUtils::GetCursorSpellingAnsi(cr);
            func.returnType = TypeID(ClangUtils::GetTypeSpellingAnsi(clang_getResultType(cursorType)));
            func.isStatic   = (clang_CXXMethod_isStatic(cr) != 0);
            func.isVirtual  = (clang_CXXMethod_isVirtual(cr) != 0);
            func.isConst    = (clang_CXXMethod_isConst(cr) != 0);
            func.access     = ClangUtils::GetAccessLevel(cr);
            func.comment    = ClangUtils::GetCursorComment(cr);
            func.uniqueName = func.name;
            func.lineNumber = (int)lineNumber;

            if (macro.HasApi())
            {
                MarkAPI const& api = macro.GetApi();
                func.APINoProxy     = api.IsNoProxy;
                func.APIIsSealed    = api.IsSealed;
                func.APIIsStatic    = api.IsStatic;
                func.APIIsPropertie = api.IsProperty;

                func.attributes = api.attributes;
                func.marshalAs  = api.marshalAs;
            }

            int numArgs = clang_Cursor_getNumArguments(cr);
            for (int i = 0; i < numArgs; ++i)
            {
                CXCursor argCr = clang_Cursor_getArgument(cr, i);

                TypeInfoParam param;
                FillTypeInfoParam(argCr, param);
                func.params.push_back(param);
            }

            pClass->functions.emplace_back(func);
        }

        return true;
    }

    //-------------------------------------------------------------------------

    CXChildVisitResult VisitStructureContents(CXCursor cr, CXCursor parent, CXClientData pClientData)
    {
        ClangParserContext *pContext = static_cast<ClangParserContext *>(pClientData);
        TypeInfoStruct*     pClass   = static_cast<TypeInfoStruct*>(pContext->pParentReflectedType);

        int const lineNumber = ClangUtils::GetLineNumberForCursor(cr);
        uint32 const declStartPosition = ClangUtils::GetStartPositionForCursor(cr);
        CXCursorKind kind              = clang_getCursorKind(cr);

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

            // Detect if the base class is a ScriptingObject
            static const char* ScriptingObjectBases[] = {
                "SE::ScriptingObject",
                "SE::ManagedScriptingObject",
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
                    pClass->isScriptingObject = true;
                    break;
                }
            }

            // If base class itself is already known as ScriptingObject, propagate
            if (!pClass->isScriptingObject)
            {
                TypeInfoBase const* pBaseType = pContext->pDatabase->GetType(pClass->parentTypeID);
                if (pBaseType && pBaseType->isAPI && pBaseType->IsFlag(TypeInfoBase::Flag::IsStruct))
                {
                    auto const* pBaseStruct = static_cast<TypeInfoStruct const*>(pBaseType);
                    if (pBaseStruct->isScriptingObject)
                    {
                        pClass->isScriptingObject = true;
                    }
                }
            }

            // Populate binding info fields for base class
            if (pClass->isAPI)
            {
                pClass->baseClassName = fullyQualifiedName;
            }
        }
        else if (kind == CXCursor_Constructor)
        {
            // VisitConstructor(cr);
        }
        else if (kind == CXCursor_FieldDecl) // 实例字段
        {
            if (!VisitField(cr, pContext, pClass, declStartPosition, lineNumber))
            {
                return CXChildVisit_Break;
            }
        }
        else if (kind == CXCursor_VarDecl) // 静态变量
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

    static bool VisitTemplateField(CXCursor            cr,
                                   ClangParserContext* pContext,
                                   TypeInfoStructTemplate* pClass,
                                   uint32_t const      declStartPosition,
                                   int const           lineNumber)
    {
        MarkMacro filedMacro;
        if (pContext->FindMarkMacro(pClass->headerID, cr, filedMacro, MacroTypeEnum::SEField))
        {
            TypeInfoFieldTemplate field;
            field.name = ClangUtils::GetCursorSpellingAnsi(cr);
            field.type = ParseTemplateTypeRef(pContext, clang_getCursorType(cr), pClass->templateParameters);
            field.isReflect = filedMacro.hasReflect;
            field.isAPI = filedMacro.HasApi();
            field.isStatic = ClangUtils::IsStatic(cr);
            field.lineNumber = (int)lineNumber;
            field.comment = ClangUtils::GetCursorComment(cr);
            if (field.comment.empty())
            {
                field.comment = filedMacro.macroComment;
            }
            ApplyFieldOptions(filedMacro, field);
            pClass->fields.push_back(field);
        }

        MarkMacro eventMacro;
        if (pContext->FindMarkMacro(pClass->headerID, cr, eventMacro, MacroTypeEnum::SEEvent))
        {
            TypeInfoEventTemplate evt;
            CXType eventType = clang_getCursorType(cr);

            evt.isReflect = eventMacro.hasReflect;
            evt.isAPI = eventMacro.HasApi();
            evt.name = ClangUtils::GetCursorSpellingAnsi(cr);
            evt.cppType = ParseTemplateTypeRef(pContext, eventType, pClass->templateParameters);
            evt.isStatic = ClangUtils::IsStatic(cr);
            evt.access = ClangUtils::GetAccessLevel(cr);
            evt.comment = ClangUtils::GetCursorComment(cr);
            evt.lineNumber = (int)lineNumber;

            const int numTemplateArguments = clang_Type_getNumTemplateArguments(eventType);
            for (int i = 0; i < numTemplateArguments; i++)
            {
                CXType argumentType = clang_Type_getTemplateArgumentAsType(eventType, i);
                if (argumentType.kind == CXType_Invalid)
                {
                    continue;
                }

                TypeInfoParamTemplate param;
                param.name = Utils::String::Format("arg{0}", i);
                param.type = ParseTemplateTypeRef(pContext, argumentType, pClass->templateParameters);
                evt.params.push_back(param);
            }

            ApplyEventOptions(eventMacro, evt);
            pClass->events.push_back(evt);
        }

        return true;
    }

    static bool VisitTemplateMethod(CXCursor cr,
                                    ClangParserContext* pContext,
                                    TypeInfoStructTemplate* pClass,
                                    uint32_t const declStartPosition,
                                    int const lineNumber)
    {
        MarkMacro macro;
        if (pContext->FindMarkMacro(pClass->headerID, cr, macro, MacroTypeEnum::SEFunction))
        {
            TypeInfoFuncTemplate func;

            CXType cursorType = clang_getCursorType(cr);

            func.isReflect = macro.hasReflect;
            func.isAPI = macro.HasApi();

            func.name = ClangUtils::GetCursorSpellingAnsi(cr);
            func.returnType = ParseTemplateTypeRef(pContext, clang_getResultType(cursorType), pClass->templateParameters);
            func.isStatic = (clang_CXXMethod_isStatic(cr) != 0);
            func.isVirtual = (clang_CXXMethod_isVirtual(cr) != 0);
            func.isConst = (clang_CXXMethod_isConst(cr) != 0);
            func.access = ClangUtils::GetAccessLevel(cr);
            func.comment = ClangUtils::GetCursorComment(cr);
            func.uniqueName = func.name;
            func.lineNumber = (int)lineNumber;

            if (macro.HasApi())
            {
                MarkAPI const& api = macro.GetApi();
                func.APINoProxy     = api.IsNoProxy;
                func.APIIsSealed    = api.IsSealed;
                func.APIIsStatic    = api.IsStatic;
                func.APIIsPropertie = api.IsProperty;

                func.attributes = api.attributes;
                func.marshalAs  = api.marshalAs;
            }

            int numArgs = clang_Cursor_getNumArguments(cr);
            for (int i = 0; i < numArgs; ++i)
            {
                CXCursor argCr = clang_Cursor_getArgument(cr, i);

                TypeInfoParamTemplate param;
                FillTypeInfoParamTemplate(argCr, param, pClass->templateParameters);
                func.params.push_back(param);
            }

            pClass->functions.emplace_back(func);
        }

        return true;
    }

    CXChildVisitResult VisitTemplateStructureContents(CXCursor cr, CXCursor parent, CXClientData pClientData)
    {
        ClangParserContext* pContext = static_cast<ClangParserContext*>(pClientData);
        auto* pClass = static_cast<TypeInfoStructTemplate*>(pContext->pParentReflectedType);

        int const lineNumber = ClangUtils::GetLineNumberForCursor(cr);
        uint32 const declStartPosition = ClangUtils::GetStartPositionForCursor(cr);
        CXCursorKind kind = clang_getCursorKind(cr);

        if (kind == CXCursor_CXXBaseSpecifier)
        {
            if (pClass->baseType.IsValid())
            {
                return CXChildVisit_Continue;
            }

            clang::CXXBaseSpecifier* pBaseSpecifier = (clang::CXXBaseSpecifier*)cr.data[0];
            TypeRefTemplate baseType;
            std::string baseTypeText = pBaseSpecifier->getType().getAsString().c_str();
            if (!TryParseTemplateTypeRef(baseTypeText, baseType, pClass->templateParameters))
            {
                pContext->LogError("Failed to parse template base type for class: {0}, base class = {1}", pClass->name, ClangUtils::GetCursorDisplayName(cr));
                return CXChildVisit_Break;
            }
            pClass->baseType = std::move(baseType);
            std::string const baseSimpleName = pClass->baseType.ToCppString(false);
            static const char* ScriptingObjectBases[] = {
                "SE::ScriptingObject",
                "SE::ManagedScriptingObject",
                "SE::BinaryAsset",
                "SE::SceneObject",
                "SE::Asset",
                "SE::Script",
                "SE::Actor",
            };
            for (const char* name : ScriptingObjectBases)
            {
                if (baseSimpleName == name)
                {
                    pClass->isScriptingObject = true;
                    break;
                }
            }

            if (!pClass->isScriptingObject)
            {
                TypeInfoBase const* pBaseType = pContext->pDatabase->GetType(TypeID(baseSimpleName));
                if (pBaseType && pBaseType->isAPI && pBaseType->IsFlag(TypeInfoBase::Flag::IsStruct))
                {
                    auto const* pBaseStruct = static_cast<TypeInfoStruct const*>(pBaseType);
                    pClass->isScriptingObject = pBaseStruct->isScriptingObject;
                }
            }
        }
        else if (kind == CXCursor_FieldDecl)
        {
            if (!VisitTemplateField(cr, pContext, pClass, declStartPosition, lineNumber))
            {
                return CXChildVisit_Break;
            }
        }
        else if (kind == CXCursor_VarDecl)
        {
            if (!VisitTemplateField(cr, pContext, pClass, declStartPosition, lineNumber))
            {
                return CXChildVisit_Break;
            }
        }
        else if (kind == CXCursor_CXXMethod)
        {
            VisitTemplateMethod(cr, pContext, pClass, declStartPosition, lineNumber);
        }
        else if (kind == CXCursor_UnionDecl)
        {
            clang_visitChildren(cr, VisitTemplateStructureContents, pContext);
        }
        else if (kind == CXCursor_StructDecl)
        {
            clang_visitChildren(cr, VisitTemplateStructureContents, pContext);
        }

        return CXChildVisit_Continue;
    }

    CXChildVisitResult VisitTemplateStructure(ClangParserContext*     pContext,
                                              CXCursor&               cr,
                                              std::string_view const& headerFilePath,
                                              HeaderID const          headerID)
    {
        MarkMacro     macro;
        MacroTypeEnum macroType = MacroTypeEnum::SEClass;

        if (pContext->FindMarkMacro(headerID, cr, macro, MacroTypeEnum::SEClass))
        {
            macroType = MacroTypeEnum::SEClass;
        }
        else if (pContext->FindMarkMacro(headerID, cr, macro, MacroTypeEnum::SEStruct))
        {
            macroType = MacroTypeEnum::SEStruct;
        }
        else if (pContext->FindMarkMacro(headerID, cr, macro, MacroTypeEnum::SEInterface))
        {
            macroType = MacroTypeEnum::SEInterface;
        }
        else
        {
            return CXChildVisit_Continue;
        }

        if (!macro.hasTemplate)
        {
            // pContext->LogError("Cannot register template type ({0}) without Template flag", cursorName);
            return CXChildVisit_Continue;
        }

        auto   cursorName = ClangUtils::GetCursorDisplayName(cr);
        size_t index      = cursorName.find_first_of("<");
        cursorName        = cursorName.substr(0, index);

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
        if (macro.HasApi() && !macro.GetApi().inBuildMapType.empty())
        {
            pContext->LogError("API(InBuild(\"{0}\")) is not supported on template type ({1})", macro.GetApi().inBuildMapType, cursorName);
            return CXChildVisit_Break;
        }

        auto structDesc = std::make_unique<TypeInfoStructTemplate>();
        structDesc->typeID             = pContext->GenerateTypeID(fullyQualifiedCursorName);
        structDesc->headerID           = headerID;
        structDesc->name               = cursorName;
        structDesc->namespaceScopeList = pContext->GetNamespaces();
        structDesc->structScopeList    = pContext->GetStructScopes();
        structDesc->templateParameters = templateParameters;
        structDesc->isAbstract         = clang_CXXRecord_isAbstract(cr);
        structDesc->isStruct           = macroType == MacroTypeEnum::SEStruct;
        structDesc->isReflect          = false;
        structDesc->isAPI              = macro.HasApi();
        structDesc->comment            = ClangUtils::GetCursorComment(cr);

        ApplyStructOptions(macro, *structDesc);

        void* pPreviousParentReflectedType = pContext->pParentReflectedType;
        pContext->pParentReflectedType     = structDesc.get();
        {
            clang_visitChildren(cr, VisitTemplateStructureContents, pContext);
        }
        pContext->pParentReflectedType = pPreviousParentReflectedType;

        pContext->AddTemplateType(std::move(structDesc));

        if (pContext->HasErrorOccured())
        {
            return CXChildVisit_Break;
        }

        return CXChildVisit_Continue;
    }

    CXChildVisitResult VisitStructure(ClangParserContext *pContext, CXCursor &cr, std::string_view const &headerFilePath, HeaderID const headerID, bool isStruct)
    {
        std::string cursorName = ClangUtils::GetCursorDisplayName(cr);

        std::string fullyQualifiedCursorName;
        if (!ClangUtils::GetQualifiedNameForType(clang_getCursorType(cr), fullyQualifiedCursorName))
        {
            pContext->LogError("Failed to get qualified type for cursor: {0}", fullyQualifiedCursorName);
            return CXChildVisit_Break;
        }

        MarkMacro macro;
        //-------------------------------------------------------------------------
        if (pContext->FindMarkMacro(headerID, cr, macro, MacroTypeEnum::SEMeta))
        {
            TypeID typeID = pContext->GenerateTypeID(fullyQualifiedCursorName);
            auto classDescriptor = std::make_unique<TypeInfoBase>(typeID, cursorName, TypeInfoBase::Flag::IsMeta);
            classDescriptor->headerID = headerID;
            classDescriptor->namespaceScopeList = pContext->GetNamespaces();
            classDescriptor->structScopeList = pContext->GetStructScopes();
            pContext->pDatabase->RegisterType(std::move(classDescriptor), false);

        }
        else
        {
            //-------------------------------------------------------------------------
            MacroTypeEnum macroType = MacroTypeEnum::SEClass;
            if (isStruct)
            {
                if (pContext->FindMarkMacro(headerID, cr, macro, MacroTypeEnum::SEStruct))
                {
                    macroType = MacroTypeEnum::SEStruct;
                }
            }
            else
            {
                if (pContext->FindMarkMacro(headerID, cr, macro, MacroTypeEnum::SEClass))
                {
                    macroType = MacroTypeEnum::SEClass;
                }
                else if (pContext->FindMarkMacro(headerID, cr, macro, MacroTypeEnum::SEInterface))
                {
                    macroType = MacroTypeEnum::SEInterface;
                }
            }

            if (macro.IsValid())
            {
                // Modules
                // if (macro.IsModuleMacro())
                //{
                //    std::string const moduleName = Utils::String::Format("{0}{1}", pContext->GetCurrentNamespace(),
                //    cursorName);

                //    if (!pContext->SetModuleClassName(headerFilePath, moduleName))
                //    {
                //        // Could not find originating project for detected registered module class
                //        pContext->LogError("Cant find the source project for this module class: {0}", headerFilePath);
                //        return CXChildVisit_Break;
                //    }
                //}

                if (fullyQualifiedCursorName == "SE::SpriteAtlas")
                {
                    fullyQualifiedCursorName = "SE::SpriteAtlas";
                }

                //-------------------------------------------------------------------------
                StringID typeID                = pContext->GenerateTypeID(fullyQualifiedCursorName);
                auto     structDesc            = std::make_unique<TypeInfoStruct>(typeID, cursorName);
                structDesc->headerID           = headerID;
                structDesc->namespaceScopeList = pContext->GetNamespaces();
                structDesc->structScopeList    = pContext->GetStructScopes();
                structDesc->isAbstract         = clang_CXXRecord_isAbstract(cr);
                structDesc->isStruct           = isStruct;
                structDesc->isReflect          = macro.hasReflect;
                structDesc->isAPI              = macro.HasApi();
                structDesc->comment            = ClangUtils::GetCursorComment(cr);

                pContext->GetAssemblyInfoForHeader(headerID, structDesc->assemblyName, structDesc->assemblyDir);

                ApplyStructOptions(macro, *structDesc);

                // Record current parent type, and update it to the new type
                void* pPreviousParentReflectedType = pContext->pParentReflectedType;
                pContext->pParentReflectedType     = structDesc.get();
                {
                    clang_visitChildren(cr, VisitStructureContents, pContext);
                }
                // Reset parent type back to original parent
                pContext->pParentReflectedType = pPreviousParentReflectedType;

                pContext->pDatabase->RegisterType(std::move(structDesc), false);

                if (pContext->HasErrorOccured())
                {
                    return CXChildVisit_Break;
                }
            }
        }

        return CXChildVisit_Continue;
    }


}

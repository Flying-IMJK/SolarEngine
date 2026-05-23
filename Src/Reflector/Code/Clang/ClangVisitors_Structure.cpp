#include "ClangVisitors_Structure.h"
#include "ClangVisitors_Enum.h"
#include "../ReflectorSettingsAndUtils.h"
#include "../Database/ReflectionDatabase.h"
#include "Core/TypeSystem/Types.h"
#include "ClangVisitors_Macro.h"

//-------------------------------------------------------------------------

namespace SE::ReflectTool
{
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

    static void GetFieldTypeInfo(ClangParserContext *pContext, ReflectedType *pType, CXType type, FieldTypeInfo &info)
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

    static void GetAllDerivedProperties(ReflectionDatabase const *pDatabase, StringID parentTypeID, List<ReflectedProperty> &results)
    {
        ReflectedType const *pParentDesc = pDatabase->GetType(parentTypeID);
        if (pParentDesc != nullptr)
        {
            GetAllDerivedProperties(pDatabase, pParentDesc->parentTypeID, results);
            for (auto &parentProperty : pParentDesc->properties)
            {
                results.Add(parentProperty);
            }
        }
    }

    static void VisitConstructor(CXCursor cr, ReflectedType *pType)
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

    //-------------------------------------------------------------------------

    CXChildVisitResult VisitStructureContents(CXCursor cr, CXCursor parent, CXClientData pClientData)
    {
        auto pContext = reinterpret_cast<ClangParserContext *>(pClientData);
        auto pClass = reinterpret_cast<ReflectedType *>(pContext->m_pParentReflectedType);

        int const lineNumber = ClangUtils::GetLineNumberForCursor(cr);
        CXCursorKind kind = clang_getCursorKind(cr);
        switch (kind)
        {
        // Add base class
        case CXCursor_CXXBaseSpecifier:
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
            GetAllDerivedProperties(pContext->m_pDatabase, pClass->parentTypeID, pClass->properties);

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
        }
        break;

        // Extract property info
        case CXCursor_FieldDecl:
        {
            ReflectionMacro propertyReflectionMacro;
            if (pContext->FindReflectionMacroForProperty(pClass->headerID, lineNumber, propertyReflectionMacro))
            {
                pClass->properties.Add(ReflectedProperty(ClangUtils::GetCursorDisplayName(cr).ToStringAnsi(), lineNumber));
                ReflectedProperty &propertyDesc = pClass->properties.Last();

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
                    propertyDesc.description = propertyReflectionMacro.macroComment.ToStringAnsi();
                }

                auto type = clang_getCursorType(cr);
                auto const pFieldQualType = ClangUtils::GetQualType(type);
                auto typeSpelling = ClangUtils::GetString(clang_getTypeSpelling(type));

                // Check if template parameter
                if (pFieldQualType->isTemplateTypeParmType())
                {
                    pContext->LogError(SE_TEXT("Cannot expose template argument member ({0}) in class ({1})!"), propertyDesc.name, pClass->name);
                    return CXChildVisit_Break;
                }

                // Check if this field is a c-style array
                //-------------------------------------------------------------------------

                if (pFieldQualType->isArrayType())
                {
                    if (pFieldQualType->isVariableArrayType() || pFieldQualType->isIncompleteArrayType())
                    {
                        pContext->LogError(SE_TEXT("Variable size array properties are not supported! Please change to List or fixed size!"));
                        return CXChildVisit_Break;
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
                        return CXChildVisit_Break;
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
                propertyDesc.metaData = propertyReflectionMacro.macroContents.ToStringAnsi();

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
                        ReflectedType const *pFlagTypeDesc = pContext->m_pDatabase->GetType(propertyDesc.templateArgTypeName.ToString());
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
                    ReflectedType const *pPropertyTypeDesc = pContext->m_pDatabase->GetType(propertyDesc.typeID);
                    if (pPropertyTypeDesc == nullptr)
                    {
                        pContext->LogError(SE_TEXT("Unsupported type encountered: {0} for property: {1} in class: {2}"), propertyDesc.typeName, propertyDesc.name, pClass->name);
                        return CXChildVisit_Break;
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
            break;
        }

		// TODO 支持方法
        case CXCursor_CXXMethod:

        // Constructor Method
        case CXCursor_Constructor:
            // VisitConstructor(cr);
            break;
        default:
            break;
        }
        return CXChildVisit_Continue;
    }

    CXChildVisitResult VisitResourceStructureContents(CXCursor cr, CXCursor parent, CXClientData pClientData)
    {
        auto pContext = reinterpret_cast<ClangParserContext *>(pClientData);
        auto pResource = reinterpret_cast<ReflectedResourceType *>(pContext->m_pParentReflectedType);

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

    CXChildVisitResult VisitStructure(ClangParserContext *pContext, CXCursor &cr, StringView const &headerFilePath, HeaderID const headerID)
    {
        auto cursorName = ClangUtils::GetCursorDisplayName(cr);

        String fullyQualifiedCursorName;
        if (!ClangUtils::GetQualifiedNameForType(clang_getCursorType(cr), fullyQualifiedCursorName))
        {
            pContext->LogError(SE_TEXT("Failed to get qualified type for cursor: {0}"), fullyQualifiedCursorName);
            return CXChildVisit_Break;
        }

        ReflectionMacro macro;
        //-------------------------------------------------------------------------
        int const lineNumber = ClangUtils::GetLineNumberForCursor(cr);
        if (pContext->FindReflectionMacroForMeta(headerID, lineNumber, macro))
        {
            TypeID typeID = pContext->GenerateTypeID(fullyQualifiedCursorName);
            ReflectedType classDescriptor(typeID, cursorName.ToStringAnsi());
            classDescriptor.headerID = headerID;
            classDescriptor.namespaceName = pContext->GetCurrentNamespace().ToStringAnsi();
            classDescriptor.flags.Flag(ReflectedType::Flags::IsMeta, true);
            pContext->m_pDatabase->RegisterType(&classDescriptor, pContext->m_detectDevOnlyTypesAndProperties);
        }

        //-------------------------------------------------------------------------
        if (pContext->FindReflectionMacroForType(headerID, cr, macro))
        {
            if (!pContext->IsInEngineNamespace())
            {
                pContext->LogError(SE_TEXT("You cannot register types for reflection that are outside the engine namespace ({0}). Type: {1}, File: {2}"), Settings::g_engineNamespace, fullyQualifiedCursorName, headerFilePath);
                return CXChildVisit_Break;
            }

            ENGINE_ASSERT(macro.IsValid());

            // Modules
            if (macro.IsModuleMacro() && !pContext->m_detectDevOnlyTypesAndProperties)
            {
				String const moduleName = pContext->GetCurrentNamespace() + cursorName;

                if (!pContext->SetModuleClassName(headerFilePath, moduleName))
                {
                    // Could not find originating project for detected registered module class
                    pContext->LogError(SE_TEXT("Cant find the source project for this module class: {0}"), headerFilePath);
                    return CXChildVisit_Break;
                }
            }

            //-------------------------------------------------------------------------

            if (macro.IsReflectedTypeMacro())
            {
                auto cursorType = clang_getCursorType(cr);

                StringID typeID = pContext->GenerateTypeID(fullyQualifiedCursorName);
                ReflectedType classDescriptor(typeID, cursorName.ToStringAnsi());
                classDescriptor.headerID = headerID;
                classDescriptor.namespaceName = pContext->GetCurrentNamespace().ToStringAnsi();
                classDescriptor.flags.Flag(ReflectedType::Flags::IsAbstract, clang_CXXRecord_isAbstract(cr));
                classDescriptor.flags.Flag(ReflectedType::Flags::IsStruct, true);

                // Record current parent type, and update it to the new type
                void *pPreviousParentReflectedType = pContext->m_pParentReflectedType;
                pContext->m_pParentReflectedType = &classDescriptor;
                {
                    clang_visitChildren(cr, VisitStructureContents, pContext);
                }
                // Reset parent type back to original parent
                pContext->m_pParentReflectedType = pPreviousParentReflectedType;

                if (pContext->HasErrorOccured())
                {
                    return CXChildVisit_Break;
                }

                pContext->m_pDatabase->RegisterType(&classDescriptor, pContext->m_detectDevOnlyTypesAndProperties);
            }
        }

        return CXChildVisit_Continue;
    }


}
#include "ClangUtils.h"
#include "CodeGenerators/CodeGenerator_BindingsTypeMap.h"
#include "Core/Dictionary.h"
#include "Core/FileSystem.h"

#include <unordered_map>
//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    namespace ClangUtils
    {
        // Custom conversion type
        Dictionary<std::string, std::string> mapType = {};

        // HashSet<std::string> templateTypeConvert = { "SE::Vector2Base", "SE::Vector3Base", "SE::Vector4Base" };

        void GetDiagnostics(CXTranslationUnit& TU, std::vector<std::string>& diagnostics)
        {
            auto const numDiagnostics = clang_getNumDiagnostics(TU);
            for (auto i = 0u; i < numDiagnostics; i++)
            {
                CXDiagnostic diagnostic = clang_getDiagnostic(TU, i);
                diagnostics.push_back(
                    GetString(clang_formatDiagnostic(diagnostic, clang_defaultDiagnosticDisplayOptions())));
                clang_disposeDiagnostic(diagnostic);
            }
        }

        std::string GetHeaderPathForCursor(CXCursor cr)
        {
            CXFile              pFile;
            CXSourceRange const cursorRange = clang_getCursorExtent(cr);
            clang_getExpansionLocation(clang_getRangeStart(cursorRange), &pFile, nullptr, nullptr, nullptr);

            std::string HeaderFilePath;
            if (pFile != nullptr)
            {
                CXString clangFilePath = clang_File_tryGetRealPathName(pFile);
                HeaderFilePath         = std::string(clang_getCString(clangFilePath));
                FileSystem::NormalizePath(HeaderFilePath);
                clang_disposeString(clangFilePath);
            }
            FileSystem::NormalizePath(HeaderFilePath);
            return HeaderFilePath;
        }

        bool GetQualifiedNameForType(clang::QualType type, std::string& qualifiedName)
        {
            clang::Type const* pType = type.getTypePtr();

            if (pType->isArrayType())
            {
                auto elementType = pType->castAsArrayTypeUnsafe()->getElementType();
                if (!GetQualifiedNameForType(elementType, qualifiedName))
                {
                    return false;
                }
            }
            else if (pType->isBooleanType())
            {
                qualifiedName = "bool";
            }
            else if (pType->isBuiltinType())
            {
                auto const* pBT = pType->getAs<clang::BuiltinType>();
                switch (pBT->getKind())
                {
                    case clang::BuiltinType::Char_S:
                        qualifiedName = "int8";
                        break;

                    case clang::BuiltinType::Char_U:
                        qualifiedName = "uint8";
                        break;

                    case clang::BuiltinType::UChar:
                        qualifiedName = "uint8";
                        break;

                    case clang::BuiltinType::SChar:
                        qualifiedName = "int8";
                        break;

                    case clang::BuiltinType::Char16:
                        qualifiedName = "uint16";
                        break;

                    case clang::BuiltinType::Char32:
                        qualifiedName = "uint32";
                        break;

                    case clang::BuiltinType::UShort:
                        qualifiedName = "uint16";
                        break;

                    case clang::BuiltinType::Short:
                        qualifiedName = "int16";
                        break;

                    case clang::BuiltinType::UInt:
                        qualifiedName = "uint32";
                        break;

                    case clang::BuiltinType::Int:
                        qualifiedName = "int32";
                        break;

                    case clang::BuiltinType::ULongLong:
                        qualifiedName = "uint64";
                        break;

                    case clang::BuiltinType::LongLong:
                        qualifiedName = "int64";
                        break;

                    case clang::BuiltinType::Float:
                        qualifiedName = "float";
                        break;

                    case clang::BuiltinType::Double:
                        qualifiedName = "double";
                        break;

                    default: {
                        return false;
                    }
                };
            }
            else if (pType->isPointerType() || pType->isReferenceType())
            {
                // Do Nothing
            }
            else if (pType->isRecordType())
            {
                clang::RecordDecl const* pRecordDecl = pType->getAs<clang::RecordType>()->getDecl();
                ENGINE_ASSERT(pRecordDecl != nullptr);

                qualifiedName = pRecordDecl->getQualifiedNameAsString().c_str();
                /*if (templateTypeConvert.Contains(qualifiedName))
                {
                    auto* spec = (clang::ClassTemplateSpecializationDecl*)pRecordDecl;
                    if (spec != nullptr)
                    {
                        // 获取模板参数列表
                        const clang::TemplateArgumentList& arguments = spec->getTemplateArgs();
                        qualifiedName += "<";
                        for (unsigned i = 0, e = arguments.size(); i != e; ++i)
                        {
                            if (i > 0)
                            {
                                qualifiedName += ", ";
                            }
                            qualifiedName += arguments[i].getAsType().getAsString().c_str();
                        }
                        qualifiedName += ">";
                    }
                }*/
            }
            else if (pType->isEnumeralType())
            {
                clang::NamedDecl const* pNamedDecl = pType->getAs<clang::EnumType>()->getDecl();
                ENGINE_ASSERT(pNamedDecl != nullptr);
                qualifiedName = pNamedDecl->getQualifiedNameAsString().c_str();
            }
            else if (pType->getTypeClass() == clang::Type::Typedef)
            {
                clang::TypedefType const*     typedefType      = pType->getAs<clang::TypedefType>();
                clang::TypedefNameDecl const* pTypedefNameDecl = typedefType->getDecl();
                ENGINE_ASSERT(pTypedefNameDecl != nullptr);
                qualifiedName = pTypedefNameDecl->getQualifiedNameAsString().c_str();

                /*				// 检查typedef是否为模板实例化。
                                const auto* tmplSpecType =
                   pTypedefNameDecl->getUnderlyingType()->getAs<clang::TemplateSpecializationType>(); if (tmplSpecType
                   != nullptr)
                                {
                                    auto className = tmplSpecType->getTypeClassName();
                                    auto tmplArgs = tmplSpecType->template_arguments();
                                    // 递归获取名称，并包含其模板参数。
                                    //qualifiedName =
                                }
                                else
                                {
                                    // 不是模板实例化，按常规方式获取限定名称。

                                }*/
            }
            else
            {
                return false;
            }
            return true;
        }

        bool GetAllBaseClasses(std::vector<StringID>& baseClasses, clang::CXXBaseSpecifier& baseSpecifier)
        {
			std::string fullyQualifiedName;
            if (!ClangUtils::GetQualifiedNameForType(baseSpecifier.getType(), fullyQualifiedName))
            {
                return false;
            }

            baseClasses.push_back(StringID(fullyQualifiedName));

            clang::CXXRecordDecl *pBaseSpecifierRecordDecl = baseSpecifier.getType()->getAsCXXRecordDecl();
            // 检查指针是否为NULL
            if (!pBaseSpecifierRecordDecl)
            {
                return true;
            }

            for (auto parentBaseSpecifier : pBaseSpecifierRecordDecl->bases())
            {
                if (!GetAllBaseClasses(baseClasses, parentBaseSpecifier))
                {
                    return false;
                }
            }

            return true;
        }

        AccessLevel GetAccessLevel(CXCursor cr, AccessLevel defaultAccess)
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

        std::string GetCursorComment(CXCursor cr)
        {
            std::string    result;
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

        std::string GetParameterDefaultValue(CXCursor argCr)
        {
            CXSourceRange     range           = clang_getCursorExtent(argCr);
            CXTranslationUnit translationUnit = clang_Cursor_getTranslationUnit(argCr);
            CXToken*          tokens          = nullptr;
            uint32            numTokens       = 0;
            clang_tokenize(translationUnit, range, &tokens, &numTokens);

            std::string result;
            bool        foundEquals = false;
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

        bool IsStatic(CXCursor cr) { return (clang_Cursor_getStorageClass(cr) == CX_SC_Static); }

        void FillTypeInfoParam(CXCursor argCr, TypeInfoParam& param)
        {
            CXType argType = clang_getCursorType(argCr);

            FillTypeInfo(argType, param.type);

            param.defaultValue = ClangUtils::GetParameterDefaultValue(argCr);
            param.comment      = ClangUtils::GetCursorComment(argCr);
            param.name = ClangUtils::GetCursorSpellingAnsi(argCr);
            if (param.name.empty())
            {
                param.name = "arg";
            }

            // Preserve the public API direction independently from the C++
            // declarator. Mutable lvalue references are bidirectional until an
            // explicit API_PARAM(Out) annotation is available in the parser.
            param.direction = param.type.isRef && !param.type.isConst
                ? ApiParameterDirection::Ref
                : ApiParameterDirection::In;
        }

        void FillTypeInfo(CXType argType, TypeInfo& param)
        {
            param = TypeInfo();
            clang::QualType const fieldQualType = ClangUtils::GetQualType(argType);
            if (fieldQualType->isArrayType())
            {
                if (fieldQualType->isVariableArrayType() || fieldQualType->isIncompleteArrayType())
                {
                    ENGINE_ASSERT("Variable size array properties are not supported! Please change to List or fixed size!");
                }

                auto const pArrayType = (clang::ConstantArrayType*)fieldQualType.getTypePtr();
                param.arraySize = (int32)pArrayType->getSize().getSExtValue();
                argType = clang_getElementType(argType);
            }

            CXType canonical = clang_getCanonicalType(argType);
            param.isPointer = (canonical.kind == CXType_Pointer);
            param.pointerDepth = 0;
            CXType pointerType = canonical;
            while (pointerType.kind == CXType_Pointer)
            {
                ++param.pointerDepth;
                pointerType = clang_getCanonicalType(clang_getPointeeType(pointerType));
            }
            param.isRef = (canonical.kind == CXType_LValueReference ||
                           canonical.kind == CXType_RValueReference);
            param.isMoveRef = canonical.kind == CXType_RValueReference;

            CXType valueType = argType;
            if (param.isPointer || param.isRef)
            {
                CXType pointeeType = clang_getPointeeType(argType);
                if (pointeeType.kind == CXType_Invalid)
                {
                    pointeeType = clang_getPointeeType(canonical);
                }
                if (pointeeType.kind != CXType_Invalid)
                {
                    valueType = pointeeType;
                }
            }

            param.isConst = clang_isConstQualifiedType(valueType) != 0;

            std::string typeName = ClangUtils::GetTypeSpellingAnsi(valueType);
            GetQualifiedNameForType(valueType, typeName);

            param.typeID = TypeID(typeName);


            int numTemplateArgs = clang_Type_getNumTemplateArguments(valueType);
            if (numTemplateArgs > 0)
            {
                param.genericityArgs.resize(numTemplateArgs);
                for (int i = 0; i < numTemplateArgs; ++i)
                {
                    CXType templateArg = clang_Type_getTemplateArgumentAsType(valueType, i);
                    if (templateArg.kind == CXType_Invalid)
                        continue;

                    FillTypeInfo(templateArg, param.genericityArgs[i]);
                }
            }

        }

        static bool IsObjectReferenceWrapper(TypeInfo const& type)
        {
            if (type.genericityArgs.size() != 1)
            {
                return false;
            }

            const std::string& name = type.typeID.ToString();
            return name == "AssetRef" || name == "WeakAssetRef" || name == "SoftAssetRef" ||
                   name == "AssetReference" || name == "WeakAssetReference" || name == "SoftAssetReference" ||
                   name == "ScriptingObjectReference";
        }

        static bool IsDynamicCollection(TypeInfo const& type)
        {
            const std::string& name = type.typeID.ToString();
            if (name == "BytesContainer")
            {
                return true;
            }
            if (name == "Array" || name == "Span" || name == "List" || name == "DataContainer" || name == "HashSet")
            {
                return type.genericityArgs.size() == 1;
            }
            return name == "Dictionary" && type.genericityArgs.size() == 2;
        }

        bool TypeIsPod(TypeDatabase const& database, TypeInfo const& cppType, std::vector<TypeID>& stack)
        {
            if (cppType.arraySize > 0)
            {
                TypeInfo elementType = cppType;
                elementType.arraySize = 0;
                return TypeIsPod(database, elementType, stack);
            }

            TypeInfoBase const* declaration = database.ResolveTypeDeclaration(cppType);
            if (cppType.isPointer || cppType.isRef)
            {
                // Pointers are raw-copyable only when the boundary keeps the same
                // address representation. Object pointers require a managed handle.
                if (declaration && declaration->IsFlag(TypeInfoBase::Flag::IsClassStruct))
                {
                    auto const* structType = static_cast<TypeInfoStruct const*>(declaration);
                    if (structType->isScriptingObject || (!structType->isStruct && structType->isAPI))
                    {
                        return false;
                    }
                }
                return true;
            }

            if (IsObjectReferenceWrapper(cppType) || IsDynamicCollection(cppType))
            {
                return false;
            }

            if (IsKnownBlittableBuiltin(cppType))
            {
                return true;
            }

            if (!declaration)
            {
                // Unknown values must never silently become raw-copyable.
                return false;
            }

            if (declaration->IsFlag(TypeInfoBase::Flag::IsEnum))
            {
                return true;
            }
            if (!declaration->IsFlag(TypeInfoBase::Flag::IsClassStruct))
            {
                return false;
            }

            auto const* structType = static_cast<TypeInfoStruct const*>(declaration);
            if (!structType->APIMarshalAs.empty())
            {
                return false;
            }
            if (!structType->isStruct)
            {
                return false;
            }
            return CalculateStructureIsPod(database, *structType, stack);
        }

        bool CalculateStructureIsPod(TypeDatabase const& database, TypeInfoBase const& type, std::vector<TypeID>& stack)
        {
            if (!type.IsFlag(TypeInfoBase::Flag::IsClassStruct))
            {
                return false;
            }

            auto const& structType = static_cast<TypeInfoStruct const&>(type);
            if (!structType.isStruct || !structType.APIMarshalAs.empty() || structType.APIIsInterface || !structType.interfaces.empty())
            {
                return false;
            }
            if (Utils::Vector::Contains(stack, type.typeID))
            {
                return false;
            }

            stack.push_back(type.typeID);
            bool isPod = true;

            if (structType.parentTypeID != TypeID::Invalid)
            {
                TypeInfoBase const* baseType = database.GetType(structType.parentTypeID);
                isPod                        = baseType != nullptr && baseType->IsFlag(TypeInfoBase::Flag::IsClassStruct) &&
                                               CalculateStructureIsPod(database, *baseType, stack);
            }

            for (int i = 0; isPod && i < structType.fields.size(); ++i)
            {
                TypeInfoField const& field = structType.fields[i];
                if (!field.isStatic && (!field.marshalAs.empty() || !TypeIsPod(database, field.type, stack)))
                {
                    isPod = false;
                }
            }

            stack.pop_back();
            return isPod;
        }


    } // namespace ClangUtils

} // namespace SE::BuildTool

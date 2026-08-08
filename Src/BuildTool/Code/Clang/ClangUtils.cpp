#include "ClangUtils.h"
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
    } // namespace ClangUtils

} // namespace SE::BuildTool
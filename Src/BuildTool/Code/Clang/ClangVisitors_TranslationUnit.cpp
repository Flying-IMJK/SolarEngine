#include "ClangVisitors_TranslationUnit.h"
#include "ClangVisitors_Macro.h"
#include "ClangVisitors_Enum.h"
#include "Clangvisitors_Structure.h"

//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    static void SplitTemplateArguments(std::string const& text, std::vector<std::string>& outArgs)
    {
        int32 depth = 0;
        int32 start = 0;
        for (int32 i = 0; i < text.length(); i++)
        {
            char const c = text[(size_t)i];
            if (c == '<')
            {
                depth++;
            }
            else if (c == '>')
            {
                depth--;
            }
            else if (c == ',' && depth == 0)
            {
                std::string arg = text.substr((size_t)start, (size_t)(i - start));
                Utils::String::TrimStart(arg);
                Utils::String::TrimEnd(arg);
                outArgs.push_back(arg);
                start = i + 1;
            }
        }

        if (start < text.length())
        {
            std::string arg = text.substr((size_t)start);
            Utils::String::TrimStart(arg);
            Utils::String::TrimEnd(arg);
            outArgs.push_back(arg);
        }
    }

    static bool TryParseTemplateSpecialization(std::string typeName, std::string& outTemplateName, std::vector<std::string>& outTemplateArgs)
    {
        Utils::String::TrimStart(typeName);
        Utils::String::TrimEnd(typeName);
        if (Utils::String::StartsWith(typeName, "struct "))
        {
            typeName = typeName.substr(7);
        }
        else if (Utils::String::StartsWith(typeName, "class "))
        {
            typeName = typeName.substr(6);
        }

        int32 const openIdx = Utils::String::Find(typeName, '<');
        int32 const closeIdx = Utils::String::FindLast(typeName, '>');
        if (openIdx == INVALID_INDEX || closeIdx == INVALID_INDEX || closeIdx <= openIdx)
        {
            return false;
        }

        outTemplateName = typeName.substr(0, (size_t)openIdx);
        Utils::String::TrimStart(outTemplateName);
        Utils::String::TrimEnd(outTemplateName);

        std::string argText = typeName.substr((size_t)openIdx + 1, (size_t)(closeIdx - openIdx - 1));
        SplitTemplateArguments(argText, outTemplateArgs);
        return !outTemplateName.empty() && !outTemplateArgs.empty();
    }

    static CXChildVisitResult VisitTypeDef(ClangParserContext* pContext, CXCursor cr, HeaderID headerID)
    {
        MarkMacro macro;
        if (!pContext->FindMarkMacro(headerID, cr, macro, ReflectionMacroType::SETypeDef))
        {
            return CXChildVisit_Continue;
        }

        if (macro.HasContent("Alias"))
        {
            return CXChildVisit_Continue;
        }

        CXType underlyingType = clang_getTypedefDeclUnderlyingType(cr);
        std::string underlyingTypeName = ClangUtils::GetTypeSpellingAnsi(underlyingType);

        std::string templateTypeName;
        std::vector<std::string> templateArguments;
        if (!TryParseTemplateSpecialization(underlyingTypeName, templateTypeName, templateArguments))
        {
            pContext->LogError("SE_TYPEDEF can only generate concrete types from template specializations. Typedef: {0}, underlying type: {1}",
                               ClangUtils::GetCursorDisplayName(cr),
                               underlyingTypeName);
            return CXChildVisit_Break;
        }

        ClangParserContext::TypeDefData typeDef;
        typeDef.headerID = headerID;
        typeDef.lineNumber = (int32)ClangUtils::GetLineNumberForCursor(cr);
        typeDef.macro = macro;
        typeDef.name = ClangUtils::GetCursorDisplayName(cr);
        typeDef.templateTypeName = templateTypeName;
        typeDef.templateArguments = templateArguments;
        typeDef.namespaceScopeList = pContext->GetNamespaces();
        typeDef.structScopeList = pContext->GetStructScopes();
        pContext->AddTypeDef(typeDef);
        return CXChildVisit_Continue;
    }

    CXChildVisitResult VisitTranslationUnit( CXCursor cr, CXCursor parent, CXClientData pClientData )
    {
        auto pContext = static_cast<ClangParserContext*>( pClientData );
        if ( pContext->HasErrorOccured() )
        {
            return CXChildVisit_Break;
        }

		std::string headerFilePath = ClangUtils::GetHeaderPathForCursor( cr );
        if (headerFilePath.empty())
        {
            return CXChildVisit_Continue;
        }

        // Dont parse non-solution files
        if (!FileSystem::IsUnderDirectory(headerFilePath, pContext->pSolution->path))
        {
            return CXChildVisit_Continue;
        }

        // Ensure that the header file is part of the list of headers to visit
        HeaderID const headerID = HeaderInfo::GetHeaderID( headerFilePath );
        HeaderInfo const* pHeaderInfo = pContext->GetHeaderInfo( headerID );
        if ( pHeaderInfo == nullptr )
        {
            return CXChildVisit_Continue;
        }

        //-------------------------------------------------------------------------

        // Process Cursor
        CXCursorKind const kind = clang_getCursorKind( cr );
		std::string const cursorName = ClangUtils::GetCursorDisplayName( cr );
        switch ( kind )
        {
            // Classes / Structs
            case CXCursor_ClassTemplate:
            {
                return VisitTemplateStructure(pContext, cr, headerFilePath, headerID);
            }
            break;

            // Classes / Structs
            case CXCursor_ClassDecl:
            {
                // Process children before the parent so that we can correctly handle the mapping between macro and types
                // We dont want an nested registration macro to cause an unwanted type to be registered
                pContext->PushStruct( cursorName);
                clang_visitChildren( cr, VisitTranslationUnit, pClientData );
                pContext->PopStruct();

                if (pContext->HasErrorOccured())
                {
                    return CXChildVisit_Break;
                }

                return VisitStructure( pContext, cr, headerFilePath, headerID, false);
            }
            case CXCursor_StructDecl:
            {
                // Process children before the parent so that we can correctly handle the mapping between macro and types
                // We dont want an nested registration macro to cause an unwanted type to be registered
                pContext->PushStruct(cursorName);
                clang_visitChildren( cr, VisitTranslationUnit, pClientData );
                pContext->PopStruct();

                if (pContext->HasErrorOccured() )
                {
                    return CXChildVisit_Break;
                }

                return VisitStructure( pContext, cr, headerFilePath, headerID, true);
            }
            break;

            // Enums
            case CXCursor_EnumDecl:
            {
                return VisitEnum( pContext, cr, headerID );
            }
            break;

            case CXCursor_TypedefDecl:
            case CXCursor_TypeAliasDecl:
            {
                return VisitTypeDef(pContext, cr, headerID);
            }
            break;

            // Non-Type Cursors
            case CXCursor_Namespace:
            {
                pContext->PushNamespace( cursorName);
                clang_visitChildren( cr, VisitTranslationUnit, pClientData );
                pContext->PopNamespace();

                return CXChildVisit_Continue;
            }
            break;

            // Macros
            case CXCursor_MacroExpansion:
            {
                return VisitMacro( pContext, pHeaderInfo, cr, cursorName );
            }
            break;

            // Irrelevant Cursors
            default:
            {
                return CXChildVisit_Continue;
            }
        }
    }
}

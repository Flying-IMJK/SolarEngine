#include "ClangVisitors_TranslationUnit.h"
#include "ClangVisitors_Macro.h"
#include "ClangVisitors_Enum.h"
#include "Clangvisitors_Structure.h"

//-------------------------------------------------------------------------

namespace SE::ReflectTool
{
    CXChildVisitResult VisitTranslationUnit( CXCursor cr, CXCursor parent, CXClientData pClientData )
    {
        auto pContext = static_cast<ClangParserContext*>( pClientData );
        if ( pContext->HasErrorOccured() )
        {
            return CXChildVisit_Break;
        }

		String headerFilePath = ClangUtils::GetHeaderPathForCursor( cr );
        if (headerFilePath.IsEmpty())
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
		String const cursorName = ClangUtils::GetCursorDisplayName( cr ).ToString();
        switch ( kind )
        {
            // Classes / Structs
            case CXCursor_ClassTemplate:
            {
                MarkMacro macro;
                if (pContext->FindMarkMacro(headerID, cr, macro, ReflectionMacroType::SEClass))
                {
                    pContext->LogError(SE_TEXT("Cannot register template class ({0})"), cursorName.Get() );
                    return CXChildVisit_Break;
                }

                return CXChildVisit_Continue;
            }
            break;

            // Classes / Structs
            case CXCursor_ClassDecl:
            {
                // Process children before the parent so that we can correctly handle the mapping between macro and types
                // We dont want an nested registration macro to cause an unwanted type to be registered
                pContext->PushNamespace( cursorName);
                clang_visitChildren( cr, VisitTranslationUnit, pClientData );
                pContext->PopNamespace();

                if ( pContext->HasErrorOccured() )
                {
                    return CXChildVisit_Break;
                }

                return VisitStructure( pContext, cr, headerFilePath, headerID, false);
            }
            case CXCursor_StructDecl:
            {
                // Process children before the parent so that we can correctly handle the mapping between macro and types
                // We dont want an nested registration macro to cause an unwanted type to be registered
                pContext->PushNamespace( cursorName);
                clang_visitChildren( cr, VisitTranslationUnit, pClientData );
                pContext->PopNamespace();

                if ( pContext->HasErrorOccured() )
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

            // Non-Type Cursors
            case CXCursor_Namespace:
            {
                if ( pContext->IsInEngineNamespace() || pContext->IsEngineNamespace(cursorName) )
                {
                    pContext->PushNamespace( cursorName);
                    clang_visitChildren( cr, VisitTranslationUnit, pClientData );
                    pContext->PopNamespace();
                }

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
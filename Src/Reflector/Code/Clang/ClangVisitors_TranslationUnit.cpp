#include "Clangvisitors.h"

//-------------------------------------------------------------------------

namespace SE::ReflectTool
{
    CXChildVisitResult VisitTranslationUnit(CXCursor cr, CXCursor parent, CXClientData pClientData)
    {
        ClangParserContext* pContext = static_cast<ClangParserContext*>( pClientData );
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
		String const cursorName = ClangUtils::GetCursorDisplayName( cr );
        switch ( kind )
        {
            // Classes / Structs
            case CXCursor_ClassTemplate:
            {
                ReflectionMacro macro;
                if (pContext->FindReflectionMacroForType(headerID, cr, macro))
                {
                    pContext->LogError(SE_TEXT("Cannot register template class ({0})"), cursorName.Get() );
                    return CXChildVisit_Break;
                }

                return CXChildVisit_Continue;
            }

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

            // Enums
            case CXCursor_EnumDecl:
            {
                return VisitEnum( pContext, cr, headerID );
            }

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

            // Macros
            case CXCursor_MacroExpansion:
            {
                return VisitMacro( pContext, pHeaderInfo, cr, cursorName );
            }

            // Irrelevant Cursors
            default:
            {
                return CXChildVisit_Continue;
            }
        }
    }
}
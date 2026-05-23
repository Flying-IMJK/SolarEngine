#pragma once

#include "ClangParserContext.h"

//-------------------------------------------------------------------------

namespace SE::ReflectTool
{
    CXChildVisitResult VisitTranslationUnit( CXCursor cr, CXCursor parent, CXClientData pClientData );

    CXChildVisitResult VisitEnum( ClangParserContext* pContext, CXCursor cr, HeaderID const headerID );

    CXChildVisitResult VisitMacro( ClangParserContext* pContext, HeaderInfo const* pHeaderInfo, CXCursor cr, String const& cursorName );

    CXChildVisitResult VisitStructure( ClangParserContext* pContext, CXCursor& cr, StringView const& headerFilePath, HeaderID const headerID, bool isStruct);
}
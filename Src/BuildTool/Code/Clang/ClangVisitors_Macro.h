#pragma once

#include "ClangParserContext.h"

//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    CXChildVisitResult VisitMacro( ClangParserContext* pContext, HeaderInfo const* pHeaderInfo, CXCursor cr, std::string const& cursorName );
}
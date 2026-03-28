#pragma once

#include "ClangParserContext.h"

//-------------------------------------------------------------------------

namespace SE::ReflectTool
{
    CXChildVisitResult VisitMacro( ClangParserContext* pContext, HeaderInfo const* pHeaderInfo, CXCursor cr, String const& cursorName );
}
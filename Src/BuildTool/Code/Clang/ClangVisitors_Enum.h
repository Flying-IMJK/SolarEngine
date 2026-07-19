#pragma once

#include "ClangParserContext.h"

//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    CXChildVisitResult VisitEnum( ClangParserContext* pContext, CXCursor cr, HeaderID const headerID );
}
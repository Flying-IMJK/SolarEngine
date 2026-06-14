#pragma once

#include "ClangParserContext.h"

//-------------------------------------------------------------------------

namespace SE::ReflectTool
{
    CXChildVisitResult VisitEnum( ClangParserContext* pContext, CXCursor cr, HeaderID const headerID );
}
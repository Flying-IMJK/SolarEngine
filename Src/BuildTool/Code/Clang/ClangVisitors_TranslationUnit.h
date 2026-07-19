#pragma once

#include "ClangParserContext.h"

//-------------------------------------------------------------------------

namespace SE::BuildTool
{
	CXChildVisitResult VisitTranslationUnit( CXCursor cr, CXCursor parent, CXClientData pClientData );
}
#pragma once

#include "ClangParserContext.h"

//-------------------------------------------------------------------------

namespace SE::ReflectTool
{
	CXChildVisitResult VisitTranslationUnit( CXCursor cr, CXCursor parent, CXClientData pClientData );
}
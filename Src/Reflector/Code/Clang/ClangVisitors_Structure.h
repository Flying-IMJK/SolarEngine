#pragma once

#include "ClangParserContext.h"

//-------------------------------------------------------------------------

namespace SE::ReflectTool
{
	CXChildVisitResult VisitStructure( ClangParserContext* pContext, CXCursor& cr, StringView const& headerFilePath, HeaderID const headerID);
}

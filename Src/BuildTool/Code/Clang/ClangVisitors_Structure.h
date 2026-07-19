#pragma once

#include "ClangParserContext.h"

//-------------------------------------------------------------------------

namespace SE::BuildTool
{
	CXChildVisitResult VisitStructure( ClangParserContext* pContext, CXCursor& cr, std::string_view const& headerFilePath, HeaderID const headerID, bool isStruct);
	CXChildVisitResult VisitTemplateStructure(ClangParserContext* pContext, CXCursor& cr, std::string_view const& headerFilePath, HeaderID const headerID);
}

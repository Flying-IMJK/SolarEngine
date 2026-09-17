#pragma once

#include "ClangParserContext.h"
#include "Database/DataTypes_Template.h"

//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    TypeInfoTemplate ParseTemplateType(ClangParserContext* pContext,
                                         CXType              type,
                                         std::vector<std::string> const& templateParameters);

    std::unique_ptr<TypeInfoStruct> InstantiateTemplateType(ClangParserContext*          pContext,
                                                            TypeInfoStructTemplate const& templateType,
                                                            ClangParserContext::TypeDefData const& typeDef);
} // namespace SE::BuildTool

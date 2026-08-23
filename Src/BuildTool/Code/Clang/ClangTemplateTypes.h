#pragma once

#include "ClangParserContext.h"
#include "Database/DataTypes_Template.h"

//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    TypeRefTemplate ParseTemplateTypeRef(ClangParserContext* pContext,
                                         CXType              type,
                                         std::vector<std::string> const& templateParameters);

    bool TryParseTemplateTypeRef(std::string const& typeText,
                                 TypeRefTemplate&   outType,
                                 std::vector<std::string> const& templateParameters = {});

    std::unique_ptr<TypeInfoStruct> InstantiateTemplateType(ClangParserContext*          pContext,
                                                            TypeInfoStructTemplate const& templateType,
                                                            ClangParserContext::TypeDefData const& typeDef);
} // namespace SE::BuildTool

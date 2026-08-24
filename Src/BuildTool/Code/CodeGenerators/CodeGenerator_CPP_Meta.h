#pragma once

#include "../Database/TypeDatabase.h"
#include <ThirdParty/mustache.hpp>

using namespace kainjow;

namespace SE::BuildTool
{
    class Generator;

    void CppGenerateMeta(Generator*          generator,
                         TypeDatabase const& database,
                         std::stringstream&  codeFile,
                         TypeInfoBase const& type,
                         std::string         templateStr);

    void CppParseMeta(Generator* generator, mustache::data& metaList, std::string const& metaContext);
}
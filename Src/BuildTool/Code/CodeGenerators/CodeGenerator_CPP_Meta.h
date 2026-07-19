#pragma once

#include "../Database/ReflectionDatabase.h"
#include "../mustache.hpp"

using namespace kainjow;

namespace SE::BuildTool
{
    class Generator;

    void CppGenerateMeta(Generator* generator, ReflectionDatabase const& database, std::stringstream& codeFile, TypeData const& type, std::string templateStr);

    void CppParseMeta(Generator* generator, mustache::data& metaList, std::string const& metaContext);
}
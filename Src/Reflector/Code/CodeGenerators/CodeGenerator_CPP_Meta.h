#pragma once

#include "../Database/ReflectionDatabase.h"
#include "../mustache.hpp"

using namespace kainjow;

namespace SE::ReflectTool
{
    class Generator;

    void CppGenerateMeta(Generator* generator, ReflectionDatabase const& database, std::stringstream& codeFile, DataType const& type, std::string templateStr);

    void CppParseMeta(Generator* generator, mustache::data& metaList, StringAnsi const& metaContext);
}
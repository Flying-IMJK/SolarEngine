#pragma once

#include "../Database/ReflectionDatabase.h"
#include "../mustache.hpp"
#include <sstream>

using namespace kainjow;

namespace SE::ReflectTool
{
    class Generator;

    void CppGenerateType(Generator* generator, ReflectionDatabase const& database, std::stringstream& codeFile, String const& exportMacro, ReflectedType const& type, ReflectedType const& parentType, std::string templateStr);
}
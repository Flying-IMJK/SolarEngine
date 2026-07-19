#pragma once
#include "../Database/ReflectionDatabase.h"
#include "../mustache.hpp"
#include <sstream>

using namespace kainjow;

namespace SE::BuildTool
{
    class Generator;

    void CppGenerateEnum(Generator* generator, std::stringstream &codeFile, std::string const &exportMacro, TypeData const &type, std::string templateStr);
}
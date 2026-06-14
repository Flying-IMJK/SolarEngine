#pragma once
#include "../Database/ReflectionDatabase.h"
#include "../mustache.hpp"
#include <sstream>

using namespace kainjow;

namespace SE::ReflectTool
{
    class Generator;

    void CppGenerateEnum(Generator* generator, std::stringstream &codeFile, String const &exportMacro, DataType const &type, std::string templateStr);
}
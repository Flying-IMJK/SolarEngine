#pragma once
#include "../Database/TypeDatabase.h"
#include <ThirdParty/mustache.hpp>     
#include <sstream>

using namespace kainjow;

namespace SE::BuildTool
{
    class Generator;

    void CppGenerateEnum(Generator* generator, std::stringstream &codeFile, std::string const &exportMacro, TypeInfoEnum const &type, std::string templateStr);
}
#pragma once

#include <string>

//-------------------------------------------------------------------------

namespace SE::BuildTool
{
	struct TypeData;
	class ReflectionDatabase;
	class Generator;

	void CppGenerateType(Generator* generator,  ReflectionDatabase const& database, std::stringstream& codeFile, std::string const& exportMacro,
            TypeData const& type, TypeData const& parentType, std::string templateStr);
}
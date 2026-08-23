#pragma once

#include <string>

//-------------------------------------------------------------------------

namespace SE::BuildTool
{
	struct TypeInfoBase;
	class TypeDatabase;
	class Generator;

	void CppGenerateType(Generator* generator,  TypeDatabase const& database, std::stringstream& codeFile, std::string const& exportMacro,
                         TypeInfoStruct const&    type,
                         TypeInfoStruct const&     parentType,
                         std::string               templateStr);
}
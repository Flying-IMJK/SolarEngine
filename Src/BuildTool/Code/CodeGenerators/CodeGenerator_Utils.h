#pragma once

#include <Core/String.h>
#include "Database/DataTypes.h"

namespace SE::BuildTool
{
	struct ApiClass;
	struct ApiInjectedCode;
	struct ApiParam;

	namespace CodeGeneratorUtils
	{
		std::string GetNativeName(const std::vector<std::string>& nameSpaceName, const std::vector<std::string>& structScopes, const std::string& name);

		std::string GetFullCSTypeName(const std::vector<std::string>& nameSpaceName, const std::string& name);

		std::string GetFullCSNameSpaceName(const std::vector<std::string>& nameSpaceName);

		std::string GetFullCTypeName(const std::vector<std::string>& nameSpaceName, const std::string& name);

		std::string GetFullCNameSpaceName(const std::vector<std::string>& nameSpaceName);

		std::string GetInternalClassName(const std::string& name);

		std::string QualifyCppType(const std::string& type);

		std::string DeriveAssemblyCSharpType(std::string const& assemblyName);

		std::string RemovePreNameSpace(std::string & namespaceName);

		std::string GetAccessString(AccessLevel access);

		// C# source composition helpers shared by binding generators.
		bool IsCSharpCode(const ApiInjectedCode& code);
		std::string MakeCSharpIdentifier(const std::string& identifier);
		std::string EscapeCSharpXml(const std::string& text);
		void AppendCSharpComment(std::string& output, const std::string& indent, const std::string& comment);
		bool IsValidCSharpAttributeList(const std::string& attributes);
		void AppendCSharpLibraryImport(std::string& output, const std::string& assemblyName, const std::string& entryPoint);
		bool UsesCSharpOutResult(const std::string& cppType);
		std::string GetCSharpStructAbiFieldType(const std::string& cppType);
		std::string GetCSharpCollectionCountExpression(const std::string& cppType, const std::string& expression);
		std::string GetCSharpStructFieldFromAbi(const std::string& cppType, const std::string& expression);
		std::string GetCSharpStructFieldToAbi(const std::string& cppType, const std::string& expression);
		std::string NormalizeCSharpDefaultValue(const ApiParam& param);

		bool SaveFile(const std::string& path, const std::string& content);

	}
}

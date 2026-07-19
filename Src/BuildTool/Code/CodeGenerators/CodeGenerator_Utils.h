#pragma once

#include <Core/String.h>

namespace SE::BuildTool
{
	struct ApiClass;

	namespace CodeGeneratorUtils
	{
		std::string GetNativeName(const std::vector<std::string>& nameSpaceName, const std::vector<std::string>& structScopes, const std::string& name);

		std::string GetFullCSTypeName(const std::vector<std::string>& nameSpaceName, const std::string& name);

		std::string GetFullCSNameSpaceName(const std::vector<std::string>& nameSpaceName);

		std::string GetFullCTypeName(const std::vector<std::string>& nameSpaceName, const std::string& name);

		std::string GetFullCNameSpaceName(const std::vector<std::string>& nameSpaceName);

		std::string GetInternalClassName(const ApiClass& cls);

		std::string QualifyCppType(const std::string& type);

		std::string DeriveAssemblyCSharpType(std::string const& assemblyName);

		std::string RemovePreNameSpace(std::string & namespaceName);

	}
}

#include "CodeGenerator_Utils.h"

#include "Database/DataTypes.h"

namespace SE::BuildTool::CodeGeneratorUtils
{
	std::string GetNativeName(const std::vector<std::string>& nameSpaceName, const std::vector<std::string>& structScopes, const std::string& name)
	{
		if (!nameSpaceName.empty() && !structScopes.empty())
		{
			return Utils::String::Format("{0}::{1}::{2}", Utils::CombineStringList(nameSpaceName, "::"), Utils::CombineStringList(structScopes, "::"), name);
		}
		else if (!nameSpaceName.empty())
		{
			return Utils::String::Format("{0}::{1}", Utils::CombineStringList(nameSpaceName, "::"), name);
		}
		else if (!structScopes.empty())
		{
			return Utils::String::Format("{0}::{1}", Utils::CombineStringList(structScopes, "::"), name);
		}

		return name;
	}

	std::string GetFullCSTypeName(const std::vector<std::string>& nameSpaceName, const std::string& name)
	{
		if (!nameSpaceName.empty())
		{
			return Utils::String::Format("{0}.{1}", Utils::CombineStringList(nameSpaceName, "."), name);
		}
		return name;
	}

	std::string GetFullCSNameSpaceName(const std::vector<std::string>& nameSpaceName)
	{
		if (!nameSpaceName.empty())
		{
			return Utils::CombineStringList(nameSpaceName, ".");
		}

		return "";
	}

	std::string GetFullCTypeName(const std::vector<std::string>& nameSpaceName, const std::string& name)
	{
		if (!nameSpaceName.empty())
		{
			return Utils::String::Format("{0}:{1}", Utils::CombineStringList(nameSpaceName, "::"), name);
		}
		return name;
	}

	std::string GetFullCNameSpaceName(const std::vector<std::string>& nameSpaceName)
	{
		if (!nameSpaceName.empty())
		{
			return Utils::CombineStringList(nameSpaceName, "::");
		}
		return "";
	}

	std::string GetInternalClassName(const ApiClass& cls)
	{
		return Utils::String::Format("{0}Internal", cls.name);
	}

    std::string QualifyCppType(const std::string& type)
    {
		std::string result = type;
		if (Utils::String::StartsWith(result, "SE::"))
		{
			result = "::" + result;
		}

		int pos;
		while ((pos = Utils::String::Find(result, " SE::")) != INVALID_INDEX)
		{
			result = result.substr(0, pos + 1) + "::" + result.substr(pos + 1);
		}
		while ((pos = Utils::String::Find(result, "<SE::")) != INVALID_INDEX)
		{
			result = result.substr(0, pos + 1) + "::" + result.substr(pos + 1);
		}
		while ((pos = Utils::String::Find(result, ",SE::")) != INVALID_INDEX)
		{
			result = result.substr(0, pos + 1) + "::" + result.substr(pos + 1);
		}
		while ((pos = Utils::String::Find(result, ", SE::")) != INVALID_INDEX)
		{
			result = result.substr(0, pos + 2) + "::" + result.substr(pos + 2);
		}
		while ((pos = Utils::String::Find(result, "(SE::")) != INVALID_INDEX)
		{
			result = result.substr(0, pos + 1) + "::" + result.substr(pos + 1);
		}

		return result;
    }

    std::string DeriveAssemblyCSharpType(std::string const& assemblyName)
	{
		std::string assemblyType = assemblyName;
		if (Utils::String::StartsWith(assemblyType, "SE."))
		{
			assemblyType = assemblyType.substr(3);
		}
		return assemblyType;
	}

    std::string RemovePreNameSpace(std::string& namespaceName)
    {
		if (!namespaceName.empty() && Utils::String::Contains(namespaceName, "SE::"))
		{
			return namespaceName.substr(4);
		}

		return namespaceName;
    }
}

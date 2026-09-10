#include "CodeGenerator_Utils.h"

#include <fstream>

#include "CodeGenerator_BindingsDataTypes.h"
#include "CodeGenerator_BindingsTypeMap.h"
#include "Database/DataTypes.h"

namespace SE::BuildTool::CodeGeneratorUtils
{
	std::string GetFullNativeName(const std::vector<std::string>& nameSpaceName, const std::vector<std::string>& structScopes, const std::string& name, bool useGlobal)
	{
		if (!nameSpaceName.empty() && !structScopes.empty())
		{
            if (useGlobal)
            {
				return Utils::String::Format("::{0}::{1}::{2}", Utils::CombineStringList(nameSpaceName, "::"), Utils::CombineStringList(structScopes, "::"), name);
            }
            else
            {
				return Utils::String::Format("{0}::{1}::{2}", Utils::CombineStringList(nameSpaceName, "::"), Utils::CombineStringList(structScopes, "::"), name);
            }
		}
		else if (!nameSpaceName.empty())
		{
			if (useGlobal)
            {
				return Utils::String::Format("::{0}::{1}", Utils::CombineStringList(nameSpaceName, "::"), name);
			}
            else
            {
				return Utils::String::Format("{0}::{1}", Utils::CombineStringList(nameSpaceName, "::"), name);
            }
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

	std::string GetInternalClassName(const std::string& name)
	{
		return Utils::String::Format("{0}Internal", name);
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

	std::string GetAccessString(AccessLevel access)
	{
		switch (access)
		{
		case AccessLevel::Private:   return "private";
		case AccessLevel::Protected: return "protected";
		case AccessLevel::Public:    return "public";
		case AccessLevel::Internal:  return "internal";
		default:                     return "public";
		}
    }

	TypeInfo WithoutArray(TypeInfo type)
	{
		type.arraySize = 0;
		return type;
	}

	std::string GetPropertyName(TypeInfoFunc const& function)
	{
		if ((Utils::String::StartsWith(function.name, "Get") || Utils::String::StartsWith(function.name, "Set"))
			&& function.name.length() > 3)
		{
			return function.name.substr(3);
		}
		return function.name;
	}

	std::string MakeCSharpIdentifier(const std::string& identifier)
	{
		static const char* keywords[] =
		{
			"abstract", "as", "base", "bool", "break", "byte", "case", "catch", "char",
			"checked", "class", "const", "continue", "decimal", "default", "delegate",
			"do", "double", "else", "enum", "event", "explicit", "extern", "false",
			"finally", "fixed", "float", "for", "foreach", "goto", "if", "implicit",
			"in", "int", "interface", "internal", "is", "lock", "long", "namespace",
			"new", "null", "object", "operator", "out", "override", "params", "private",
			"protected", "public", "readonly", "ref", "return", "sbyte", "sealed",
			"short", "sizeof", "stackalloc", "static", "string", "struct", "switch",
			"this", "throw", "true", "try", "typeof", "uint", "ulong", "unchecked",
			"unsafe", "ushort", "using", "virtual", "void", "volatile", "while", nullptr
		};

		for (int i = 0; keywords[i] != nullptr; i++)
		{
			if (identifier == keywords[i])
				return Utils::String::Format("@{0}", identifier);
		}
		return identifier;
	}

	std::string EscapeCSharpXml(const std::string& text)
	{
		std::string result = text;
		Utils::String::ReplaceAll(result, "&", "&amp;");
		Utils::String::ReplaceAll(result, "<", "&lt;");
		Utils::String::ReplaceAll(result, ">", "&gt;");
		return result;
	}

	void AppendCSharpComment(std::string& output, const std::string& indent, const std::string& comment)
	{
		if (comment.empty())
			return;
		output += Utils::String::Format("{0}/// <summary>\n", indent);
		output += Utils::String::Format("{0}/// {1}\n", indent, EscapeCSharpXml(comment));
		output += Utils::String::Format("{0}/// </summary>\n", indent);
	}

	bool IsValidCSharpAttributeList(const std::string& attributes)
	{
		if (attributes.empty() || !Utils::String::StartsWith(attributes, "["))
			return false;

		const char* raw = attributes.c_str();
		for (int i = 0; i < attributes.length(); ++i)
		{
			const unsigned char ch = (unsigned char)raw[i];
			if (ch < 32 && ch != '\r' && ch != '\n' && ch != '\t')
				return false;
		}
		return true;
	}

	void AppendCSharpLibraryImport(std::string& output, const std::string& assemblyName, const std::string& entryPoint)
	{
		output += Utils::String::Format(
			"        [LibraryImport(\"{0}\", EntryPoint = \"{1}\", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(Interop.StringMarshaller))]\n",
			assemblyName, entryPoint);
	}

	bool SaveFile(const std::string& path, const std::string& content)
	{
		std::string parentDir = FileSystem::GetParentDirectory(path);
		if (!FileSystem::DirectoryExists(parentDir))
			FileSystem::CreateDirectory(parentDir);

		std::string existing;
		if (FileSystem::FileExists(path) && Utils::ReadAllText(path, existing) && existing == content.c_str())
		{
			return true;
		}

		std::string pathAnsi(path);
		std::ofstream f(pathAnsi.c_str(), std::ios::out | std::ios::trunc);
		if (!f.is_open())
			return false;
		f << content;
		return true;
	}
}

#include "CodeGenerator_CPP_Meta.h"

#include "CodeGenerator_Utils.h"

namespace SE::BuildTool
{
	static mustache::data GenerateFile(TypeData const &type)
	{
		mustache::data generatorData;

		if (type.isDevOnly)
		{
			generatorData.set("isDevOnlyBegin", "#ifdef SGE_DEVELOPMENT");
			generatorData.set("isDevOnlyEnd", "#endif");
		}

		std::string namespaceName = CodeGeneratorUtils::GetFullCNameSpaceName(type.namespaceScopeList);

		generatorData.set("namespace", namespaceName);
		generatorData.set("typeName", type.name.c_str());
		generatorData.set("typeIDUint", std::to_string(type.typeID));

		return generatorData;
	}

	void CppGenerateMeta(Generator* generator, ReflectionDatabase const& database, std::stringstream& codeFile, TypeData const& type, std::string templateStr)
	{
		ENGINE_ASSERT(type.IsFlag(TypeData::Flags::IsMeta));

		mustache::data data = GenerateFile(type);
		mustache::mustache tmpl(templateStr);

		codeFile << tmpl.render(data);
	}

	void CppParseMeta(Generator* generator, mustache::data& metaList, std::string const& metaContext)
	{
		/*TypeMetaInfo const* metaInfo = Types::GetMetaTypeInfo("SG::{{metaType}}");
		if (metaInfo == nullptr)
		{
			rapidjson::Document document;
			document.Parse(metaContext.c_str(), metaContext.length());
			if (document.GetParseError() == rapidjson::kParseErrorNone)
			{
				Json::Array metaDatas = document.GetArray();

				TypeMetaAttribute* meta = metaInfo->Create();
				meta->Parse(metaDatas);
			}
			else
			{
				LOG_ERROR("Types", "Invalid metadata type context", SE_TEXT("SG::{{metaType}}"));
			}
		}
		else
		{
			LOG_ERROR("Types", "Invalid metadata type", SE_TEXT("SG::{{metaType}}"));
		}*/
	}
}


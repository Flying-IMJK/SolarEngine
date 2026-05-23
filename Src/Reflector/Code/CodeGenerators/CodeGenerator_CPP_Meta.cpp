#include "CodeGenerator_CPP_Meta.h"

#include <Core/TypeSystem/Types.h>
#include <Core/TypeSystem/Info/TypeMetaInfo.h>
#include <core/typesystem/metadata/typemetaattribute.h>

namespace SE::ReflectTool
{
	static mustache::data GenerateFile(ReflectedType const &type)
	{
		mustache::data generatorData;

		if (type.m_isDevOnly)
		{
			generatorData.set("isDevOnlyBegin", "#ifdef SGE_DEVELOPMENT");
			generatorData.set("isDevOnlyEnd", "#endif");
		}

		generatorData.set("namespace", type.namespaceName.Get());
		generatorData.set("typeName", type.name.Get());
		generatorData.set("typeIDUint", std::to_string((type.typeID)));

		return generatorData;
	}

	void CppGenerateMeta(Generator* generator, ReflectionDatabase const& database, std::stringstream& codeFile, ReflectedType const& type, std::string templateStr)
	{
		ENGINE_ASSERT(type.IsMeta());

		mustache::data data = GenerateFile(type);
		mustache::mustache tmpl(templateStr);

		codeFile << tmpl.render(data);
	}

	void CppParseMeta(Generator* generator, mustache::data& metaList, StringAnsi const& metaContext)
	{
		/*TypeMetaInfo const* metaInfo = Types::GetMetaTypeInfo("SG::{{metaType}}");
		if (metaInfo == nullptr)
		{
			rapidjson::Document document;
			document.Parse(metaContext.Get(), metaContext.Length());
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



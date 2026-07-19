
#include "JsonStorageProxy.h"
#include "Runtime/Core/Platform/File.h"
#include "Runtime/Core/Logging/Logging.h"
#include "Runtime/Core/Serialization/Json.h"
#include "Runtime/Core/Serialization/JsonTools.h"
#include "Runtime/Core/Serialization/JsonWriters.hpp"
#include "Runtime/Core/Logging/Exceptions/JsonParseException.h"
#include "Runtime/Core/Profiler/ProfilerCPU.h"

namespace SE
{
	bool JsonStorageProxy::IsValidExtension(const StringView& extension)
	{
		return extension == SE_TEXT("scene") || extension == SE_TEXT("prefab") || extension == SE_TEXT("json");
	}

	bool JsonStorageProxy::GetAssetInfo(const StringView& path, UID& resultId, TypeID& resultDataTypeID)
	{
		PROFILE_CPU();
		// TODO: we could just open file and start reading until we find 'ID:..' without parsing whole file - could be much more faster

		// Load file
		List<byte> fileData;
		if (!File::ReadAllBytes(path, fileData))
		{
			return false;
		}

		// Parse data
		Json::Document document;
		{
			PROFILE_CPU_NAMED("Json.Parse");
			document.Parse((const char*)fileData.Get(), fileData.Count());
		}
		if (document.HasParseError())
		{
			Log::JsonParseException(document.GetParseError(), document.GetErrorOffset(), path);
			return false;
		}

		// Try get asset metadata
		auto idNode = document.FindMember("ID");
		auto typeNameNode = document.FindMember("TypeID");
		if (idNode != document.MemberEnd() && typeNameNode != document.MemberEnd())
		{
			// Found
			resultId = JsonTools::GetGuid(idNode->value);
			resultDataTypeID = TypeID(typeNameNode->value.GetUint());
			return true;
		}

		return false;
	}

	#if SE_EDITOR

	void ChangeIds(Json::Value& obj, Json::Document& document, const StringAnsi& srcId, const StringAnsi& dstId)
	{
		if (obj.IsObject())
		{
			for (Json::Value::MemberIterator i = obj.MemberBegin(); i != obj.MemberEnd(); ++i)
			{
				ChangeIds(i->value, document, srcId, dstId);
			}
		}
		else if (obj.IsArray())
		{
			for (rapidjson::SizeType i = 0; i < obj.Size(); i++)
			{
				ChangeIds(obj[i], document, srcId, dstId);
			}
		}
		else if (obj.IsString())
		{
			if (StringUtils::Compare(srcId.Get(), obj.GetString()) == 0)
			{
				obj.SetString(dstId.Get(), document.GetAllocator());
			}
		}
	}

	#endif

	bool JsonStorageProxy::ChangeId(const StringView& path, const UID& newId)
	{
	#if SE_EDITOR
		PROFILE_CPU();

		// Load file
		List<byte> fileData;
		if (!File::ReadAllBytes(path, fileData))
		{
			return false;
		}

		// Parse data
		Json::Document document;
		{
			PROFILE_CPU_NAMED("Json.Parse");
			document.Parse((const char*)fileData.Get(), fileData.Count());
		}
		if (document.HasParseError())
		{
	//        Log::JsonParseException(document.GetParseError(), document.GetErrorOffset(), path);
			return false;
		}

		// Try get asset metadata
		auto idNode = document.FindMember("ID");
		if (idNode == document.MemberEnd())
		{
			return true;
		}

		// Change IDs
		auto oldIdStr = idNode->value.GetString();
		auto newIdStr = newId.ToString(UID::FormatType::N).ToStringAnsi();
		ChangeIds(document, document, oldIdStr, newIdStr);

		// Save to file
		Json::StringBuffer buffer;
		PrettyJsonWriter writer(buffer);
		document.Accept(writer.GetWriter());
		if (File::WriteAllBytes(path, (byte*)buffer.GetString(), (int32)buffer.GetSize()))
		{
			return true;
		}

		return false;

	#else

		LOG_WARNING("Resource", "Editing cooked content is invalid.");
		return true;

	#endif
	}
}
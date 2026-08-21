#include "SLC2Reader.h"

#include "Runtime/Core/Serialization/Json.h"
#include "Runtime/Core/Serialization/JsonTools.h"

namespace SE
{
	namespace
	{
		bool RequireObjectMember(const Json::Value& object, const char* name, Json::Value::ConstMemberIterator& member, String& error)
		{
			if (!object.IsObject())
			{
				error = SE_TEXT("SLC2 node is not an object.");
				return false;
			}
			member = object.FindMember(name);
			if (member == object.MemberEnd())
			{
				error = String(SE_TEXT("SLC2 missing field: ")) + StringAnsi(name).ToString();
				return false;
			}
			return true;
		}

		bool RequireString(const Json::Value& object, const char* name, String& error)
		{
			Json::Value::ConstMemberIterator member;
			if (!RequireObjectMember(object, name, member, error))
			{
				return false;
			}
			if (!member->value.IsString())
			{
				error = String(SE_TEXT("SLC2 field must be string: ")) + StringAnsi(name).ToString();
				return false;
			}
			return true;
		}

		bool RequireArray(const Json::Value& object, const char* name, Json::Value::ConstMemberIterator& member, String& error)
		{
			if (!RequireObjectMember(object, name, member, error))
			{
				return false;
			}
			if (!member->value.IsArray())
			{
				error = String(SE_TEXT("SLC2 field must be array: ")) + StringAnsi(name).ToString();
				return false;
			}
			return true;
		}

		bool ValidateLayout(const Json::Value& layout, String& error)
		{
			if (!layout.IsObject())
			{
				error = SE_TEXT("SLC2 layout must be an object.");
				return false;
			}
			if (!RequireString(layout, "pipelineLayoutFingerprint", error))
			{
				return false;
			}

			Json::Value::ConstMemberIterator types;
			if (!RequireArray(layout, "types", types, error))
			{
				return false;
			}

			Json::Value::ConstMemberIterator blocks;
			if (!RequireArray(layout, "parameterBlocks", blocks, error))
			{
				return false;
			}

			return true;
		}

		bool ValidateStage(const Json::Value& stage, String& error)
		{
			if (!RequireString(stage, "stage", error))
			{
				return false;
			}
			if (!RequireString(stage, "entryPoint", error))
			{
				return false;
			}
			if (!RequireString(stage, "code", error))
			{
				return false;
			}
			return true;
		}
	}

	bool SLC2Reader::ReadAndValidate(const byte* data, const int32 length, String& error)
	{
		if (data == nullptr || length <= 0)
		{
			error = SE_TEXT("SLC2 data is empty.");
			return false;
		}

		Json::Document document;
		document.Parse((const char*)data, length);
		if (document.HasParseError() || !document.IsObject())
		{
			error = SE_TEXT("SLC2 data is not valid JSON.");
			return false;
		}

		if (JsonTools::GetString(document, "format") != SE_TEXT("SLC2"))
		{
			error = SE_TEXT("SLC2 format field mismatch.");
			return false;
		}
		if (JsonTools::GetInt(document, "version", 0) != 2)
		{
			error = SE_TEXT("SLC2 version field mismatch.");
			return false;
		}

		Json::Value::ConstMemberIterator programs;
		if (!RequireArray(document, "programs", programs, error))
		{
			return false;
		}
		if (programs->value.Empty())
		{
			error = SE_TEXT("SLC2 must contain at least one program.");
			return false;
		}

		for (auto programIt = programs->value.Begin(); programIt != programs->value.End(); ++programIt)
		{
			if (!RequireString(*programIt, "programId", error))
			{
				return false;
			}

			Json::Value::ConstMemberIterator targets;
			if (!RequireArray(*programIt, "targets", targets, error))
			{
				return false;
			}
			if (targets->value.Empty())
			{
				error = SE_TEXT("SLC2 program must contain at least one target.");
				return false;
			}

			for (auto targetIt = targets->value.Begin(); targetIt != targets->value.End(); ++targetIt)
			{
				if (!RequireString(*targetIt, "targetKey", error))
				{
					return false;
				}

				Json::Value::ConstMemberIterator variants;
				if (!RequireArray(*targetIt, "variants", variants, error))
				{
					return false;
				}
				if (variants->value.Empty())
				{
					error = SE_TEXT("SLC2 target must contain at least one variant.");
					return false;
				}

				for (auto variantIt = variants->value.Begin(); variantIt != variants->value.End(); ++variantIt)
				{
					if (!RequireString(*variantIt, "variant", error))
					{
						return false;
					}

					Json::Value::ConstMemberIterator layout;
					if (!RequireObjectMember(*variantIt, "layout", layout, error))
					{
						return false;
					}
					if (!ValidateLayout(layout->value, error))
					{
						return false;
					}

					Json::Value::ConstMemberIterator stages;
					if (!RequireArray(*variantIt, "stages", stages, error))
					{
						return false;
					}
					if (stages->value.Empty())
					{
						error = SE_TEXT("SLC2 variant must contain at least one stage.");
						return false;
					}
					for (auto stageIt = stages->value.Begin(); stageIt != stages->value.End(); ++stageIt)
					{
						if (!ValidateStage(*stageIt, error))
						{
							return false;
						}
					}
				}
			}
		}

		return true;
	}

	bool SLC2Reader::ReadAndValidate(const List<byte>& data, String& error)
	{
		return ReadAndValidate(data.Get(), data.Count(), error);
	}
}

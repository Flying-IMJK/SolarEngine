
#include "JsonTools.h"
#include "ISerializable.h"
#include "Runtime/Core/Types/Collections/Dictionary.h"
//#include "Core/Types/CommonValue.h"
#include "Runtime/Core/Types/DateTime.h"
#include "Runtime/Core/Profiler/ProfilerCPU.h"
//#include "Engine/Scripting/ScriptingObjectReference.h"
//#include "Engine/Utilities/Encryption.h"

namespace SE
{
	void ChangeIdsInternal(Json::Value& obj, Json::Document& document, const Dictionary<UID, UID>& mapping)
	{
		if (obj.IsObject())
		{
			for (Json::Value::MemberIterator i = obj.MemberBegin(); i != obj.MemberEnd(); ++i)
			{
				ChangeIdsInternal(i->value, document, mapping);
			}
		}
		else if (obj.IsArray())
		{
			for (rapidjson::SizeType i = 0; i < obj.Size(); i++)
			{
				ChangeIdsInternal(obj[i], document, mapping);
			}
		}
		else if (obj.IsString() && obj.GetStringLength() == 32)
		{
			auto value = JsonTools::GetGuid(obj);
			if (mapping.TryGet(value, value))
			{
				// Unoptimized version:
				//obj.SetString(value.ToString(SGUID::FormatType::N).ToSTD().c_str(), 32, document.GetAllocator());

				// Optimized version:
				char buffer[32] =
					{
						// @formatter:off
					'0','0','0','0','0','0','0','0','0','0',
					'0','0','0','0','0','0','0','0','0','0',
					'0','0','0','0','0','0','0','0','0','0',
					'0','0'
					// @formatter:on
					};
				static const char* digits = "0123456789abcdef";
				uint32 n = value.A;
				char* p = buffer + 7;
				do
				{
					*p-- = digits[n & 0xf];
				} while ((n >>= 4) != 0);
				n = value.B;
				p = buffer + 15;
				do
				{
					*p-- = digits[n & 0xf];
				} while ((n >>= 4) != 0);
				n = value.C;
				p = buffer + 23;
				do
				{
					*p-- = digits[n & 0xf];
				} while ((n >>= 4) != 0);
				n = value.D;
				p = buffer + 31;
				do
				{
					*p-- = digits[n & 0xf];
				} while ((n >>= 4) != 0);
				obj.SetString(buffer, 32, document.GetAllocator());
			}
		}
	}

	void JsonTools::ChangeIds(Document& doc, const Dictionary<UID, UID>& mapping)
	{
		if (mapping.IsEmpty())
			return;
		PROFILE_CPU();
		ChangeIdsInternal(doc, doc, mapping);
	}

	Float2 JsonTools::GetFloat2(const Value& value)
	{
		Float2 result;
		const auto mX = value.FindMember("X");
		const auto mY = value.FindMember("Y");
		result.x = mX != value.MemberEnd() ? mX->value.GetFloat() : 0.0f;
		result.y = mY != value.MemberEnd() ? mY->value.GetFloat() : 0.0f;
		return result;
	}

	Float3 JsonTools::GetFloat3(const Value& value)
	{
		Float3 result;
		const auto mX = value.FindMember("X");
		const auto mY = value.FindMember("Y");
		const auto mZ = value.FindMember("Z");
		result.x = mX != value.MemberEnd() ? mX->value.GetFloat() : 0.0f;
		result.y = mY != value.MemberEnd() ? mY->value.GetFloat() : 0.0f;
		result.z = mZ != value.MemberEnd() ? mZ->value.GetFloat() : 0.0f;
		return result;
	}

	Float4 JsonTools::GetFloat4(const Value& value)
	{
		Float4 result;
		const auto mX = value.FindMember("X");
		const auto mY = value.FindMember("Y");
		const auto mZ = value.FindMember("Z");
		const auto mW = value.FindMember("W");
		result.x = mX != value.MemberEnd() ? mX->value.GetFloat() : 0.0f;
		result.y = mY != value.MemberEnd() ? mY->value.GetFloat() : 0.0f;
		result.z = mZ != value.MemberEnd() ? mZ->value.GetFloat() : 0.0f;
		result.w = mW != value.MemberEnd() ? mW->value.GetFloat() : 0.0f;
		return result;
	}

	Double2 JsonTools::GetDouble2(const Value& value)
	{
		Double2 result;
		const auto mX = value.FindMember("X");
		const auto mY = value.FindMember("Y");
		result.x = mX != value.MemberEnd() ? mX->value.GetDouble() : 0.0;
		result.y = mY != value.MemberEnd() ? mY->value.GetDouble() : 0.0;
		return result;
	}

	Double3 JsonTools::GetDouble3(const Value& value)
	{
		Double3 result;
		const auto mX = value.FindMember("X");
		const auto mY = value.FindMember("Y");
		const auto mZ = value.FindMember("Z");
		result.x = mX != value.MemberEnd() ? mX->value.GetDouble() : 0.0;
		result.y = mY != value.MemberEnd() ? mY->value.GetDouble() : 0.0;
		result.z = mZ != value.MemberEnd() ? mZ->value.GetDouble() : 0.0;
		return result;
	}

	Double4 JsonTools::GetDouble4(const Value& value)
	{
		Double4 result;
		const auto mX = value.FindMember("X");
		const auto mY = value.FindMember("Y");
		const auto mZ = value.FindMember("Z");
		const auto mW = value.FindMember("W");
		result.x = mX != value.MemberEnd() ? mX->value.GetDouble() : 0.0;
		result.y = mY != value.MemberEnd() ? mY->value.GetDouble() : 0.0;
		result.z = mZ != value.MemberEnd() ? mZ->value.GetDouble() : 0.0;
		result.w = mW != value.MemberEnd() ? mW->value.GetDouble() : 0.0;
		return result;
	}

	Color JsonTools::GetColor(const Value& value)
	{
		Color result;
		const auto mR = value.FindMember("R");
		const auto mG = value.FindMember("G");
		const auto mB = value.FindMember("B");
		const auto mA = value.FindMember("A");
		result.r = mR != value.MemberEnd() ? mR->value.GetFloat() : 0.0f;
		result.g = mG != value.MemberEnd() ? mG->value.GetFloat() : 0.0f;
		result.b = mB != value.MemberEnd() ? mB->value.GetFloat() : 0.0f;
		result.a = mA != value.MemberEnd() ? mA->value.GetFloat() : 0.0f;
		return result;
	}

	Quaternion JsonTools::GetQuaternion(const Value& value)
	{
		Quaternion result;
		const auto mX = value.FindMember("X");
		const auto mY = value.FindMember("Y");
		const auto mZ = value.FindMember("Z");
		const auto mW = value.FindMember("W");
		Float4 v;
		v.x = mX != value.MemberEnd() ? mX->value.GetFloat() : 0.0f;
		v.y = mY != value.MemberEnd() ? mY->value.GetFloat() : 0.0f;
		v.z = mZ != value.MemberEnd() ? mZ->value.GetFloat() : 0.0f;
		v.w = mW != value.MemberEnd() ? mW->value.GetFloat() : 0.0f;

		return Quaternion(v);
	}

	Matrix JsonTools::GetMatrix(const Value& value)
	{
		Matrix result;
	    GetFloat(result.M11, value, "M11");
	    GetFloat(result.M12, value, "M12");
	    GetFloat(result.M13, value, "M13");
	    GetFloat(result.M14, value, "M14");
	    GetFloat(result.M21, value, "M21");
	    GetFloat(result.M22, value, "M22");
	    GetFloat(result.M23, value, "M23");
	    GetFloat(result.M24, value, "M24");
	    GetFloat(result.M31, value, "M31");
	    GetFloat(result.M32, value, "M32");
	    GetFloat(result.M33, value, "M33");
	    GetFloat(result.M34, value, "M34");
	    GetFloat(result.M41, value, "M41");
	    GetFloat(result.M42, value, "M42");
	    GetFloat(result.M43, value, "M43");
	    GetFloat(result.M44, value, "M44");
		return result;
	}

	Transform JsonTools::GetTransform(const Value& value)
	{
		return Transform();
/*    return Transform(
        GetVector3(value, "Translation", Vector3::Zero),
        GetQuaternion(value, "Orientation", Quaternion::Identity),
        GetFloat3(value, "Scale", Float3::One)
    );*/
	}

	void JsonTools::GetTransform(Transform& result, const Value& value)
	{
/*    GetVector3(result.Translation, value, "Translation");
    GetQuaternion(result.Orientation, value, "Orientation");
    GetFloat3(result.Scale, value, "Scale");*/
	}

	UID JsonTools::GetGuid(const Value& value)
	{
		if (!value.IsString())
			return UID::Empty;
		ENGINE_ASSERT(value.GetStringLength() == 32);

		// Split
		const char* a = value.GetString();
		const char* b = a + 8;
		const char* c = b + 8;
		const char* d = c + 8;

		// Parse
		UID result;
		StringUtils::ParseHex(a, 8, &result.A);
		StringUtils::ParseHex(b, 8, &result.B);
		StringUtils::ParseHex(c, 8, &result.C);
		StringUtils::ParseHex(d, 8, &result.D);
		return result;
	}

	DateTime JsonTools::GetDate(const Value& value)
	{
		return DateTime(value.GetInt64());
	}

	DateTime JsonTools::GetDateTime(const Value& value)
	{
		return DateTime(value.GetInt64());
	}

}

#pragma once

#include "Json.h"
#include "Core/Types/Strings/String.h"
#include "Core/Types/UID.h"
#include "Core/Types/DateTime.h"
#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"
#include "Core/Math/Color.h"
#include "Core/Math/Quaternion.h"
#include "Core/Math/Transform.h"
#include "Core/Math/Rectangle.h"
#include "Core/Math/Plane.h"
#include "Core/Math/Matrix.h"
#include "ISerializable.h"

namespace SE
{
	struct CommonValue;

	/// <summary>
	/// Json value container utilities.
	/// </summary>
	class SE_API_CORE JsonTools
	{
	public:
		typedef Json::Document Document;
		typedef Json::Value Value;

	public:
		FORCE_INLINE static void MergeDocuments(Document& target, Document& source)
		{
			MergeObjects(target, source, target.GetAllocator());
		}

		static void MergeObjects(Value& target, Value& source, Value::AllocatorType& allocator)
		{
			ASSERT(target.IsObject() && source.IsObject());
			for (auto itr = source.MemberBegin(); itr != source.MemberEnd(); ++itr)
				target.AddMember(itr->name, itr->value, allocator);
		}

		static void ChangeIds(Document& doc, const Dictionary<UID, UID, HeapAllocation>& mapping);

	public:
		FORCE_INLINE static Float2 GetVector2(const Value& value)
		{
	#if USE_LARGE_WORLDS
			return GetDouble2(value);
	#else
			return GetFloat2(value);
	#endif
		}

		FORCE_INLINE static Float3 GetVector3(const Value& value)
		{
	#if USE_LARGE_WORLDS
			return GetDouble3(value);
	#else
			return GetFloat3(value);
	#endif
		}

		FORCE_INLINE static Float4 GetVector4(const Value& value)
		{
	#if USE_LARGE_WORLDS
			return GetDouble4(value);
	#else
			return GetFloat4(value);
	#endif
		}

		static Float2 GetFloat2(const Value& value);
		static Float3 GetFloat3(const Value& value);
		static Float4 GetFloat4(const Value& value);

		static Double2 GetDouble2(const Value& value);
		static Double3 GetDouble3(const Value& value);
		static Double4 GetDouble4(const Value& value);

		static Color GetColor(const Value& value);
		static Quaternion GetQuaternion(const Value& value);
		static Matrix GetMatrix(const Value& value);
		static Transform GetTransform(const Value& value);
		static void GetTransform(Transform& result, const Value& value);

		static Rectangle GetRectangle(const Value& value)
		{
			return Rectangle();
/*			return Rectangle(
				GetVector2(value, "Location", Float2::Zero),
				GetVector2(value, "Size", Float2::Zero)
			);*/
		}

		static UID GetGuid(const Value& value);
		static DateTime GetDate(const Value& value);
		static DateTime GetDateTime(const Value& value);

	public:
		FORCE_INLINE static String GetString(const Value& node, const char* name)
		{
			auto member = node.FindMember(name);
			return member != node.MemberEnd() ? member->value.GetText() : String::Empty;
		}

	public:
		FORCE_INLINE static bool GetBool(const Value& node, const char* name, const bool defaultValue)
		{
			auto member = node.FindMember(name);
			return member != node.MemberEnd() ? member->value.GetBool() : defaultValue;
		}

		FORCE_INLINE static float GetFloat(const Value& node, const char* name, const float defaultValue)
		{
			auto member = node.FindMember(name);
			return member != node.MemberEnd() && member->value.IsNumber() ? member->value.GetFloat() : defaultValue;
		}

		FORCE_INLINE static int32 GetInt(const Value& node, const char* name, const int32 defaultValue)
		{
			auto member = node.FindMember(name);
			return member != node.MemberEnd() && member->value.IsInt() ? member->value.GetInt() : defaultValue;
		}

		template<class T>
		FORCE_INLINE static T GetEnum(const Value& node, const char* name, const T defaultValue)
		{
			auto member = node.FindMember(name);
			return member != node.MemberEnd() && member->value.IsInt() ? static_cast<T>(member->value.GetInt()) : defaultValue;
		}

		FORCE_INLINE static String GetString(const Value& node, const char* name, const String& defaultValue)
		{
			auto member = node.FindMember(name);
			return member != node.MemberEnd() ? member->value.GetText() : defaultValue;
		}

	#define DECLARE_GETTER(type) \
		FORCE_INLINE static type Get##type(const Value& node, const char* name, const type& defaultValue) \
		{ \
			const auto member = node.FindMember(name); \
			return member != node.MemberEnd() ? Get##type(member->value) : defaultValue; \
		}

		DECLARE_GETTER(Rectangle);
		DECLARE_GETTER(Transform);
/*		DECLARE_GETTER(Vector2);
		DECLARE_GETTER(Vector3);
		DECLARE_GETTER(Vector4);*/
		DECLARE_GETTER(Float2);
		DECLARE_GETTER(Float3);
		DECLARE_GETTER(Float4);
		DECLARE_GETTER(Double2);
		DECLARE_GETTER(Double3);
		DECLARE_GETTER(Double4);
		DECLARE_GETTER(Color);
		DECLARE_GETTER(Quaternion);
		DECLARE_GETTER(Matrix);
		DECLARE_GETTER(DateTime);

	#undef DECLARE_GETTER

		FORCE_INLINE static UID GetGuid(const Value& node, const char* name)
		{
			auto member = node.FindMember(name);
			return member != node.MemberEnd() ? GetGuid(member->value) : UID::Empty;
		}

		FORCE_INLINE static bool GetGuidIfValid(UID& result, const Value& node, const char* name)
		{
			auto member = node.FindMember(name);
			if (member != node.MemberEnd())
			{
				result = GetGuid(member->value);
				return result.IsValid();
			}
			return false;
		}

	public:
		FORCE_INLINE static void GetBool(bool& result, const Value& node, const char* name)
		{
			const auto member = node.FindMember(name);
			if (member != node.MemberEnd() && member->value.IsBool())
			{
				result = member->value.GetBool();
			}
		}

		FORCE_INLINE static void GetFloat(float& result, const Value& node, const char* name)
		{
			const auto member = node.FindMember(name);
			if (member != node.MemberEnd() && member->value.IsNumber())
			{
				result = member->value.GetFloat();
			}
		}

		FORCE_INLINE static void GetInt(byte& result, const Value& node, const char* name)
		{
			const auto member = node.FindMember(name);
			if (member != node.MemberEnd() && member->value.IsInt())
			{
				result = member->value.GetInt();
			}
		}

		FORCE_INLINE static void GetInt(int32& result, const Value& node, const char* name)
		{
			const auto member = node.FindMember(name);
			if (member != node.MemberEnd() && member->value.IsInt())
			{
				result = member->value.GetInt();
			}
		}

		FORCE_INLINE static void GetInt(uint32& result, const Value& node, const char* name)
		{
			const auto member = node.FindMember(name);
			if (member != node.MemberEnd() && member->value.IsInt())
			{
				result = member->value.GetInt();
			}
		}

		FORCE_INLINE static void GetInt(int16& result, const Value& node, const char* name)
		{
			const auto member = node.FindMember(name);
			if (member != node.MemberEnd() && member->value.IsInt())
			{
				result = member->value.GetInt();
			}
		}

		FORCE_INLINE static void GetInt(uint16& result, const Value& node, const char* name)
		{
			const auto member = node.FindMember(name);
			if (member != node.MemberEnd() && member->value.IsInt())
			{
				result = member->value.GetInt();
			}
		}

		template<class T>
		FORCE_INLINE static void GetEnum(T& result, const Value& node, const char* name)
		{
			const auto member = node.FindMember(name);
			if (member != node.MemberEnd() && member->value.IsInt())
			{
				result = static_cast<T>(member->value.GetInt());
			}
		}

		FORCE_INLINE static void GetString(String& result, const Value& node, const char* name)
		{
			const auto member = node.FindMember(name);
			if (member != node.MemberEnd())
			{
				result = member->value.GetText();
			}
		}

	#define DECLARE_GETTER(type) \
		FORCE_INLINE static void Get##type(type& result, const Value& node, const char* name) \
		{ \
			const auto member = node.FindMember(name); \
			if (member != node.MemberEnd()) \
			{ \
				result = Get##type(member->value); \
			} \
		}

		DECLARE_GETTER(Rectangle);
/*		DECLARE_GETTER(Vector2);
		DECLARE_GETTER(Vector3);
		DECLARE_GETTER(Vector4);*/
		DECLARE_GETTER(Float2);
		DECLARE_GETTER(Float3);
		DECLARE_GETTER(Float4);
		DECLARE_GETTER(Double2);
		DECLARE_GETTER(Double3);
		DECLARE_GETTER(Double4);
		DECLARE_GETTER(Color);
		DECLARE_GETTER(Quaternion);
		DECLARE_GETTER(DateTime);

	#undef DECLARE_GETTER

	#define DECLARE_GETTER(type) \
		FORCE_INLINE static void Get##type(type& result, const Value& node, const char* name) \
		{ \
			const auto member = node.FindMember(name); \
			if (member != node.MemberEnd()) \
			{ \
				Get##type(result, member->value); \
			} \
		}

		DECLARE_GETTER(Transform);

	#undef DECLARE_GETTER

		template<typename T>
		FORCE_INLINE static void GetReference(T& result, const Value& node, const char* name)
		{
			const auto member = node.FindMember(name);
			if (member != node.MemberEnd())
			{
				result = GetGuid(member->value);
			}
		}

		FORCE_INLINE static void GetGuid(UID& result, const Value& node, const char* name)
		{
			const auto member = node.FindMember(name);
			if (member != node.MemberEnd())
			{
				result = GetGuid(member->value);
			}
		}
	};
}
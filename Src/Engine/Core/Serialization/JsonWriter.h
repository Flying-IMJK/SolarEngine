 #pragma once

#include "Core/Types/Strings/String.h"
#include "Core/Types/Strings/StringView.h"


namespace SE
{
	struct BoundingBox;
	struct BoundingSphere;
	struct Plane;
	struct Ray;
	class ISerializable;

	// Helper macro for JSON serialization keys (reduces allocations count)
	#define JKEY(keyname) Key(keyname, ARRAY_SIZE(keyname) - 1)

	/// <summary>
	/// Base class for Json writers.
	/// </summary>
	class SE_API_CORE JsonWriter
	{
	public:
		typedef char CharType;

		virtual ~JsonWriter() = default;

		virtual void Key(const char* str, int32 length) = 0;
		virtual void String(const char* str, int32 length) = 0;
		virtual void RawValue(const char* str, int32 length) = 0;
		virtual void Bool(bool d) = 0;
		virtual void Int(int32 d) = 0;
		virtual void Int64(int64 d) = 0;
		virtual void Uint(uint32 d) = 0;
		virtual void Uint64(uint64 d) = 0;
		virtual void Float(float d) = 0;
		virtual void Double(double d) = 0;

		virtual void StartObject() = 0;
		virtual void EndObject() = 0;
		virtual void StartArray() = 0;
		virtual void EndArray(int32 count = 0) = 0;

	public:
		inline void Key(const StringAnsiView& str)
		{
			Key(str.Get(), str.Length());
		}

		void Key(const StringView& str);

		inline void String(const char* str)
		{
			String(str, StringUtils::Length(str));
		}

		inline void String(const StringAnsiView& str)
		{
			String(str.Get(), str.Length());
		}

		void String(const Char* str);

		void String(const Char* str, const int32 length);

		void String(const ::SE::String& value);

		void String(const StringView& value);

		void String(const StringAnsi& value)
		{
			String(value.Get(), value.Length());
		}

		inline void RawValue(const StringAnsi& str)
		{
			RawValue(str.Get(), str.Length());
		}

		void RawValue(const StringView& str);

		inline void RawValue(const CharType* json)
		{
			RawValue(json, StringUtils::Length(json));
		}

		// Raw bytes blob serialized as base64 string
		void Blob(const void* data, int32 length);

		template<typename T>
		inline void Enum(const T value)
		{
			Int(static_cast<int32>(value));
		}

		void DateTime(const DateTime& value);
		void Float2(const Float2& value);
		void Float3(const Float3& value);
		void Float4(const Float4& value);
		void Int2(const Int2& value);
		void Int4(const Int4& value);
		void Color(const Color& value);
		void Quaternion(const Quaternion& value);
		void Ray(const Ray& value);
		void Matrix(const Matrix& value);
		void Transform(const ::SE::Transform& value);
		void Transform(const ::SE::Transform& value, const ::SE::Transform* other);
		void Plane(const Plane& value);
		void Rectangle(const Rectangle& value);
		void BoundingSphere(const BoundingSphere& value);
		void BoundingBox(const BoundingBox& value);
		void UUID(const UID& value);
		void Object(ISerializable* value, const void* otherObj);
	};
}

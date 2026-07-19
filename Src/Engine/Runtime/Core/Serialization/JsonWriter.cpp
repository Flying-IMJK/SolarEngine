#include "JsonWriter.h"

#include "Runtime/Core/Math/Color.h"
#include "Runtime/Core/Math/Plane.h"
#include "Runtime/Core/Math/Line.h"
#include "Runtime/Core/Math/Rectangle.h"
#include "Runtime/Core/Types/DateTime.h"
#include "Runtime/Core/Math/Transform.h"
#include "Runtime/Core/Types/UID.h"
#include "Runtime/Core/Math/Matrix.h"
//#include "Engine/Level/Prefabs/Prefab.h"
//#include "Engine/Level/SceneObject.h"
#include "ISerializable.h"
#include "Runtime/Core/Encoding.h"
#include "Runtime/Core/Types/Strings/StringConverter.h"

#include "JsonWriters.hpp"
#include "Runtime/Core/Math/BoundingVolumes.h"

namespace SE
{
	template class JsonWriterBase<PrettyJsonWriterImpl>;

	template class JsonWriterBase<CompactJsonWriterImpl>;


	void JsonWriter::Blob(const void* data, int32 length)
	{
		List<char> base64;
		Encoding::Base64::Encode(static_cast<const byte*>(data), length, base64.Get());
		RawValue(base64.Get(), base64.Count());
	}

	void JsonWriter::DateTime(const ::SE::DateTime& value)
	{
		Int64(value.Ticks);
	}

	void JsonWriter::Float2(const ::SE::Float2& value)
	{
		StartObject();
		JKEY("X");
		Float(value.x);
		JKEY("Y");
		Float(value.y);
		EndObject();
	}

	void JsonWriter::Float3(const ::SE::Float3& value)
	{
		StartObject();
		JKEY("X");
		Float(value.x);
		JKEY("Y");
		Float(value.y);
		JKEY("Z");
		Float(value.z);
		EndObject();
	}

	void JsonWriter::Float4(const ::SE::Float4& value)
	{
		StartObject();
		JKEY("X");
		Float(value.x);
		JKEY("Y");
		Float(value.y);
		JKEY("Z");
		Float(value.z);
		JKEY("W");
		Float(value.w);
		EndObject();
	}

	void JsonWriter::Int2(const ::SE::Int2& value)
	{
		StartObject();
		JKEY("X");
		Int(value.x);
		JKEY("Y");
		Int(value.y);
		EndObject();
	}

	void JsonWriter::Int4(const ::SE::Int4& value)
	{
		StartObject();
		JKEY("X");
		Int(value.x);
		JKEY("Y");
		Int(value.y);
		JKEY("Z");
		Int(value.z);
		JKEY("W");
		Int(value.w);
		EndObject();
	}

	void JsonWriter::Color(const ::SE::Color& value)
	{
		StartObject();
		JKEY("R");
		Float(value.r);
		JKEY("G");
		Float(value.g);
		JKEY("B");
		Float(value.b);
		JKEY("A");
		Float(value.a);
		EndObject();
	}

	void JsonWriter::Quaternion(const ::SE::Quaternion& value)
	{
		StartObject();
		JKEY("X");
		Float(value.x);
		JKEY("Y");
		Float(value.y);
		JKEY("Z");
		Float(value.z);
		JKEY("W");
		Float(value.w);
		EndObject();
	}
/*
	void JsonWriter::Ray(const ::Ray& value)
	{
		StartObject();
		JKEY("Position");
		Vector3(value.Position);
		JKEY("Direction");
		Vector3(value.Direction);
		EndObject();
	}
*/
	void JsonWriter::Matrix(const ::SE::Matrix& value)
	{
		StartObject();
		JKEY("M00");
		Float(value.Values[0][0]);
		JKEY("M01");
		Float(value.Values[0][1]);
		JKEY("M02");
		Float(value.Values[0][2]);
		JKEY("M03");
		Float(value.Values[0][3]);
		JKEY("M10");
		Float(value.Values[1][0]);
		JKEY("M11");
		Float(value.Values[1][1]);
		JKEY("M12");
		Float(value.Values[1][2]);
		JKEY("M12");
		Float(value.Values[1][3]);
		JKEY("M20");
		Float(value.Values[2][0]);
		JKEY("M21");
		Float(value.Values[2][1]);
		JKEY("M22");
		Float(value.Values[2][2]);
		JKEY("M23");
		Float(value.Values[2][3]);
		JKEY("M30");
		Float(value.Values[3][0]);
		JKEY("M31");
		Float(value.Values[3][1]);
		JKEY("M32");
		Float(value.Values[3][2]);
		JKEY("M33");
		Float(value.Values[3][3]);
		EndObject();
	}

	void JsonWriter::Transform(const ::SE::Transform& value)
	{
		StartObject();
		if (!value.Translation.IsZero())
		{
			JKEY("Translation");
			Float3(value.Translation);
		}
		if (!value.Orientation.IsIdentity())
		{
			JKEY("Orientation");
			Quaternion(value.Orientation);
		}
		if (value.Scale != 1)
		{
			JKEY("Scale");
			Float3(value.Scale);
		}
		EndObject();
	}

	void JsonWriter::Transform(const ::SE::Transform& value, const ::SE::Transform* other)
	{
		StartObject();
		if (other)
		{
			if (!Math::NearEqual(value.Translation, other->Translation))
			{
				JKEY("Translation");
				Float3(value.Translation);
			}
			if (!Math::NearEqual(value.Orientation, other->Orientation))
			{
				JKEY("Orientation");
				Quaternion(value.Orientation);
			}
			if (!Math::NearEqual(value.Scale, other->Scale))
			{
				JKEY("Scale");
				Float3(value.Scale);
			}
		}
		else
		{
			JKEY("Translation");
			Float3(value.Translation);
			JKEY("Orientation");
			Quaternion(value.Orientation);
			JKEY("Scale");
			Float3(value.Scale);
		}
		EndObject();
	}

	void JsonWriter::Plane(const ::SE::Plane& value)
	{
		StartObject();
		JKEY("Normal");
		Float3(value.Normal);
		JKEY("D");
		Float(value.D);
		EndObject();
	}

	void JsonWriter::Rectangle(const ::SE::Rectangle& value)
	{
		StartObject();
		JKEY("Location");
		Float2(value.Location);
		JKEY("Size");
		Float2(value.Size);
		EndObject();
	}

	void JsonWriter::BoundingSphere(const ::SE::BoundingSphere& value)
	{
		StartObject();
		JKEY("Center");
		Float3(value.Center);
		JKEY("Radius");
		Float(value.Radius);
		EndObject();
	}

	void JsonWriter::BoundingBox(const ::SE::BoundingBox& value)
	{
		StartObject();
		JKEY("Minimum");
		Float3(value.Minimum);
		JKEY("Maximum");
		Float3(value.Maximum);
		EndObject();
	}

	void JsonWriter::UUID(const UID& value)
	{
		// Unoptimized version:
		//Text(value.ToString(Guid::FormatType::N));

		// Optimized version:
		// @formatter:off
		char buffer[32] =
			{
				'0','0', '0', '0', '0', '0', '0', '0',
				'0','0', '0', '0', '0', '0', '0', '0',
				'0','0', '0', '0', '0', '0', '0', '0',
				'0','0', '0', '0', '0', '0', '0', '0'
			};
		// @formatter:on
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
		String(buffer, 32);
	}

	void JsonWriter::String(const Char* str)
	{
		const StringAsUTF8<256> buf(str);
		String(buf.Get());
	}

	void JsonWriter::String(const Char* str, const int32 length)
	{
		const StringAsUTF8<256> buf(str, length);
		String(buf.Get());
	}

	void JsonWriter::String(const ::SE::String& value)
	{
		const StringAsUTF8<256> buf(*value, value.Length());
		String(buf.Get());
	}

	void JsonWriter::String(const StringView& value)
	{
		const StringAsUTF8<256> buf(*value, value.Length());
		String(buf.Get());
	}

	void JsonWriter::RawValue(const StringView& str)
	{
		const StringAsUTF8<256> buf(*str, str.Length());
		RawValue(buf.Get(), buf.Length());
	}

	void JsonWriter::Key(const StringView& str)
	{
		const StringAsUTF8<256> buf(*str, str.Length());
		Key(buf.Get(), buf.Length());
	}

	void JsonWriter::Object(ISerializable* value, const void* otherObj)
	{
		StartObject();
		SerializeContext context(*this, otherObj);
		value->Serialize(context);
		EndObject();
	}
}

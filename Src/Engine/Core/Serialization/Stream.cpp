// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "ReadStream.h"
#include "WriteStream.h"
//#include "JsonWriters.h"
//#include "JsonSerializer.h"
#include "MemoryReadStream.h"
#include "Core/Types/Collections/Dictionary.h"
//#include "Engine/Debug/Exceptions/JsonParseException.h"
#include "ISerializable.h"
#include "JsonWriters.hpp"
#include "Core/Profiler/ProfilerCPU.h"
//#include "Engine/Scripting/Internal/ManagedSerialization.h"
//#include "Engine/Scripting/Scripting.h"
//#include "Engine/Scripting/ScriptingObject.h"
//#include "Engine/Scripting/ManagedCLR/MClass.h"
//#include "Engine/Scripting/ManagedCLR/MCore.h"
//#include "Engine/Scripting/ManagedCLR/MUtils.h"


namespace SE
{

	void ReadStream::Read(StringAnsi& data)
	{
		int32 length;
		ReadInt32(&length);
		if (length < 0 || length > STREAM_MAX_STRING_LENGTH)
		{
			_hasError = true;
			data = "";
			return;
		}

		data.ReserveSpace(length);
		if (length == 0)
			return;
		char * ptr = data.Get();
		ENGINE_ASSERT(ptr != nullptr);
		ReadBytes(ptr, length);
	}

	void ReadStream::Read(StringAnsi& data, int8 lock)
	{
		int32 length;
		ReadInt32(&length);
		if (length < 0 || length > STREAM_MAX_STRING_LENGTH)
		{
			_hasError = true;
			data = "";
			return;
		}

		data.ReserveSpace(length);
		if (length == 0)
			return;
		char* ptr = data.Get();
		ENGINE_ASSERT(ptr != nullptr);
		ReadBytes(ptr, length);

		for (int32 i = 0; i < length; i++)
		{
			*ptr = *ptr ^ lock;
			ptr++;
		}
	}

	void ReadStream::Read(String& data)
	{
		int32 length;
		ReadInt32(&length);
		if (length <= 0 || length > STREAM_MAX_STRING_LENGTH)
		{
			if (length != 0)
				_hasError = true;
			data.Clear();
			return;
		}

		data.ReserveSpace(length);
		Char* ptr = data.Get();
		ENGINE_ASSERT(ptr != nullptr);
		ReadBytes(ptr, length * sizeof(char));
	}

	void ReadStream::Read(String& data, int16 lock)
	{
		int32 length;
		ReadInt32(&length);
		if (length <= 0 || length > STREAM_MAX_STRING_LENGTH)
		{
			if (length != 0)
				_hasError = true;
			data.Clear();
			return;
		}

		data.ReserveSpace(length);
		Char* ptr = data.Get();
		ENGINE_ASSERT(ptr != nullptr);
		ReadBytes(ptr, length * sizeof(Char));

		for (int32 i = 0; i < length; i++)
		{
			*ptr = *ptr ^ lock;
			ptr++;
		}
	}

/*	void ReadStream::Read(CommonValue& data)
	{
		byte type;
		ReadByte(&type);
		switch (static_cast<CommonType>(type))
		{
		case CommonType::Bool:
			data.Set(ReadBool());
			break;
		case CommonType::Integer:
		{
			int32 v;
			ReadInt32(&v);
			data.Set(v);
		}
			break;
		case CommonType::Float:
		{
			float v;
			ReadFloat(&v);
			data.Set(v);
		}
			break;
		case CommonType::Vector2:
		{
			Float2 v;
			Read(v);
			data.Set(v);
		}
			break;
		case CommonType::Vector3:
		{
			Float3 v;
			Read(v);
			data.Set(v);
		}
			break;
		case CommonType::Vector4:
		{
			Float4 v;
			Read(v);
			data.Set(v);
		}
			break;
		case CommonType::Color:
		{
			Color v;
			Read(v);
			data.Set(v);
		}
			break;
		case CommonType::Guid:
		{
			Guid v;
			Read(v);
			data.Set(v);
		}
			break;
		case CommonType::String:
		{
			String v;
			ReadString(&v, 953);
			data.Set(v);
		}
			break;
		case CommonType::Box:
		{
			BoundingBox v;
			ReadBoundingBox(&v);
			data.Set(v);
		}
			break;
		case CommonType::Rotation:
		{
			Quaternion v;
			Read(v);
			data.Set(v);
		}
			break;
		case CommonType::Transform:
		{
			Transform v;
			ReadTransform(&v);
			data.Set(v);
		}
			break;
		case CommonType::Sphere:
		{
			BoundingSphere v;
			ReadBoundingSphere(&v);
			data.Set(v);
		}
			break;
		case CommonType::Rectangle:
		{
			Rectangle v;
			Read(v);
			data.Set(v);
		}
		case CommonType::Ray:
		{
			Ray v;
			ReadRay(&v);
			data.Set(v);
		}
			break;
		case CommonType::Matrix:
		{
			Matrix v;
			Read(v);
			data.Set(v);
		}
			break;
		case CommonType::Blob:
		{
			int32 length;
			Read(length);
			data.SetBlob(length);
			if (length > 0)
			{
				ReadBytes(data.AsBlob.Data, length);
			}
		}
			break;
		default: CRASH;
		}
	}*/
/*
	void ReadStream::ReadJson(ISerializable* obj)
	{
		int32 engineBuild, size;
		ReadInt32(&engineBuild);
		ReadInt32(&size);
		if (obj)
		{
			if (const auto memoryStream = dynamic_cast<MemoryReadStream*>(this))
			{
				JsonSerializer::LoadFromBytes(obj, Span<byte>((byte*)memoryStream->Move(size), size), engineBuild);
			}
			else
			{
				void* data = PlatformAllocator::Allocate(size);
				ReadBytes(data, size);
				JsonSerializer::LoadFromBytes(obj, Span<byte>((byte*)data, size), engineBuild);
				Allocator::Free(data);
			}
		}
		else
			SetPosition(GetPosition() + size);
	}
*/
	void ReadStream::ReadStringAnsi(StringAnsi* data)
	{
		Read(*data);
	}

	void ReadStream::ReadStringAnsi(StringAnsi* data, int8 lock)
	{
		Read(*data, lock);
	}

	void ReadStream::ReadString(String* data)
	{
		Read(*data);
	}

	void ReadStream::ReadString(String* data, int16 lock)
	{
		Read(*data, lock);
	}

/*	void ReadStream::ReadCommonValue(CommonValue* data)
	{
		Read(*data);
	}*/

/*	void ReadStream::ReadBoundingBox(BoundingBox* box, bool useDouble)
	{
#if USE_LARGE_WORLDS
		if (useDouble)
        Read(box);
    else
    {
        Float3 min, max;
        Read(min);
        Read(max);
        box->Minimum = min;
        box->Maximum = max;
    }
#else
		if (useDouble)
		{
			Double3 min, max;
			Read(min);
			Read(max);
			box->Minimum = min;
			box->Maximum = max;
		}
		else
			Read(*box);
#endif
	}

	void ReadStream::ReadBoundingSphere(BoundingSphere* sphere, bool useDouble)
	{
#if USE_LARGE_WORLDS
		if (useDouble)
        Read(*sphere);
    else
    {
        Float3 center;
        float radius;
        Read(center);
        Read(radius);
        sphere->Center = center;
        sphere->Radius = radius;
    }
#else
		if (useDouble)
		{
			Double3 center;
			double radius;
			Read(center);
			Read(radius);
			sphere->Center = center;
			sphere->Radius = (float)radius;
		}
		else
			Read(*sphere);
#endif
	}

	void ReadStream::ReadTransform(Transform* transform, bool useDouble)
	{
#if USE_LARGE_WORLDS
		if (useDouble)
        Read(*transform);
    else
    {
        Float3 translation;
        Read(translation);
        Read(transform->Orientation);
        Read(transform->Scale);
        transform->Translation = translation;
    }
#else
		if (useDouble)
		{
			Double3 translation;
			Read(translation);
			Read(transform->Orientation);
			Read(transform->Scale);
			transform->Translation = translation;
		}
		else
			Read(*transform);
#endif
	}

	void ReadStream::ReadRay(Ray* ray, bool useDouble)
	{
#if USE_LARGE_WORLDS
		if (useDouble)
        Read(*ray);
    else
    {
        Float3 position, direction;
        Read(position);
        Read(direction);
        ray->Position = position;
        ray->Direction = direction;
    }
#else
		if (useDouble)
		{
			Double3 position, direction;
			Read(position);
			Read(direction);
			ray->Position = position;
			ray->Direction = direction;
		}
		else
			Read(*ray);
#endif
	}*/

	void WriteStream::WriteText(const StringView& text)
	{
		WriteBytes(text.Get(), sizeof(Char) * text.Length());
	}

	void WriteStream::WriteText(const StringAnsiView& text)
	{
		WriteBytes(text.Get(), sizeof(char) * text.Length());
	}

	void WriteStream::Write(const StringView& data)
	{
		const int32 length = data.Length();
		ENGINE_ASSERT(length < STREAM_MAX_STRING_LENGTH);
		WriteInt32(length);
		WriteBytes(*data, length * sizeof(char));
	}

	void WriteStream::Write(const StringView& data, int16 lock)
	{
		ENGINE_ASSERT(data.Length() < STREAM_MAX_STRING_LENGTH);
		WriteInt32(data.Length());
		for (int32 i = 0; i < data.Length(); i++)
			WriteUint16((uint16)((uint16)data[i] ^ lock));
	}

	void WriteStream::Write(const StringAnsiView& data)
	{
		const int32 length = data.Length();
		ENGINE_ASSERT(length < STREAM_MAX_STRING_LENGTH);
		WriteInt32(length);
		WriteBytes(data.Get(), length);
	}

	void WriteStream::Write(const StringAnsiView& data, int8 lock)
	{
		const int32 length = data.Length();
		ENGINE_ASSERT(length < STREAM_MAX_STRING_LENGTH);
		WriteInt32(length);
		for (int32 i = 0; i < length; i++)
			WriteUint8((uint8)((uint8)data[i] ^ lock));
	}

/*	void WriteStream::Write(const CommonValue& data)
	{
		WriteByte(static_cast<byte>(data.Type));
		switch (data.Type)
		{
		case CommonType::Bool:
			WriteBool(data.AsBool);
			break;
		case CommonType::Integer:
			WriteInt32(data.AsInteger);
			break;
		case CommonType::Float:
			WriteFloat(data.AsFloat);
			break;
		case CommonType::Vector2:
			Write(data.AsVector2);
			break;
		case CommonType::Vector3:
			Write(data.AsVector3);
			break;
		case CommonType::Vector4:
			Write(data.AsVector4);
			break;
		case CommonType::Color:
			Write(data.AsColor);
			break;
		case CommonType::Guid:
			Write(data.AsGuid);
			break;
		case CommonType::String:
			WriteString(data.AsString, 953);
			break;
		case CommonType::Box:
			WriteBoundingBox(data.AsBox);
			break;
		case CommonType::Rotation:
			Write(data.AsRotation);
			break;
		case CommonType::Transform:
			WriteTransform(data.AsTransform);
			break;
		case CommonType::Sphere:
			WriteBoundingSphere(data.AsSphere);
			break;
		case CommonType::Rectangle:
			Write(data.AsRectangle);
			break;
		case CommonType::Ray:
			WriteRay(data.AsRay);
			break;
		case CommonType::Matrix:
			Write(data.AsMatrix);
			break;
		case CommonType::Blob:
			WriteInt32(data.AsBlob.Length);
			if (data.AsBlob.Length > 0)
				WriteBytes(data.AsBlob.Data, data.AsBlob.Length);
			break;
		default: CRASH;
		}
	}
	*/

	void WriteStream::WriteJson(ISerializable* obj, const void* otherObj)
	{
		WriteInt32(0);
		if (obj)
		{
			Json::StringBuffer buffer;
			CompactJsonWriter writer(buffer);
			writer.StartObject();

			SerializeContext context = {writer, const_cast<void*>(otherObj)};
			obj->Serialize(context);
			writer.EndObject();

			WriteInt32((int32)buffer.GetSize());
			WriteBytes((byte*)buffer.GetString(), (int32)buffer.GetSize());
		}
		else
			WriteInt32(0);
	}

	void WriteStream::WriteJson(const StringAnsiView& json)
	{
		WriteInt32(0);
		WriteInt32((int32)json.Length());
		WriteBytes((byte*)json.Get(), (int32)json.Length());
	}

	void WriteStream::WriteString(const StringView& data)
	{
		Write(data);
	}

	void WriteStream::WriteString(const StringView& data, int16 lock)
	{
		Write(data, lock);
	}

	void WriteStream::WriteStringAnsi(const StringAnsiView& data)
	{
		Write(data);
	}

	void WriteStream::WriteStringAnsi(const StringAnsiView& data, int8 lock)
	{
		Write(data, lock);
	}

/*	void WriteStream::WriteCommonValue(const CommonValue& data)
	{
		Write(data);
	}*/

/*	void WriteStream::WriteBoundingBox(const BoundingBox& box, bool useDouble)
	{
#if USE_LARGE_WORLDS
		if (useDouble)
        Write(box);
    else
    {
        Float3 min = box.Minimum, max = box.Maximum;
        Write(min);
        Write(max);
    }
#else
		if (useDouble)
		{
			Double3 min = box.Minimum, max = box.Maximum;
			Write(min);
			Write(max);
		}
		else
			Write(box);
#endif
	}

	void WriteStream::WriteBoundingSphere(const BoundingSphere& sphere, bool useDouble)
	{
#if USE_LARGE_WORLDS
		if (useDouble)
        Write(sphere);
    else
    {
        Float3 center = sphere.Center;
        float radius = (float)sphere.Radius;
        Write(center);
        Write(radius);
    }
#else
		if (useDouble)
		{
			Double3 center = sphere.Center;
			float radius = (float)sphere.Radius;
			Write(center);
			Write(radius);
		}
		else
			Write(sphere);
#endif
	}

	void WriteStream::WriteTransform(const Transform& transform, bool useDouble)
	{
#if USE_LARGE_WORLDS
		if (useDouble)
        Write(transform);
    else
    {
        Float3 translation = transform.Translation;
        Write(translation);
        Write(transform.Orientation);
        Write(transform.Scale);
    }
#else
		if (useDouble)
		{
			Double3 translation = transform.Translation;
			Write(translation);
			Write(transform.Orientation);
			Write(transform.Scale);
		}
		else
			Write(transform);
#endif
	}

	void WriteStream::WriteRay(const Ray& ray, bool useDouble)
	{
#if USE_LARGE_WORLDS
		if (useDouble)
        Write(ray);
    else
    {
        Float3 position = ray.Position, direction = ray.Direction;
        Write(position);
        Write(direction);
    }
#else
		if (useDouble)
		{
			Double3 position = ray.Position, direction = ray.Direction;
			Write(position);
			Write(direction);
		}
		else
			Write(ray);
#endif
	}*/

/*	List<byte> JsonSerializer::SaveToBytes(ISerializable* obj)
	{
		Array<byte> result;
		if (obj)
		{
			rapidjson_flax::StringBuffer buffer;
			CompactJsonWriter writer(buffer);
			writer.StartObject();
			obj->Serialize(writer, nullptr);
			writer.EndObject();
			result.Set((byte*)buffer.GetString(), (int32)buffer.GetSize());
		}
		return result;
	}

	void JsonSerializer::LoadFromBytes(ISerializable* obj, const Span<byte>& data, int32 engineBuild)
	{
		if (!obj || data.Length() == 0)
			return;

		ISerializable::SerializeDocument document;
		{
			PROFILE_CPU_NAMED("Json.Parse");
			document.Parse((const char*)data.Get(), data.Length());
		}
		if (document.HasParseError())
		{
			Log::JsonParseException(document.GetParseError(), document.GetErrorOffset());
			return;
		}

		auto modifier = Cache::ISerializeModifier.Get();
		modifier->EngineBuild = engineBuild;
		obj->Deserialize(document, modifier.Value);
	}*/

}
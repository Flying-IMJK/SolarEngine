// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "Serialization.h"
#include "Core/Types/Variable.h"
#include "Core/Types/DateTime.h"
#include "Core/Types/TimeSpan.h"
#include "Core/Math/Quaternion.h"
#include "Core/Types/UID.h"
#include "Core/Math/Rectangle.h"
#include "Core/Math/Transform.h"
#include "Core/Math/Color.h"
#include "Core/Math/Matrix.h"

namespace SE
{
	DeserializeContext::StreamScope::StreamScope(DeserializeContext& context, DeserializeStream& stream) : context(&context), lastStream()
	{
		lastStream = context.stream;
		context.stream = &stream;
	}

	DeserializeContext::StreamScope::StreamScope(DeserializeContext& context, const DeserializeStream& stream) : context(&context)
	{
		lastStream = context.stream;
		context.stream = &const_cast<DeserializeStream&>(stream);
	}

	DeserializeContext::StreamScope::~StreamScope()
	{
		context->stream = lastStream;
		lastStream = nullptr;
	}

	void ISerializable::DeserializeIfExists(DeserializeContext& context, const char* memberName)
	{
		auto member = context.stream->FindMember(memberName);
		if (member != context.stream->MemberEnd())
		{
			DeserializeContext::StreamScope::StreamScope(context, member->value);
			Deserialize(context);
		}

	}

#define DESERIALIZE_HELPER(context, name, var, defaultValue)		\
    {																\
        const auto m = SERIALIZE_FIND_MEMBER(context.stream, name); \
        if (m != context.stream->MemberEnd())						\
        {															\
			DeserializeContext::StreamScope::StreamScope(context, m->value);	\
            Deserialize(context, var);								\
        }															\
        else														\
            var = defaultValue;										\
    }

/*	bool Serialization::ShouldSerialize(const VariantType& v, const void* otherObj)
	{
		return !otherObj || v != *(VariantType*)otherObj;
	}

	void Serialization::Serialize(SerializeContext& context, const VariantType& v)
	{
		if (v.TypeName == nullptr)
		{
			context.stream.Int(static_cast<int32>(v.Type));
		}
		else
		{
			context.stream.StartObject();

			context.stream.JKEY("Type");
			context.stream.Int(static_cast<int32>(v.Type));

			context.stream.JKEY("TypeName");
			context.stream.String(v.TypeName);

			context.stream.EndObject();
		}
	}

	void Serialization::Deserialize(DeserializeContext& context, VariantType& v)
	{
		if (context.stream.IsObject())
		{
			const auto mType = SERIALIZE_FIND_MEMBER(stream, "Type");
			if (mType != context.stream.MemberEnd())
				v.Type = (VariantType::Types)mType->value.GetInt();
			else
				v.Type = VariantType::Null;
			const auto mTypeName = SERIALIZE_FIND_MEMBER(stream, "TypeName");
			if (mTypeName != context.stream.MemberEnd() && mTypeName->value.IsString())
				v.SetTypeName(StringAnsiView(mTypeName->value.GetStringAnsiView()));
		}
		else
		{
			v.Type = (VariantType::Types)context.stream.GetInt();
		}
	}

	bool Serialization::ShouldSerialize(const Variant& v, const void* otherObj)
	{
		return !otherObj || v != *(Variant*)otherObj;
	}

	void Serialization::Serialize(SerializeContext& context, const Variant& v)
	{
		context.stream.StartObject();

		context.stream.JKEY("Type");
		Serialize(stream, v.Type, nullptr);

		context.stream.JKEY("Value");
		switch (v.Type.Type)
		{
		case VariantType::Null:
		case VariantType::Void:
			context.stream.StartObject();
			context.stream.EndObject();
			break;
		case VariantType::Bool:
			context.stream.Bool(v.AsBool);
			break;
		case VariantType::Int:
			context.stream.Int(v.AsInt);
			break;
		case VariantType::Uint:
			context.stream.Uint(v.AsUint);
			break;
		case VariantType::Int64:
			context.stream.Int64(v.AsInt64);
			break;
		case VariantType::Uint64:
		case VariantType::Enum:
			context.stream.Uint64(v.AsUint64);
			break;
		case VariantType::Float:
			context.stream.Float(v.AsFloat);
			break;
		case VariantType::Double:
			context.stream.Double(v.AsDouble);
			break;
		case VariantType::Pointer:
			context.stream.Uint64((uint64)(uintptr)v.AsPointer);
			break;
		case VariantType::String:
			if (v.AsBlob.Data)
				context.stream.String((const Char*)v.AsBlob.Data, v.AsBlob.Length / sizeof(Char) - 1);
			else
				context.stream.String("", 0);
			break;
		case VariantType::Blob:
			context.stream.Blob(v.AsBlob.Data, v.AsBlob.Length);
			break;
		case VariantType::Object:
			context.stream.Guid(v.AsObject ? v.AsObject->GetID() : Guid::Empty);
			break;
		case VariantType::Asset:
			context.stream.Guid(v.AsAsset ? v.AsAsset->GetID() : Guid::Empty);
			break;
		case VariantType::Float2:
			context.stream.Float2(*(Float2*)v.AsData);
			break;
		case VariantType::Float3:
			context.stream.Float3(*(Float3*)v.AsData);
			break;
		case VariantType::Float4:
			context.stream.Float4(*(Float4*)v.AsData);
			break;
		case VariantType::Double2:
			context.stream.Double2(v.AsDouble2());
			break;
		case VariantType::Double3:
			context.stream.Double3(v.AsDouble3());
			break;
		case VariantType::Double4:
			context.stream.Double4(v.AsDouble4());
			break;
		case VariantType::Int2:
			context.stream.Int2(v.AsInt2());
			break;
		case VariantType::Int3:
			context.stream.Int3(v.AsInt3());
			break;
		case VariantType::Int4:
			context.stream.Int4(v.AsInt4());
			break;
		case VariantType::Color:
			context.stream.Color(v.AsColor());
			break;
		case VariantType::Guid:
			context.stream.Guid(v.AsGuid());
			break;
		case VariantType::BoundingSphere:
			context.stream.BoundingSphere(v.AsBoundingSphere());
			break;
		case VariantType::Quaternion:
			context.stream.Quaternion(v.AsQuaternion());
			break;
		case VariantType::Rectangle:
			context.stream.Rectangle(v.AsRectangle());
			break;
		case VariantType::BoundingBox:
			context.stream.BoundingBox(v.AsBoundingBox());
			break;
		case VariantType::Transform:
			context.stream.Transform(v.AsTransform());
			break;
		case VariantType::Ray:
			context.stream.Ray(v.AsRay());
			break;
		case VariantType::Matrix:
			context.stream.Matrix(v.AsMatrix());
			break;
		case VariantType::Array:
			Serialize(stream, v.AsArray(), nullptr);
			break;
		case VariantType::Dictionary:
			Serialize(stream, *v.AsDictionary, nullptr);
			break;
		case VariantType::Typename:
			if (v.AsBlob.Data)
				context.stream.String((const char*)v.AsBlob.Data, v.AsBlob.Length - 1);
			else
				context.stream.String("", 0);
			break;
		case VariantType::ManagedObject:
		case VariantType::Structure:
		{
#if USE_CSHARP
			MObject* obj;
        if (v.Type.Type == VariantType::Structure)
            obj = MUtils::BoxVariant(v);
        else
            obj = (MObject*)v;
        ManagedSerialization::Serialize(stream, obj);
#endif
			break;
		}
		default:
			Platform::CheckFailed("", __FILE__, __LINE__);
			context.stream.StartObject();
			context.stream.EndObject();
		}

		context.stream.EndObject();
	}

	void Serialization::Deserialize(DeserializeContext& context, Variant& v)
	{
		const auto mType = SERIALIZE_FIND_MEMBER(stream, "Type");
		if (mType == context.stream.MemberEnd())
			return;
		VariantType type;
		Deserialize(mType->value, type);
		v.SetType(MoveTemp(type));

		const auto mValue = SERIALIZE_FIND_MEMBER(stream, "Value");
		if (mValue == context.stream.MemberEnd())
			return;
		auto& value = mValue->value;
		Guid id;
		switch (v.Type.Type)
		{
		case VariantType::Null:
		case VariantType::Void:
			break;
		case VariantType::Bool:
			v.AsBool = value.GetBool();
			break;
		case VariantType::Int:
			v.AsInt = value.GetInt();
			break;
		case VariantType::Uint:
			v.AsUint = value.GetUint();
			break;
		case VariantType::Int64:
			v.AsInt64 = value.GetInt64();
			break;
		case VariantType::Uint64:
		case VariantType::Enum:
			v.AsUint64 = value.GetUint64();
			break;
		case VariantType::Float:
			v.AsFloat = value.GetFloat();
			break;
		case VariantType::Double:
			v.AsDouble = value.GetDouble();
			break;
		case VariantType::Pointer:
			v.AsPointer = (void*)(uintptr)value.GetUint64();
			break;
		case VariantType::String:
			CHECK(value.IsString());
			v.SetString(value.GetStringAnsiView());
			break;
		case VariantType::Object:
			Deserialize(value, id);
			modifier->IdsMapping.TryGet(id, id);
			v.SetObject(FindObject(id, ScriptingObject::GetStaticClass()));
			break;
		case VariantType::Asset:
			Deserialize(value, id);
			v.SetAsset(LoadAsset(id, Asset::TypeInitializer));
			break;
		case VariantType::Blob:
			CHECK(value.IsString());
			id.A = value.GetStringLength();
			v.SetBlob(id.A);
			Encryption::Base64Decode(value.GetString(), id.A, (byte*)v.AsBlob.Data);
			break;
		case VariantType::Float2:
			Deserialize(value, *(Float2*)v.AsData);
			break;
		case VariantType::Float3:
			Deserialize(value, *(Float3*)v.AsData);
			break;
		case VariantType::Float4:
			Deserialize(value, *(Float4*)v.AsData);
			break;
		case VariantType::Double2:
			Deserialize(value, *(Double2*)v.AsData);
			break;
		case VariantType::Double3:
			Deserialize(value, *(Double3*)v.AsData);
			break;
		case VariantType::Double4:
			Deserialize(value, *(Double4*)v.AsBlob.Data);
			break;
		case VariantType::Int2:
			Deserialize(value, *(Int2*)v.AsData);
			break;
		case VariantType::Int3:
			Deserialize(value, *(Int3*)v.AsData);
			break;
		case VariantType::Int4:
			Deserialize(value, *(Int4*)v.AsData);
			break;
		case VariantType::Color:
			Deserialize(value, *(Color*)v.AsData);
			break;
		case VariantType::Guid:
			Deserialize(value, *(Guid*)v.AsData);
			break;
		case VariantType::BoundingSphere:
			Deserialize(value, v.AsBoundingSphere());
			break;
		case VariantType::Quaternion:
			Deserialize(value, *(Quaternion*)v.AsData);
			break;
		case VariantType::Rectangle:
			Deserialize(value, *(Rectangle*)v.AsData);
			break;
		case VariantType::BoundingBox:
			Deserialize(value, v.AsBoundingBox());
			break;
		case VariantType::Transform:
			Deserialize(value, v.AsTransform());
			break;
		case VariantType::Ray:
			Deserialize(value, v.AsRay());
			break;
		case VariantType::Matrix:
			Deserialize(value, *(Matrix*)v.AsBlob.Data);
			break;
		case VariantType::Array:
			Deserialize(value, *(Array<Variant, HeapAllocation>*)v.AsData);
			break;
		case VariantType::Dictionary:
			Deserialize(value, *v.AsDictionary);
			break;
		case VariantType::Typename:
			CHECK(value.IsString());
			v.SetTypename(value.GetStringAnsiView());
			break;
		case VariantType::ManagedObject:
		case VariantType::Structure:
		{
#if USE_CSHARP
			auto obj = (MObject*)v;
        if (!obj && v.Type.TypeName)
        {
            MClass* klass = MUtils::GetClass(v.Type);
            if (!klass)
            {
                LOG(Error, "Invalid variant type {0}", v.Type);
                return;
            }
            obj = MCore::Object::New(klass);
            if (!obj)
            {
                LOG(Error, "Failed to managed instance of the variant type {0}", v.Type);
                return;
            }
            if (!klass->IsValueType())
                MCore::Object::Init(obj);
            if (v.Type.Type == VariantType::ManagedObject)
                v.SetManagedObject(obj);
        }
        ManagedSerialization::Deserialize(value, obj);
        if (v.Type.Type == VariantType::Structure)
            v = MUtils::UnboxVariant(obj);
#endif
			break;
		}
		default:
			Platform::CheckFailed("", __FILE__, __LINE__);
		}
	}*/

	bool Serialization::ShouldSerialize(const UID& v, const void* otherObj)
	{
		return v.IsValid();
	}

	void Serialization::Serialize(SerializeContext& context, const UID& v)
	{
		context.stream.UUID(v);
	}

	void Serialization::Deserialize(DeserializeContext& context, UID& v)
	{
		if (!context.stream->IsString() || context.stream->GetStringLength() != 32)
		{
			v = UID::Empty;
			return;
		}
		const char* a = context.stream->GetString();
		const char* b = a + 8;
		const char* c = b + 8;
		const char* d = c + 8;
		StringUtils::ParseHex(a, 8, &v.A);
		StringUtils::ParseHex(b, 8, &v.B);
		StringUtils::ParseHex(c, 8, &v.C);
		StringUtils::ParseHex(d, 8, &v.D);
	}

	bool Serialization::ShouldSerialize(const DateTime& v, const void* otherObj)
	{
		return !otherObj || v != *(DateTime*)otherObj;
	}

	void Serialization::Serialize(SerializeContext& context, const DateTime& v)
	{
		context.stream.DateTime(v);
	}

	void Serialization::Deserialize(DeserializeContext& context, DateTime& v)
	{
		v.Ticks = context.stream->GetInt64();
	}

	bool Serialization::ShouldSerialize(const TimeSpan& v, const void* otherObj)
	{
		return !otherObj || v != *(TimeSpan*)otherObj;
	}

/*	void Serialization::Serialize(SerializeContext& context, const TimeSpan& v)
	{
		context.stream.Int64(v.Ticks);
	}

	void Serialization::Deserialize(DeserializeContext& context, TimeSpan& v)
	{
		v.Ticks = context.stream.GetInt64();
	}

	bool Serialization::ShouldSerialize(const Version& v, const void* otherObj)
	{
		return !otherObj || v != *(Version*)otherObj;
	}

	void Serialization::Serialize(SerializeContext& context, const Version& v)
	{
		context.stream.String(v.ToString());
	}

	void Serialization::Deserialize(DeserializeContext& context, Version& v)
	{
		if (context.stream.IsString())
		{
			Version::Parse(context.stream.GetText(), &v);
		}
		else if (context.stream.IsObject())
		{
			const auto mMajor = SERIALIZE_FIND_MEMBER(stream, "Major");
			if (mMajor != context.stream.MemberEnd())
			{
				const auto mMinor = SERIALIZE_FIND_MEMBER(stream, "Minor");
				if (mMinor != context.stream.MemberEnd())
				{
					const auto mBuild = SERIALIZE_FIND_MEMBER(stream, "Build");
					if (mBuild != context.stream.MemberEnd())
					{
						const auto mRevision = SERIALIZE_FIND_MEMBER(stream, "Revision");
						if (mRevision != context.stream.MemberEnd())
							v = Version(mMajor->value.GetInt(), mMinor->value.GetInt(), mBuild->value.GetInt(), mRevision->value.GetInt());
						else
							v = Version(mMajor->value.GetInt(), mMinor->value.GetInt(), mBuild->value.GetInt());
					}
					else
						v = Version(mMajor->value.GetInt(), mMinor->value.GetInt());
				}
				else
					v = Version(mMajor->value.GetInt(), 0);
			}
			else
				v = Version();
		}
	}*/

	bool Serialization::ShouldSerialize(const Float2& v, const void* otherObj)
	{
		return !otherObj || !Float2::NearEqual(v, *(Float2*)otherObj, SERIALIZE_EPSILON);
	}

	void Serialization::Deserialize(DeserializeContext& context, Float2& v)
	{
		const auto mX = SERIALIZE_FIND_MEMBER(context.stream, "X");
		const auto mY = SERIALIZE_FIND_MEMBER(context.stream, "Y");
		v.x = mX != context.stream->MemberEnd() ? mX->value.GetFloat() : 0.0f;
		v.y = mY != context.stream->MemberEnd() ? mY->value.GetFloat() : 0.0f;
	}

	bool Serialization::ShouldSerialize(const Float3& v, const void* otherObj)
	{
		return !otherObj || !Float3::NearEqual(v, *(Float3*)otherObj, SERIALIZE_EPSILON);
	}

	void Serialization::Deserialize(DeserializeContext& context, Float3& v)
	{
		const auto mX = SERIALIZE_FIND_MEMBER(context.stream, "X");
		const auto mY = SERIALIZE_FIND_MEMBER(context.stream, "Y");
		const auto mZ = SERIALIZE_FIND_MEMBER(context.stream, "Z");
		v.x = mX != context.stream->MemberEnd() ? mX->value.GetFloat() : 0.0f;
		v.y = mY != context.stream->MemberEnd() ? mY->value.GetFloat() : 0.0f;
		v.z = mZ != context.stream->MemberEnd() ? mZ->value.GetFloat() : 0.0f;
	}

	bool Serialization::ShouldSerialize(const Float4& v, const void* otherObj)
	{
		return !otherObj || !Float4::NearEqual(v, *(Float4*)otherObj, SERIALIZE_EPSILON);
	}

	void Serialization::Deserialize(DeserializeContext& context, Float4& v)
	{
		const auto mX = SERIALIZE_FIND_MEMBER(context.stream, "X");
		const auto mY = SERIALIZE_FIND_MEMBER(context.stream, "Y");
		const auto mZ = SERIALIZE_FIND_MEMBER(context.stream, "Z");
		const auto mW = SERIALIZE_FIND_MEMBER(context.stream, "W");
		v.x = mX != context.stream->MemberEnd() ? mX->value.GetFloat() : 0.0f;
		v.y = mY != context.stream->MemberEnd() ? mY->value.GetFloat() : 0.0f;
		v.z = mZ != context.stream->MemberEnd() ? mZ->value.GetFloat() : 0.0f;
		v.w = mW != context.stream->MemberEnd() ? mW->value.GetFloat() : 0.0f;
	}


	bool Serialization::ShouldSerialize(const Int2& v, const void* otherObj)
	{
		return !otherObj || !(v == *(Int2*)otherObj);
	}

	void Serialization::Deserialize(DeserializeContext& context, Int2& v)
	{
		const auto mX = SERIALIZE_FIND_MEMBER(context.stream, "X");
		const auto mY = SERIALIZE_FIND_MEMBER(context.stream, "Y");
		v.x = mX != context.stream->MemberEnd() ? mX->value.GetInt() : 0;
		v.y = mY != context.stream->MemberEnd() ? mY->value.GetInt() : 0;
	}


	bool Serialization::ShouldSerialize(const Int4& v, const void* otherObj)
	{
		return !otherObj || !(v == *(Int4*)otherObj);
	}

	void Serialization::Deserialize(DeserializeContext& context, Int4& v)
	{
		const auto mX = SERIALIZE_FIND_MEMBER(context.stream, "X");
		const auto mY = SERIALIZE_FIND_MEMBER(context.stream, "Y");
		const auto mZ = SERIALIZE_FIND_MEMBER(context.stream, "Z");
		const auto mW = SERIALIZE_FIND_MEMBER(context.stream, "W");
		v.x = mX != context.stream->MemberEnd() ? mX->value.GetInt() : 0;
		v.y = mY != context.stream->MemberEnd() ? mY->value.GetInt() : 0;
		v.z = mZ != context.stream->MemberEnd() ? mZ->value.GetInt() : 0;
		v.w = mW != context.stream->MemberEnd() ? mW->value.GetInt() : 0;
	}

	bool Serialization::ShouldSerialize(const Quaternion& v, const void* otherObj)
	{
		return !otherObj || !Quaternion::NearEqual(v, *(Quaternion*)otherObj, SERIALIZE_EPSILON);
	}

	void Serialization::Deserialize(DeserializeContext& context, Quaternion& v)
	{
		const auto mX = SERIALIZE_FIND_MEMBER(context.stream, "X");
		const auto mY = SERIALIZE_FIND_MEMBER(context.stream, "Y");
		const auto mZ = SERIALIZE_FIND_MEMBER(context.stream, "Z");
		const auto mW = SERIALIZE_FIND_MEMBER(context.stream, "W");
		v.x = mX != context.stream->MemberEnd() ? mX->value.GetFloat() : 0.0f;
		v.y = mY != context.stream->MemberEnd() ? mY->value.GetFloat() : 0.0f;
		v.z = mZ != context.stream->MemberEnd() ? mZ->value.GetFloat() : 0.0f;
		v.w = mW != context.stream->MemberEnd() ? mW->value.GetFloat() : 0.0f;
	}

	bool Serialization::ShouldSerialize(const Color& v, const void* otherObj)
	{
		return !otherObj || !Color::NearEqual(v, *(Color*)otherObj, SERIALIZE_EPSILON);
	}

	void Serialization::Deserialize(DeserializeContext& context, Color& v)
	{
		const auto mR = SERIALIZE_FIND_MEMBER(context.stream, "R");
		const auto mG = SERIALIZE_FIND_MEMBER(context.stream, "G");
		const auto mB = SERIALIZE_FIND_MEMBER(context.stream, "B");
		const auto mA = SERIALIZE_FIND_MEMBER(context.stream, "A");
		v.r = mR != context.stream->MemberEnd() ? mR->value.GetFloat() : 0.0f;
		v.g = mG != context.stream->MemberEnd() ? mG->value.GetFloat() : 0.0f;
		v.b = mB != context.stream->MemberEnd() ? mB->value.GetFloat() : 0.0f;
		v.a = mA != context.stream->MemberEnd() ? mA->value.GetFloat() : 0.0f;
	}

	bool Serialization::ShouldSerialize(const Color32& v, const void* otherObj)
	{
		return !otherObj || v != *(Color32*)otherObj;
	}

	void Serialization::Serialize(SerializeContext& context, const Color32& v)
	{
		context.stream.StartObject();
		context.stream.JKEY("R");
		context.stream.Int(v.r);
		context.stream.JKEY("G");
		context.stream.Int(v.g);
		context.stream.JKEY("B");
		context.stream.Int(v.b);
		context.stream.JKEY("A");
		context.stream.Int(v.a);
		context.stream.EndObject();
	}

	void Serialization::Deserialize(DeserializeContext& context, Color32& v)
	{
		const auto mR = SERIALIZE_FIND_MEMBER(context.stream, "R");
		const auto mG = SERIALIZE_FIND_MEMBER(context.stream, "G");
		const auto mB = SERIALIZE_FIND_MEMBER(context.stream, "B");
		const auto mA = SERIALIZE_FIND_MEMBER(context.stream, "A");
		v.r = mR != context.stream->MemberEnd() ? mR->value.GetInt() : 0;
		v.g = mG != context.stream->MemberEnd() ? mG->value.GetInt() : 0;
		v.b = mB != context.stream->MemberEnd() ? mB->value.GetInt() : 0;
		v.a = mA != context.stream->MemberEnd() ? mA->value.GetInt() : 0;
	}

	bool Serialization::ShouldSerialize(const Rectangle& v, const void* otherObj)
	{
		return !otherObj || !Rectangle::NearEqual(v, *(Rectangle*)otherObj, SERIALIZE_EPSILON);
	}

	void Serialization::Deserialize(DeserializeContext& context, Rectangle& v)
	{
		DESERIALIZE_HELPER(context, "Location", v.Location, Float2::Zero);
		DESERIALIZE_HELPER(context, "Size", v.Size, Float2::Zero);
	}

	bool Serialization::ShouldSerialize(const Transform& v, const void* otherObj)
	{
		return !otherObj || !Transform::NearEqual(v, *(Transform*)otherObj, SERIALIZE_EPSILON);
	}

	void Serialization::Deserialize(DeserializeContext& context, Transform& v)
	{
		auto& lastStream = context.stream;

		{
			const auto m = SERIALIZE_FIND_MEMBER(context.stream, "Translation");
			if (m != context.stream->MemberEnd())
			{
				Float3 value = v.Translation;
				context.stream = &m->value;
				Deserialize(context, value);
			}
		}
		{
			const auto m = SERIALIZE_FIND_MEMBER(context.stream, "Scale");
			if (m != context.stream->MemberEnd())
			{
				Float3 value = v.Scale;
				context.stream = &m->value;
				Deserialize(context, value);
			}
		}
		{
			const auto m = SERIALIZE_FIND_MEMBER(context.stream, "Orientation");
			if (m != context.stream->MemberEnd())
			{
				Quaternion value = v.Orientation;
				context.stream = &m->value;
				Deserialize(context, value);
			}
		}

		context.stream = lastStream;
	}

	bool Serialization::ShouldSerialize(const Matrix& v, const void* otherObj)
	{
		return !otherObj || v != *(Matrix*)otherObj;
	}

	void Serialization::Deserialize(DeserializeContext& context, Matrix& v)
	{
		DESERIALIZE_HELPER(context, "M00", v.Values[0][0], 0);
		DESERIALIZE_HELPER(context, "M01", v.Values[0][1], 0);
		DESERIALIZE_HELPER(context, "M02", v.Values[0][2], 0);
		DESERIALIZE_HELPER(context, "M03", v.Values[0][3], 0);
		DESERIALIZE_HELPER(context, "M10", v.Values[1][0], 0);
		DESERIALIZE_HELPER(context, "M11", v.Values[1][1], 0);
		DESERIALIZE_HELPER(context, "M12", v.Values[1][2], 0);
		DESERIALIZE_HELPER(context, "M13", v.Values[1][3], 0);
		DESERIALIZE_HELPER(context, "M20", v.Values[2][0], 0);
		DESERIALIZE_HELPER(context, "M21", v.Values[2][1], 0);
		DESERIALIZE_HELPER(context, "M22", v.Values[2][2], 0);
		DESERIALIZE_HELPER(context, "M23", v.Values[2][3], 0);
		DESERIALIZE_HELPER(context, "M30", v.Values[3][0], 0);
		DESERIALIZE_HELPER(context, "M31", v.Values[3][1], 0);
		DESERIALIZE_HELPER(context, "M32", v.Values[3][2], 0);
		DESERIALIZE_HELPER(context, "M33", v.Values[3][3], 0);
	}

}

#undef DESERIALIZE_HELPER

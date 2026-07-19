#pragma once

#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Core/Types/Collections/Dictionary.h"
#include "Runtime/Core/Encoding.h"
#include "Runtime/Core/Types/BitFlags.h"
#include "ISerializable.h"
#include "Json.h"
#include "JsonWriter.h"


//struct Version;
//struct VariantType;
//template<typename T>
//class ScriptingObjectReference;
//template<typename T>
//class SoftObjectReference;
//template<typename T>
//class AssetReference;
//template<typename T>
//class WeakAssetReference;
//template<typename T>
//class SoftAssetReference;

namespace SE
{
	// The floating-point values serialization epsilon for equality checks precision
	#define SERIALIZE_EPSILON 1e-7f
	#define SERIALIZE_EPSILON_DOUBLE 1e-17

	// Helper macro to cast object on diff serialization
	#define SERIALIZE_GET_OTHER_OBJ(type, object) const auto other = static_cast<const type*>(object)

	// Helper macro to use find member in the stream by name (skips strlen call if string is constant)
	#define SERIALIZE_FIND_MEMBER(stream, name) stream->FindMember(Json::Value(name, ARRAY_SIZE(name) - 1))

	// Serialization helper macro
	#define SERIALIZE(name) \
		if (Serialization::ShouldSerialize(name, other ? &other->name : nullptr))			\
		{																					\
			context.stream.JKEY(#name);														\
			SerializeContext::ObjectScope scope(context, other ? &other->name : nullptr);	\
			Serialization::Serialize(context, name);										\
		}

	// Serialization helper macro (for private members, or with custom member name)
	#define SERIALIZE_MEMBER(name, member) \
		if (Serialization::ShouldSerialize(member, other ? &other->member : nullptr))	\
		{																				\
			context.stream.JKEY(#name);													\
			{																			\
				SerializeContext::ObjectScope scope(context, other ? &other->member : nullptr);	\
				Serialization::Serialize(context, member);										\
			}																					\
		}

	// Deserialization helper macro
	#define DESERIALIZE(name)												\
		{																	\
			const auto e = SERIALIZE_FIND_MEMBER(context.stream, #name);	\
			if (e != context.stream->MemberEnd())							\
			{																\
				DeserializeContext::StreamScope scope(context, e->value);	\
				Serialization::Deserialize(context, name);					\
			}																\
		}

	// Deserialization helper macro (for private members, or with custom member name)
	#define DESERIALIZE_MEMBER(name, member)								\
		{																	\
			const auto e = SERIALIZE_FIND_MEMBER(context.stream, #name);	\
			if (e != context.stream->MemberEnd())							\
			{																\
				DeserializeContext::StreamScope scope(context, e->value);	\
				Serialization::Deserialize(context, member);				\
			}																\
		}

	#define SERIALIZE_BIT(name)			\
	if (!other || name != other->name)	\
	{									\
		context.stream.JKEY(#name);				\
		context.stream.Bool(name != 0);			\
	}

	#define SERIALIZE_BIT_MEMBER(name, member)	\
	if (!other || member != other->member)		\
	{											\
		context.stream.JKEY(#name);						\
		context.stream.Bool(member != 0);				\
	}

	#define DESERIALIZE_BIT(name)										\
	{																	\
		const auto e = SERIALIZE_FIND_MEMBER(context.stream, #name);	\
		if (e != context.stream->MemberEnd() && e->value.IsBool())		\
			name = e->value.GetBool() ? 1 : 0;							\
	}

	#define DESERIALIZE_BIT_MEMBER(name, member)						\
	{																	\
		const auto e = SERIALIZE_FIND_MEMBER(context.stream, #name);	\
		if (e != context.stream->MemberEnd() && e->value.IsBool())		\
			member = e->value.GetBool() ? 1 : 0;						\
	}


	namespace Serialization
	{
		FORCE_INLINE int32 DeserializeInt(DeserializeContext& context)
		{
			int32 result = 0;
			if (context.stream->IsInt())
				result = context.stream->GetInt();
			else if (context.stream->IsFloat())
				result = (int32)context.stream->GetFloat();
			else if (context.stream->IsString())
				StringUtils::Parse(context.stream->GetString(), &result);
			return result;
		}

		// In-build types

		FORCE_INLINE bool ShouldSerialize(const bool& v, const void* otherObj)
		{
			return !otherObj || v != *(bool*)otherObj;
		}
		FORCE_INLINE void Serialize(SerializeContext& context, const bool& v)
		{
			context.stream.Bool(v);
		}
		FORCE_INLINE void Deserialize(DeserializeContext& context, bool& v)
		{
			v = context.stream->GetBool();
		}

		FORCE_INLINE bool ShouldSerialize(const int8& v, const void* otherObj)
		{
			return !otherObj || v != *(int8*)otherObj;
		}
		FORCE_INLINE void Serialize(SerializeContext& context, const int8& v)
		{
			context.stream.Int(v);
		}
		FORCE_INLINE void Deserialize(DeserializeContext& context, int8& v)
		{
			v = (int8)DeserializeInt(context);
		}

		FORCE_INLINE bool ShouldSerialize(const char& v, const void* otherObj)
		{
			return !otherObj || v != *(char*)otherObj;
		}
		FORCE_INLINE void Serialize(SerializeContext& context, const char& v)
		{
			context.stream.Int(v);
		}
		FORCE_INLINE void Deserialize(DeserializeContext& context, char& v)
		{
			v = (char)context.stream->GetInt();
		}

		FORCE_INLINE bool ShouldSerialize(const Char& v, const void* otherObj)
		{
			return !otherObj || v != *(Char*)otherObj;
		}
		FORCE_INLINE void Serialize(SerializeContext& context, const Char& v)
		{
			context.stream.Int(v);
		}
		FORCE_INLINE void Deserialize(DeserializeContext& context, Char& v)
		{
			v = (Char)context.stream->GetInt();
		}

		FORCE_INLINE bool ShouldSerialize(const int16& v, const void* otherObj)
		{
			return !otherObj || v != *(int16*)otherObj;
		}
		FORCE_INLINE void Serialize(SerializeContext& context, const int16& v)
		{
			context.stream.Int(v);
		}
		FORCE_INLINE void Deserialize(DeserializeContext& context, int16& v)
		{
			v = (int16)DeserializeInt(context);
		}

		FORCE_INLINE bool ShouldSerialize(const int32& v, const void* otherObj)
		{
			return !otherObj || v != *(int32*)otherObj;
		}
		FORCE_INLINE void Serialize(SerializeContext& context, const int32& v)
		{
			context.stream.Int(v);
		}
		FORCE_INLINE void Deserialize(DeserializeContext& context, int32& v)
		{
			v = DeserializeInt(context);
		}

		FORCE_INLINE bool ShouldSerialize(const int64& v, const void* otherObj)
		{
			return !otherObj || v != *(int64*)otherObj;
		}
		FORCE_INLINE void Serialize(SerializeContext& context, const int64& v)
		{
			context.stream.Int64(v);
		}
		FORCE_INLINE void Deserialize(DeserializeContext& context, int64& v)
		{
			v = context.stream->GetInt64();
		}

		FORCE_INLINE bool ShouldSerialize(const uint8& v, const void* otherObj)
		{
			return !otherObj || v != *(uint8*)otherObj;
		}
		FORCE_INLINE void Serialize(SerializeContext& context, const uint8& v)
		{
			context.stream.Uint(v);
		}
		FORCE_INLINE void Deserialize(DeserializeContext& context, uint8& v)
		{
			v = context.stream->GetUint();
		}

		FORCE_INLINE bool ShouldSerialize(const uint16& v, const void* otherObj)
		{
			return !otherObj || v != *(uint16*)otherObj;
		}
		FORCE_INLINE void Serialize(SerializeContext& context, const uint16& v)
		{
			context.stream.Uint(v);
		}
		FORCE_INLINE void Deserialize(DeserializeContext& context, uint16& v)
		{
			v = context.stream->GetUint();
		}

		FORCE_INLINE bool ShouldSerialize(const uint32& v, const void* otherObj)
		{
			return !otherObj || v != *(uint32*)otherObj;
		}
		FORCE_INLINE void Serialize(SerializeContext& context, const uint32& v)
		{
			context.stream.Uint(v);
		}
		FORCE_INLINE void Deserialize(DeserializeContext& context, uint32& v)
		{
			v = context.stream->GetUint();
		}

		FORCE_INLINE bool ShouldSerialize(const uint64& v, const void* otherObj)
		{
			return !otherObj || v != *(uint64*)otherObj;
		}
		FORCE_INLINE void Serialize(SerializeContext& context, const uint64& v)
		{
			context.stream.Uint64(v);
		}
		FORCE_INLINE void Deserialize(DeserializeContext& context, uint64& v)
		{
			v = context.stream->GetUint64();
		}

		FORCE_INLINE bool ShouldSerialize(const float& v, const void* otherObj)
		{
			return !otherObj || fabsf(v - *(float*)otherObj) > SERIALIZE_EPSILON;
		}
		FORCE_INLINE void Serialize(SerializeContext& context, const float& v)
		{
			context.stream.Float(v);
		}
		FORCE_INLINE void Deserialize(DeserializeContext& context, float& v)
		{
			v = context.stream->GetFloat();
		}

		FORCE_INLINE bool ShouldSerialize(const double& v, const void* otherObj)
		{
			return !otherObj || fabs(v - *(double*)otherObj) > SERIALIZE_EPSILON_DOUBLE;
		}
		FORCE_INLINE void Serialize(SerializeContext& context, const double& v)
		{
			context.stream.Double(v);
		}
		FORCE_INLINE void Deserialize(DeserializeContext& context, double& v)
		{
			v = context.stream->GetDouble();
		}

		// Enum

		template<typename T>
		FORCE_INLINE typename TEnableIf<TIsEnum<T>::Value, bool>::Type ShouldSerialize(const T& v, const void* otherObj)
		{
			return !otherObj || v != *(T*)otherObj;
		}
		template<typename T>
		FORCE_INLINE typename TEnableIf<TIsEnum<T>::Value>::Type Serialize(SerializeContext& context, const T& v)
		{
			context.stream.Uint((uint32)v);
		}
		template<typename T>
		FORCE_INLINE typename TEnableIf<TIsEnum<T>::Value>::Type Deserialize(DeserializeContext& context, T& v)
		{
			v = (T)DeserializeInt(context);
		}

		template<typename T>
		FORCE_INLINE bool ShouldSerialize(const EnumFlags<T>& v, const void* otherObj)
		{
			return !otherObj || v != *(EnumFlags<T>*)otherObj;
		}
		template<typename T>
		FORCE_INLINE void Serialize(SerializeContext& context, const EnumFlags<T>& v)
		{
			context.stream.Uint64(v.Get());
		}

		template<typename T>
		FORCE_INLINE void Deserialize(DeserializeContext& context, EnumFlags<T>& v)
		{
			v.Set(DeserializeInt(context));
		}

		// Common types

		SE_API_RUNTIME bool ShouldSerialize(const UID& v, const void* otherObj);
		SE_API_RUNTIME void Serialize(SerializeContext& context, const UID& v);
		SE_API_RUNTIME void Deserialize(DeserializeContext& context, UID& v);

		SE_API_RUNTIME bool ShouldSerialize(const DateTime& v, const void* otherObj);
		SE_API_RUNTIME void Serialize(SerializeContext& context, const DateTime& v);
		SE_API_RUNTIME void Deserialize(DeserializeContext& context, DateTime& v);

		SE_API_RUNTIME bool ShouldSerialize(const TimeSpan& v, const void* otherObj);
		SE_API_RUNTIME void Serialize(SerializeContext& context, const TimeSpan& v);
		SE_API_RUNTIME void Deserialize(DeserializeContext& context, TimeSpan& v);

		FORCE_INLINE bool ShouldSerialize(const String& v, const void* otherObj)
		{
			return !otherObj || v != *(String*)otherObj;
		}
		FORCE_INLINE void Serialize(SerializeContext& context, const String& v)
		{
			context.stream.String(v);
		}
		FORCE_INLINE void Deserialize(DeserializeContext& context, String& v)
		{
			v = context.stream->GetString();
		}

		FORCE_INLINE bool ShouldSerialize(const StringAnsi& v, const void* otherObj)
		{
			return !otherObj || v != *(StringAnsi*)otherObj;
		}
		FORCE_INLINE void Serialize(SerializeContext& context, const StringAnsi& v)
		{
			context.stream.String(v);
		}
		FORCE_INLINE void Deserialize(DeserializeContext& context, StringAnsi& v)
		{
			v = context.stream->GetString();
		}


		// Math types

		SE_API_RUNTIME bool ShouldSerialize(const Float2& v, const void* otherObj);
		FORCE_INLINE void Serialize(SerializeContext& context, const Float2& v)
		{
			context.stream.Float2(v);
		}
		SE_API_RUNTIME void Deserialize(DeserializeContext& context, Float2& v);

		SE_API_RUNTIME bool ShouldSerialize(const Float3& v, const void* otherObj);
		FORCE_INLINE void Serialize(SerializeContext& context, const Float3& v)
		{
			context.stream.Float3(v);
		}
		SE_API_RUNTIME void Deserialize(DeserializeContext& context, Float3& v);

		SE_API_RUNTIME bool ShouldSerialize(const Float4& v, const void* otherObj);
		FORCE_INLINE void Serialize(SerializeContext& context, const Float4& v)
		{
			context.stream.Float4(v);
		}
		SE_API_RUNTIME void Deserialize(DeserializeContext& context, Float4& v);



		SE_API_RUNTIME bool ShouldSerialize(const Int2& v, const void* otherObj);
		FORCE_INLINE void Serialize(SerializeContext& context, const Int2& v)
		{
			context.stream.Int2(v);
		}
		SE_API_RUNTIME void Deserialize(DeserializeContext& context, Int2& v);


		SE_API_RUNTIME bool ShouldSerialize(const Int4& v, const void* otherObj);
		FORCE_INLINE void Serialize(SerializeContext& context, const Int4& v)
		{
			context.stream.Int4(v);
		}
		SE_API_RUNTIME void Deserialize(DeserializeContext& context, Int4& v);

		SE_API_RUNTIME bool ShouldSerialize(const Quaternion& v, const void* otherObj);
		FORCE_INLINE void Serialize(SerializeContext& context, const Quaternion& v)
		{
			context.stream.Quaternion(v);
		}
		SE_API_RUNTIME void Deserialize(DeserializeContext& context, Quaternion& v);

		SE_API_RUNTIME bool ShouldSerialize(const Color& v, const void* otherObj);
		FORCE_INLINE void Serialize(SerializeContext& context, const Color& v)
		{
			context.stream.Color(v);
		}
		SE_API_RUNTIME void Deserialize(DeserializeContext& context, Color& v);

		SE_API_RUNTIME bool ShouldSerialize(const Color32& v, const void* otherObj);
		SE_API_RUNTIME void Serialize(SerializeContext& context, const Color32& v);
		SE_API_RUNTIME void Deserialize(DeserializeContext& context, Color32& v);

		SE_API_RUNTIME bool ShouldSerialize(const BoundingBox& v, const void* otherObj);
		FORCE_INLINE void Serialize(SerializeContext& context, const BoundingBox& v)
		{
			context.stream.BoundingBox(v);
		}
		SE_API_RUNTIME void Deserialize(DeserializeContext& context, BoundingBox& v);

		SE_API_RUNTIME bool ShouldSerialize(const BoundingSphere& v, const void* otherObj);
		FORCE_INLINE void Serialize(SerializeContext& context, const BoundingSphere& v)
		{
			context.stream.BoundingSphere(v);
		}
		SE_API_RUNTIME void Deserialize(DeserializeContext& context, BoundingSphere& v);

		SE_API_RUNTIME bool ShouldSerialize(const Ray& v, const void* otherObj);
		FORCE_INLINE void Serialize(SerializeContext& context, const Ray& v)
		{
			context.stream.Ray(v);
		}
		SE_API_RUNTIME void Deserialize(DeserializeContext& context, Ray& v);

		SE_API_RUNTIME bool ShouldSerialize(const Rectangle& v, const void* otherObj);
		FORCE_INLINE void Serialize(SerializeContext& context, const Rectangle& v)
		{
			context.stream.Rectangle(v);
		}
		SE_API_RUNTIME void Deserialize(DeserializeContext& context, Rectangle& v);

		SE_API_RUNTIME bool ShouldSerialize(const Transform& v, const void* otherObj);
		FORCE_INLINE void Serialize(SerializeContext& context, const Transform& v)
		{
			context.stream.Transform(v, (const Transform*)context.otherObj);
		}
		SE_API_RUNTIME void Deserialize(DeserializeContext& context, Transform& v);

		SE_API_RUNTIME bool ShouldSerialize(const Matrix& v, const void* otherObj);
		FORCE_INLINE void Serialize(SerializeContext& context, const Matrix& v)
		{
			context.stream.Matrix(v);
		}
		SE_API_RUNTIME void Deserialize(DeserializeContext& context, Matrix& v);

		// ISerializable

		FORCE_INLINE bool ShouldSerialize(const ISerializable& v, const void* otherObj)
		{
			return true;
		}
		FORCE_INLINE void Serialize(SerializeContext& context, const ISerializable& v)
		{
			context.stream.StartObject();
			const_cast<ISerializable*>(&v)->Serialize(context);
			context.stream.EndObject();
		}
		FORCE_INLINE void Deserialize(DeserializeContext& context, ISerializable& v)
		{
			v.Deserialize(context);
		}

		template<typename T>
		FORCE_INLINE typename TEnableIf<TIsBaseOf<ISerializable, T>::Value, bool>::Type ShouldSerialize(const ISerializable& v, const void* otherObj)
		{
			return true;
		}
		template<typename T>
		FORCE_INLINE typename TEnableIf<TIsBaseOf<ISerializable, T>::Value>::Type Serialize(SerializeContext& context, const ISerializable& v)
		{
			context.stream.StartObject();
			const_cast<ISerializable*>(&v)->Serialize(context);
			context.stream.EndObject();
		}
		template<typename T>
		FORCE_INLINE typename TEnableIf<TIsBaseOf<ISerializable, T>::Value>::Type Deserialize(DeserializeContext& context, ISerializable& v)
		{
			v.Deserialize(context);
		}
/*

		// Scripting Object

		template<typename T>
		FORCE_INLINE typename TEnableIf<TIsBaseOf<ScriptingObject, T>::Value, bool>::Type ShouldSerialize(const T*& v, const void* otherObj)
		{
			return !otherObj || v != *(T**)otherObj;
		}
		template<typename T>
		FORCE_INLINE typename TEnableIf<TIsBaseOf<ScriptingObject, T>::Value>::Type Serialize(SerializeContext& context, const T*& v)
		{
			context.stream.Guid(v ? v->GetID() : Guid::Empty);
		}
		template<typename T>
		FORCE_INLINE typename TEnableIf<TIsBaseOf<ScriptingObject, T>::Value>::Type Deserialize(DeserializeContext& context, T*& v)
		{
			Guid id;
			Deserialize(stream, id);
			modifier->IdsMapping.TryGet(id, id);
			v = (T*)::FindObject(id, T::GetStaticClass());
		}

		// Scripting Object Reference

		template<typename T>
		FORCE_INLINE bool ShouldSerialize(const ScriptingObjectReference<T>& v, const void* otherObj)
		{
			return !otherObj || v.Get() != ((ScriptingObjectReference<T>*)otherObj)->Get();
		}
		template<typename T>
		FORCE_INLINE void Serialize(SerializeContext& context, const ScriptingObjectReference<T>& v)
		{
			context.stream.Guid(v.GetID());
		}
		template<typename T>
		FORCE_INLINE void Deserialize(DeserializeContext& context, ScriptingObjectReference<T>& v)
		{
			Guid id;
			Deserialize(stream, id);
			modifier->IdsMapping.TryGet(id, id);
			v = id;
		}

		// Soft Object Reference

		template<typename T>
		FORCE_INLINE bool ShouldSerialize(const SoftObjectReference<T>& v, const void* otherObj)
		{
			return !otherObj || v.Get() != ((SoftObjectReference<T>*)otherObj)->Get();
		}
		template<typename T>
		FORCE_INLINE void Serialize(SerializeContext& context, const SoftObjectReference<T>& v)
		{
			context.stream.Guid(v.GetID());
		}
		template<typename T>
		FORCE_INLINE void Deserialize(DeserializeContext& context, SoftObjectReference<T>& v)
		{
			Guid id;
			Deserialize(stream, id);
			modifier->IdsMapping.TryGet(id, id);
			v = id;
		}

		// Soft Asset Reference

		template<typename T>
		FORCE_INLINE bool ShouldSerialize(const SoftAssetReference<T>& v, const void* otherObj)
		{
			return !otherObj || v.Get() != ((SoftAssetReference<T>*)otherObj)->Get();
		}
		template<typename T>
		FORCE_INLINE void Serialize(SerializeContext& context, const SoftAssetReference<T>& v)
		{
			context.stream.Guid(v.GetID());
		}
		template<typename T>
		FORCE_INLINE void Deserialize(DeserializeContext& context, SoftAssetReference<T>& v)
		{
			Guid id;
			Deserialize(stream, id);
			v = id;
		}
*/

		// List

		template<typename T, typename AllocationType = HeapAllocation>
		FORCE_INLINE bool ShouldSerialize(const List<T, AllocationType>& v, const void* otherObj)
		{
			if (!otherObj)
				return true;
			const auto other = (const List<T, AllocationType>*)otherObj;
			if (v.Count() != other->Count())
				return true;
			const T* vPtr = v.Get();
			for (int32 i = 0; i < v.Count(); i++)
			{
				if (ShouldSerialize(vPtr[i], (const void*)&other->At(i)))
					return true;
			}
			return false;
		}
		template<typename T, typename AllocationType = HeapAllocation>
		FORCE_INLINE void Serialize(SerializeContext& context, const List<T, AllocationType>& v)
		{
			context.stream.StartArray();
			const T* vPtr = v.Get();
			for (int32 i = 0; i < v.Count(); i++)
				Serialize(context, vPtr[i]);
			context.stream.EndArray();
		}
		template<typename T, typename AllocationType = HeapAllocation>
		FORCE_INLINE void Deserialize(DeserializeContext& context, List<T, AllocationType>& v)
		{
			if (context.stream->IsArray())
			{
				const auto& streamArray = context.stream->GetArray();
				v.Resize(streamArray.Size());
				T* vPtr = v.Get();
				for (int32 i = 0; i < v.Count(); i++)
					Deserialize(streamArray[i], vPtr[i]);
			}
			else if (TIsPODType<T>::Value && context.stream->IsString())
			{
				// T[] encoded as Base64
				const StringAnsiView streamView(context.stream->GetString());
				v.Resize(Encoding::Base64::DecodeLength(streamView.Get(), streamView.Length()) / sizeof(T));
				Encoding::Base64::Decode((uint8*)*streamView, streamView.Length(), (uint8*)v.Get());
			}
		}

		// Dictionary

		template<typename KeyType, typename ValueType>
		FORCE_INLINE bool ShouldSerialize(SerializeContext& context, const Dictionary<KeyType, ValueType>& v)
		{
			if (!context.otherObj)
				return true;
			const auto other = (const Dictionary<KeyType, ValueType>*)context.otherObj;
			if (v.Count() != other->Count())
				return true;
			for (auto& i : v)
			{
				if (!other->ContainsKey(i.Key) || ShouldSerialize(i.value, (const void*)&other->At(i.Key)))
					return true;
			}
			return false;
		}
		template<typename KeyType, typename ValueType>
		FORCE_INLINE void Serialize(SerializeContext& context, const Dictionary<KeyType, ValueType>& v)
		{
			context.stream.StartArray();
			for (auto& i : v)
			{
				context.stream.StartObject();
				context.stream.JKEY("Key");
				Serialize(context, i.Key);
				context.stream.JKEY("Value");
				Serialize(context, i.value);
				context.stream.EndObject();
			}
			context.stream.EndArray();
		}
		template<typename KeyType, typename ValueType>
		FORCE_INLINE void Deserialize(DeserializeContext& context, Dictionary<KeyType, ValueType>& v)
		{
			v.Clear();
			if (context.stream->IsArray())
			{
				const auto& streamArray = context.stream->GetArray();
				const int32 size = streamArray.Size();
				v.EnsureCapacity(size * 3);
				for (int32 i = 0; i < size; i++)
				{
					auto& streamItem = streamArray[i];
					const auto mKey = SERIALIZE_FIND_MEMBER((&streamItem), "Key");
					const auto mValue = SERIALIZE_FIND_MEMBER((&streamItem), "Value");
					if (mKey != streamItem.MemberEnd() && mValue != streamItem.MemberEnd())
					{
						KeyType key;
						Deserialize(mKey->value, key);
						Deserialize(mValue->value, v[key]);
					}
				}
			}
			else if (context.stream->IsObject())
			{
				const int32 size = context.stream->MemberCount();
				v.EnsureCapacity(size * 3);
				for (auto i = context.stream->MemberBegin(); i != context.stream->MemberEnd(); ++i)
				{
					KeyType key;
					Deserialize(i->name, key);
					Deserialize(i->value, v[key]);
				}
			}
		}
	}
}

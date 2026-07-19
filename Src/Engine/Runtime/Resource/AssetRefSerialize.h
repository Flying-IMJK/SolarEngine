#pragma once

#include "AssetRef.h"

#include "Runtime/Core/Serialization/Serialization.h"

namespace SE
{
	/*namespace Serialization
	{
		// Asset Reference
		template<typename T>
		FORCE_INLINE bool ShouldSerialize(const AssetRef<T>& v, const void* otherObj)
		{
			return !otherObj || v.Get() != ((AssetRef<T>*)otherObj)->Get();
		}

		template<typename T>
		FORCE_INLINE void Serialize(ISerializable::SerializeStream& stream, const AssetRef<T>& v, const void* otherObj)
		{
			stream.UUID(v.GetID());
		}
		template<typename T>
		FORCE_INLINE void Deserialize(ISerializable::DeserializeStream& stream, AssetRef<T>& v)
		{
			UID id;
			Deserialize(stream, id);
			v = id;
		}

		// Weak Asset Reference

		template<typename T>
		FORCE_INLINE bool ShouldSerialize(const WeakAssetRef<T>& v, const void* otherObj)
		{
			return !otherObj || v.Get() != ((WeakAssetRef<T>*)otherObj)->Get();
		}
		template<typename T>
		FORCE_INLINE void Serialize(ISerializable::SerializeStream& stream, const WeakAssetRef<T>& v, const void* otherObj)
		{
			stream.UUID(v.GetID());
		}
		template<typename T>
		FORCE_INLINE void Deserialize(ISerializable::DeserializeStream& stream, WeakAssetRef<T>& v)
		{
			UID id;
			Deserialize(stream, id);
			v = id;
		}
	}*/

}

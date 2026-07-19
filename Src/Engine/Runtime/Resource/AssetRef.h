#pragma once

#include "Runtime/Core/Serialization/Serialization.h"

#include "AssetContent.h"
#include "Runtime/Resource/Asset.h"

namespace SE
{
	/// <summary>
	/// Asset reference utility. Keeps reference to the linked asset object and handles load/unload events.
	/// </summary>
	class SE_API_RUNTIME AssetRefBase
	{
		NON_COPYABLE(AssetRefBase)
	protected:
		Asset* _asset = nullptr;

	public:
		/// <summary>
		/// The asset loaded event (fired when asset gets loaded or is already loaded after change).
		/// </summary>
		Action Loaded;

		/// <summary>
		/// The asset unloading event (should cleanup refs to it).
		/// </summary>
		Action Unload;

		/// <summary>
		/// Action fired when field gets changed (link a new asset or change to the another value).
		/// </summary>
		Action Changed;

	public:

		/// <summary>
		/// Initializes a new instance of the <see cref="AssetReferenceBase"/> class.
		/// </summary>
		AssetRefBase() = default;

		/// <summary>
		/// Finalizes an instance of the <see cref="AssetReferenceBase"/> class.
		/// </summary>
		~AssetRefBase();

	public:
		/// <summary>
		/// Gets the asset ID or Guid::Empty if not set.
		/// </summary>
		FORCE_INLINE UID GetID() const
		{
			return _asset ? _asset->GetID() : UID::Empty;
		}

		/// <summary>
		/// 是否存在引用
		/// </summary>
		FORCE_INLINE bool HasReference() const
		{
			return _asset != nullptr;
		}

		/// <summary>
		/// Gets the asset property value as string.
		/// </summary>
		String ToString() const;

	protected:
		void OnSet(Asset* asset);
		void OnLoaded(Asset* asset);
		void OnUnloaded(Asset* asset);
	};

	/// <summary>
	/// Asset reference utility. Keeps reference to the linked asset object and handles load/unload events.
	/// </summary>
	template<typename T>
	class AssetRef : public AssetRefBase
	{
	public:
		typedef T AssetType;
		typedef AssetRef<T> Type;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="AssetReference"/> class.
		/// </summary>
		AssetRef()
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="AssetReference"/> class.
		/// </summary>
		/// <param name="asset">The asset to set.</param>
		AssetRef(T* asset)
		{
			OnSet(asset);
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="AssetReference"/> class.
		/// </summary>
		/// <param name="other">The other.</param>
		AssetRef(const AssetRef& other)
		{
			OnSet(other._asset);
		}

		AssetRef(AssetRef&& other)
		{
			OnSet(other._asset);
			other.OnSet(nullptr);
		}

		AssetRef& operator=(AssetRef&& other)
		{
			if (&other != this)
			{
				OnSet(other._asset);
				other.OnSet(nullptr);
			}
			return *this;
		}

		/// <summary>
		/// Finalizes an instance of the <see cref="AssetReference"/> class.
		/// </summary>
		~AssetRef()
		{
		}

	public:
		FORCE_INLINE AssetRef& operator=(const AssetRef& other)
		{
			OnSet(other._asset);
			return *this;
		}

		FORCE_INLINE AssetRef& operator=(T* other)
		{
			OnSet((Asset*)other);
			return *this;
		}

		FORCE_INLINE AssetRef& operator=(const UID& id)
		{
			OnSet( AssetContent::LoadAsync(id, Typeof<T>()));
			return *this;
		}

		FORCE_INLINE bool operator==(T* other) const
		{
			return _asset == other;
		}

		FORCE_INLINE bool operator==(const AssetRef& other) const
		{
			return _asset == other._asset;
		}

		FORCE_INLINE bool operator!=(T* other) const
		{
			return _asset != other;
		}

		FORCE_INLINE bool operator!=(const AssetRef& other) const
		{
			return _asset != other._asset;
		}

		/// <summary>
		/// Implicit conversion to the bool.
		/// </summary>
		FORCE_INLINE operator T*() const
		{
			return (T*)_asset;
		}

		/// <summary>
		/// Implicit conversion to the asset.
		/// </summary>
		FORCE_INLINE operator bool() const
		{
			return _asset != nullptr;
		}

		/// <summary>
		/// Implicit conversion to the asset.
		/// </summary>
		FORCE_INLINE T* operator->() const
		{
			return (T*)_asset;
		}

		/// <summary>
		/// Gets the asset.
		/// </summary>
		FORCE_INLINE T* Get() const
		{
			return (T*)_asset;
		}

		/// <summary>
		/// Gets the asset as a given type (static cast).
		/// </summary>
		template<typename U>
		FORCE_INLINE U* As() const
		{
			return (U*)_asset;
		}

	public:
		/// <summary>
		/// Sets the asset reference.
		/// </summary>
		/// <param name="asset">The asset.</param>
		void Set(T* asset)
		{
			OnSet(asset);
		}
	};

	template<typename T>
	uint32 GetHash(const AssetRef<T>& key)
	{
		return GetHash(key.GetID());
	}


	/// <summary>
	/// The asset soft reference. Asset gets referenced (loaded) on actual use (ID reference is resolving it).
	/// </summary>
	class SE_API_RUNTIME SoftAssetRefBase
	{
	protected:
		Asset* _asset = nullptr;
		UID _id = UID::Empty;

	public:
		/// <summary>
		/// Action fired when field gets changed (link a new asset or change to the another value).
		/// </summary>
		Delegate<> Changed;

	public:
		NON_COPYABLE(SoftAssetRefBase);

		/// <summary>
		/// Initializes a new instance of the <see cref="SoftAssetReferenceBase"/> class.
		/// </summary>
		SoftAssetRefBase() = default;

		/// <summary>
		/// Finalizes an instance of the <see cref="SoftAssetReferenceBase"/> class.
		/// </summary>
		~SoftAssetRefBase();

	public:
		/// <summary>
		/// Gets the asset ID or Guid::Empty if not set.
		/// </summary>
		FORCE_INLINE UID GetID() const
		{
			return _id;
		}

		/// <summary>
		/// Gets the asset property value as string.
		/// </summary>
		String ToString() const;

	protected:
		void OnSet(Asset* asset);
		void OnSet(const UID& id);
		// void OnResolve(const ScriptingTypeHandle& type);
		void OnUnloaded(Asset* asset);
	};

	/// <summary>
	/// The asset soft reference. Asset gets referenced (loaded) on actual use (ID reference is resolving it).
	/// </summary>
	template<typename T>
	class SoftAssetRef : public SoftAssetRefBase
	{
	public:
		typedef T AssetType;
		typedef SoftAssetRef<T> Type;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="SoftAssetReference"/> class.
		/// </summary>
		SoftAssetRef()
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="SoftAssetReference"/> class.
		/// </summary>
		/// <param name="asset">The asset to set.</param>
		SoftAssetRef(T* asset)
		{
			OnSet(asset);
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="SoftAssetReference"/> class.
		/// </summary>
		/// <param name="other">The other.</param>
		SoftAssetRef(const SoftAssetRef& other)
		{
			OnSet(other.GetID());
		}

		SoftAssetRef(const UID& id)
		{
			OnSet(id);
		}

		SoftAssetRef(SoftAssetRef&& other)
		{
			OnSet(other.GetID());
			other.OnSet(UID::Empty);
		}

		/// <summary>
		/// Finalizes an instance of the <see cref="SoftAssetReference"/> class.
		/// </summary>
		~SoftAssetRef()
		{
		}

	public:
		FORCE_INLINE bool operator==(T* other) const
		{
			return Get() == other;
		}
		FORCE_INLINE bool operator==(const SoftAssetRef& other) const
		{
			return GetID() == other.GetID();
		}
		FORCE_INLINE bool operator==(const UID& other) const
		{
			return GetID() == other;
		}
		FORCE_INLINE bool operator!=(T* other) const
		{
			return Get() != other;
		}
		FORCE_INLINE bool operator!=(const SoftAssetRef& other) const
		{
			return GetID() != other.GetID();
		}
		FORCE_INLINE bool operator!=(const UID& other) const
		{
			return GetID() != other;
		}
		SoftAssetRef& operator=(const SoftAssetRef& other)
		{
			if (this != &other)
				OnSet(other.GetID());
			return *this;
		}
		SoftAssetRef& operator=(SoftAssetRef&& other)
		{
			if (this != &other)
			{
				OnSet(other.GetID());
				other.OnSet(nullptr);
			}
			return *this;
		}
		FORCE_INLINE SoftAssetRef& operator=(const T& other)
		{
			OnSet(&other);
			return *this;
		}
		FORCE_INLINE SoftAssetRef& operator=(T* other)
		{
			OnSet((Asset*)other);
			return *this;
		}
		FORCE_INLINE SoftAssetRef& operator=(const UID& id)
		{
			OnSet(id);
			return *this;
		}
		FORCE_INLINE operator T*() const
		{
			return (T*)Get();
		}
		FORCE_INLINE operator bool() const
		{
			return Get() != nullptr;
		}
		FORCE_INLINE T* operator->() const
		{
			return (T*)Get();
		}
		template<typename U>
		FORCE_INLINE U* As() const
		{
			return static_cast<U*>(Get());
		}

	public:
		/// <summary>
		/// Gets the asset (or null if unassigned).
		/// </summary>
		T* Get() const
		{
			if (!_asset)
			{
				// const_cast<SoftAssetReference*>(this)->OnResolve(T::TypeInitializer);
			}
			return (T*)_asset;
		}

		/// <summary>
		/// Gets managed instance object (or null if no asset linked).
		/// </summary>
		/*MObject* GetManagedInstance() const
		{
			auto asset = Get();
			return asset ? asset->GetOrCreateManagedInstance() : nullptr;
		}*/

		/// <summary>
		/// Determines whether asset is assigned and managed instance of the asset is alive.
		/// </summary>
		bool HasManagedInstance() const
		{
			auto asset = Get();
			return asset && asset->HasManagedInstance();
		}

		/// <summary>
		/// Gets the managed instance object or creates it if missing or null if not assigned.
		/// </summary>
		/*MObject* GetOrCreateManagedInstance() const
		{
			auto asset = Get();
			return asset ? asset->GetOrCreateManagedInstance() : nullptr;
		}*/

		/// <summary>
		/// Sets the asset.
		/// </summary>
		/// <param name="id">The object ID. Uses Scripting to find the registered asset of the given ID.</param>
		FORCE_INLINE void Set(const UID& id)
		{
			OnSet(id);
		}

		/// <summary>
		/// Sets the asset.
		/// </summary>
		/// <param name="asset">The asset.</param>
		FORCE_INLINE void Set(T* asset)
		{
			OnSet(asset);
		}
	};

	template<typename T>
	uint32 GetHash(const SoftAssetRef<T>& key)
	{
		return GetHash(key.GetID());
	}


	/// <summary>
	/// Asset reference utility that doesn't add reference to that asset. Handles asset unload event.
	/// </summary>
	class WeakAssetRefBase
	{
		NON_COPYABLE(WeakAssetRefBase)
		typedef Delegate<> EventType;

	protected:
		Asset* _asset = nullptr;

	public:
		/// <summary>
		/// The asset unloading event (should cleanup refs to it).
		/// </summary>
		EventType Unload;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="WeakAssetReferenceBase"/> class.
		/// </summary>
		WeakAssetRefBase() = default;

		/// <summary>
		/// Finalizes an instance of the <see cref="WeakAssetReferenceBase"/> class.
		/// </summary>
		~WeakAssetRefBase();

	public:
		/// <summary>
		/// Gets the asset ID or SGUID::Empty if not set.
		/// </summary>
		FORCE_INLINE UID GetID() const
		{
			return _asset ? _asset->GetID() : UID::Empty;
		}

		/// <summary>
		/// Gets managed instance object (or null if no asset set).
		/// </summary>
		/*		FORCE_INLINE MObject* GetManagedInstance() const
				{
					return _asset ? _asset->GetOrCreateManagedInstance() : nullptr;
				}*/

		/// <summary>
		/// Gets the asset property value as string.
		/// </summary>
		String ToString() const;

	protected:
		void OnSet(Asset* asset);
		void OnUnloaded(Asset* asset);
	};

	/// <summary>
	/// Asset reference utility that doesn't add reference to that asset. Handles asset unload event.
	/// </summary>
	template<typename T>
	class WeakAssetRef : public WeakAssetRefBase
	{
		static_assert(TIsBaseOf<Asset, T>::Value);
	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="WeakAssetReference"/> class.
		/// </summary>
		WeakAssetRef()
			: WeakAssetRefBase()
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="WeakAssetReference"/> class.
		/// </summary>
		/// <param name="asset">The asset to set.</param>
		WeakAssetRef(T* asset)
			: WeakAssetRefBase()
		{
			OnSet(asset);
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="WeakAssetReference"/> class.
		/// </summary>
		/// <param name="other">The other.</param>
		WeakAssetRef(const WeakAssetRef& other)
		{
			OnSet(other.Get());
		}

		WeakAssetRef(WeakAssetRef&& other)
		{
			OnSet(other.Get());
			other.OnSet(nullptr);
		}

		WeakAssetRef& operator=(WeakAssetRef&& other)
		{
			if (&other != this)
			{
				OnSet(other.Get());
				other.OnSet(nullptr);
			}
			return *this;
		}

		/// <summary>
		/// Finalizes an instance of the <see cref="WeakAssetReference"/> class.
		/// </summary>
		~WeakAssetRef()
		{
		}

	public:
		FORCE_INLINE WeakAssetRef& operator=(const WeakAssetRef& other)
		{
			OnSet(other.Get());
			return *this;
		}

		FORCE_INLINE WeakAssetRef& operator=(T* other)
		{
			OnSet((Asset*)other);
			return *this;
		}

		FORCE_INLINE WeakAssetRef& operator=(const UID& id)
		{
			OnSet((Asset*)AssetContent::LoadAsync(id, Typeof<T>()));
			return *this;
		}

		FORCE_INLINE bool operator==(T* other) const
		{
			return _asset == other;
		}

		FORCE_INLINE bool operator==(const WeakAssetRef& other) const
		{
			return _asset == other._asset;
		}

		/// <summary>
		/// Implicit conversion to the bool.
		/// </summary>
		FORCE_INLINE operator T*() const
		{
			return (T*)_asset;
		}

		/// <summary>
		/// Implicit conversion to the asset.
		/// </summary>
		FORCE_INLINE operator bool() const
		{
			return _asset != nullptr;
		}

		/// <summary>
		/// Implicit conversion to the asset.
		/// </summary>
		FORCE_INLINE T* operator->() const
		{
			return (T*)_asset;
		}

		/// <summary>
		/// Gets the asset.
		/// </summary>
		FORCE_INLINE T* Get() const
		{
			return (T*)_asset;
		}

		/// <summary>
		/// Gets the asset as a given type (static cast).
		/// </summary>
		template<typename U>
		FORCE_INLINE U* As() const
		{
			return (U*)_asset;
		}

	public:
		/// <summary>
		/// Sets the asset reference.
		/// </summary>
		/// <param name="asset">The asset.</param>
		void Set(T* asset)
		{
			OnSet(asset);
		}
	};

	template<typename T>
	uint32 GetHash(const WeakAssetRef<T>& key)
	{
		return GetHash(key.GetID());
	}

	namespace Serialization
	{
		// Asset Reference

		template<typename T>
		inline bool ShouldSerialize(const AssetRef<T>& v, const void* otherObj)
		{
			return !otherObj || v.Get() != ((AssetRef<T>*)otherObj)->Get();
		}
		template<typename T>
		inline void Serialize(SerializeContext& context, const AssetRef<T>& v)
		{
			context.stream.UUID(v.GetID());
		}
		template<typename T>
		inline void Deserialize(DeserializeContext& context, AssetRef<T>& v)
		{
			UID id;
			Deserialize(context, id);
			v = id;
		}

		// Weak Asset Reference

		template<typename T>
		inline bool ShouldSerialize(const WeakAssetRef<T>& v, const void* otherObj)
		{
			return !otherObj || v.Get() != ((WeakAssetRef<T>*)otherObj)->Get();
		}
		template<typename T>
		inline void Serialize(SerializeContext& context, const WeakAssetRef<T>& v)
		{
			context.stream.UUID(v.GetID());
		}
		template<typename T>
		inline void Deserialize(DeserializeContext& context, WeakAssetRef<T>& v)
		{
			UID id;
			Deserialize(context, id);
			v = id;
		}

		// Soft Asset Reference
		template<typename T>
		inline bool ShouldSerialize(const SoftAssetRef<T>& v, const void* otherObj)
		{
			return !otherObj || v.Get() != ((SoftAssetRef<T>*)otherObj)->Get();
		}
		template<typename T>
		inline void Serialize(SerializeContext& context, const SoftAssetRef<T>& v)
		{
			context.stream.UUID(v.GetID());
		}
		template<typename T>
		inline void Deserialize(DeserializeContext& context, SoftAssetRef<T>& v)
		{
			UID id;
			Deserialize(context, id);
			v = id;
		}
	}
}

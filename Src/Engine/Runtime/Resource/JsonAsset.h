#pragma once

#include "Runtime/Core/Serialization/Json.h"
#include "Runtime/Core/Serialization/ISerializable.h"
#include "Runtime/Resource/Asset.h"

namespace SE
{
    /// <summary>
    /// Base class for all Json-format assets.
    /// </summary>
    /// <seealso cref="Asset" />
	SE_CLASS(Reflect, API, NoSpawn, Abstract)
    class SE_API_RUNTIME JsonAssetBase : public Asset
    {
        SE_DEFINE_CLASS(JsonAssetBase, Asset);
    	ASSET_HEADER(JsonAssetBase);
    protected:
        String m_Path;
        bool m_IsVirtualDocument = false;

    public:

        /// <summary>
        /// The parsed json document.
        /// </summary>
    	SerializeDocument Document;

        /// <summary>
        /// The data node (reference from Document or Document itself).
        /// </summary>
    	// const rapidjson::GenericValue<rapidjson::UTF8<>, rapidjson::MemoryPoolAllocator<Json::FlaxAllocator>>*
    	DeserializeStream* Data;

        /// <summary>
        /// The data type name from the header. Allows to recognize the data type.
        /// </summary>
        TypeID DataTypeId;

        /// <summary>
        /// The serialized data engine build number. Can be used to convert/upgrade data between different formats across different engine versions.
        /// </summary>
        int32 DataEngineBuild;

        /// <summary>
        /// The Json data (as string).
        /// </summary>
        String GetData() const;

        /// <summary>
        /// The Json data (as string).
        /// </summary>
        void SetData(const StringView& value);

        /// <summary>
        /// Initializes the virtual Json asset with custom data.
        /// </summary>
        /// <remarks>Can be used only for virtual assets created at runtime.</remarks>
        /// <param name="dataTypeId">The data type name from the header. Allows to recognize the data type.</param>
        /// <param name="dataJson">The Json with serialized data.</param>
        /// <returns>True if failed, otherwise false.</returns>
        bool Init(const TypeID& dataTypeId, const StringAnsiView& dataJson);

#if SE_EDITOR
        /// <summary>
        /// Parses Json string to find any object references inside it. It can produce list of references to assets and/or scene objects. Supported only in Editor.
        /// </summary>
        /// <param name="json">The Json string.</param>
        /// <param name="output">The output list of object IDs references by the asset (appended, not cleared).</param>
        static void GetReferences(const StringAnsiView& json, List<UID, HeapAllocation>& output);

        /// <summary>
        /// Saves this asset to the file. Supported only in Editor.
        /// </summary>
        /// <param name="path">The custom asset path to use for the saving. Use empty value to save this asset to its own storage location. Can be used to duplicate asset. Must be specified when saving virtual asset.</param>
        /// <returns>True if cannot save data, otherwise false.</returns>
        bool Save(const StringView& path = StringView::Empty) const;

        /// <summary>
        /// Saves this asset to the Json Writer buffer (both ID, Typename header and Data contents). Supported only in Editor.
        /// </summary>
        /// <param name="writer">The output Json Writer to write asset.</param>
        /// <returns>True if cannot save data, otherwise false.</returns>
        bool Save(JsonWriter& writer) const;
#endif
    public:
        // [Asset]
        const String& GetPath() const override;
        uint64 GetMemoryUsage() const override;
#if SE_EDITOR
        void GetReferences(List<UID, HeapAllocation>& output) const override;
#endif

    protected:
        // [Asset]
        LoadResult ProcessLoadAsset() override;
        void Unload(bool isReloading) override;
#if SE_EDITOR
        void onRename(const StringView& newPath) override;
#endif

        // Gets the serialized Json data (from runtime state).
        virtual void OnGetData(Json::StringBuffer& buffer) const;
    };


	/// <summary>
	/// Generic type of Json-format asset. It provides the managed representation of this resource data so it can be accessed via C# API.
	/// </summary>
	/// <seealso cref="JsonAssetBase" />
	SE_CLASS(Reflect, API, NoSpawn)
	class SE_API_RUNTIME JsonAsset : public JsonAssetBase
	{
		SE_DEFINE_CLASS(JsonAsset, JsonAssetBase);
		ASSET_HEADER(JsonAsset);
	private:
		bool _isAfterReload = false;

	public:
		/// <summary>
		/// The scripting type of the deserialized unmanaged object instance (e.g. PhysicalMaterial).
		/// </summary>
		TypeID InstanceType;

		/// <summary>
		/// The deserialized unmanaged object instance (e.g. PhysicalMaterial). Might be null if asset was loaded before binary module with that asset was loaded (use GetInstance for this case).
		/// </summary>
		void* Instance;

		/// <summary>
		/// Gets the deserialized native object instance of the given type. Returns null if asset is not loaded or loaded object has different type.
		/// </summary>
		/// <returns>The asset instance object or null.</returns>
		template<typename T>
		T* GetInstance() const
		{
			const_cast<JsonAsset*>(this)->CreateInstance();
			const TypeID& type = Typeof<T>();
			return Instance && Types::IsTypeDerivedFrom(InstanceType, type) ? (T*)Instance : nullptr;
		}

		uint64 GetMemoryUsage() const override;

	protected:
		// [JsonAssetBase]
		LoadResult ProcessLoadAsset() override;
		void Unload(bool isReloading) override;
		void onLoaded_MainThread() override;

	private:
		bool CreateInstance();
		void DeleteInstance();
	};

} // SE

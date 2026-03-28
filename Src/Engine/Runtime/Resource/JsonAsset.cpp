

#include "JsonAsset.h"

#include "AssetContent.h"
#include "AssetsCache.h"
#include "Core/Logging/Exceptions/JsonParseException.h"
#include "Core/Profiler/ProfilerCPU.h"
#include "Core/Serialization/JsonWriters.hpp"
#include "Core/Thread/Threading.h"
#include "Core/Types/Collections/DataContainer.h"
#include "Core/Platform/File.h"
#include "Core/Serialization/Json.h"
#include "Core/Serialization/JsonTools.h"

namespace SE
{
    JsonAssetBase::JsonAssetBase(const AssetInfo* info)
        : Asset(info)
        , m_Path(info->path)
        , Data(nullptr)
        , DataEngineBuild(0)
    {
    }

    JsonAssetBase::JsonAssetBase() :
        m_Path(String::Empty)
        , Data(nullptr)
        , DataEngineBuild(0)
    {
    }

    String JsonAssetBase::GetData() const
    {
        if (Data == nullptr)
            return String::Empty;
        PROFILE_CPU_NAMED("JsonAsset.GetData");
        Json::StringBuffer buffer;
        OnGetData(buffer);
        return String(buffer.GetString(), (int32)buffer.GetSize());
    }

    void JsonAssetBase::SetData(const StringView& value)
    {
        if (!IsLoaded())
            return;
        PROFILE_CPU_NAMED("JsonAsset.SetData");
        const StringAnsi dataJson(value);
        Threading::ScopeLock lock(Locker);
        const TypeID dataTypeName = DataTypeId;
        if (!Init(dataTypeName, dataJson))
        {
            LOG_ERROR("Resource", "Failed to set Json asset data.");
        }
    }

    bool JsonAssetBase::Init(const TypeID& dataTypeId, const StringAnsiView& dataJson)
    {
        Unload(true);
        DataTypeId = dataTypeId;
        // DataEngineBuild = FLAXENGINE_VERSION_BUILD;

        // Parse json document
        {
            PROFILE_CPU_NAMED("Json.Parse");
            Document.Parse(dataJson.Get(), dataJson.Length());
        }
        if (Document.HasParseError())
        {
            Log::JsonParseException(Document.GetParseError(), Document.GetErrorOffset());
            return true;
        }
        Data = &Document;
        m_IsVirtualDocument = true;

        // Load asset-specific data
        return ProcessLoadAsset() != LoadResult::Ok;
    }

    void JsonAssetBase::OnGetData(Json::StringBuffer& buffer) const
    {
        PrettyJsonWriter writerObj(buffer);
        Data->Accept(writerObj.GetWriter());
    }

    const String& JsonAssetBase::GetPath() const
    {
#if SE_EDITOR
        return m_Path;
#else
        // In build all assets are packed into packages so use ID for original path lookup
        return AssetContent::GetRegistry()->GetEditorAssetPath(_id);
#endif
    }

    uint64 JsonAssetBase::GetMemoryUsage() const
    {
        Locker.Lock();
        uint64 result = Asset::GetMemoryUsage();
        result += sizeof(JsonAssetBase) - sizeof(Asset);
        if (Data)
            result += Document.GetAllocator().Capacity();
        Locker.Unlock();
        return result;
    }

#if SE_EDITOR

    void FindIds(DeserializeStream& node, List<UID>& output)
    {
        if (node.IsObject())
        {
            for (auto i = node.MemberBegin(); i != node.MemberEnd(); ++i)
            {
                FindIds(i->value, output);
            }
        }
        else if (node.IsArray())
        {
            for (rapidjson::SizeType i = 0; i < node.Size(); i++)
            {
                FindIds(node[i], output);
            }
        }
        else if (node.IsString())
        {
            if (node.GetStringLength() == 32)
            {
                // Try parse as Guid in format `N` (32 hex chars)
                UID id;
                if (!UID::Parse(node.GetStringAnsiView(), id))
                {
                    output.Add(id);
                }
            }
        }
    }

    void JsonAssetBase::GetReferences(const StringAnsiView& json, List<UID>& output)
    {
        SerializeDocument document;
        document.Parse(json.Get(), json.Length());
        if (document.HasParseError())
            return;
        FindIds(document, output);
    }

    bool JsonAssetBase::Save(const StringView& path) const
    {
        // Validate state
        if (!WaitForLoaded())
        {
            LOG_ERROR("Resource", "Asset loading failed. Cannot save it.");
            return false;
        }
        if (IsVirtual() && path.IsEmpty())
        {
            LOG_ERROR("Resource", "To save virtual asset asset you need to specify the target asset path location.");
            return false;
        }
        Threading::ScopeLock lock(Locker);

        // Serialize to json to the buffer
        Json::StringBuffer buffer;
        PrettyJsonWriter writerObj(buffer);
        Save(writerObj);

        // Save json to file
        if (!File::WriteAllBytes(path.HasChars() ? path : StringView(GetPath()), (byte*)buffer.GetString(), (int32)buffer.GetSize()))
        {
            LOG_ERROR("Resource", "Cannot save \'{0}\'", ToString());
            return false;
        }

        return true;
    }

    bool JsonAssetBase::Save(JsonWriter& writer) const
    {
        // Validate state
        if (!WaitForLoaded())
        {
            LOG_ERROR("Resource", "Asset loading failed. Cannot save it.");
            return false;
        }
        Threading::ScopeLock lock(Locker);

        writer.StartObject();
        {
            // Json resource header
            writer.JKEY("ID");
            writer.UUID(GetID());
            writer.JKEY("TypeID");
            writer.Uint(DataTypeId);
            // writer.JKEY("EngineBuild");
            // writer.Int(FLAXENGINE_VERSION_BUILD);

            // Json resource data
            Json::StringBuffer dataBuffer;
            OnGetData(dataBuffer);
            writer.JKEY("Data");
            writer.RawValue(dataBuffer.GetString(), (int32)dataBuffer.GetSize());
        }
        writer.EndObject();

        return true;
    }

    void JsonAssetBase::GetReferences(List<UID>& output) const
    {
        if (Data == nullptr)
            return;

        // Unified way to find asset references inside a generic asset.
        // This could deserialize managed/unmanaged object or load actors in case of SceneAsset or PrefabAsset.
        // But this would be performance killer.
        // The fastest way is to just iterate through the json and find all the Guid values.
        // It produces many invalid ids (like refs to scene objects).
        // But it's super fast, super low-memory and doesn't involve any advanced systems integration.

        FindIds(*Data, output);
    }

#endif

    Asset::LoadResult JsonAssetBase::ProcessLoadAsset()
    {
        if (IsVirtual() || m_IsVirtualDocument)
            return LoadResult::Ok;

        // Load data (raw json file in editor, cooked asset in build game)
#if SE_EDITOR
        BytesContainer data;
        if (!File::ReadAllBytes(m_Path, data))
        {
            LOG_ERROR("Resource", "Filed to load json asset data. {0}", ToString());
            return LoadResult::CannotLoadData;
        }
        if (data.Length() == 0)
        {
            return LoadResult::MissingDataChunk;
        }
#else
        // Get the asset storage container but don't load it now
        const auto storage = ContentStorageManager::GetStorage(_path, true);
        if (!storage)
            return LoadResult::CannotLoadStorage;

        // Load header
        AssetInitData initData;
        if (storage->LoadAssetHeader(GetID(), initData))
            return LoadResult::CannotLoadInitData;

        // Load the actual data
        auto chunk = initData.Header.Chunks[0];
        if (chunk == nullptr)
            return LoadResult::MissingDataChunk;
        if (storage->LoadAssetChunk(chunk))
            return LoadResult::CannotLoadData;
        auto& data = chunk->Data;
#endif

        // Parse json document
        {
            PROFILE_CPU_NAMED("Json.Parse");
            Document.Parse(data.Get<char>(), data.Length());
        }
        if (Document.HasParseError())
        {
            Log::JsonParseException(Document.GetParseError(), Document.GetErrorOffset());
            return LoadResult::CannotLoadData;
        }

        // Gather information from the header
        const auto id = JsonTools::GetGuid(Document, "ID");
        if (id != GetID())
        {
            LOG_WARNING("Resource", "Invalid json asset id. Asset: {0}, serialized: {1}.", GetID(), id);
            return LoadResult::InvalidData;
        }
        DataTypeId = TypeID(JsonTools::GetString(Document, "TypeID"));
        // DataEngineBuild = JsonTools::GetInt(Document, "EngineBuild", FLAXENGINE_VERSION_BUILD);
        auto dataMember = Document.FindMember("Data");
        if (dataMember == Document.MemberEnd())
        {
            LOG_WARNING("Resource", "Missing json asset data.");
            return LoadResult::InvalidData;
        }
        Data = &dataMember->value;

        return LoadResult::Ok;
    }

    void JsonAssetBase::Unload(bool isReloading)
    {
        SerializeDocument tmp;
        Document.Swap(tmp);
        Data = nullptr;
        DataTypeId = TypeID(0u);
        DataEngineBuild = 0;
        m_IsVirtualDocument = false;
    }

#if SE_EDITOR

    void JsonAssetBase::onRename(const StringView& newPath)
    {
        Threading::ScopeLock lock(Locker);

        // Rename
        m_Path = newPath;
    }

#endif


    JsonAsset::JsonAsset(const AssetInfo* info) : JsonAssetBase(info)
    {
    }

    uint64 JsonAsset::GetMemoryUsage() const
    {
        Locker.Lock();
        uint64 result = JsonAssetBase::GetMemoryUsage();
        result += sizeof(JsonAsset) - sizeof(JsonAssetBase);
        /*if (Instance && InstanceType)
            result += InstanceType.GetType().Size;*/
        Locker.Unlock();
        return result;
    }

    Asset::LoadResult JsonAsset::ProcessLoadAsset()
    {
        const auto result = JsonAssetBase::ProcessLoadAsset();
        if (result != LoadResult::Ok || IsInternalType())
            return result;

        if (!CreateInstance())
            return LoadResult::Failed;

/*#if SE_EDITOR
        // Reload instance when module with this type gets reloaded
        Level::ScriptsReloadStart.Bind<JsonAsset, &JsonAsset::OnScriptsReloadStart>(this);
        Level::ScriptsReloaded.Bind<JsonAsset, &JsonAsset::OnScriptsReloaded>(this);
#endif

        // Destroy instance on scripting shutdown (eg. asset from scripts)
        Scripting::ScriptsUnload.Bind<JsonAsset, &JsonAsset::DeleteInstance>(this);*/

        return LoadResult::Ok;
    }

    void JsonAsset::Unload(bool isReloading)
    {
/*#if SE_EDITOR
        Level::ScriptsReloadStart.Unbind<JsonAsset, &JsonAsset::OnScriptsReloadStart>(this);
        Level::ScriptsReloaded.Unbind<JsonAsset, &JsonAsset::OnScriptsReloaded>(this);
#endif
        Scripting::ScriptsUnload.Unbind<JsonAsset, &JsonAsset::DeleteInstance>(this);*/
        DeleteInstance();
        _isAfterReload |= isReloading;

        JsonAssetBase::Unload(isReloading);
    }

    void JsonAsset::onLoaded_MainThread()
    {
        JsonAssetBase::onLoaded_MainThread();

        // Special case for Settings assets to flush them after edited and saved in Editor
        /*const StringAsANSI<> dataTypeNameAnsi(DataTypeName.Get(), DataTypeName.Length());
        const auto typeHandle = Scripting::FindScriptingType(StringAnsiView(dataTypeNameAnsi.Get(), DataTypeName.Length()));
        if (Instance && typeHandle && typeHandle.IsSubclassOf(SettingsBase::TypeInitializer) && _isAfterReload)
        {
            _isAfterReload = false;
            ((SettingsBase*)Instance)->Apply();
        }*/
    }

    bool JsonAsset::CreateInstance()
    {

        return true;
    }

    void JsonAsset::DeleteInstance()
    {
    }
} // SE
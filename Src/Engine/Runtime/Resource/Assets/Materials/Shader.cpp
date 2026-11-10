#include <Runtime/Resource/Assets/Materials/Shader.h>

#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Resource/Factories/BinaryAssetFactory.h>

namespace SE
{
    BINARY_ASSET_FACTORY(Shader, true);

    Shader::Shader(const SpawnParams& params, const AssetInfo* pInfo) : BinaryAsset(params, pInfo)
    {
        ENGINE_ASSERT(GPUDevice::instance);
        Platform::MemoryClear(&m_ShaderHeader, sizeof(m_ShaderHeader));
        m_GPUShader = GPUDevice::instance->CreateSLC2Shader(pInfo ? StringView(pInfo->path) : StringView::Empty);
    }

    Shader::~Shader() { DeleteObjectSafe(m_GPUShader); }

    bool Shader::SelectProgram(const String& name, const List<String>& selection, SLC2GPUShaderProgram*& output)
    {
        output = nullptr;
        return IsLoaded() && m_GPUShader && m_GPUShader->GetOrCreateProgram(name, selection, output);
    }

    bool Shader::SelectProgram(const String& name, const List<String>& selection, ShaderProgramInstance* pInstance)
    {
        SLC2GPUShaderProgram* program = nullptr;
        return pInstance && SelectProgram(name, selection, program) && pInstance->Initialize(program);
    }

    bool Shader::OnInit(AssetInitData& initData)
    {
        if (initData.SerializedVersion != ShaderStorage::Header::Version ||
            initData.CustomData.Length() != sizeof(m_ShaderHeader))
            return false;
        Platform::MemoryCopy(&m_ShaderHeader, initData.CustomData.Get(), sizeof(m_ShaderHeader));
        return true;
    }

    Asset::LoadResult Shader::load()
    {
        ShaderCacheResult cache;
        if (!LoadShaderCache(cache) || !m_GPUShader || !m_GPUShader->Load(cache.Data.Get(), cache.Data.Length()))
            return LoadResult::Failed;
        RegisterForShaderReloads(this, cache);
        return LoadResult::Ok;
    }

    void Shader::Unload(bool isReloading)
    {
        UnregisterForShaderReloads(this);
        if (m_GPUShader)
            m_GPUShader->ReleaseGPU();
    }

    AssetChunksFlag Shader::GetChunksToPreload() const
    {
        return ShaderStorage::GetCachingMode() == ShaderStorage::CachingMode::AssetInternal ?
                   GET_CHUNK_FLAG(GetCacheChunkIndex()) | GET_CHUNK_FLAG(SHADER_FILE_CHUNK_DEPENDENCIES) :
                   0;
    }

    uint32 Shader::GetSerializedVersion() const { return ShaderStorage::Header::Version; }

#if SE_EDITOR
    bool Shader::Save()
    {
        AssetInitData data;
        data.SerializedVersion = GetSerializedVersion();
        data.CustomData.Link(&m_ShaderHeader);
        return SaveAsset(data);
    }
#endif
} // namespace SE

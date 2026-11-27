
#include <Runtime/Resource/Assets/Materials/Material.h>

#include <Runtime/Core/Encoding.h>
#include <Runtime/Core/Serialization/MemoryReadStream.h>
#include <Runtime/Core/Serialization/MemoryWriteStream.h>
#include <Runtime/Core/Thread/Threading.h>
#include <Runtime/Graphics/Base/GPUUtils.h>
#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Render/Assets/Material/MaterialShader.h>
#include <Runtime/Resource/Assets/Materials/Shader.h>
#include <Runtime/Resource/Assets/Materials/ShaderCacheManager.h>
#include <Runtime/Resource/Factories/BinaryAssetFactory.h>
#include <Runtime/ShaderCompilation/ShaderCompileTypes.h>

#if SE_EDITOR
#include <Runtime/Core/Serialization/JsonWriters.hpp>
#include <Runtime/Resource/Assets/Materials/Generator/MaterialGenerator.h>
#endif

namespace SE
{
    BINARY_ASSET_FACTORY(Material, false);

    Material::Material(const SpawnParams& params, const AssetInfo* info) : MaterialBase(params, info)
    {
        Platform::MemoryClear(&m_ShaderHeader, sizeof(m_ShaderHeader));
        m_ShaderAsset.Unload.Bind<Material, &Material::OnShaderUnloaded>(this);
    }

    Material::~Material() { Unload(false); }

    uint32 Material::GetSerializedVersion() const { return ShaderStorage::Header::Version; }

    bool Material::SetShaderReference(Shader* pShader)
    {
        if (IsGraphShader() || !IsLoaded() || !Threading::IsMainThread())
        {
            LOG_ERROR("Resource", "Shader references can only be changed on a loaded Shader material between draws on the main thread.");
            return false;
        }
        if (!LoadShader(pShader))
            return false;

        Threading::ScopeLock lock(Locker);
        pShader->GetMaterialParameters(Params);
        ParamsChanged();
        return true;
    }

    bool Material::LoadShader(Shader* pShader)
    {
        if (!pShader || (!pShader->IsLoaded() && !pShader->WaitForLoaded()))
        {
            LOG_ERROR("Resource", "Cannot load the Shader referenced by material {0}.", ToString());
            return false;
        }

        Threading::ScopeLock lock(Locker);
        Threading::ScopeLock shaderLock(pShader->Locker);
        if (!pShader->IsLoaded())
        {
            return false;
        }

        MemoryReadStream emptyStream(nullptr, 0);
        MaterialShader*  material;
        if (IsNullRenderer())
        {
            material = MaterialShader::CreateDummy(emptyStream, m_ShaderHeader.Material.Info);
        }
        else
        {
            material = MaterialShader::Create(pShader->GetGPUShader(), m_ShaderHeader.Material.Info);
        }

        if (!material)
        {
            LOG_ERROR("Resource", "Shader {0} does not satisfy the material Program contract.", pShader->ToString());
            return false;
        }

        // Publish only after the candidate is valid. A resource reload uses the same reference.
        if (m_ShaderAsset.Get() != pShader)
        {
            ReleaseShader();
            m_ShaderAsset = pShader;
            pShader->OnReloadingEvent.Bind<Material, &Material::OnShaderReloading>(this);
            pShader->OnLoadedEvent.Bind<Material, &Material::OnShaderLoaded>(this);
        }
        else
        {
            Delete(m_MaterialShader);
        }

        m_MaterialShader                = material;
        m_ShaderHeader.Material.ShaderID = pShader->GetID();
        return true;
    }

    void Material::ReleaseShader()
    {
        if (Shader* shader = m_ShaderAsset.Get())
        {
            shader->OnReloadingEvent.Unbind<Material, &Material::OnShaderReloading>(this);
            shader->OnLoadedEvent.Unbind<Material, &Material::OnShaderLoaded>(this);
        }
        Delete(m_MaterialShader);
        m_MaterialShader = nullptr;
    }

    void Material::OnShaderReloading(Asset* pAsset)
    {
        Threading::ScopeLock lock(Locker);
        if (pAsset == m_ShaderAsset.Get())
        {
            // Programs are owned by the resource, so release their users before Shader::Unload.
            Delete(m_MaterialShader);
            m_MaterialShader = nullptr;
        }
    }

    void Material::OnShaderLoaded(Asset* pAsset)
    {
        Threading::ScopeLock lock(Locker);
        if (pAsset == m_ShaderAsset.Get() && !m_MaterialShader)
        {
            if (LoadShader(m_ShaderAsset.Get()))
            {
                m_ShaderAsset->GetMaterialParameters(Params);
                ParamsChanged();
            }
        }
    }

    void Material::OnShaderUnloaded()
    {
        Threading::ScopeLock lock(Locker);
        ReleaseShader();
    }

    bool Material::IsMaterialInstance() const { return false; }

    const MaterialInfo& Material::GetInfo() const
    {
        if (m_MaterialShader)
        {
            return m_MaterialShader->GetInfo();
        }

        static MaterialInfo EmptyInfo;
        return EmptyInfo;
    }

    GPUShader* Material::GetShader() const { return m_MaterialShader ? m_MaterialShader->GetShader() : nullptr; }

    bool Material::IsReady() const
    {
        return m_MaterialShader && m_MaterialShader->IsReady();
    }

    DrawPass Material::GetDrawModes() const
    {
        if (m_MaterialShader)
        {
            return m_MaterialShader->GetDrawModes();
        }
        return DrawPass::None;
    }

    bool Material::CanUseLightmap() const
    {
        return m_MaterialShader && m_MaterialShader->CanUseLightmap();
    }

    bool Material::CanUseInstancing(InstancingHandler& handler) const
    {
        return m_MaterialShader && m_MaterialShader->CanUseInstancing(handler);
    }

    void Material::Bind(BindParameters& params)
    {
        if (!IsReady())
        {
            return;
        }

        MaterialParamsLink* lastLink = params.paramsLink;
        MaterialParamsLink link;
        link.This = &Params;
        if (lastLink)
        {
            while (lastLink->Down)
            {
                lastLink = lastLink->Down;
            }
            lastLink->Down = &link;
        }
        else
        {
            params.paramsLink = &link;
        }
        link.Up = lastLink;
        link.Down = nullptr;

        m_MaterialShader->Bind(params);

        if (lastLink)
        {
            lastLink->Down = nullptr;
        }
        else
        {
            params.paramsLink = nullptr;
        }
    }

    ShaderProgramInstance* Material::GetProgram() { return IsReady() ? m_MaterialShader->GetProgram() : nullptr; }

#if SE_EDITOR

    namespace
    {
        void OnGeneratorError(ShaderGraph<>::Node* node, ShaderGraphBox* box, const StringView& text)
        {
            LOG_ERROR("Resource", "Material error: {0} (Node:{1}:{2}, Box:{3})", text, node ? node->Type : -1, node ? node->ID : -1, box ? box->ID : -1);
        }
    }

#endif

    Asset::LoadResult Material::load()
    {
        ENGINE_ASSERT(m_MaterialShader == nullptr);
        StorageChunk* materialParamsChunk;

        // Wait for the GPU Device to be ready (eg. case when loading material before GPU init)
#define IS_GPU_NOT_READY() (GPUDevice::instance == nullptr || GPUDevice::instance->GetState() != GPUDevice::DeviceState::Ready)
        if (!Threading::IsMainThread() && IS_GPU_NOT_READY())
        {
            int32 timeout = 1000;
            while (IS_GPU_NOT_READY() && timeout-- > 0)
            {
                Platform::Sleep(1);
            }
            if (IS_GPU_NOT_READY())
            {
                return LoadResult::InvalidData;
            }
        }
#undef IS_GPU_NOT_READY

        // Special case for Null renderer
        if (IsGraphShader() && IsNullRenderer())
        {
            // Hack loading
            MemoryReadStream shaderCacheStream(nullptr, 0);
            m_MaterialShader = MaterialShader::CreateDummy(shaderCacheStream, m_ShaderHeader.Material.Info);
            if (m_MaterialShader == nullptr)
            {
                LOG_WARNING("Resource", "Cannot load material.");
                return LoadResult::Failed;
            }
            materialParamsChunk = GetChunk(SHADER_FILE_CHUNK_MATERIAL_PARAMS);
            if (materialParamsChunk != nullptr && materialParamsChunk->IsValid())
            {
                MemoryReadStream materialParamsStream(materialParamsChunk->Get(), materialParamsChunk->Size());
                if (Params.Load(&materialParamsStream))
                {
                    LOG_WARNING("Resource", "Cannot load material parameters.");
                    return LoadResult::Failed;
                }
            }
            else
            {
                // Don't use parameters
                Params.Dispose();
            }
            ParamsChanged();
            return LoadResult::Ok;
        }

        // Generate the current ShaderArtifact source from the authored graph when needed.
#if SE_EDITOR

        const bool generateSource = IsGraphShader() && (!HasChunk(SHADER_FILE_CHUNK_SOURCE) || HasDependenciesModified());
        if (generateSource)
        {
            auto              lock = storage->Lock();
            MaterialGenerator generator;
            generator.Error.Bind(&OnGeneratorError);
            LOG_INFO("Resource", "Generating ShaderArtifact material '{0}'...", ToString());

            // Load or create material surface
            MaterialLayer* layer;
            if (HasChunk(SHADER_FILE_CHUNK_VISJECT_SURFACE))
            {
                // Load graph
                if (!LoadChunks(GET_CHUNK_FLAG(SHADER_FILE_CHUNK_VISJECT_SURFACE)))
                {
                    LOG_WARNING("Resource", "Cannot load \'{0}\' data from chunk {1}.", ToString(), SHADER_FILE_CHUNK_VISJECT_SURFACE);
                    return LoadResult::Failed;
                }

                // Get stream with graph data
                auto             surfaceChunk = GetChunk(SHADER_FILE_CHUNK_VISJECT_SURFACE);
                MemoryReadStream stream(surfaceChunk->Get(), surfaceChunk->Size());

                // Load layer
                layer = MaterialLayer::Load(GetID(), &stream, m_ShaderHeader.Material.Info, ToString());
            }
            else
            {
                // Create default layer
                layer = MaterialLayer::CreateDefault(GetID());

                // Create surface chunk
                auto surfaceChunk = GetOrCreateChunk(SHADER_FILE_CHUNK_VISJECT_SURFACE);
                if (surfaceChunk == nullptr)
                {
                    return LoadResult::MissingDataChunk;
                }

                // Save layer to the chunk data
                MemoryWriteStream stream(512);
                layer->Graph.Save(&stream, false);
                surfaceChunk->Data.Copy(stream.GetHandle(), stream.GetPosition());
            }
            if (!layer)
            {
                return LoadResult::InvalidData;
            }
            layer->FeaturesFlags    = m_ShaderHeader.Material.Info.FeaturesFlags;
            layer->UsageFlags       = m_ShaderHeader.Material.Info.UsageFlags;
            layer->Domain           = m_ShaderHeader.Material.Info.Domain;
            layer->BlendMode        = m_ShaderHeader.Material.Info.BlendMode;
            layer->ShadingModel     = m_ShaderHeader.Material.Info.ShadingModel;
            layer->MaskThreshold    = m_ShaderHeader.Material.Info.MaskThreshold;
            layer->OpacityThreshold = m_ShaderHeader.Material.Info.OpacityThreshold;
            generator.AddLayer(layer);

            // Get chunk with material parameters
            materialParamsChunk = GetOrCreateChunk(SHADER_FILE_CHUNK_MATERIAL_PARAMS);
            if (materialParamsChunk == nullptr)
            {
                return LoadResult::MissingDataChunk;
            }
            materialParamsChunk->Data.Release();

            // Generate material source code and metadata
            MemoryWriteStream source(64 * 1024);
            MaterialInfo      info = m_ShaderHeader.Material.Info;
            if (!generator.Generate(source, info, materialParamsChunk->Data))
            {
                LOG_ERROR("Resource", "Cannot generate material source code for \'{0}\'. Please see log for more info.", ToString());
                return LoadResult::Failed;
            }

            // Update asset dependencies
            ClearDependencies();
            for (auto& asset : generator.Assets)
            {
                if (TypeAs<MaterialBase>(asset) /* || IsOfType<MaterialFunction>(asset)*/)
                {
                    AddDependency(asset.As<BinaryAsset>());
                }
            }

#if BUILD_DEBUG && SE_EDITOR
            // Dump generated material source to the temporary file
            BinaryModule::Locker.Lock();
            source.SaveToFile(Globals::ProjectCacheFolder / TEXT("material.txt"));
            BinaryModule::Locker.Unlock();
#endif

            // The generator appends a NUL that is not part of the JSON source string.
            Json::StringBuffer sourceDescription;
            CompactJsonWriter  writer(sourceDescription);
            writer.StartObject();
            writer.JKEY("inlineCode");
            writer.Bool(true);
            writer.JKEY("source");
            writer.String(reinterpret_cast<const char*>(source.GetHandle()), source.GetPosition() - 1);
            writer.EndObject();

            StorageChunk* sourceChunk = GetOrCreateChunk(SHADER_FILE_CHUNK_SOURCE);
            if (!sourceChunk)
            {
                return LoadResult::MissingDataChunk;
            }
            sourceChunk->Data.Copy(reinterpret_cast<const byte*>(sourceDescription.GetString()), static_cast<int32>(sourceDescription.GetSize()));
            Encoding::EncryptBytes(sourceChunk->Get(), sourceChunk->Size());

            // Clear shader cache
            for (int32 chunkIndex = 1; chunkIndex < 14; chunkIndex++)
            {
                ReleaseChunk(chunkIndex);
            }

            // Setup shader header
            m_ShaderHeader.Material.GraphVersion = MATERIAL_GRAPH_VERSION;
            m_ShaderHeader.Material.Info         = info;
        }

#endif

        if (HasChunk(SHADER_FILE_CHUNK_MATERIAL_PARAMS) && 
            !HasChunkLoaded(SHADER_FILE_CHUNK_MATERIAL_PARAMS) && 
            !LoadChunks(GET_CHUNK_FLAG(SHADER_FILE_CHUNK_MATERIAL_PARAMS)))
        {
            return LoadResult::Failed;
        }

        // Load material parameters
        materialParamsChunk = GetChunk(SHADER_FILE_CHUNK_MATERIAL_PARAMS);
        if (materialParamsChunk != nullptr && materialParamsChunk->IsValid())
        {
            MemoryReadStream materialParamsStream(materialParamsChunk->Get(), materialParamsChunk->Size());
            if (Params.Load(&materialParamsStream))
            {
                LOG_WARNING("Resource", "Cannot load material parameters.");
                return LoadResult::Failed;
            }
        }
        else
        {
            // Don't use parameters
            Params.Dispose();
        }

#if SE_EDITOR
        if (generateSource)
        {
            // Save only after Params has consumed the generated parameter chunk.
            if (!Save())
            {
                LOG_ERROR("Resource", "Cannot save \'{0}\'", ToString());
                return LoadResult::Failed;
            }

            // Invalidate shader cache
            ShaderCacheManager::RemoveCache(GetID());
        }
#endif

        // Load shader cache (it may call compilation or gather cached data)
        ShaderCacheResult shaderCache;
        if (IsGraphShader())
        {
            if (!LoadShaderCache(shaderCache))
            {
                LOG_ERROR("Resource", "Cannot load \'{0}\' shader cache.", ToString());
                return LoadResult::Failed;
            }
            else
            {
                // Load material (load shader from cache, load params, setup pipeline stuff)
                MemoryReadStream shaderCacheStream(shaderCache.Data.Get(), shaderCache.Data.Length());
#if GPU_ENABLE_RESOURCE_NAMING
                const StringView name(GetPath());
#else
                const StringView name;
#endif
                m_MaterialShader = MaterialShader::Create(name, shaderCacheStream, m_ShaderHeader.Material.Info);
                if (m_MaterialShader == nullptr)
                {
                    LOG_WARNING("Resource", "Cannot load material.");
                    return LoadResult::Failed;
                }
            }
        }
        else
        {
            Shader* shader = AssetContent::LoadAsync<Shader>(m_ShaderHeader.Material.ShaderID);
            if (!LoadShader(shader))
            {
                return LoadResult::Failed;
            }
        }

        ParamsChanged();

        if (IsGraphShader())
        {
            RegisterForShaderReloads(this, shaderCache);
        }

        return LoadResult::Ok;
    }

    void Material::Unload(bool isReloading)
    {
        UnregisterForShaderReloads(this);
        ReleaseShader();
        m_ShaderAsset = nullptr;
        Params.Dispose();
    }

    AssetChunksFlag Material::GetChunksToPreload() const
    {
        if (!IsGraphShader())
        {
            return 0;
        }
        AssetChunksFlag result = 0;
        const auto cachingMode = ShaderStorage::GetCachingMode();
        if (cachingMode == ShaderStorage::CachingMode::AssetInternal && !IsNullRenderer())
        {
            result |= GET_CHUNK_FLAG(GetCacheChunkIndex());
        }

        result |= GET_CHUNK_FLAG(SHADER_FILE_CHUNK_MATERIAL_PARAMS);
        return result;
    }

#if SE_EDITOR

    void Material::OnDependencyModified(BinaryAsset* asset)
    {
        BinaryAsset::OnDependencyModified(asset);

        if (IsGraphShader())
        {
            Reload();
        }
    }

    void Material::InitCompilationRequest(ShaderCompileRequest& request, StringAnsi& preamble)
    {
        // Base
        IShaderAsset::InitCompilationRequest(request, preamble);

        // Ensure that this call is valid (material features switches may depend on target compilation platform)
        ENGINE_ASSERT(request.Targets[0] != ShaderProfile::Unknown);

        // Prepare
        auto&      info                           = m_ShaderHeader.Material.Info;
        const bool isSurfaceOrTerrainOrDeformable = info.Domain == MaterialDomain::Surface || info.Domain == MaterialDomain::Terrain || info.Domain == MaterialDomain::Deformable;
        const bool useCustomData = info.ShadingModel == MaterialShadingModel::Subsurface || info.ShadingModel == MaterialShadingModel::Foliage;
        const bool useForward = ((info.Domain == MaterialDomain::Surface || info.Domain == MaterialDomain::Deformable) && info.BlendMode != MaterialBlendMode::Opaque) || info.Domain == MaterialDomain::Particle;
        const bool useTess = info.TessellationMode != TessellationMethod::None &&
                             GPUUtils::CanSupportTessellation(request.Targets[0]) && isSurfaceOrTerrainOrDeformable;

        const bool useDistortion =
                (info.Domain == MaterialDomain::Surface || info.Domain == MaterialDomain::Deformable || info.Domain == MaterialDomain::Particle) &&
                info.BlendMode != MaterialBlendMode::Opaque &&
                EnumHasAnyFlags(info.UsageFlags, MaterialUsage::UseRefraction) &&
                EnumHasNoneFlags(info.FeaturesFlags, MaterialFeatures::DisableDistortion);

        // @formatter:off
        static const char* Numbers[] =
        {
            "0","1","2","3","4","5","6","7","8","9","10",
            "11","12","13","14","15","16","17","18","19",
            "20","21","22","23","24","25","26","27","28",
            "29","30","31","32","33","34","35","36","37",
            "38","39","40","41","42","43","44","45","46",
            "47","48","49","50","51","52","53","54","55",
            "56","57","58","59","60","61","62","63","64",
            "65","66","67","68","69",
        };
        // @formatter:on

        const auto define = [&preamble](const char* pName, const char* pValue) {
            preamble += "#define ";
            preamble += pName;
            preamble += " ";
            preamble += pValue;
            preamble += "\n";
        };

        // Setup material definitions before loading the Slang module.
        define("MATERIAL_DOMAIN", Numbers[(int32)info.Domain]);
        define("MATERIAL_BLEND", Numbers[(int32)info.BlendMode]);
        define("MATERIAL_SHADING_MODEL", Numbers[(int32)info.ShadingModel]);
        define("MATERIAL_MASKED", Numbers[EnumHasAnyFlags(info.UsageFlags, MaterialUsage::UseMask) ? 1 : 0]);
        define("DECAL_BLEND_MODE", Numbers[(int32)info.DecalBlendingMode]);
        define("USE_EMISSIVE", Numbers[EnumHasAnyFlags(info.UsageFlags, MaterialUsage::UseEmissive) ? 1 : 0]);
        define("USE_NORMAL", Numbers[EnumHasAnyFlags(info.UsageFlags, MaterialUsage::UseNormal) ? 1 : 0]);
        define("USE_POSITION_OFFSET",
               Numbers[EnumHasAnyFlags(info.UsageFlags, MaterialUsage::UsePositionOffset) ? 1 : 0]);
        define("USE_VERTEX_COLOR", Numbers[EnumHasAnyFlags(info.UsageFlags, MaterialUsage::UseVertexColor) ? 1 : 0]);
        define("USE_DISPLACEMENT", Numbers[EnumHasAnyFlags(info.UsageFlags, MaterialUsage::UseDisplacement) ? 1 : 0]);
        define("USE_DITHERED_LOD_TRANSITION",
               Numbers[EnumHasAnyFlags(info.FeaturesFlags, MaterialFeatures::DitheredLODTransition) ? 1 : 0]);
        define("USE_GBUFFER_CUSTOM_DATA", Numbers[useCustomData ? 1 : 0]);
        define("USE_REFLECTIONS",
               Numbers[EnumHasAnyFlags(info.FeaturesFlags, MaterialFeatures::DisableReflections) ? 0 : 1]);
        if (!EnumHasAnyFlags(info.FeaturesFlags, MaterialFeatures::DisableReflections) && EnumHasAnyFlags(info.FeaturesFlags, MaterialFeatures::ScreenSpaceReflections))
            define("MATERIAL_REFLECTIONS", Numbers[1]);
        define("USE_FOG", Numbers[EnumHasAnyFlags(info.FeaturesFlags, MaterialFeatures::DisableFog) ? 0 : 1]);
        if (useForward)
        {
            define("USE_PIXEL_NORMAL_OFFSET_REFRACTION",
                   Numbers[EnumHasAnyFlags(info.FeaturesFlags, MaterialFeatures::PixelNormalOffsetRefraction) ? 1 : 0]);
            switch (info.TransparentLightingMode)
            {
            case MaterialTransparentLightingMode::Surface:
                break;
            case MaterialTransparentLightingMode::SurfaceNonDirectional:
                define("LIGHTING_NO_DIRECTIONAL", "1");
                break;
            }
        }

        // TODO: don't compile VS_Depth for deferred/forward materials if material doesn't use position offset or masking

        define("USE_TESSELLATION", Numbers[useTess ? 1 : 0]);
        define("TESSELLATION_IN_CONTROL_POINTS", "3");
        if (useTess)
        {
            switch (info.TessellationMode)
            {
            case TessellationMethod::Flat:
                define("MATERIAL_TESSELLATION", "MATERIAL_TESSELLATION_FLAT");
                break;
            case TessellationMethod::PointNormal:
                define("MATERIAL_TESSELLATION", "MATERIAL_TESSELLATION_PN");
                break;
            case TessellationMethod::Phong:
                define("MATERIAL_TESSELLATION", "MATERIAL_TESSELLATION_PHONG");
                break;
            }
            define("MAX_TESSELLATION_FACTOR",
                   Numbers[Math::Min<int32>(info.MaxTessellationFactor, ARRAY_SIZE(Numbers) - 1)]);
        }

        // Helper macros (used by the parser)
        define("IS_SURFACE", Numbers[info.Domain == MaterialDomain::Surface ? 1 : 0]);
        define("IS_POST_FX", Numbers[info.Domain == MaterialDomain::PostProcess ? 1 : 0]);
        define("IS_GUI", Numbers[info.Domain == MaterialDomain::GUI ? 1 : 0]);
        define("IS_DECAL", Numbers[info.Domain == MaterialDomain::Decal ? 1 : 0]);
        define("IS_TERRAIN", Numbers[info.Domain == MaterialDomain::Terrain ? 1 : 0]);
        define("IS_PARTICLE", Numbers[info.Domain == MaterialDomain::Particle ? 1 : 0]);
        define("IS_DEFORMABLE", Numbers[info.Domain == MaterialDomain::Deformable ? 1 : 0]);
        define("USE_FORWARD", Numbers[useForward ? 1 : 0]);
        define("USE_DEFERRED",
               Numbers[isSurfaceOrTerrainOrDeformable && info.BlendMode == MaterialBlendMode::Opaque ? 1 : 0]);
        define("USE_DISTORTION", Numbers[useDistortion ? 1 : 0]);
    }

#endif

    bool Material::OnInit(AssetInitData& initData)
    {
        if (initData.SerializedVersion != ShaderStorage::Header::Version || initData.CustomData.Length() != sizeof(m_ShaderHeader))
        {
            LOG_WARNING("Resource", "Invalid material version or header.");
            return false;
        }

        Platform::MemoryCopy(&m_ShaderHeader, initData.CustomData.Get(), sizeof(m_ShaderHeader));
        return true;
    }

#if SE_EDITOR

    void Material::GetReferences(List<UID>& output) const
    {
        MaterialBase::GetReferences(output);
        if (!IsGraphShader() && m_ShaderHeader.Material.ShaderID.IsValid())
        {
            output.Add(m_ShaderHeader.Material.ShaderID);
        }
    }

    bool Material::Save()
    {
        Threading::ScopeLock lock(Locker);

        auto parent = GetShaderAsset();
        AssetInitData data;
        data.SerializedVersion = GetSerializedVersion();
        data.CustomData.Link(&m_ShaderHeader);

        MemoryWriteStream parameters(16);
        Params.Save(&parameters);
        SetChunk(SHADER_FILE_CHUNK_MATERIAL_PARAMS, ToSpan(parameters.GetHandle(), parameters.GetPosition()));

        parent->metadata.Release();
        return parent->SaveAsset(data);
    }

#endif
} // SE


#include <Runtime/Resource/Assets/Materials/Material.h>

#include <Runtime/Core/Encoding.h>
#include <Runtime/Core/Serialization/MemoryReadStream.h>
#include <Runtime/Core/Thread/Threading.h>
#include <Runtime/Graphics/Base/GPUUtils.h>
#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Render/Assets/Material/MaterialShader.h>
#include <Runtime/Resource/Assets/Materials/ShaderCacheManager.h>
#include <Runtime/Resource/Factories/BinaryAssetFactory.h>
#include <Runtime/ShaderCompilation/ShaderCompileTypes.h>

#if SE_EDITOR
#include <Runtime/Resource/Assets/Materials/Generator/MaterialGenerator.h>
#endif

namespace SE
{
    BINARY_ASSET_FACTORY(Material, false);

    Material::Material(const SpawnParams& params, const AssetInfo* info) : MaterialBase(params, info)
    {
    }

    bool Material::IsMaterialInstance() const
    {
        return false;
    }

    MaterialShader* Material::GetMaterialShaderObject() const
    {
        return m_MaterialShader;
    }

    const MaterialInfo& Material::GetInfo() const
    {
        if (m_MaterialShader)
        {
            return m_MaterialShader->GetInfo();
        }

        static MaterialInfo EmptyInfo;
        return EmptyInfo;
    }

    SLC2GPUShader* Material::GetShader() const { return m_MaterialShader ? m_MaterialShader->GetShader() : nullptr; }

    bool Material::IsReady() const
    {
        return m_MaterialShader && m_MaterialShader->IsReady();
    }

    DrawPass Material::GetDrawModes() const
    {
        if (m_MaterialShader)
            return m_MaterialShader->GetDrawModes();
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
        params.Program = nullptr;
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

    ShaderProgramInstance* Material::GetProgram()
    { 
        return m_MaterialShader->GetProgram();
    }

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
                Platform::Sleep(1);
            if (IS_GPU_NOT_READY())
                return LoadResult::InvalidData;
        }
#undef IS_GPU_NOT_READY

        // Special case for Null renderer
        if (IsNullRenderer())
        {
            // Hack loading
            MemoryReadStream shaderCacheStream(nullptr, 0);
            m_MaterialShader = MaterialShader::CreateDummy(shaderCacheStream, _shaderHeader.Material.Info);
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

        // Generate the current SLC2 source from the authored graph when needed.
#if SE_EDITOR

        const bool sourceFilesCurrent = LoadMaterialSourceFiles();
        if (!HasChunk(SHADER_FILE_CHUNK_SOURCE) || !sourceFilesCurrent || HasDependenciesModified())
        {
            auto lock = storage->Lock();
            MaterialGenerator generator;
            generator.Error.Bind(&OnGeneratorError);
            LOG_INFO("Resource", "Generating SLC2 material '{0}'...", ToString());

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
                auto surfaceChunk = GetChunk(SHADER_FILE_CHUNK_VISJECT_SURFACE);
                MemoryReadStream stream(surfaceChunk->Get(), surfaceChunk->Size());

                // Load layer
                layer = MaterialLayer::Load(GetID(), &stream, _shaderHeader.Material.Info, ToString());
            }
            else
            {
                // Create default layer
                layer = MaterialLayer::CreateDefault(GetID());

                // Create surface chunk
                auto surfaceChunk = GetOrCreateChunk(SHADER_FILE_CHUNK_VISJECT_SURFACE);
                if (surfaceChunk == nullptr)
                    return LoadResult::MissingDataChunk;

                // Save layer to the chunk data
                MemoryWriteStream stream(512);
                layer->Graph.Save(&stream, false);
                surfaceChunk->Data.Copy(stream.GetHandle(), stream.GetPosition());
            }
            if (!layer)
                return LoadResult::InvalidData;
            layer->FeaturesFlags    = _shaderHeader.Material.Info.FeaturesFlags;
            layer->UsageFlags       = _shaderHeader.Material.Info.UsageFlags;
            layer->Domain           = _shaderHeader.Material.Info.Domain;
            layer->BlendMode        = _shaderHeader.Material.Info.BlendMode;
            layer->ShadingModel     = _shaderHeader.Material.Info.ShadingModel;
            layer->MaskThreshold    = _shaderHeader.Material.Info.MaskThreshold;
            layer->OpacityThreshold = _shaderHeader.Material.Info.OpacityThreshold;
            generator.AddLayer(layer);

            // Get chunk with material parameters
            materialParamsChunk = GetOrCreateChunk(SHADER_FILE_CHUNK_MATERIAL_PARAMS);
            if (materialParamsChunk == nullptr)
                return LoadResult::MissingDataChunk;
            materialParamsChunk->Data.Release();

            // Generate material source code and metadata
            MemoryWriteStream source(64 * 1024);
            MaterialInfo info = _shaderHeader.Material.Info;
            if (!generator.Generate(source, info, materialParamsChunk->Data))
            {
                LOG_ERROR("Resource", "Cannot generate material source code for \'{0}\'. Please see log for more info.", ToString());
                return LoadResult::Failed;
            }

            // Update asset dependencies
            ClearDependencies();
            for (auto& asset : generator.Assets)
            {
                if (TypeAs<MaterialBase>(asset)/* || IsOfType<MaterialFunction>(asset)*/)
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

            // Encrypt source code
            Encoding::EncryptBytes((byte*)source.GetHandle(), source.GetPosition());

            // Set new source code chunk
            SetChunk(SHADER_FILE_CHUNK_SOURCE, ToSpan(source.GetHandle(), source.GetPosition()));

            // Clear shader cache
            for (int32 chunkIndex = 1; chunkIndex < 14; chunkIndex++)
                ReleaseChunk(chunkIndex);

            m_SourceFiles = generator.SourceFiles;
            if (!SaveMaterialSourceFiles())
                return LoadResult::Failed;

            // Setup shader header
            Platform::MemoryClear(&_shaderHeader, sizeof(_shaderHeader));
            _shaderHeader.Material.GraphVersion = MATERIAL_GRAPH_VERSION;
            _shaderHeader.Material.Info = info;

            // Save to file
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
            m_MaterialShader = MaterialShader::Create(name, shaderCacheStream, _shaderHeader.Material.Info);
            if (m_MaterialShader == nullptr)
            {
                LOG_WARNING("Resource", "Cannot load material.");
                return LoadResult::Failed;
            }
        }

        if (HasChunk(SHADER_FILE_CHUNK_MATERIAL_PARAMS) && !HasChunkLoaded(SHADER_FILE_CHUNK_MATERIAL_PARAMS) &&
            !LoadChunks(GET_CHUNK_FLAG(SHADER_FILE_CHUNK_MATERIAL_PARAMS)))
            return LoadResult::Failed;

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
        ParamsChanged();

        RegisterForShaderReloads(this, shaderCache);

        return LoadResult::Ok;
    }

    void Material::Unload(bool isReloading)
    {
        UnregisterForShaderReloads(this);

        if (m_MaterialShader)
        {
            m_MaterialShader->Unload();
            Delete(m_MaterialShader);
            m_MaterialShader = nullptr;
        }

        Params.Dispose();
    }

    AssetChunksFlag Material::GetChunksToPreload() const
    {
        AssetChunksFlag result = 0;
        const auto cachingMode = ShaderStorage::GetCachingMode();
        if (cachingMode == ShaderStorage::CachingMode::AssetInternal && !IsNullRenderer())
            result |= GET_CHUNK_FLAG(GetCacheChunkIndex());

        result |= GET_CHUNK_FLAG(SHADER_FILE_CHUNK_MATERIAL_PARAMS);
#if SE_EDITOR
        result |= GET_CHUNK_FLAG(MATERIAL_FILE_CHUNK_SOURCE_FILES);
#endif
        return result;
    }

#if SE_EDITOR

    void Material::OnDependencyModified(BinaryAsset* asset)
    {
        BinaryAsset::OnDependencyModified(asset);

        Reload();
    }

    void Material::InitCompilationRequest(ShaderCompileRequest& request, StringAnsi& preamble)
    {
        // Base
        IShaderAsset::InitCompilationRequest(request, preamble);

        // Ensure that this call is valid (material features switches may depend on target compilation platform)
        ENGINE_ASSERT(request.Targets[0] != ShaderProfile::Unknown);

        // Prepare
        auto& info = _shaderHeader.Material.Info;
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
        // Validate version
        if (initData.SerializedVersion != ShaderStorage::Header::Version)
        {
            LOG_WARNING("Resource", "Invalid shader serialized version.");
            return false;
        }

        // Validate data
        if (initData.CustomData.Length() != sizeof(_shaderHeader))
        {
            LOG_WARNING("Resource", "Invalid shader header.");
            return false;
        }

        // Load header 'as-is'
        Platform::MemoryCopy(&_shaderHeader, initData.CustomData.Get(), sizeof(_shaderHeader));

        return true;
    }

    BytesContainer Material::LoadSurface(bool createDefaultIfMissing)
    {
        BytesContainer result;
        if (!WaitForLoaded() && !LastLoadFailed())
            return result;
        Threading::ScopeLock lock(Locker);

        // Check if has that chunk
        if (HasChunk(SHADER_FILE_CHUNK_VISJECT_SURFACE))
        {
            // Load graph
            if (LoadChunks(GET_CHUNK_FLAG(SHADER_FILE_CHUNK_VISJECT_SURFACE)))
            {
                // Get stream with graph data
                const auto data = GetChunk(SHADER_FILE_CHUNK_VISJECT_SURFACE);
                result.Copy(data->Data);
                return result;
            }
        }

        LOG_WARNING("Resource", "Material \'{0}\' surface data is missing.", ToString());

#if COMPILE_WITH_MATERIAL_GRAPH

        // Check if create default surface
        if (createDefaultIfMissing)
        {
            // Create default layer
            const auto layer = MaterialLayer::CreateDefault(GetID());

            // Serialize layer to stream
            MemoryWriteStream stream(256);
            layer->Graph.Save(&stream, false);

            // Set output data
            result.Copy(stream.GetHandle(), stream.GetPosition());
            return result;
        }

#endif

        return result;
    }

#if SE_EDITOR

    bool Material::SaveSurface(const BytesContainer& data, const MaterialInfo& info)
    {
        // Wait for asset to be loaded or don't if last load failed (eg. by shader source compilation error)
        if (LastLoadFailed())
        {
            LOG_WARNING("Resource", "Saving asset that failed to load.");
        }
        else if (!WaitForLoaded())
        {
            LOG_ERROR("Resource", "Asset loading failed. Cannot save it.");
            return false;
        }

        Threading::ScopeLock lock(Locker);

        // Release all chunks
        for (int32 i = 0; i < ASSET_FILE_DATA_CHUNKS; i++)
        {
            ReleaseChunk(i);
        }

        // Update material info
        Platform::MemoryClear(&_shaderHeader, sizeof(_shaderHeader));
        _shaderHeader.Material.GraphVersion = MATERIAL_GRAPH_VERSION;
        _shaderHeader.Material.Info = info;

        // Set Visject Surface data
        auto visjectSurfaceChunk = GetOrCreateChunk(SHADER_FILE_CHUNK_VISJECT_SURFACE);
        ENGINE_ASSERT(visjectSurfaceChunk != nullptr);
        visjectSurfaceChunk->Data.Copy(data);

        if (!Save())
        {
            LOG_ERROR("Resource", "Cannot save \'{0}\'", ToString());
            return false;
        }

        // Invalidate shader cache
        ShaderCacheManager::RemoveCache(GetID());

        return true;
    }

    bool Material::Save()
    {
        auto parent = GetShaderAsset();
        AssetInitData data;
        data.SerializedVersion = ShaderStorage::Header::Version;
        data.CustomData.Link(&_shaderHeader);
        parent->metadata.Release();
        return parent->SaveAsset(data);
    }

#endif
} // SE

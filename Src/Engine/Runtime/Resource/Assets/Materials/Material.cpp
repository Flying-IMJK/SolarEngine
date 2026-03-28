
#include "Material.h"

#include "ShaderCacheManager.h"
#include "Core/Serialization/MemoryReadStream.h"
#include "Core/Thread/Threading.h"
#include "Core/Utilities/Encoding.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/Base/GPUUtils.h"
#include "Runtime/Render/Assets/Material/MaterialShader.h"
#include "Runtime/ShaderCompilation/ShaderCompilationContext.h"
#include "Runtime/Resource/Factories/BinaryAssetFactory.h"

#if SE_EDITOR
#include "Generator/MaterialGenerator.h"
#endif

namespace SE
{
    BINARY_ASSET_FACTORY(Material, false);

    Material::Material(const AssetInfo* info) : MaterialBase(info)
    {
    }

    bool Material::IsMaterialInstance() const
    {
        return false;
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

    GPUShader* Material::GetShader() const
    {
        return m_MaterialShader ? m_MaterialShader->GetShader() : nullptr;
    }

    bool Material::IsReady() const
    {
        return m_MaterialShader && m_MaterialShader->IsReady();
    }

    EnumFlags<DrawPass> Material::GetDrawModes() const
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
        ENGINE_ASSERT(IsReady());

        MaterialParamsLink* lastLink = params.paramsLink;
        MaterialParamsLink link;
        link.This = &Params;
        if (lastLink)
        {
            while (lastLink->Down)
                lastLink = lastLink->Down;
            lastLink->Down =&link;
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

        // If engine was compiled with shaders compiling service:
        // - Material should be changed in need to convert it to the newer version (via Visject Surface)
        //   Shader should be recompiled if shader source code has been modified
        // otherwise:
        // - If material version is not supported then material cannot be loaded
#if SE_EDITOR

        // Check if current engine has different materials version or convert it by force or has no source generated at all
        if (_shaderHeader.Material.GraphVersion != MATERIAL_GRAPH_VERSION
    #if MATERIAL_AUTO_GENERATE_MISSING_SOURCE
            || !HasChunk(SHADER_FILE_CHUNK_SOURCE)
    #endif
            || HasDependenciesModified()
    #if COMPILE_WITH_DEV_ENV
            // Set to true to enable force GPU shader regeneration (don't commit it)
            || false
    #endif
        )
        {
            // Guard file with the lock during shader generation (prevents FlaxStorage::Tick from messing with the file)
            auto lock = storage->Lock();

            // Prepare
            MaterialGenerator generator;
            generator.Error.Bind(&OnGeneratorError);
            if (_shaderHeader.Material.GraphVersion != MATERIAL_GRAPH_VERSION)
                LOG_INFO("Resource", "Converting material \'{0}\', from version {1} to {2}...", ToString(), _shaderHeader.Material.GraphVersion, MATERIAL_GRAPH_VERSION);
            else
                LOG_INFO("Resource", "Updating material \'{0}\'...", ToString());

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
            generator.AddLayer(layer);

            // Get chunk with material parameters
            materialParamsChunk = GetOrCreateChunk(SHADER_FILE_CHUNK_MATERIAL_PARAMS);
            if (materialParamsChunk == nullptr)
                return LoadResult::MissingDataChunk;
            materialParamsChunk->Data.Release();

            // Generate material source code and metadata
            MemoryWriteStream newMaterialMeta(1024);
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

        // Ensure that material is in the current version (whole materials pipeline depends on that)
        if (_shaderHeader.Material.GraphVersion != MATERIAL_GRAPH_VERSION)
        {
            LOG_FATAL("Resource", "Unsupported material version: {0} in material \'{1}\'. Current is {2}.", _shaderHeader.Material.GraphVersion, ToString(), MATERIAL_GRAPH_VERSION);
            return LoadResult::Failed;
        }

#endif
        // Load shader cache (it may call compilation or gather cached data)
        ShaderCacheResult shaderCache;
        if (!LoadShaderCache(shaderCache))
        {
            LOG_ERROR("Resource", "Cannot load \'{0}\' shader cache.", ToString());
#if 1
            return LoadResult::Failed;
#else
            // Custom path: don't fail asset loading but use dummy material instead (surface and parameters will be available for editing)
            LOG(Error, "Using dummy material shader for \'{0}\'.", ToString());
            MemoryWriteStream dummyShader(64);
            {
                dummyShader.WriteInt32(6);
                dummyShader.WriteInt32(0);
                dummyShader.WriteInt32(0);
                dummyShader.WriteByte(0);
                dummyShader.WriteByte(0);
            }
            MemoryReadStream shaderCacheStream(dummyShader.GetHandle(), dummyShader.GetPosition());
            _materialShader = MaterialShader::CreateDummy(shaderCacheStream, _shaderHeader.Material.Info);
            if (_materialShader == nullptr)
            {
                LOG_WARNING("Resource", "Cannot load material.");
                return LoadResult::Failed;
            }
#endif
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
        return result;
    }

#if SE_EDITOR

    void Material::OnDependencyModified(BinaryAsset* asset)
    {
        BinaryAsset::OnDependencyModified(asset);

        Reload();
    }

    void Material::InitCompilationOptions(ShaderCompilationOptions& options)
    {
        // Base
        IShaderAsset::InitCompilationOptions(options);


        // Ensure that this call is valid (material features switches may depend on target compilation platform)
        ENGINE_ASSERT(options.Profile != ShaderProfile::Unknown);

        // Prepare
        auto& info = _shaderHeader.Material.Info;
        const bool isSurfaceOrTerrainOrDeformable = info.Domain == MaterialDomain::Surface || info.Domain == MaterialDomain::Terrain || info.Domain == MaterialDomain::Deformable;
        const bool useCustomData = info.ShadingModel == MaterialShadingModel::Subsurface || info.ShadingModel == MaterialShadingModel::Foliage;
        const bool useForward = ((info.Domain == MaterialDomain::Surface || info.Domain == MaterialDomain::Deformable) && info.BlendMode != MaterialBlendMode::Opaque) || info.Domain == MaterialDomain::Particle;
        const bool useTess =
                info.TessellationMode != TessellationMethod::None &&
                GPUUtils::CanSupportTessellation(options.Profile) && isSurfaceOrTerrainOrDeformable;

        const bool useDistortion =
                (info.Domain == MaterialDomain::Surface || info.Domain == MaterialDomain::Deformable || info.Domain == MaterialDomain::Particle) &&
                info.BlendMode != MaterialBlendMode::Opaque &&
                info.UsageFlags.IsFlag(MaterialUsage::UseRefraction) &&
                !info.FeaturesFlags.IsFlag(MaterialFeatures::DisableDistortion);

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

        // Setup shader macros
        options.Macros.Add({ "MATERIAL_DOMAIN", Numbers[(int32)info.Domain] });
        options.Macros.Add({ "MATERIAL_BLEND", Numbers[(int32)info.BlendMode] });
        options.Macros.Add({ "MATERIAL_SHADING_MODEL", Numbers[(int32)info.ShadingModel] });
        options.Macros.Add({ "MATERIAL_MASKED", Numbers[info.UsageFlags.IsFlag(MaterialUsage::UseMask) ? 1 : 0] });
        options.Macros.Add({ "DECAL_BLEND_MODE", Numbers[(int32)info.DecalBlendingMode] });
        options.Macros.Add({ "USE_EMISSIVE", Numbers[info.UsageFlags.IsFlag(MaterialUsage::UseEmissive) ? 1 : 0] });
        options.Macros.Add({ "USE_NORMAL", Numbers[info.UsageFlags.IsFlag( MaterialUsage::UseNormal) ? 1 : 0] });
        options.Macros.Add({ "USE_POSITION_OFFSET", Numbers[info.UsageFlags.IsFlag( MaterialUsage::UsePositionOffset) ? 1 : 0] });
        options.Macros.Add({ "USE_VERTEX_COLOR", Numbers[info.UsageFlags.IsFlag( MaterialUsage::UseVertexColor) ? 1 : 0] });
        options.Macros.Add({ "USE_DISPLACEMENT", Numbers[info.UsageFlags.IsFlag( MaterialUsage::UseDisplacement) ? 1 : 0] });
        options.Macros.Add({ "USE_DITHERED_LOD_TRANSITION", Numbers[info.FeaturesFlags.IsFlag(MaterialFeatures::DitheredLODTransition) ? 1 : 0] });
        options.Macros.Add({ "USE_GBUFFER_CUSTOM_DATA", Numbers[useCustomData ? 1 : 0] });
        options.Macros.Add({ "USE_REFLECTIONS", Numbers[info.FeaturesFlags.IsFlag(MaterialFeatures::DisableReflections) ? 0 : 1] });
        if (!info.FeaturesFlags.IsFlag(MaterialFeatures::DisableReflections) && info.FeaturesFlags.IsFlag(MaterialFeatures::ScreenSpaceReflections))
            options.Macros.Add({ "MATERIAL_REFLECTIONS", Numbers[1] });
        options.Macros.Add({ "USE_FOG", Numbers[info.FeaturesFlags.IsFlag(MaterialFeatures::DisableFog) ? 0 : 1] });
        if (useForward)
        {
            options.Macros.Add({ "USE_PIXEL_NORMAL_OFFSET_REFRACTION", Numbers[info.FeaturesFlags.IsFlag(MaterialFeatures::PixelNormalOffsetRefraction) ? 1 : 0] });
            switch (info.TransparentLightingMode)
            {
            case MaterialTransparentLightingMode::Surface:
                break;
            case MaterialTransparentLightingMode::SurfaceNonDirectional:
                options.Macros.Add({ "LIGHTING_NO_DIRECTIONAL", "1" });
                break;
            }
        }

        // TODO: don't compile VS_Depth for deferred/forward materials if material doesn't use position offset or masking

        options.Macros.Add({ "USE_TESSELLATION", Numbers[useTess ? 1 : 0] });
        options.Macros.Add({ "TESSELLATION_IN_CONTROL_POINTS", "3" });
        if (useTess)
        {
            switch (info.TessellationMode)
            {
            case TessellationMethod::Flat:
                options.Macros.Add({ "MATERIAL_TESSELLATION", "MATERIAL_TESSELLATION_FLAT" });
                break;
            case TessellationMethod::PointNormal:
                options.Macros.Add({ "MATERIAL_TESSELLATION", "MATERIAL_TESSELLATION_PN" });
                break;
            case TessellationMethod::Phong:
                options.Macros.Add({ "MATERIAL_TESSELLATION", "MATERIAL_TESSELLATION_PHONG" });
                break;
            }
            options.Macros.Add({ "MAX_TESSELLATION_FACTOR", Numbers[Math::Min<int32>(info.MaxTessellationFactor, ARRAY_SIZE(Numbers) - 1)] });
        }

        // Helper macros (used by the parser)
        options.Macros.Add({ "IS_SURFACE", Numbers[info.Domain == MaterialDomain::Surface ? 1 : 0] });
        options.Macros.Add({ "IS_POST_FX", Numbers[info.Domain == MaterialDomain::PostProcess ? 1 : 0] });
        options.Macros.Add({ "IS_GUI", Numbers[info.Domain == MaterialDomain::GUI ? 1 : 0] });
        options.Macros.Add({ "IS_DECAL", Numbers[info.Domain == MaterialDomain::Decal ? 1 : 0] });
        options.Macros.Add({ "IS_TERRAIN", Numbers[info.Domain == MaterialDomain::Terrain ? 1 : 0] });
        options.Macros.Add({ "IS_PARTICLE", Numbers[info.Domain == MaterialDomain::Particle ? 1 : 0] });
        options.Macros.Add({ "IS_DEFORMABLE", Numbers[info.Domain == MaterialDomain::Deformable ? 1 : 0] });
        options.Macros.Add({ "USE_FORWARD", Numbers[useForward ? 1 : 0] });
        options.Macros.Add({ "USE_DEFERRED", Numbers[isSurfaceOrTerrainOrDeformable && info.BlendMode == MaterialBlendMode::Opaque ? 1 : 0] });
        options.Macros.Add({ "USE_DISTORTION", Numbers[useDistortion ? 1 : 0] });

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
        if (WaitForLoaded() && !LastLoadFailed())
            return result;
        Threading::ScopeLock lock(Locker);

        // Check if has that chunk
        if (HasChunk(SHADER_FILE_CHUNK_VISJECT_SURFACE))
        {
            // Load graph
            if (!LoadChunks(GET_CHUNK_FLAG(SHADER_FILE_CHUNK_VISJECT_SURFACE)))
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
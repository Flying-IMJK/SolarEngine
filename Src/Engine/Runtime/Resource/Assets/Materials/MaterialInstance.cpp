
#include "MaterialInstance.h"

#include "Runtime/Utilities/Variant.h"
#include "Runtime/Resource/Factories/BinaryAssetFactory.h"
#include "Runtime/Core/Serialization/MemoryReadStream.h"
#include "Runtime/Core/Serialization/MemoryWriteStream.h"
#include "Runtime/Core/Thread/Threading.h"

namespace SE
{
    BINARY_ASSET_FACTORY(MaterialInstance, false);


    MaterialInstance::MaterialInstance(const SpawnParams& params, const AssetInfo* info) : MaterialBase(params, info)
    {
    }

    void MaterialInstance::OnBaseSet()
    {
        Threading::ScopeLock lock(_baseMaterial->Locker);
        ENGINE_ASSERT(_baseMaterial->IsLoaded());

        _baseMaterial->AddReference();
        _baseMaterial->OnUnloadedEvent.Bind<MaterialInstance, &MaterialInstance::OnBaseUnloaded>(this);
        _baseMaterial->ParamsChanged.Bind<MaterialInstance, &MaterialInstance::OnBaseParamsChanged>(this);

        // Sync parameters with the base parameters to ensure all data is valid for rendering (constants offset and resource register)
        MaterialParams& baseParams = _baseMaterial->Params;
        Params.m_VersionHash = 0;
        if (Params.Count() != baseParams.Count())
        {
            // Params changed
            OnBaseParamsChanged();
            return;
        }
        for (int32 i = 0; i < Params.Count(); i++)
        {
            MaterialParameter& param = Params[i];
            MaterialParameter& baseParam = baseParams[i];

            if (param.GetID() != baseParam.GetID() || param.GetParameterType() != baseParam.GetParameterType())
            {
                // Params changed
                OnBaseParamsChanged();
                return;
            }

            param.m_IsPublic = baseParam.m_IsPublic;
            param.m_RegisterIndex = baseParam.m_RegisterIndex;
            param.m_Offset = baseParam.m_Offset;
            param.m_Name = baseParam.m_Name;
        }

        // Params are valid
        Params.m_VersionHash = baseParams.m_VersionHash;
        ParamsChanged();
    }

    void MaterialInstance::OnBaseUnset()
    {
        _baseMaterial->RemoveReference();
        _baseMaterial->OnUnloadedEvent.Unbind<MaterialInstance, &MaterialInstance::OnBaseUnloaded>(this);
        _baseMaterial->ParamsChanged.Unbind<MaterialInstance, &MaterialInstance::OnBaseParamsChanged>(this);
    }

    void MaterialInstance::OnBaseUnloaded(Asset* p)
    {
        SetBaseMaterial(nullptr);
    }

    void MaterialInstance::OnBaseParamsChanged()
    {
        Threading::ScopeLock lock(Locker);

        // Skip if version has not been changed and the hash is the same
        auto baseParams = &_baseMaterial->Params;
        if (Params.GetVersionHash() == baseParams->GetVersionHash())
            return;

        //LOG(Info, "Updating material instance params \'{0}\' (base: \'{1}\')", ToString(), _baseMaterial->ToString());

        // Cache previous parameters
        MaterialParams oldParams;
        Params.Clone(oldParams);

        // Get the newest parameters
        baseParams->Clone(Params);

        // Override all public parameters by default
        for (auto& param : Params)
            param.SetIsOverride(param.IsPublic());

        // Copy previous parameters values
        for (int32 i = 0; i < oldParams.Count(); i++)
        {
            const MaterialParameter& oldParam = oldParams[i];
            MaterialParameter* param = Params.Get(oldParam.GetParameterID());
            if (param)
            {
                // Check type
                if (oldParam.GetParameterType() == param->GetParameterType())
                {
                    // Restore value
                    const Variant value = oldParam.GetValue();
                    param->SetValue(value);
                    param->SetIsOverride(oldParam.IsOverride());
                }
                else
                {
                    LOG_INFO("Render", "Param {0} changed type from {1}", param->ToString(), oldParam.ToString());
                }
            }
        }

        ParamsChanged();
    }

    bool MaterialInstance::IsMaterialInstance() const
    {
        return true;
    }

#if SE_EDITOR

    void MaterialInstance::GetReferences(List<UID>& output) const
    {
        // Base
        MaterialBase::GetReferences(output);

        if (_baseMaterial)
            output.Add(_baseMaterial->GetID());
    }

#endif

    const MaterialInfo& MaterialInstance::GetInfo() const
    {
        if (_baseMaterial)
            return _baseMaterial->GetInfo();

        static MaterialInfo EmptyInfo;
        return EmptyInfo;
    }

    GPUShader* MaterialInstance::GetShader() const
    {
        return _baseMaterial ? _baseMaterial->GetShader() : nullptr;
    }

    bool MaterialInstance::IsReady() const
    {
        return IsLoaded() && _baseMaterial && _baseMaterial->IsReady();
    }

    EnumFlags<DrawPass> MaterialInstance::GetDrawModes() const
    {
        if (_baseMaterial)
            return _baseMaterial->GetDrawModes();
        return DrawPass::None;
    }

    bool MaterialInstance::CanUseLightmap() const
    {
        return _baseMaterial && _baseMaterial->CanUseLightmap();
    }

    bool MaterialInstance::CanUseInstancing(InstancingHandler& handler) const
    {
        return _baseMaterial && _baseMaterial->CanUseInstancing(handler);
    }

    void MaterialInstance::Bind(BindParameters& params)
    {
        //ENGINE_ASSERT(IsReady());
        //ENGINE_ASSERT(_baseMaterial->Params.GetVersionHash() == Params.GetVersionHash());

        auto lastLink = params.paramsLink;
        MaterialParamsLink link;
        link.This = &Params;
        if (lastLink)
        {
            while (lastLink->Down)
                lastLink = lastLink->Down;
            lastLink->Down = &link;
        }
        else
        {
            params.paramsLink = &link;
        }
        link.Up = lastLink;
        link.Down = nullptr;

        _baseMaterial->Bind(params);

        if (lastLink)
        {
            lastLink->Down = nullptr;
        }
        else
        {
            params.paramsLink = nullptr;
        }
    }

    Asset::LoadResult MaterialInstance::load()
    {
        // Get main chunk
        auto chunk0 = GetChunk(0);
        if (chunk0 == nullptr || !chunk0->IsValid())
            return LoadResult::MissingDataChunk;
        MemoryReadStream headerStream(chunk0->Get(), chunk0->Size());

        // Load base material
        UID baseMaterialId;
        headerStream.Read(baseMaterialId);
        auto baseMaterial = AssetContent::LoadAsync<MaterialBase>(baseMaterialId);

        // Load parameters
        Params.Load(&headerStream);

        if (baseMaterial && !baseMaterial->WaitForLoaded())
        {
            _baseMaterial = baseMaterial;
            OnBaseSet();
        }
        else
        {
            // Clear parameters if has no material loaded
            _baseMaterial = nullptr;
            Params.Dispose();
            ParamsChanged();
        }

        return LoadResult::Ok;
    }

    void MaterialInstance::Unload(bool isReloading)
    {
        if (_baseMaterial)
        {
            OnBaseUnset();
            _baseMaterial = nullptr;
        }
        Params.Dispose();
    }

    AssetChunksFlag MaterialInstance::GetChunksToPreload() const
    {
        return GET_CHUNK_FLAG(0);
    }

    void MaterialInstance::SetBaseMaterial(MaterialBase* baseMaterial)
    {
        if (baseMaterial != nullptr && baseMaterial->WaitForLoaded())
        {
            LOG_WARNING("Render", "Cannot set base material of {0} to {1} because it failed to load.", ToString(), baseMaterial->ToString());
            return;
        }

        Threading::ScopeLock lock(Locker);

        if (baseMaterial == _baseMaterial)
            return;

#if !BUILD_RELEASE
        // Prevent recursion
        auto mi = TypeCast<MaterialInstance>(baseMaterial);
        while (mi)
        {
            if (mi == this)
            {
                LOG_ERROR("Render","Cannot set base material of {0} to {1} because it's recursive.", ToString(), baseMaterial->ToString());
                return;
            }
            mi = TypeCast<MaterialInstance>(mi->GetBaseMaterial());
        }
#endif

        // Release previous parameters
        Params.Dispose();

        // Set new value
        if (_baseMaterial)
        {
            OnBaseUnset();
        }
        _baseMaterial = baseMaterial;
        if (baseMaterial)
        {
            _baseMaterial = baseMaterial;
            OnBaseSet();
        }
    }

#if SE_EDITOR

    bool MaterialInstance::Save(const StringView& path)
    {
        // Validate state
        if (WaitForLoaded())
        {
            LOG_ERROR("Render","Asset loading failed. Cannot save it.");
            return true;
        }
        if (IsVirtual() && path.IsEmpty())
        {
            LOG_ERROR("Render","To save virtual asset asset you need to specify the target asset path location.");
            return true;
        }

        Threading::ScopeLock lock(Locker);

        // Save instance data
        MemoryWriteStream stream(512);
        {
            // Save base material ID
            const UID baseMaterialId = _baseMaterial ? _baseMaterial->GetID() : UID::Empty;
            stream.Write(baseMaterialId);

            // Save parameters
            Params.Save(&stream);
        }
        SetChunk(0, ToSpan(stream.GetHandle(), stream.GetPosition()));

        // Setup asset data
        AssetInitData data;
        data.SerializedVersion = 4;

        // Save data
        const bool saveResult = path.HasChars() ? SaveAsset(path, data) : SaveAsset(data, true);
        if (saveResult)
        {
            LOG_ERROR("Render","Cannot save \'{0}\'", ToString());
            return true;
        }

        return false;
    }
    
#endif
} // SE
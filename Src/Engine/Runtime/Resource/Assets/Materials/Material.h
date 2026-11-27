#pragma once

#include <Runtime/Resource/AssetRef.h>
#include <Runtime/Resource/Assets/Materials/IShaderAsset.h>
#include <Runtime/Resource/Assets/Materials/MaterialBase.h>

namespace SE
{
    class MaterialGenerator;
    class MaterialShader;
    class Shader;

#define MATERIAL_VERSION 1



    // <summary>
    /// Material asset that contains shader for rendering models on the GPU.
    /// </summary>
    SE_CLASS(Reflect, API(NoSpawn))
    class SE_API_RUNTIME Material final : public MaterialBase, public IShaderAsset
    {
    private:
        SE_DEFINE_CLASS(Material, MaterialBase);
        ASSET_HEADER(Material);
        friend class MaterialGenerator;
    private:
        MaterialShader* m_MaterialShader = nullptr;
        AssetRef<Shader> m_ShaderAsset;
        ShaderStorage::Header m_ShaderHeader;

    public:
        ~Material() override;

        bool IsGraphShader() const override { return m_ShaderHeader.Material.GraphVersion != -1; }

        Shader* GetShaderReference() const { return m_ShaderAsset.Get(); }
        bool   SetShaderReference(Shader* pShader);

        uint32 GetSerializedVersion() const override;

#if SE_EDITOR
        void GetReferences(List<UID>& output) const override;

        // [ShaderAssetBase]
        void InitCompilationRequest(ShaderCompileRequest& request, StringAnsi& preamble) override;
#endif

    public:
        // [MaterialBase]
        bool IsMaterialInstance() const override;

        // [IMaterial]
        const MaterialInfo& GetInfo() const override;
        GPUShader*      GetShader() const override;
        bool                IsReady() const override;
        DrawPass            GetDrawModes() const override;
        bool CanUseLightmap() const override;
        bool CanUseInstancing(InstancingHandler& handler) const override;
        void Bind(BindParameters& params) override;

        ShaderProgramInstance* GetProgram() override;

#if SE_EDITOR
        // Internal compilation saves may run before the asset has finished loading.
        bool Save() override;
#endif

    protected:
        // [BinaryAsset]
        bool OnInit(AssetInitData& initData) override;

        // [ShaderAssetBase]
        BinaryAsset* GetShaderAsset() const override
        {
            return (BinaryAsset*)this;
        }

        // [MaterialBase]
        LoadResult load() override;
        void Unload(bool isReloading) override;
        AssetChunksFlag GetChunksToPreload() const override;

#if SE_EDITOR
        void OnDependencyModified(BinaryAsset* asset) override;
#endif

    private:
        bool LoadShader(Shader* pShader);
        void ReleaseShader();
        void OnShaderLoaded(Asset* pAsset);
        void OnShaderReloading(Asset* pAsset);
        void OnShaderUnloaded();
    };
} // SE

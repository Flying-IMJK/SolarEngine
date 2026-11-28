#pragma once

#include <Runtime/Graphics/Shaders/GPUShader.h>
#include <Runtime/Graphics/Shaders/ShaderProgramInstance.h>
#include <Runtime/Resource/Assets/Materials/IShaderAsset.h>

namespace SE
{
    class MaterialParams;

    SE_CLASS(Reflect, API(NoSpawn))
    class SE_API_RUNTIME Shader final : public BinaryAsset, public IShaderAsset
    {
        SE_DEFINE_CLASS(Shader, BinaryAsset)
        ASSET_HEADER(Shader);

    public:
        ~Shader() override;
        
        SE_FUNCTION(API())
        bool SelectProgram(const String& name, const List<String>& selection, SE_PARAM(API(Out)) GPUShaderProgram*& output);

        SE_FUNCTION(API())
        bool SelectProgram(const String& name, const List<String>& selection, ShaderProgramInstance* pInstance);

        SE_FUNCTION(API())
        GPUShader* GetGPUShader() const { return m_GPUShader; }

        const List<ShaderMaterialParameter>& GetMaterialParameters();
        uint32 GetSerializedVersion() const override;

#if SE_EDITOR
        bool Save() override;
#endif

    protected:
        bool            OnInit(AssetInitData& initData) override;
        LoadResult      load() override;
        void            Unload(bool isReloading) override;
        AssetChunksFlag GetChunksToPreload() const override;
        BinaryAsset*    GetShaderAsset() const override { return const_cast<Shader*>(this); }

    private:
        GPUShader*        m_GPUShader = nullptr;
        ShaderStorage::Header m_ShaderHeader;
    };
} // namespace SE

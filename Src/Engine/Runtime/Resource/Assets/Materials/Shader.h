#pragma once

#include <Runtime/Graphics/Shaders/SLC2GPUShader.h>
#include <Runtime/Graphics/Shaders/ShaderProgramInstance.h>
#include <Runtime/Resource/Assets/Materials/IShaderAsset.h>

namespace SE
{
    /// Shared SLC2 shader asset for handwritten Slang programs.
    SE_CLASS(Reflect, API(NoSpawn))
    class SE_API_RUNTIME Shader final : public BinaryAsset, public IShaderAsset
    {
        SE_DEFINE_CLASS(Shader, BinaryAsset)
        ASSET_HEADER(Shader);

    public:
        ~Shader() override;
        // Render-thread selection. The asset owns the immutable Program; release instances and pipelines
        // referencing it before the asset reloads or unloads. Failure clears output.
        bool SelectProgram(const String& programName, const List<String>& selection, SLC2GPUShaderProgram*& output);
        bool SelectProgram(const String& programName, const List<String>& selection, ShaderProgramInstance* pInstance);
        SLC2GPUShader* GetGPUShader() const { return m_GPUShader; }
        uint32         GetSerializedVersion() const override;

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
        SLC2GPUShader*        m_GPUShader = nullptr;
        ShaderStorage::Header m_ShaderHeader;
    };
} // namespace SE

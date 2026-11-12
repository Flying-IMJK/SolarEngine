#pragma once

#include <Runtime/Core/Types/Collections/List.h>
#include <Runtime/Graphics/Base/GPUPipelineState.h>
#include <Runtime/Render/Assets/Material/IMaterial.h>

namespace SE
{
#define MATERIAL_GRAPH_VERSION 1

    class MemoryReadStream;
    class GPUShader;
    class GPUShaderProgram;

    /// Owns the compiled ShaderArtifact material and its render-thread Program instances and pipeline states.
    /// Release all instances and pipelines before releasing the immutable Programs they reference.
    class MaterialShader : public IMaterial
    {
    protected:
        MaterialInfo m_Info;
        ShaderProgramInstance* m_SelectedProgram = nullptr;

        explicit MaterialShader(const StringView& name);
        bool BindProgram(BindParameters& params, const String& programName, const GPUPipelineState::Description& description);
        GPUPipelineState::Description GetSurfacePipelineDescription(const BindParameters& params) const;
        bool HasProgram(const String& programName) const;

    public:
        virtual ~MaterialShader();

        ShaderProgramInstance* GetProgram() override;

        const MaterialInfo& GetInfo() const override;
        GPUShader*      GetShader() const override;
        bool                IsReady() const override;
        virtual void Unload();

        static MaterialShader* Create(const StringView& name, MemoryReadStream& shaderCacheStream, const MaterialInfo& info);
        static MaterialShader* CreateDummy(MemoryReadStream& shaderCacheStream, const MaterialInfo& info);

    private:
        struct ProgramState
        {
            String                 Name;
            bool                   Instanced = false;
            GPUShaderProgram*  Program   = nullptr;
            ShaderProgramInstance* Instance  = nullptr;

            GPUPipelineState::Description Description;
            GPUPipelineState*             Pipeline = nullptr;
        };

        bool Load(MemoryReadStream& shaderCacheStream, const MaterialInfo& info);

        bool                m_IsLoaded = false;
        GPUShader*      m_Shader   = nullptr;
        List<ProgramState>  m_Programs;
    };
} // namespace SE

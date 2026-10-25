/*#pragma once

#include "MaterialInfo.h"
#include "MaterialParams.h"
#include "Runtime/API.h"
#include "Runtime/Core/Memory/Memory.h"
#include "Runtime/Graphics/Base/GPUPipelineState.h"
#include "Runtime/Graphics/Shaders/Reflection/ShaderProgramReflection.h"
#include "Runtime/Graphics/Shaders/SLC2GPUShader.h"
#include "Runtime/Render/RenderEnum.h"

namespace SE
{
    struct DrawCall;
    struct RenderContext;
    class ShaderProgramInstance;
    class SLC2GPUShaderProgram;

    enum class MaterialGeometryMode : byte
    {
        Static,
        Instanced,
        Skinned,
    };

    enum class MaterialRuntimeBindingSource : byte
    {
        PerView,
        PerDraw,
        SceneTexture,
        TextureGroupSampler,
        GameplayGlobal,
        GlobalSDF,
        DefaultResource,
    };

    struct SE_API_RUNTIME MaterialDrawContext
    {
        DrawPass Pass = DrawPass::None;
        MaterialGeometryMode GeometryMode = MaterialGeometryMode::Static;
        const RenderContext* Render = nullptr;
        const DrawCall* Draw = nullptr;
        uint64 ViewRevision = 0;
    };

    struct SE_API_RUNTIME MaterialSLC2Binding
    {
        ShaderProgramInstance* ProgramInstance = nullptr;
        SLC2GPUPipelineState* Pipeline = nullptr;
        uint64 GenerationID = 0;

        void Clear()
        {
            ProgramInstance = nullptr;
            Pipeline = nullptr;
            GenerationID = 0;
        }
    };

    struct SE_API_RUNTIME MaterialParameterBinding
    {
        String ShaderPath;
        UID ParameterID;
        ShaderProgramBindingKind BindingKind = ShaderProgramBindingKind::Uniform;
    };

    struct SE_API_RUNTIME MaterialProgramContract
    {
        DrawPass Pass = DrawPass::None;
        MaterialGeometryMode GeometryMode = MaterialGeometryMode::Static;
        ShaderProgramSelection Selection;
        SLC2GPUPipelineState::Description PipelineDescription = SLC2GPUPipelineState::Description::Default;
    };

    class SE_API_RUNTIME IMaterialRuntimeBindingProvider
    {
    public:
        virtual ~IMaterialRuntimeBindingProvider() = default;

        virtual bool SupportsBinding(MaterialRuntimeBindingSource source, MaterialParameterType parameterType, const ShaderProgramBinding& binding) const = 0;
        virtual bool WriteBinding(MaterialRuntimeBindingSource source, const MaterialParameter* parameter, const ShaderProgramBinding& binding, const MaterialDrawContext& context, ShaderProgramInstance& instance) const = 0;
    };

    struct SE_API_RUNTIME MaterialSLC2RuntimeData
    {
        List<byte> SLC2Data;
        List<MaterialProgramContract> Programs;
        List<MaterialParameterBinding> Bindings;
        // Provider 生命周期必须覆盖安装后的 MaterialRuntimeGeneration。
        IMaterialRuntimeBindingProvider* BindingProvider = nullptr;
    };

    class MaterialRuntimeProgram;
    class MaterialProgramInstanceCacheEntry;

    // 每个 MaterialBase 独占一份缓存，ParameterBlock 状态不会在 MaterialInstance 之间共享。
    class SE_API_RUNTIME MaterialProgramInstanceCache
    {
    public:
        ~MaterialProgramInstanceCache();

        void Clear();

    private:
        friend class MaterialRuntimeGeneration;
        List<MaterialProgramInstanceCacheEntry*> m_Entries;
    };

    // 一代完整且不可拆分的 Material SLC2 运行时快照。
    class SE_API_RUNTIME MaterialRuntimeGeneration
    {
    public:
        MaterialRuntimeGeneration() = default;
        ~MaterialRuntimeGeneration();

        static SharedPtr<MaterialRuntimeGeneration> Create(MaterialSLC2RuntimeData&& data,
                                                           const MaterialInfo&       materialInfo,
                                                           const MaterialParams&     parameters,
                                                           String&                   error);

        uint64 GetID() const;
        bool Prepare(const MaterialDrawContext& context, const List<const MaterialParams*>& parameterChain, MaterialProgramInstanceCache& cache, MaterialSLC2Binding& output);

    private:
        const MaterialRuntimeProgram* FindProgram(DrawPass pass, MaterialGeometryMode geometryMode) const;

        uint64 m_ID = 0;
        WeakPtr<MaterialRuntimeGeneration> m_Self;
        SLC2GPUShader* m_Shader = nullptr;
        IMaterialRuntimeBindingProvider* m_BindingProvider = nullptr;
        List<MaterialParameterBinding> m_Bindings;
        List<MaterialRuntimeProgram*> m_Programs;
    };
}*/

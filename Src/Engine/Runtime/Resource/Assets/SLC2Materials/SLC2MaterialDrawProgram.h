#pragma once

#include <Runtime/Graphics/Shaders/ShaderProgramInstance.h>
#include <Runtime/Resource/Assets/SLC2Materials/SLC2MaterialPipeline.h>

namespace SE
{
    class GPUContext;
    struct RenderGeometry;

    // Owns one mutable Program instance and retains the matching Pipeline and GraphicShader Generation. Externally
    // bound resources remain caller-owned through backend descriptor and draw submission.
    class SE_API_RUNTIME SLC2MaterialDrawProgram final
    {
    public:
        // Consumes a non-null selection and shares a non-null initialized Pipeline.
        SLC2MaterialDrawProgram(UniquePtr<ShaderProgramInstance> selection,
                                SharedPtr<SLC2MaterialPipeline>                pipeline);
        // Returned references and pointers are non-owning and remain valid only while this draw program lives.
        ShaderProgramInstance& GetInstance() { return *m_Selection; }
        SLC2GPUPipelineState*  GetPipeline() const { return m_Pipeline->State; }
        // Binds Pipeline and borrowed Geometry to context without issuing a draw. Call only from context's GPU
        // submission thread. False means the caller must not draw using the rejected state or Geometry.
        bool Prepare(GPUContext& context, const RenderGeometry& geometry) const;

    private:
        UniquePtr<ShaderProgramInstance> m_Selection;
        SharedPtr<SLC2MaterialPipeline> m_Pipeline;
    };
} // namespace SE

#pragma once

#include <Runtime/Graphics/Base/GPUPipelineState.h>

namespace SE
{
    enum class SLC2MaterialGeometryMode : byte
    {
        Static,
        Instanced,
        Skinned,
    };

    struct SLC2MaterialPipelineState
    {
        String                            ProgramId;
        SLC2GPUPipelineState::Description State;
    };

    // Cache entries retain their Program Generation until the Vulkan Pipeline is destroyed.
    class SLC2MaterialPipeline final
    {
    public:
        ~SLC2MaterialPipeline();
        String                             ProgramId;
        const SLC2GPUShaderProgram*        Program    = nullptr;
        SLC2GPUPipelineState*              State = nullptr;

        static bool ValidateState(const SLC2GPUPipelineState::Description& state);
    };
} // namespace SE

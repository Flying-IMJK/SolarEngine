#pragma once

#include "Runtime/API.h"
#include "Runtime/Render/RenderEnum.h"
#include "MaterialInfo.h"

namespace SE
{
    class SLC2GPUPipelineState;
    struct StaticMeshInstanceData;
    class GPUConstantBuffer;
    class GPUContext;
    struct RenderContext;
    class GPUTextureView;
    class SLC2GPUShader;
    class ShaderProgramInstance;
    class MaterialParamsLink;
    struct DrawCall;

    /// <summary>
    /// Interface for material objects.
    /// </summary>
    class SE_API_RUNTIME IMaterial
    {
    public:
        /// <summary>
        /// Gets the material info, structure which describes material surface.
        /// </summary>
        /// <returns>The constant reference to the material descriptor.</returns>
        virtual const MaterialInfo& GetInfo() const = 0;

        /// <summary>
        /// Gets the shader resource.
        /// </summary>
        /// <returns>The material shader resource.</returns>
        virtual SLC2GPUShader* GetShader() const = 0;

        /// <summary>
        /// whether material is a target shader.
        /// </summary>
        FORCE_INLINE bool IsDeformable(MaterialDomain domain) const
        {
            return GetInfo().Domain == domain;
        }

        /// <summary>
        /// Returns true if material is ready for rendering.
        /// </summary>
        /// <returns>True if can render that material</returns>
        virtual bool IsReady() const = 0;

        /// <summary>
        /// Gets the mask of render passes supported by this material.
        /// </summary>
        /// <returns>The draw passes supported by this material.</returns>
        virtual DrawPass GetDrawModes() const
        {
            return DrawPass::None;
        }

        /// <summary>
        /// Returns true if material can use lightmaps (this includes lightmaps offline baking and sampling at runtime).
        /// </summary>
        /// <returns>True if can use lightmaps, otherwise false</returns>
        virtual bool CanUseLightmap() const
        {
            return false;
        }

        /// <summary>
        /// The instancing handling used to hash, batch and write draw calls.
        /// </summary>
        struct InstancingHandler
        {
            void (*GetHash)(const DrawCall& drawCall, uint32& batchKey);
            bool (*CanBatch)(const DrawCall& a, const DrawCall& b);
            void (*WriteDrawCall)(StaticMeshInstanceData* instanceData, const DrawCall& drawCall);
        };

        /// <summary>
        /// Returns true if material can use draw calls instancing.
        /// </summary>
        /// <param name="handler">The output data for the instancing handling used to hash, batch and write draw calls. Valid only when function returns true.</param>
        /// <returns>True if can use instancing, otherwise false.</returns>
        virtual bool CanUseInstancing(InstancingHandler& handler) const
        {
#if BUILD_DEBUG
            handler = { nullptr, nullptr, nullptr };
#endif
            return false;
        }

    public:
        /// <summary>
        /// Settings for the material binding to the graphics pipeline.
        /// </summary>
        struct BindParameters
        {
            GPUContext* gpuContext;
            const RenderContext& renderContext;
            const DrawCall* firstDrawCall;
            int32 drawCallsCount;
            MaterialParamsLink* paramsLink = nullptr;
            void* customData = nullptr;
            float timeParam;
            bool                 UseInstancing = false;

            /// <summary>
            /// The input scene color. It's optional and used in forward/postFx rendering.
            /// </summary>
            GPUTextureView* Input = nullptr;

            BindParameters(GPUContext* context, const RenderContext& renderContext);
            BindParameters(GPUContext* context, const RenderContext& renderContext, const DrawCall& drawCall);
            BindParameters(GPUContext* context, const RenderContext& renderContext, const DrawCall* firstDrawCall, int32 drawCallsCount);
        };

        /// <summary>
        /// Binds the material state to the GPU pipeline. Should be called before the draw command.
        /// </summary>
        /// <param name="params">The material bind settings.</param>
        virtual void Bind(BindParameters& params) = 0;

        /// Returns the instance selected by the latest successful Bind, or nullptr after a failed Bind or unload.
        /// Bind render parameters and submit on the render thread before rebinding or unloading the material.
        /// The material owns the instance and its immutable Program; instances can be shared by material assets.
        virtual ShaderProgramInstance* GetProgram() = 0;

    };
}

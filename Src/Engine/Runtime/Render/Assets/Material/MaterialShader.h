#pragma once
#include "IMaterial.h"
#include "Runtime/Graphics/Base/GPUPipelineState.h"

namespace SE
{
    /// <summary>
    /// Current materials shader version.
    /// </summary>
    #define MATERIAL_GRAPH_VERSION 1

    class MemoryReadStream;
    class GPUPipelineState;

    /// <summary>
    /// Represents material shader that can be used to render objects, visuals or effects. Contains a dedicated shader.
    /// </summary>
    class MaterialShader : public IMaterial
    {
    protected:
        struct PipelineStateCache
        {
            GPUPipelineState* PS[6];
            GPUPipelineState::Description Desc;

            PipelineStateCache();

            void Init(GPUPipelineState::Description& desc)
            {
                Desc = desc;
            }

            GPUPipelineState* GetPS(CullMode mode, bool wireframe);

            GPUPipelineState* InitPS(CullMode mode, bool wireframe);

            void Release();
        };

    protected:
        bool m_IsLoaded;
        GPUShader* m_Shader;
        GPUConstantBuffer* m_CB;
        List<byte> m_CBData;
        MaterialInfo m_Info;

    protected:
        /// <summary>
        /// Init
        /// </summary>
        /// <param name="name">Material resource name</param>
        MaterialShader(const StringView& name);

    public:
        /// <summary>
        /// Finalizes an instance of the <see cref="MaterialShader"/> class.
        /// </summary>
        virtual ~MaterialShader();

    public:
        /// <summary>
        /// Creates and loads the material from the data.
        /// </summary>
        /// <param name="name">Material resource name</param>
        /// <param name="shaderCacheStream">Stream with compiled shader data</param>
        /// <param name="info">Loaded material info structure</param>
        /// <returns>The created and loaded material or null if failed.</returns>
        static MaterialShader* Create(const StringView& name, MemoryReadStream& shaderCacheStream, const MaterialInfo& info);

        /// <summary>
        /// Creates the dummy material used by the Null rendering backend to mock object but not perform any rendering.
        /// </summary>
        /// <param name="shaderCacheStream">The shader cache stream.</param>
        /// <param name="info">The material information.</param>
        /// <returns>The created and loaded material or null if failed.</returns>
        static MaterialShader* CreateDummy(MemoryReadStream& shaderCacheStream, const MaterialInfo& info);

        /// <summary>
        /// Clears the loaded data.
        /// </summary>
        virtual void Unload();

    protected:
        bool Load(MemoryReadStream& shaderCacheStream, const MaterialInfo& info);
        virtual bool OnLoad() = 0;

    public:
        // [IMaterial]
        const MaterialInfo& GetInfo() const override;
        GPUShader* GetShader() const override;
        bool IsReady() const override;
    };
} // SE


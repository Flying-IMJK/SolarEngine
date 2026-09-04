#pragma once

//#include "Runtime/Graphics/GPUConfig.h"
#include "Runtime/Graphics/Base/GPUPipelineState.h"
#include "GPUDeviceVulkan.h"
#include "VulkanTypes.h"
#include "DescriptorSetVulkan.h"
#include "CmdBufferVulkan.h"
#include "Runtime/Graphics/Shaders/ShaderBindingSnapshot.h"

namespace SE
{
    class PipelineLayoutVulkan;
    class SLC2ShaderProgramVulkan;
    struct SLC2VulkanDescriptorBinding;
    

    /// <summary>
    /// Vulkan Compute管线状态对象
    /// </summary>
    class ComputePipelineStateVulkan
    {
    private:
        GPUDeviceVulkan*      _device;
        VkPipeline            m_Handle;
        PipelineLayoutVulkan* m_Layout;

    public:
        ComputePipelineStateVulkan(GPUDeviceVulkan* device, VkPipeline pipeline, PipelineLayoutVulkan* layout);
        ~ComputePipelineStateVulkan();

    public:
        /// <summary>
        /// The cached shader descriptor infos for compute shader.
        /// </summary>
        const SpirvShaderDescriptorInfo* descriptorInfo;

        DescriptorSetWriteContainerVulkan dsWriteContainer;
        DescriptorSetWriterVulkan         dsWriter;

        const DescriptorSetLayoutVulkan* descriptorSetsLayout          = nullptr;
        TypedDescriptorPoolSetVulkan*    currentTypedDescriptorPoolSet = nullptr;
        List<VkDescriptorSet>            descriptorSetHandles;

        inline bool AcquirePoolSet(CmdBufferVulkan* cmdBuffer)
        {
            // Pipeline state has no current descriptor pools set or set owner is not current - acquire a new pool set
            DescriptorPoolSetContainerVulkan* cmdBufferPoolSet = cmdBuffer->GetDescriptorPoolSet();
            if (currentTypedDescriptorPoolSet == nullptr ||
                currentTypedDescriptorPoolSet->GetOwner() != cmdBufferPoolSet)
            {
                if (currentTypedDescriptorPoolSet)
                    currentTypedDescriptorPoolSet->GetOwner()->refs--;
                currentTypedDescriptorPoolSet = cmdBufferPoolSet->AcquireTypedPoolSet(*descriptorSetsLayout);
                currentTypedDescriptorPoolSet->GetOwner()->refs++;
                return true;
            }
            return false;
        }

        inline bool AllocateDescriptorSets()
        {
            return currentTypedDescriptorPoolSet->AllocateDescriptorSets(*descriptorSetsLayout,
                                                                         descriptorSetHandles.Get());
        }

        List<uint32> DynamicOffsets;

    public:
        void Bind(CmdBufferVulkan* cmdBuffer)
        {
            vkCmdBindDescriptorSets(cmdBuffer->GetHandle(),
                                    VK_PIPELINE_BIND_POINT_COMPUTE,
                                    GetLayout()->handle,
                                    0,
                                    descriptorSetHandles.Count(),
                                    descriptorSetHandles.Get(),
                                    DynamicOffsets.Count(),
                                    DynamicOffsets.Get());
        }

    public:
        VkPipeline GetHandle() const { return m_Handle; }

        PipelineLayoutVulkan* GetLayout() const { return m_Layout; }
    };

    /// <summary>
    /// Vulkan Graphics管线状态对象
    /// </summary>
    class GPUPipelineStateVulkan : public GPUResourceVulkan<GPUPipelineState>
    {
    private:
        Dictionary<RenderPassVulkan*, VkPipeline> m_Pipelines;
        VkGraphicsPipelineCreateInfo              m_Desc;
        VkPipelineShaderStageCreateInfo           m_ShaderStages[(int)ShaderStage::Max];
        VkPipelineInputAssemblyStateCreateInfo    m_DescInputAssembly;
        VkPipelineTessellationStateCreateInfo     m_DescTessellation;
        VkPipelineViewportStateCreateInfo         m_DescViewport;
        VkPipelineDynamicStateCreateInfo          m_DescDynamic;
        VkDynamicState                            m_DynamicStates[3];
        VkPipelineMultisampleStateCreateInfo      m_DescMultisample;
        VkPipelineDepthStencilStateCreateInfo     m_DescDepthStencil;
        VkPipelineRasterizationStateCreateInfo    m_DescRasterization;
        VkPipelineColorBlendStateCreateInfo       m_DescColorBlend;
        VkPipelineColorBlendAttachmentState       m_DescColorBlendAttachments[GPU_MAX_RT_BINDED];
        PipelineLayoutVulkan*                     m_Layout;

    public:
        /// <summary>
        /// Initializes a new instance of the <see cref="GPUPipelineStateVulkan"/> class.
        /// </summary>
        /// <param name="device">The graphics device.</param>
        GPUPipelineStateVulkan(GPUDeviceVulkan* device);

    public:
        /// <summary>
        /// The bitmask of stages that exist in this pipeline.
        /// </summary>
        uint32 usedStagesMask;

        uint32 blendEnable : 1;
        uint32 depthReadEnable : 1;
        uint32 depthWriteEnable : 1;
        uint32 stencilReadEnable : 1;
        uint32 stencilWriteEnable : 1;

        /// <summary>
        /// The bitmask of stages that have descriptors.
        /// </summary>
        uint32 hasDescriptorsPerStageMask;

        /// <summary>
        /// The cached shader bindings per stage.
        /// </summary>
        const ShaderBindings* shaderBindingsPerStage[DescriptorSet::GraphicsStagesCount];

        /// <summary>
        /// The cached shader descriptor infos per stage.
        /// </summary>
        const SpirvShaderDescriptorInfo* descriptorInfoPerStage[DescriptorSet::GraphicsStagesCount];

        const VkPipelineVertexInputStateCreateInfo* GetVertexInputState() const { return m_Desc.pVertexInputState; }

        DescriptorSetWriteContainerVulkan dsWriteContainer;
        DescriptorSetWriterVulkan         dsWriter[DescriptorSet::GraphicsStagesCount];

        const DescriptorSetLayoutVulkan* descriptorSetsLayout          = nullptr;
        TypedDescriptorPoolSetVulkan*    currentTypedDescriptorPoolSet = nullptr;
        List<VkDescriptorSet>            descriptorSetHandles;

        List<uint32> dynamicOffsets;

    public:
        inline bool AcquirePoolSet(CmdBufferVulkan* cmdBuffer)
        {
            // Lazy init
            if (!descriptorSetsLayout)
            {
                GetLayout();
            }

            // Pipeline state has no current descriptor pools set or set owner is not current - acquire a new pool set
            DescriptorPoolSetContainerVulkan* cmdBufferPoolSet = cmdBuffer->GetDescriptorPoolSet();
            if (currentTypedDescriptorPoolSet == nullptr ||
                currentTypedDescriptorPoolSet->GetOwner() != cmdBufferPoolSet)
            {
                if (currentTypedDescriptorPoolSet)
                {
                    currentTypedDescriptorPoolSet->GetOwner()->refs--;
                }
                currentTypedDescriptorPoolSet = cmdBufferPoolSet->AcquireTypedPoolSet(*descriptorSetsLayout);
                currentTypedDescriptorPoolSet->GetOwner()->refs++;
                return true;
            }
            return false;
        }

        /// <summary>
        /// Gets the Vulkan pipeline layout for this pipeline state.
        /// </summary>
        /// <returns>The layout.</returns>
        PipelineLayoutVulkan* GetLayout();

        /// <summary>
        /// Gets the Vulkan graphics pipeline object for the given rendering state. Uses depth buffer and render targets
        /// formats and multi-sample levels to setup a proper PSO. Uses caching.
        /// </summary>
        /// <param name="renderPass">The render pass.</param>
        /// <returns>Vulkan graphics pipeline object.</returns>
        VkPipeline GetState(RenderPassVulkan* renderPass);

    public:
        // [GPUPipelineState]
        bool IsValid() const final override;
        bool Init(const Description& desc) final override;

    protected:
        // [GPUResourceVulkan]
        void OnReleaseGPU() override;
    };


    /// <summary>
    /// SLC2 Vulkan pipeline 公共 descriptor 绑定状态。
    /// Compute 与 Graphics 都使用 ShaderBindingSnapshot 写 descriptor，区别只在 pipeline bind point。
    /// </summary>
    class SLC2PipelineStateVulkanBase : public GPUResourceVulkan<SLC2GPUPipelineState>
    {
    public:
        // [SLC2GPUPipelineState]
        bool IsValid() const final override;

    protected:
        SLC2PipelineStateVulkanBase(GPUDeviceVulkan* device);
        ~SLC2PipelineStateVulkanBase();

        PipelineLayoutVulkan*            GetLayout() const;
        const DescriptorSetLayoutVulkan* GetDescriptorSetLayout() const;
        bool                             PrepareAndBindDescriptors(GPUContextVulkan*                        context,
                                                                   const ShaderBindingSnapshot&             snapshot,
                                                                   const List<SLC2VulkanDescriptorBinding>& bindings,
                                                                   VkPipelineBindPoint                      bindPoint);

    private:
        bool                               SetupDescriptorWrites();
        bool                               WriteSnapshot(GPUContextVulkan*                        context,
                                                         const ShaderBindingSnapshot&             snapshot,
                                                         const List<SLC2VulkanDescriptorBinding>& bindings);

        bool                               WriteUniformData(GPUContextVulkan*                        context,
                                                            const ShaderBindingSnapshot&             snapshot,
                                                            const List<SLC2VulkanDescriptorBinding>& bindings);

        bool                               ValidateSnapshotCompleteness(const ShaderBindingSnapshot&             snapshot,
                                                                        const List<SLC2VulkanDescriptorBinding>& bindings) const;

        bool                               WriteResourceBinding(GPUContextVulkan*                   context,
                                                                const ShaderBindingResource&        resource,
                                                                const SLC2VulkanDescriptorBinding& binding);

        bool                               IsDescriptorCompatibleWithResource(VkDescriptorType descriptorType,
                                                                              ShaderRuntimeResourceType resourceType) const;

        const ShaderBindingUniformData*    FindUniformData(const ShaderBindingSnapshot& snapshot, uint32 blockId) const;

        void                               ReleaseDescriptorState();

    protected:
        PipelineLayoutVulkan*             m_Layout                        = nullptr;

    private:
        bool                              m_DescriptorWritesReady         = false;
        TypedDescriptorPoolSetVulkan*     m_CurrentTypedDescriptorPoolSet = nullptr;
        DescriptorSetWriteContainerVulkan m_DescriptorWriteContainer;
        List<VkDescriptorSet>             m_DescriptorSetHandles;
        List<uint32>                      m_DescriptorWriteSetIndices;
        List<uint32>                      m_DynamicOffsets;
    };

    /// <summary>
    /// Vulkan Compute管线状态对象
    /// 一个不可变 compute variant 对应的 Vulkan pipeline 状态。
    /// descriptor set 每次按 ShaderBindingSnapshot 写入，pipeline/layout 本身保持缓存复用。
    /// </summary>
    class SLC2ComputePipelineStateVulkan : public SLC2PipelineStateVulkanBase
    {
    public:
        SLC2ComputePipelineStateVulkan(GPUDeviceVulkan* device, VkPipeline pipeline, PipelineLayoutVulkan* layout);
        ~SLC2ComputePipelineStateVulkan();

        bool Init(const Description& desc, const SLC2GPUShaderProgram* program) override;

        VkPipeline                       GetHandle() const;
        PipelineLayoutVulkan*            GetLayout() const;
        const DescriptorSetLayoutVulkan* GetDescriptorSetLayout() const;
        bool                             PrepareAndBindDescriptors(GPUContextVulkan*                        context,
                                                                   const ShaderBindingSnapshot&             snapshot,
                                                                   const List<SLC2VulkanDescriptorBinding>& bindings);

    protected:
        // [GPUResourceVulkan]
        void OnReleaseGPU() override;

    private:
        VkPipeline m_Pipeline = VK_NULL_HANDLE;
    };

    /// <summary>
    /// Vulkan Graphics管线状态对象
    /// SLC2 graphics variant 使用独立 pipeline state，旧 GPUPipelineStateVulkan 仍保留给旧 GPUShaderProgram 路径。
    /// </summary>
    class SLC2GraphicsPipelineStateVulkan : public SLC2PipelineStateVulkanBase
    {
    public:
        SLC2GraphicsPipelineStateVulkan(GPUDeviceVulkan* device);
        ~SLC2GraphicsPipelineStateVulkan();

        virtual bool Init(const Description& desc, const SLC2GPUShaderProgram* program) override;

        bool                                     Matches(const Description& desc) const;
        VkPipeline                                GetState(RenderPassVulkan* renderPass);
        PipelineLayoutVulkan*                     GetLayout() const;
        const VkPipelineVertexInputStateCreateInfo* GetVertexInputState() const;
        bool                                      PrepareAndBindDescriptors(GPUContextVulkan*                        context,
                                                                            const ShaderBindingSnapshot&             snapshot,
                                                                            const List<SLC2VulkanDescriptorBinding>& bindings);

        uint32 blendEnable : 1;
        uint32 depthReadEnable : 1;
        uint32 depthWriteEnable : 1;
        uint32 stencilReadEnable : 1;
        uint32 stencilWriteEnable : 1;

    protected:
        // [GPUResourceVulkan]
        void OnReleaseGPU() override;

    private:
        void                                      ReleasePipelines();
        bool                                      AddShaderStage(ShaderStage stage, VkShaderStageFlagBits stageFlag);

        SLC2ShaderProgramVulkan*                  m_Program = nullptr;
        Dictionary<RenderPassVulkan*, VkPipeline> m_Pipelines;
        Description                               m_DescKey;
        VkGraphicsPipelineCreateInfo              m_Desc;
        VkPipelineShaderStageCreateInfo           m_ShaderStages[(int)ShaderStage::Max];
        bool                                      m_ShaderStagesMark[(int)ShaderStage::Max];
        VkPipelineVertexInputStateCreateInfo      m_DescVertexInput;
        VkPipelineInputAssemblyStateCreateInfo    m_DescInputAssembly;
        VkPipelineTessellationStateCreateInfo     m_DescTessellation;
        VkPipelineViewportStateCreateInfo         m_DescViewport;
        VkPipelineDynamicStateCreateInfo          m_DescDynamic;
        VkDynamicState                            m_DynamicStates[3];
        VkPipelineMultisampleStateCreateInfo      m_DescMultisample;
        VkPipelineDepthStencilStateCreateInfo     m_DescDepthStencil;
        VkPipelineRasterizationStateCreateInfo    m_DescRasterization;
        VkPipelineColorBlendStateCreateInfo       m_DescColorBlend;
        VkPipelineColorBlendAttachmentState       m_DescColorBlendAttachments[GPU_MAX_RT_BINDED];
    };
} // namespace SE

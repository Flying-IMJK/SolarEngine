#pragma once

//#include "Runtime/Graphics/GPUConfig.h"
#include "Runtime/Graphics/Base/GPUPipelineState.h"
#include "GPUDeviceVulkan.h"
#include "VulkanTypes.h"
#include "DescriptorSetVulkan.h"
#include "CmdBufferVulkan.h"
#include "Runtime/Graphics/Shaders/ShaderBindingSnapshot.h"
#include "Runtime/Graphics/Base/VertexFactoryLayout.h"

namespace SE
{
    class PipelineLayoutVulkan;
    class ShaderProgramVulkan;
    struct VulkanDescriptorBinding;

    /// <summary>
    /// ShaderArtifact Vulkan pipeline 公共 descriptor 绑定状态。
    /// Compute 与 Graphics 都使用 ShaderBindingSnapshot 写 descriptor，区别只在 pipeline bind point。
    /// </summary>
    class PipelineStateVulkanBase : public GPUResourceVulkan<GPUPipelineState>
    {
    public:
        // [GPUPipelineState]
        bool IsValid() const final override;

    protected:
        PipelineStateVulkanBase(GPUDeviceVulkan* device, PipelineLayoutVulkan* layout);
        virtual ~PipelineStateVulkanBase() override;

        PipelineLayoutVulkan*            GetLayout() const;
        const DescriptorSetLayoutVulkan* GetDescriptorSetLayout() const;
        bool                             PrepareAndBindDescriptors(GPUContextVulkan*                        context,
                                                                   const ShaderBindingSnapshot&             snapshot,
                                                                   const List<VulkanDescriptorBinding>& bindings,
                                                                   VkPipelineBindPoint                      bindPoint);

    private:
        bool                               SetupDescriptorWrites();
        bool                               WriteSnapshot(GPUContextVulkan*                        context,
                                                         const ShaderBindingSnapshot&             snapshot,
                                                         const List<VulkanDescriptorBinding>& bindings);

        bool                               WriteUniformData(GPUContextVulkan*                        context,
                                                            const ShaderBindingSnapshot&             snapshot,
                                                            const List<VulkanDescriptorBinding>& bindings);

        bool                               ValidateSnapshotCompleteness(const ShaderBindingSnapshot&             snapshot,
                                                                        const List<VulkanDescriptorBinding>& bindings) const;

        bool                               WriteResourceBinding(GPUContextVulkan*                   context,
                                                                const ShaderBindingResource&        resource,
                                                                const VulkanDescriptorBinding& binding);

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
    class ComputePipelineStateVulkan : public PipelineStateVulkanBase
    {
    public:
        ComputePipelineStateVulkan(GPUDeviceVulkan* device, VkPipeline pipeline, PipelineLayoutVulkan* layout);
        ~ComputePipelineStateVulkan() override;

        bool Init(const Description& desc, const GPUShaderProgram* program) override;

        VkPipeline                       GetHandle() const;
        PipelineLayoutVulkan*            GetLayout() const;
        const DescriptorSetLayoutVulkan* GetDescriptorSetLayout() const;
        bool                             PrepareAndBindDescriptors(GPUContextVulkan*                        context,
                                                                   const ShaderBindingSnapshot&             snapshot,
                                                                   const List<VulkanDescriptorBinding>& bindings);

    protected:
        // [GPUResourceVulkan]
        void OnReleaseGPU() override;

    private:
        VkPipeline m_Pipeline = VK_NULL_HANDLE;
    };

    /// <summary>
    /// Vulkan Graphics管线状态对象
    /// 每个 ShaderArtifact graphics variant 对应一个图形管线状态。
    /// </summary>
    class GraphicsPipelineStateVulkan : public PipelineStateVulkanBase
    {
    public:
        GraphicsPipelineStateVulkan(GPUDeviceVulkan* device);
        ~GraphicsPipelineStateVulkan() override;

        virtual bool Init(const Description& desc, const GPUShaderProgram* program) override;

        bool                                     Matches(const Description& desc) const;
        VkPipeline                                GetState(RenderPassVulkan* renderPass);
        VkPipeline                                GetState(RenderPassVulkan* renderPass, const VertexFactoryLayout& layout);
        VkPipeline                                GetPreparedState(RenderPassVulkan* renderPass);
        PipelineLayoutVulkan*                     GetLayout() const;
        const GPUShaderProgram*                GetProgram() const;
        const VkPipelineVertexInputStateCreateInfo* GetVertexInputState() const;
        bool                                      PrepareVertexInputState(const VertexFactoryLayout& layout);
        uint32                                    GetRequiredVertexBufferSlotsMask() const;
        String                                    GetRequiredVertexBufferSlotSemantics(uint32 slot) const;
        uint64                                    GetVertexFactoryLayoutHash() const;
        bool                                      PrepareAndBindDescriptors(GPUContextVulkan*                        context,
                                                                            const ShaderBindingSnapshot&             snapshot,
                                                                            const List<VulkanDescriptorBinding>& bindings);

        uint32 blendEnable : 1;
        uint32 depthReadEnable : 1;
        uint32 depthWriteEnable : 1;
        uint32 stencilReadEnable : 1;
        uint32 stencilWriteEnable : 1;

    protected:
        // [GPUResourceVulkan]
        void OnReleaseGPU() override;

    private:
        struct VertexInputState
        {
            VkPipelineVertexInputStateCreateInfo State;
            List<VkVertexInputBindingDescription> Bindings;
            List<VkVertexInputAttributeDescription> Attributes;
            String RequiredVertexBufferSlotSemantics[GPU_MAX_VB_BINDED];
            uint32 RequiredVertexBufferSlotsMask = 0;
            uint64 LayoutHash = 0;
        };

        struct PipelineCacheEntry
        {
            RenderPassVulkan* RenderPass = nullptr;
            uint64 VertexLayoutHash = 0;
            VkPipeline Pipeline = VK_NULL_HANDLE;
        };

        void                                      ReleasePipelines();
        bool                                      AddShaderStage(ShaderStage stage, VkShaderStageFlagBits stageFlag);
        bool                                      BuildVertexInputState(const VertexFactoryLayout& layout, VertexInputState& output) const;
        const VertexInputState*                   FindVertexInputState(uint64 layoutHash) const;
        VertexInputState*                         FindVertexInputState(uint64 layoutHash);
        VkPipeline                                GetStateInternal(RenderPassVulkan* renderPass, const VertexInputState& vertexInputState);

        ShaderProgramVulkan*                  m_Program = nullptr;
        List<PipelineCacheEntry>                  m_Pipelines;
        Description                               m_DescKey;
        VkGraphicsPipelineCreateInfo              m_Desc;
        VkPipelineShaderStageCreateInfo           m_ShaderStages[(int)ShaderStage::Max];
        bool                                      m_ShaderStagesMark[(int)ShaderStage::Max];
        VertexInputState                          m_EmptyVertexInputState;
        List<VertexInputState>                    m_VertexInputStates;
        const VertexInputState*                   m_CurrentVertexInputState = nullptr;
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

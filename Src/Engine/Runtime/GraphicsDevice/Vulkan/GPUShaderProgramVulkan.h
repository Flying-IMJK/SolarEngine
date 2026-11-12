#pragma once

#include <Runtime/Graphics/Shaders/GPUShaderProgram.h>
#include <Runtime/GraphicsDevice/Vulkan/VulkanTypes.h>
#include <Runtime/GraphicsDevice/Vulkan/VulkanNative.h>
#include <Runtime/GraphicsDevice/Vulkan/GPUDeviceVulkan.h>

namespace SE
{
	class GPUDeviceVulkan;
    class GPUContextVulkan;
    class ShaderBindingSnapshot;
    struct ShaderBindingUniformData;
    class ShaderBindingResource;
    class ComputePipelineStateVulkan;
    class GraphicsPipelineStateVulkan;

    enum class VulkanDescriptorTarget : byte
    {
        Resource,
        UniformBlock,
    };

    enum class VulkanResourceRole : byte
    {
        Primary,
        Counter,
    };

    // Program 初始化时把反射坐标解析为 dense runtime index。提交阶段不再接触 Block/Range/Resource 坐标。
    struct VulkanDescriptorBinding
    {
        VulkanDescriptorTarget Target      = VulkanDescriptorTarget::Resource;
        // Resource bindings always start from the frozen parent logical slot. Counter selects
        // the auxiliary buffer owned by that parent instead of caching a child resource pointer.
        VulkanResourceRole ResourceRole    = VulkanResourceRole::Primary;
        uint32           ResourceSlotIndex     = Max_uint32;
        uint32           UniformBlockIndex    = Max_uint32;
        uint32           UniformByteSize      = 0;
        uint32           Set                  = 0;
        uint32           Binding              = 0;
        uint32           ArrayElement         = 0;
        VkDescriptorType DescriptorType       = VK_DESCRIPTOR_TYPE_MAX_ENUM;
        uint32           DescriptorWriteIndex = 0;
    };

    struct VulkanStageModule
    {
        ShaderStage    Stage = ShaderStage::Max;
        StringAnsi     EntryPointAnsi;
        VkShaderModule Module = VK_NULL_HANDLE;
        int32          OutputControlPoints = 0;
    };

    class ShaderProgramVulkan final : public GPUShaderProgram
    {
    public:
        ShaderProgramVulkan(GPUDeviceVulkan* device);
        ~ShaderProgramVulkan() override;

        bool Initialize(const ShaderProgramRecord* program, const ShaderTargetRecord* target, const ShaderVariantRecord* variant) override;

        PipelineLayoutVulkan*                    GetPipelineLayout() const;
        const DescriptorSetLayoutVulkan*         GetDescriptorSetLayout() const;
        const List<VulkanDescriptorBinding>& GetDescriptorBindings() const;
        VkShaderModule                           GetShaderModule(ShaderStage stage) const;
        const char*                              GetEntryPoint(ShaderStage stage) const;
        // 延迟创建 compute pipeline；entry point 名称直接来自 ShaderArtifact stage 记录。
        ComputePipelineStateVulkan* GetOrCreateComputeState();
        bool PrepareAndBindComputeDescriptors(GPUContextVulkan* context, const ShaderBindingSnapshot& snapshot);
        bool BindGraphicsDescriptors(GPUContextVulkan*                context,
                                               const ShaderBindingSnapshot&     snapshot,
                                               GraphicsPipelineStateVulkan* state);

    private:
        GPUDeviceVulkan*                       m_Device         = nullptr;
        PipelineLayoutVulkan*                  m_PipelineLayout = nullptr;
        ComputePipelineStateVulkan*        m_ComputeState   = nullptr;
        List<VulkanStageModule>            m_StageModules;
        List<VulkanDescriptorBinding>      m_DescriptorBindings;
    };
}

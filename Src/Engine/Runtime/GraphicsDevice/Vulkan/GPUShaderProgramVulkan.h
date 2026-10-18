#pragma once

#include "Runtime/Graphics/Shaders/GPUShaderProgram.h"
#include "Runtime/Graphics/Shaders/SLC2GPUShaderProgram.h"
#include "VulkanTypes.h"
#include "VulkanNative.h"
#include "GPUDeviceVulkan.h"

namespace SE
{
	class ComputePipelineStateVulkan;

	template<typename BaseType>
	class GPUShaderProgramVulkan : public BaseType
	{
	protected:
		GPUDeviceVulkan* m_Device;

	public:

		GPUShaderProgramVulkan(GPUDeviceVulkan* device, const GPUShaderProgramInitializer& initializer, const SpirvShaderDescriptorInfo& descriptorInfo, VkShaderModule shaderModule)
			: m_Device(device)
			, shaderModule(shaderModule)
			, descriptorInfo(descriptorInfo)
		{
			BaseType::Init(initializer);
		}

		~GPUShaderProgramVulkan()
		{
			if (shaderModule)
			{
				m_Device->deferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::ShaderModule, shaderModule);
			}
		}

	public:
		VkShaderModule shaderModule;

		/// <summary>
		/// The descriptor information container.
		/// </summary>
		SpirvShaderDescriptorInfo descriptorInfo;

	public:
		// [BaseType]
		uint32 GetBufferSize() const override
		{
			return 0;
		}

		void* GetBufferHandle() const override
		{
			return (void*)shaderModule;
		}
	};


	class GPUShaderProgramVSVulkan : public GPUShaderProgramVulkan<GPUShaderProgramVS>
	{
	public:
		GPUShaderProgramVSVulkan(GPUDeviceVulkan* device, const GPUShaderProgramInitializer& initializer, const SpirvShaderDescriptorInfo& descriptorInfo, VkShaderModule shaderModule)
			: GPUShaderProgramVulkan(device, initializer, descriptorInfo, shaderModule)
		{
		}

	public:
		VkPipelineVertexInputStateCreateInfo VertexInputState;
		VkVertexInputBindingDescription VertexBindingDescriptions[VERTEX_SHADER_MAX_INPUT_ELEMENTS];
		VkVertexInputAttributeDescription VertexAttributeDescriptions[VERTEX_SHADER_MAX_INPUT_ELEMENTS];

	public:
		void* GetInputLayout() const override
		{
			return (void*)&VertexInputState;
		}

		byte GetInputLayoutSize() const override
		{
			return 0;
		}
	};


	/// <summary>
	/// Hull Shader for Vulkan backend.
	/// </summary>
	class GPUShaderProgramHSVulkan : public GPUShaderProgramVulkan<GPUShaderProgramHS>
	{
	public:
		GPUShaderProgramHSVulkan(GPUDeviceVulkan* device, const GPUShaderProgramInitializer& initializer, const SpirvShaderDescriptorInfo& descriptorInfo, VkShaderModule shaderModule, int32 controlPointsCount)
			: GPUShaderProgramVulkan(device, initializer, descriptorInfo, shaderModule)
		{
			m_ControlPointsCount = controlPointsCount;
		}
	};

	/// <summary>
	/// Domain Shader for Vulkan backend.
	/// </summary>
	class GPUShaderProgramDSVulkan : public GPUShaderProgramVulkan<GPUShaderProgramDS>
	{
	public:
		GPUShaderProgramDSVulkan(GPUDeviceVulkan* device, const GPUShaderProgramInitializer& initializer, const SpirvShaderDescriptorInfo& descriptorInfo, VkShaderModule shaderModule)
			: GPUShaderProgramVulkan(device, initializer, descriptorInfo, shaderModule)
		{
		}
	};



	/// <summary>
	/// Geometry Shader for Vulkan backend.
	/// </summary>
	class GPUShaderProgramGSVulkan : public GPUShaderProgramVulkan<GPUShaderProgramGS>
	{
	public:
		GPUShaderProgramGSVulkan(GPUDeviceVulkan* device, const GPUShaderProgramInitializer& initializer, const SpirvShaderDescriptorInfo& descriptorInfo, VkShaderModule shaderModule)
			: GPUShaderProgramVulkan(device, initializer, descriptorInfo, shaderModule)
		{
		}
	};


	/// <summary>
	/// Pixel Shader for Vulkan backend.
	/// </summary>
	class GPUShaderProgramPSVulkan : public GPUShaderProgramVulkan<GPUShaderProgramPS>
	{
	public:
		GPUShaderProgramPSVulkan(GPUDeviceVulkan* device, const GPUShaderProgramInitializer& initializer, const SpirvShaderDescriptorInfo& descriptorInfo, VkShaderModule shaderModule)
			: GPUShaderProgramVulkan(device, initializer, descriptorInfo, shaderModule)
		{
		}
	};

	/// <summary>
	/// Compute Shader for Vulkan backend.
	/// </summary>
	class GPUShaderProgramCSVulkan : public GPUShaderProgramVulkan<GPUShaderProgramCS>
	{
	private:
		ComputePipelineStateVulkan* m_PipelineState;

	public:
		GPUShaderProgramCSVulkan(GPUDeviceVulkan* device, const GPUShaderProgramInitializer& initializer, const SpirvShaderDescriptorInfo& descriptorInfo, VkShaderModule shaderModule)
			: GPUShaderProgramVulkan(device, initializer, descriptorInfo, shaderModule)
			, m_PipelineState(nullptr)
		{
		}

		/// <summary>
		/// Finalizes an instance of the <see cref="GPUShaderProgramCSVulkan"/> class.
		/// </summary>
		~GPUShaderProgramCSVulkan();

	public:
		/// <summary>
		/// Gets the state of the pipeline for the compute shader execution or creates a new one if missing.
		/// </summary>
		/// <returns>The compute pipeline state.</returns>
		ComputePipelineStateVulkan* GetOrCreateState();
	};



	//////////////////////////////////////////////

	class GPUDeviceVulkan;
    class GPUContextVulkan;
    class ShaderBindingSnapshot;
    struct ShaderBindingUniformData;
    struct ShaderBindingResource;
    class SLC2ComputePipelineStateVulkan;
    class SLC2GraphicsPipelineStateVulkan;

    enum class SLC2VulkanDescriptorTarget : byte
    {
        Resource,
        UniformBlock,
    };

    enum class SLC2VulkanResourceRole : byte
    {
        Primary,
        Counter,
    };

    // Program 初始化时把反射坐标解析为 dense runtime index。提交阶段不再接触 Block/Range/Resource 坐标。
    struct SLC2VulkanDescriptorBinding
    {
        SLC2VulkanDescriptorTarget Target      = SLC2VulkanDescriptorTarget::Resource;
        // Resource bindings always start from the frozen parent logical slot. Counter selects
        // the auxiliary buffer owned by that parent instead of caching a child resource pointer.
        SLC2VulkanResourceRole ResourceRole    = SLC2VulkanResourceRole::Primary;
        uint32           ResourceSlotIndex     = Max_uint32;
        uint32           UniformBlockIndex    = Max_uint32;
        uint32           UniformByteSize      = 0;
        uint32           Set                  = 0;
        uint32           Binding              = 0;
        uint32           ArrayElement         = 0;
        VkDescriptorType DescriptorType       = VK_DESCRIPTOR_TYPE_MAX_ENUM;
        uint32           DescriptorWriteIndex = 0;
    };

    struct SLC2VulkanStageModule
    {
        ShaderStage    Stage = ShaderStage::Max;
        StringAnsi     EntryPointAnsi;
        VkShaderModule Module = VK_NULL_HANDLE;
        int32          OutputControlPoints = 0;
    };

    class SLC2ShaderProgramVulkan final : public SLC2GPUShaderProgram
    {
    public:
        SLC2ShaderProgramVulkan(GPUDeviceVulkan* device);
        ~SLC2ShaderProgramVulkan() override;

        bool Initialize(const SLC2ProgramRecord* program, const SLC2TargetRecord* target, const SLC2VariantRecord* variant) override;

        PipelineLayoutVulkan*                    GetPipelineLayout() const;
        const DescriptorSetLayoutVulkan*         GetDescriptorSetLayout() const;
        const List<SLC2VulkanDescriptorBinding>& GetDescriptorBindings() const;
        VkShaderModule                           GetShaderModule(ShaderStage stage) const;
        const char*                              GetEntryPoint(ShaderStage stage) const;
        // 延迟创建 compute pipeline；entry point 名称直接来自 SLC2 stage 记录。
        SLC2ComputePipelineStateVulkan* GetOrCreateComputeState();
        bool PrepareAndBindComputeDescriptors(GPUContextVulkan* context, const ShaderBindingSnapshot& snapshot);
        bool PrepareAndBindGraphicsDescriptors(GPUContextVulkan*                context,
                                               const ShaderBindingSnapshot&     snapshot,
                                               SLC2GraphicsPipelineStateVulkan* state);

    private:
        GPUDeviceVulkan*                       m_Device         = nullptr;
        PipelineLayoutVulkan*                  m_PipelineLayout = nullptr;
        SLC2ComputePipelineStateVulkan*        m_ComputeState   = nullptr;
        List<SLC2VulkanStageModule>            m_StageModules;
        List<SLC2VulkanDescriptorBinding>      m_DescriptorBindings;
    };
}

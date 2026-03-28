#pragma once

#include "Runtime/Graphics/Shaders/GPUShaderProgram.h"
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
}

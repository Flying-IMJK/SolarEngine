#pragma once

#include "Runtime/Graphics/Shaders/GPUShader.h"
#include "Runtime/Graphics/Shaders/GPUConstantBuffer.h"
#include "VulkanNative.h"
#include "ResourceOwnerVulkan.h"

#include "Runtime/Core/Serialization/MemoryReadStream.h"

namespace SE
{
	class GPUContextVulkan;
	class GPUConstantBuffer;

	/// <summary>
	/// Shader for Vulkan backend.
	/// </summary>
	class GPUShaderVulkan : public GPUResourceVulkan<GPUShader>
	{
	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="GPUShaderVulkan"/> class.
		/// </summary>
		/// <param name="device">The device.</param>
		/// <param name="name">The resource name.</param>
		GPUShaderVulkan(GPUDeviceVulkan* device, const StringView& name);

	protected:
		// [GPUShader]
		GPUShaderProgram* CreateGPUShaderProgram(ShaderStage type, const GPUShaderProgramInitializer& initializer, byte* cacheBytes,
			uint32 cacheSize, MemoryReadStream& stream) override;
	};

	/// <summary>
	/// The shared ring buffer for uniform buffers uploading for Vulkan backend.
	/// </summary>
	/// <seealso cref="GPUResourceVulkan{GPUResource}" />
	/// <seealso cref="ResourceOwnerVulkan" />
	class UniformBufferUploaderVulkan : public GPUResourceVulkan<GPUResource>, public ResourceOwnerVulkan
	{
	public:
		struct Allocation
		{
			/// <summary>
			/// The allocation offset from the GPU buffer begin (in bytes).
			/// </summary>
			uint64 Offset;

			/// <summary>
			/// The allocation size (in bytes).
			/// </summary>
			uint64 Size;

			/// <summary>
			/// The GPU buffer.
			/// </summary>
			VkBuffer Buffer;

			/// <summary>
			/// The CPU memory address to the mapped buffer data. Can be used to write the uniform buffer contents to upload them to GPU.
			/// </summary>
			byte* CPUAddress;
		};

	private:
		VkBuffer m_Buffer;
		VmaAllocation m_Allocation;
		uint64 m_Size;
		uint64 m_Offset;
		uint32 m_MinAlignment;
		byte* m_Mapped;
		CmdBufferVulkan* m_FenceCmdBuffer;
		uint64 m_FenceCounter;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="UniformBufferUploaderVulkan"/> class.
		/// </summary>
		/// <param name="device">The graphics device.</param>
		UniformBufferUploaderVulkan(GPUDeviceVulkan* device);

	public:
		Allocation Allocate(uint64 size, uint32 alignment, GPUContextVulkan* context);

	public:
		// [GPUResourceVulkan]
		GPUResourceType GetResType() const final override
		{
			return GPUResourceType::Buffer;
		}

		// [ResourceOwnerVulkan]
		GPUResource* AsGPUResource() const override
		{
			return (GPUResource*)this;
		}

	protected:
		// [GPUResourceVulkan]
		void OnReleaseGPU() override;
	};

	/// <summary>
	/// Constant Buffer for Vulkan backend.
	/// </summary>
	class GPUConstantBufferVulkan : public GPUResourceVulkan<GPUConstantBuffer>, public DescriptorResourceVulkan
	{
	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="GPUConstantBufferVulkan"/> class.
		/// </summary>
		/// <param name="device">The graphics device.</param>
		/// <param name="size">The buffer size (in bytes).</param>
		GPUConstantBufferVulkan(GPUDeviceVulkan* device, uint32 size) noexcept;

	public:
		/// <summary>
		/// The last uploaded data inside the shared uniforms uploading ring buffer.
		/// </summary>
		UniformBufferUploaderVulkan::Allocation Allocation;

	public:
		// [DescriptorOwnerResourceVulkan]
		void AsDynamicUniformBuffer(GPUContextVulkan* context, VkBuffer& buffer, VkDeviceSize& offset, VkDeviceSize& range, uint32& dynamicOffset)// override
		{
			buffer = Allocation.Buffer;
			offset = 0;
			range = Allocation.Size;
			dynamicOffset = (uint32)Allocation.Offset;
		}
	};


}

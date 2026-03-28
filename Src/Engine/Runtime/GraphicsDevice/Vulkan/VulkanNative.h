#pragma once

#include "Core/Types/Variable.h"
#include "Core/Types/Collections/Queue.h"
#include "Core/Thread/Threading.h"
#include "Core/Memory/Memory.h"
#include "Core/Types/Strings/StringView.h"

#include "Runtime/Graphics/Base/GPUEnums.h"
#include "Runtime/Graphics/Textures/GPUSampler.h"
#include "Runtime/Graphics/Base/GPUResource.h"
#include "Runtime/Graphics/Base/GPUBuffer.h"

#include "VulkanInclude.h"
#include "VulkanTypes.h"
#include "Runtime/Graphics/GlobalSettings_GPU.h"

namespace SE
{
    class GPUDeviceVulkan;
	class CmdBufferVulkan;
	class RenderPassVulkan;
	class GPUContextVulkan;
	class GPUTextureVulkan;
	class GPUBufferVulkan;

	/// <summary>
	/// Vulkan后端的GPU资源
	/// </summary>
	template<class BaseType>
	class GPUResourceVulkan : public GPUResourceBase<GPUDeviceVulkan, BaseType>
	{
	public:
		GPUResourceVulkan(GPUDeviceVulkan* device, const StringView& name)
			: GPUResourceBase<GPUDeviceVulkan, BaseType>(device, name)
		{
		}
	};

	struct SE_API_RUNTIME FenceVulkan
	{
		VkFence handle;
		bool IsSignaled;
	};

	class SE_API_RUNTIME FenceManagerVulkan
	{
	private:
		GPUDeviceVulkan* m_Device = nullptr;
		List<FenceVulkan*> m_FreeFences;
		List<FenceVulkan*> m_UsedFences;

	public:
		~FenceManagerVulkan();

	public:
		void Init(GPUDeviceVulkan* device)
		{
			m_Device = device;
		}

		void Dispose();

		FenceVulkan* AllocateFence(bool createSignaled = false);

		inline bool IsFenceSignaled(FenceVulkan* fence) const
		{
			return fence->IsSignaled || CheckFenceState(fence);
		}

		// Returns true if waiting timed out or failed, false otherwise.
		bool WaitForFence(FenceVulkan* fence, uint64 timeInNanoseconds) const;

		void ResetFence(FenceVulkan* fence) const;

		// Sets the fence handle to null
		void ReleaseFence(FenceVulkan*& fence);

		// Sets the fence handle to null
		void WaitAndReleaseFence(FenceVulkan*& fence, uint64 timeInNanoseconds);

	private:
		// Returns true if fence was signaled, otherwise false.
		bool CheckFenceState(FenceVulkan* fence) const;

		void DestroyFence(FenceVulkan* fence) const;
	};

	class SE_API_RUNTIME SemaphoreVulkan
	{
	private:
		GPUDeviceVulkan* m_Device;
		VkSemaphore m_SemaphoreHandle;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="SemaphoreVulkan"/> class.
		/// </summary>
		/// <param name="device">The graphics device.</param>
		SemaphoreVulkan(GPUDeviceVulkan* device);

		/// <summary>
		/// Finalizes an instance of the <see cref="SemaphoreVulkan"/> class.
		/// </summary>
		~SemaphoreVulkan();

		/// <summary>
		/// Gets the handle.
		/// </summary>
		/// <returns>The semaphore.</returns>
		inline VkSemaphore GetHandle() const
		{
			return m_SemaphoreHandle;
		}
	};

	class SE_API_RUNTIME DeferredDeletionQueueVulkan
	{
	public:
		enum Type
		{
			RenderPass,
			Buffer,
			BufferView,
			Image,
			ImageView,
			Pipeline,
			PipelineLayout,
			Framebuffer,
			DescriptorSetLayout,
			Sampler,
			Semaphore,
			ShaderModule,
			Event,
			QueryPool,
		};

	private:
		struct Entry
		{
			uint64 FenceCounter;
			uint64 Handle;
			uint64 FrameNumber;
			VmaAllocation AllocationHandle;
			Type StructureType;
			CmdBufferVulkan* CmdBuffer;
		};

		GPUDeviceVulkan* _device;
		CriticalSection _locker;
		List<Entry> _entries;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="DeferredDeletionQueueVulkan"/> class.
		/// </summary>
		/// <param name="device">The graphics device.</param>
		DeferredDeletionQueueVulkan(GPUDeviceVulkan* device);
		DeferredDeletionQueueVulkan();

		/// <summary>
		/// Finalizes an instance of the <see cref="DeferredDeletionQueueVulkan"/> class.
		/// </summary>
		~DeferredDeletionQueueVulkan();

	public:
		template<typename T>
		inline void EnqueueResource(Type type, T handle)
		{
			static_assert(sizeof(T) <= sizeof(uint64), "Invalid handle size.");
			EnqueueGenericResource(type, (uint64)handle, VK_NULL_HANDLE);
		}

		template<typename T>
		inline void EnqueueResource(Type type, T handle, VmaAllocation allocation)
		{
			static_assert(sizeof(T) <= sizeof(uint64), "Invalid handle size.");
			EnqueueGenericResource(type, (uint64)handle, allocation);
		}

		void ReleaseResources(bool immediately = false);

	private:
		void EnqueueGenericResource(Type type, uint64 handle, VmaAllocation allocation);
	};

	/// <summary>
	/// Vulkan staging buffers manager.
	/// </summary>
	class SE_API_RUNTIME StagingManagerVulkan
	{
	private:
		struct PendingEntry
		{
			GPUBuffer* Buffer;
			CmdBufferVulkan* CmdBuffer;
			uint64 FenceCounter;
		};

		struct FreeEntry
		{
			GPUBuffer* Buffer;
			uint64 FrameNumber;
		};

		GPUDeviceVulkan* m_Device;
		CriticalSection m_Locker;
		List<GPUBuffer*> m_AllBuffers;
		List<FreeEntry> m_FreeBuffers;
		List<PendingEntry> m_PendingBuffers;

		uint64 m_AllBuffersTotalSize = 0;
		uint64 m_AllBuffersPeekSize = 0;
		uint64 m_AllBuffersAllocSize = 0;
		uint64 m_AllBuffersFreeSize = 0;

	public:
		StagingManagerVulkan(GPUDeviceVulkan* device);
		GPUBuffer* AcquireBuffer(uint32 size, GPUResourceUsage usage);
		void ReleaseBuffer(CmdBufferVulkan* cmdBuffer, GPUBuffer*& buffer);
		void ProcessPendingFree();
		void Dispose();
	};

	class SE_API_RUNTIME DummyResourcesVulkan
	{
	private:
		GPUDeviceVulkan* _device;
		GPUTextureVulkan* _dummyTextures[6];
		GPUBufferVulkan* _dummyBuffer;
		GPUBufferVulkan* _dummyVB;
		VkSampler _staticSamplers[GPU_STATIC_SAMPLERS_COUNT];

	public:
		DummyResourcesVulkan(GPUDeviceVulkan* device);

	public:
		VkSampler* GetStaticSamplers();
		GPUTextureVulkan* GetDummyTexture(SpirvShaderResourceType type);
		GPUBufferVulkan* GetDummyBuffer();
		GPUBufferVulkan* GetDummyVertexBuffer();
		void Dispose();
	};

	struct SE_API_RUNTIME RenderTargetLayoutVulkan
	{
		union
		{
			struct
			{
				uint32 Layers : 10; // Limited by GPU_MAX_TEXTURE_ARRAY_SIZE
				uint32 RTsCount : 3; // Limited by GPU_MAX_RT_BINDED
				uint32 ReadDepth : 1;
				uint32 WriteDepth : 1;
				uint32 ReadStencil : 1;
				uint32 WriteStencil : 1;
				uint32 BlendEnable : 1;
			};
			uint32 Flags;
		};
		MSAALevel MSAA;
		PixelFormat DepthFormat;
		PixelFormat RTVsFormats[GPU_MAX_RT_BINDED];
		VkExtent2D Extent;

		inline bool operator==(const RenderTargetLayoutVulkan& other) const
		{
			return Platform::MemoryCompare(this, &other, sizeof(RenderTargetLayoutVulkan)) == 0;
		}
	};

	class SE_API_RUNTIME FramebufferVulkan
	{
	public:
		struct Desc
		{
			RenderPassVulkan* renderPass;
			int32 attachmentCount;
			VkImageView attachments[GPU_MAX_RT_BINDED + 1];

			inline bool operator==(const Desc& other) const
			{
				return Platform::MemoryCompare(this, &other, sizeof(Desc)) == 0;
			}
		};

		FramebufferVulkan(GPUDeviceVulkan* device, const Desc& key, const VkExtent2D& extent, uint32 layers);
		~FramebufferVulkan();

		VkFramebuffer handle;
		VkImageView attachments[GPU_MAX_RT_BINDED + 1];
		VkExtent2D extent;
		uint32 layers;

		bool HasReference(VkImageView imageView) const;

	private:
		GPUDeviceVulkan* m_Device;
	};

	class SE_API_RUNTIME RenderPassVulkan
	{
	public:
		VkRenderPass handle;
		RenderTargetLayoutVulkan layout;
#if VULKAN_USE_DEBUG_DATA
		VkRenderPassCreateInfo debugCreateInfo;
#endif

		RenderPassVulkan(GPUDeviceVulkan* device, const RenderTargetLayoutVulkan& layout);
		~RenderPassVulkan();

	private:
		GPUDeviceVulkan* m_Device;
	};

	class SE_API_RUNTIME QueryPoolVulkan
	{
	protected:
		struct Range
		{
			uint32 Start;
			uint32 Count;
		};

		GPUDeviceVulkan* m_Device;
		VkQueryPool m_Handle;

		volatile int32 m_Count;
		const uint32 m_Capacity;
		const VkQueryType m_Type;
#if VULKAN_RESET_QUERY_POOLS
		List<Range> m_ResetRanges;
#endif

	public:
		QueryPoolVulkan(GPUDeviceVulkan* device, int32 capacity, VkQueryType type);
		~QueryPoolVulkan();

	public:
		inline VkQueryPool GetHandle() const
		{
			return m_Handle;
		}

#if VULKAN_RESET_QUERY_POOLS
		void Reset(CmdBufferVulkan* cmdBuffer);
#endif
	};

	class SE_API_RUNTIME BufferedQueryPoolVulkan : public QueryPoolVulkan
	{
	private:
		List<uint64> m_QueryOutput;
		List<uint64> m_UsedQueryBits;
		List<uint64> m_StartedQueryBits;
		List<uint64> m_ReadResultsBits;

		// Last potentially free index in the pool
		int32 m_LastBeginIndex;

	public:
		BufferedQueryPoolVulkan(GPUDeviceVulkan* device, int32 capacity, VkQueryType type);
		bool AcquireQuery(uint32& resultIndex);
		void ReleaseQuery(uint32 queryIndex);
		void MarkQueryAsStarted(uint32 queryIndex);
		bool GetResults(GPUContextVulkan* context, uint32 index, uint64& result);
		bool HasRoom() const;
	};

	class DescriptorResourceVulkan
	{
	public:
		/// <summary>
		/// Finalizes an instance of the <see cref="DescriptorOwnerResourceVulkan"/> class.
		/// </summary>
		virtual ~DescriptorResourceVulkan()
		{
		}

	public:
		/// <summary>
		/// Gets the sampler descriptor.
		/// </summary>
		/// <param name="context">The GPU context. Can be used to add memory barriers to the pipeline before binding the descriptor to the pipeline.</param>
		/// <param name="sampler">The sampler.</param>
		virtual void AsSampler(GPUContextVulkan* context, VkSampler& sampler)
		{
			ENGINE_UNIMPLEMENTED_FUNCTION();
		}

		/// <summary>
		/// Gets the image descriptor.
		/// </summary>
		/// <param name="context">The GPU context. Can be used to add memory barriers to the pipeline before binding the descriptor to the pipeline.</param>
		/// <param name="imageView">The image view.</param>
		/// <param name="layout">The image layout.</param>
		virtual void AsImage(GPUContextVulkan* context, VkImageView& imageView, VkImageLayout& layout)
		{
			ENGINE_UNIMPLEMENTED_FUNCTION();
		}

		/// <summary>
		/// Gets the storage image descriptor (VK_DESCRIPTOR_TYPE_STORAGE_IMAGE).
		/// </summary>
		/// <param name="context">The GPU context. Can be used to add memory barriers to the pipeline before binding the descriptor to the pipeline.</param>
		/// <param name="imageView">The image view.</param>
		/// <param name="layout">The image layout.</param>
		virtual void AsStorageImage(GPUContextVulkan* context, VkImageView& imageView, VkImageLayout& layout)
		{
			ENGINE_UNIMPLEMENTED_FUNCTION();
		}

		/// <summary>
		/// Gets the uniform texel buffer descriptor (VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER).
		/// </summary>
		/// <param name="context">The GPU context. Can be used to add memory barriers to the pipeline before binding the descriptor to the pipeline.</param>
		/// <param name="bufferView">The buffer view.</param>
		virtual void AsUniformTexelBuffer(GPUContextVulkan* context, VkBufferView& bufferView)
		{
			ENGINE_UNIMPLEMENTED_FUNCTION();
		}

		/// <summary>
		/// Gets the storage buffer descriptor (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER).
		/// </summary>
		/// <param name="context">The GPU context. Can be used to add memory barriers to the pipeline before binding the descriptor to the pipeline.</param>
		/// <param name="buffer">The buffer.</param>
		/// <param name="offset">The range offset (in bytes).</param>
		/// <param name="range">The range size (in bytes).</param>
		virtual void AsStorageBuffer(GPUContextVulkan* context, VkBuffer& buffer, VkDeviceSize& offset, VkDeviceSize& range)
		{
			ENGINE_UNIMPLEMENTED_FUNCTION();
		}

		/// <summary>
		/// Gets the storage texel buffer descriptor (VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER).
		/// </summary>
		/// <param name="context">The GPU context. Can be used to add memory barriers to the pipeline before binding the descriptor to the pipeline.</param>
		/// <param name="bufferView">The buffer view.</param>
		virtual void AsStorageTexelBuffer(GPUContextVulkan* context, VkBufferView& bufferView)
		{
			ENGINE_UNIMPLEMENTED_FUNCTION();
		}

		/// <summary>
		/// Gets the dynamic uniform buffer descriptor (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC).
		/// </summary>
		/// <param name="context">The GPU context. Can be used to add memory barriers to the pipeline before binding the descriptor to the pipeline.</param>
		/// <param name="buffer">The buffer.</param>
		/// <param name="offset">The range offset (in bytes).</param>
		/// <param name="range">The range size (in bytes).</param>
		/// <param name="dynamicOffset">The dynamic offset (in bytes).</param>
		virtual void AsDynamicUniformBuffer(GPUContextVulkan* context, VkBuffer& buffer, VkDeviceSize& offset, VkDeviceSize& range, uint32& dynamicOffset)
		{
			ENGINE_UNIMPLEMENTED_FUNCTION();
		}
	};

	uint32 SE_API_RUNTIME GetHash(const RenderTargetLayoutVulkan& key);

	uint32 SE_API_RUNTIME GetHash(const FramebufferVulkan::Desc& key);

	uint32 SE_API_RUNTIME GetHash(RenderPassVulkan* key);
}
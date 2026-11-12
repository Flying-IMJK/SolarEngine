#pragma once

#include "Runtime/Graphics/GPUContext.h"
//#include "Runtime/Graphics/Base/GPUConfig.h"
#include "Runtime/Graphics/GlobalSettings_GPU.h"

#include "VulkanInclude.h"
#include "VulkanTypes.h"

namespace SE
{
	class GPUDeviceVulkan;
	class QueueVulkan;
	class CmdBufferManagerVulkan;
	class CmdBufferVulkan;
	class RenderPassVulkan;
	class GraphicsPipelineStateVulkan;
	class GPUTextureViewVulkan;
	class GPUTextureVulkan;
	class GPUBufferVulkan;
	class DescriptorPoolVulkan;
	class DescriptorSetLayoutVulkan;

	/// <summary>
	/// Size of the pipeline barriers buffer size (will be auto-flushed on overflow).
	/// </summary>
	#define VK_BARRIER_BUFFER_SIZE 16

	/// <summary>
	/// The Vulkan pipeline resources layout barrier batching structure.
	/// </summary>
	struct PipelineBarrierVulkan
	{
		VkPipelineStageFlags SourceStage = 0;
		VkPipelineStageFlags DestStage = 0;
		List<VkImageMemoryBarrier, FixedAllocation<VK_BARRIER_BUFFER_SIZE>> ImageBarriers;
		List<VkBufferMemoryBarrier, FixedAllocation<VK_BARRIER_BUFFER_SIZE>> BufferBarriers;
#if VK_ENABLE_BARRIERS_DEBUG
		Array<GPUTextureViewVulkan*, FixedAllocation<VK_BARRIER_BUFFER_SIZE>> ImageBarriersDebug;
#endif

		inline bool IsFull() const
		{
			return ImageBarriers.Count() == VK_BARRIER_BUFFER_SIZE || BufferBarriers.Count() == VK_BARRIER_BUFFER_SIZE;
		}

		inline bool HasBarrier() const
		{
			return ImageBarriers.Count() + BufferBarriers.Count() != 0;
		}

		void Execute(const CmdBufferVulkan* cmdBuffer);
	};

	class GPUContextVulkan : public GPUContext
	{
	private:
		GPUDeviceVulkan* _device;
		QueueVulkan* _queue;
		CmdBufferManagerVulkan* m_CmdBufferManager;
		PipelineBarrierVulkan m_Barriers;

		int32 m_PsoDirtyFlag : 1;
		int32 m_RtDirtyFlag : 1;

		int32 _rtCount;
		int32 _vbCount;
		uint32 _stencilRef;

		// Render Geometry State
		bool m_RenderGeometryReady;
        PixelFormat _renderGeometryIndexFormat;
		uint32 _renderGeometryIndexBufferSize;
		uint32 _renderGeometryIndexOffset;
		uint64 m_RenderGeometryLayoutHash;
		uint32 m_RenderGeometryPerInstanceSlotsMask;
		uint32 m_RenderGeometryVertexElementCounts[GPU_MAX_VB_BINDED];

		RenderPassVulkan* m_RenderPass;
		GraphicsPipelineStateVulkan* m_CurrentGraphicsState;
		GPUTextureViewVulkan* _rtDepth;
		GPUTextureViewVulkan* _rtHandles[GPU_MAX_RT_BINDED];

		typedef List<DescriptorPoolVulkan*> DescriptorPoolArray;
		Dictionary<uint32, DescriptorPoolArray> _descriptorPools;
	private:
		bool OnDrawCall(ShaderProgramInstance& instance);
		void ClearRenderGeometry();

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="GPUContextVulkan"/> class.
		/// </summary>
		/// <param name="device">The graphics device.</param>
		/// <param name="queue">The commands submission device.</param>
		GPUContextVulkan(GPUDeviceVulkan* device, QueueVulkan* queue);

		/// <summary>
		/// Finalizes an instance of the <see cref="GPUContextVulkan"/> class.
		/// </summary>
		~GPUContextVulkan() override;
	public:

		inline QueueVulkan* GetQueue() const
		{
			return _queue;
		}

		inline CmdBufferManagerVulkan* GetCmdBufferManager() const
		{
			return m_CmdBufferManager;
		}

		void AddImageBarrier(VkImage image, VkImageLayout srcLayout, VkImageLayout dstLayout, const VkImageSubresourceRange& subresourceRange, GPUTextureViewVulkan* handle);
		void AddImageBarrier(GPUTextureViewVulkan* handle, VkImageLayout dstLayout);
		void AddImageBarrier(GPUTextureVulkan* texture, int32 mipSlice, int32 arraySlice, VkImageLayout dstLayout);
		void AddImageBarrier(GPUTextureVulkan* texture, VkImageLayout dstLayout);
		void AddBufferBarrier(GPUBufferVulkan* buffer, VkAccessFlags dstAccess);

		void FlushBarriers();

		// outSets must have been previously pre-allocated
		DescriptorPoolVulkan* AllocateDescriptorSets(const VkDescriptorSetAllocateInfo& descriptorSetAllocateInfo, const DescriptorSetLayoutVulkan& layout, VkDescriptorSet* outSets);

		void BeginRenderPass();
		void EndRenderPass();
	public:

		void FrameBegin() override;
		void FrameEnd() override;
		void EventBegin(const Char* name) override;
		void EventEnd() override;
		void* GetNativePtr() const override;
		bool IsDepthBufferBinded() override;
	public:
		void ResetRenderTarget() override;
		void SetRenderTarget(GPUTextureView* rt) override;
		void SetRenderTarget(GPUTextureView* rt, GPUTextureView* depthBuffer) override;
		void SetRenderTarget(const Span<GPUTextureView*>& rts, GPUTextureView* depthBuffer) override;
		void SetBlendFactor(const Float4& value) override;
		void SetStencilRef(uint32 value) override;
		void SetViewport(const Viewport& viewport) override;
		void SetScissor(const Rectangle& scissorRect) override;

	public:
		// ShaderArtifact 新路径：从 ShaderProgramInstance 冻结绑定快照，再交给 Vulkan 后端绑定 pipeline/descriptor。
		void Dispatch(ShaderProgramInstance& instance, uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ) override;
        void DispatchIndirect(ShaderProgramInstance& instance, GPUBuffer* bufferForArgs, uint32 offsetForArgs) override;
		void ResolveMultisample(GPUTexture* sourceMultisampleTexture, GPUTexture* destTexture, int32 sourceSubResource, int32 destSubResource, PixelFormat format) override;
		void DrawInstanced(ShaderProgramInstance* instance, uint32 verticesCount, uint32 instanceCount, int32 startInstance, int32 startVertex) override;
		void DrawIndexedInstanced(ShaderProgramInstance* instance, uint32 indicesCount, uint32 instanceCount, int32 startInstance, int32 startVertex, int32 startIndex) override;
        void DrawInstancedIndirect(ShaderProgramInstance& instance, GPUBuffer* bufferForArgs, uint32 offsetForArgs) override;
        void DrawIndexedInstancedIndirect(ShaderProgramInstance& instance, GPUBuffer* bufferForArgs, uint32 offsetForArgs) override;

	public:
		void BindVB(const Span<GPUBuffer*>& vertexBuffers, const uint32* vertexBuffersOffsets) override;
		void BindVB(GPUBuffer* vertexBuffers, const uint32 vertexBuffersOffsets) override;
		void BindIB(GPUBuffer* indexBuffer) override;
		bool BindRenderGeometry(const RenderGeometry& geometry) override;

		void Clear(GPUTextureView* rt, const Color& color) override;
		void ClearDepth(GPUTextureView* depthBuffer, float depthValue) override;
		void ClearUA(GPUBuffer* buf, const Float4& value) override;
		void ClearUA(GPUBuffer* buf, const uint32 value[4]) override;
		void ClearUA(GPUTexture* texture, const uint32 value[4]) override;
		void ClearUA(GPUTexture* texture, const Float4& value) override;

        void SetState(GPUPipelineState* state) override;
        GPUPipelineState* GetState() const override;

		void ClearState() override;
		void FlushState() override;
		void Flush() override;
	public:
		void UpdateBuffer(GPUBuffer* buffer, const void* data, uint32 size, uint32 offset) override;
		void CopyBuffer(GPUBuffer* dstBuffer, GPUBuffer* srcBuffer, uint32 size, uint32 dstOffset, uint32 srcOffset) override;
		void UpdateTexture(GPUTexture* texture, int32 arrayIndex, int32 mipIndex, const void* data, uint32 rowPitch, uint32 slicePitch) override;
		void CopyTexture(GPUTexture* dstResource, uint32 dstSubresource, uint32 dstX, uint32 dstY, uint32 dstZ, GPUTexture* srcResource, uint32 srcSubresource) override;
		void ResetCounter(GPUBuffer* buffer) override;
		void CopyCounter(GPUBuffer* dstBuffer, uint32 dstOffset, GPUBuffer* srcBuffer) override;
		void CopyResource(GPUResource* dstResource, GPUResource* srcResource) override;
		void CopySubresource(GPUResource* dstResource, uint32 dstSubresource, GPUResource* srcResource, uint32 srcSubresource) override;
		void UpdateCB(GPUConstantBuffer* cb, const void* data) override;
	};

}

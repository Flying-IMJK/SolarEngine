#pragma once

#include "Runtime/Core/Types/Variable.h"

#include "Runtime/API.h"
#include "Runtime/Graphics/Base/PixelFormat.h"
#include "Viewport.h"

namespace SE
{
	class MaterialBase;
	class GPUDevice;
	class GPUConstantBuffer;
	class GPUShaderProgramCS;
	class GPUBuffer;
	class GPUPipelineState;
	class GPUTexture;
	class GPUSampler;
	class GPUDevice;
	class GPUResource;
	class GPUResourceView;
	class GPUTextureView;
	class GPUBufferView;

	// Gets the GPU texture view. Checks if pointer is not null and texture has one or more mip levels loaded.
	#define GET_TEXTURE_VIEW_SAFE(t) (t && t->ResidentMipLevels() > 0 ? t->View() : nullptr)

	class SE_API_RUNTIME GPUContext
	{
		friend class GraphicSystem;
		struct PrivateData; 

	protected:
		GPUDevice* m_Device;
		PrivateData* m_PrivateData;
		double _lastRenderTime = -1;

		GPUContext(GPUDevice* device);
	public:
		/// <summary>
		/// Gets the graphics device.
		/// </summary>
		inline GPUDevice* GetDevice() const
		{
			return m_Device;
		}

		bool LoadDefaultResources();

		MaterialBase* GetDefaultMaterial() const;

		/// <summary>
		/// Begins new frame and enters commands collecting mode.
		/// </summary>
		virtual void FrameBegin();

		/// <summary>
		/// Ends the current frame rendering.
		/// </summary>
		virtual void FrameEnd();

		/// <summary>
		/// Begins the profile event.
		/// </summary>
		/// <param name="name">The name.</param>
		virtual void EventBegin(const Char* name)
		{
		}

		/// <summary>
		/// Ends the last profile event.
		/// </summary>
		virtual void EventEnd()
		{
		}

		/// <summary>
		/// Gets the native pointer to the underlying graphics device context. It's a low-level platform-specific handle.
		/// </summary>
		virtual void* GetNativePtr() const = 0;

		/// <summary>
		/// Determines whether depth buffer is binded to the pipeline.
		/// </summary>
		/// <returns><c>true</c> if  depth buffer is binded; otherwise, <c>false</c>.</returns>
		virtual bool IsDepthBufferBinded() = 0;

	public:
		/**
		 * 取消绑定的所有渲染目标，并将更改与驱动程序刷新（用于防止驱动程序检测到资源危害，例如，在缩小纹理时）。
		 */
		virtual void ResetRenderTarget() = 0;

		/// <summary>
		/// Sets the render target to the output.
		/// </summary>
		/// <param name="rt">The render target.</param>
		virtual void SetRenderTarget(GPUTextureView* rt) = 0;

		/**
		 * 将渲染目标和深度缓冲区设置为输出。
		 * @param depthBuffer
		 * @param rt
		 */
		virtual void SetRenderTarget(GPUTextureView* rt, GPUTextureView* depthBuffer) = 0;

		/**
		 * 设置输出的渲染目标和深度缓冲区。
		 * @param depthBuffer
		 * @param rts
		 */
		virtual void SetRenderTarget(const Span<GPUTextureView*>& rts, GPUTextureView* depthBuffer) = 0;

		/// <summary>
		/// Sets the blend factor that modulate values for a pixel shader, render target, or both.
		/// </summary>
		/// <param name="value">Blend factors, one for each RGBA component.</param>
		virtual void SetBlendFactor(const Float4& value) = 0;

		/// <summary>
		/// Sets the reference value for depth stencil tests.
		/// </summary>
		/// <param name="value">Reference value to perform against when doing a depth-stencil test.</param>
		virtual void SetStencilRef(uint32 value) = 0;
	public:
		/// <summary>
		/// Sets the rendering viewport and scissor rectangle.
		/// </summary>
		/// <param name="width">The width (in pixels).</param>
		/// <param name="height">The height (in pixels).</param>
		void SetViewportAndScissors(float width, float height);

		/// <summary>
		/// Sets the rendering viewport and scissor rectangle.
		/// </summary>
		/// <param name="viewport">The viewport (in pixels).</param>
		void SetViewportAndScissors(const Viewport& viewport);

		/// <summary>
		/// Sets the rendering viewport.
		/// </summary>
		/// <param name="width">The width (in pixels).</param>
		/// <param name="height">The height (in pixels).</param>
		void SetViewport(float width, float height);

		/// <summary>
		/// Sets the rendering viewport.
		/// </summary>
		/// <param name="viewport">The viewport (in pixels).</param>
		virtual void SetViewport(const Viewport& viewport) = 0;

		/// <summary>
		/// Sets the scissor rectangle.
		/// </summary>
		/// <param name="scissorRect">The scissor rectangle (in pixels).</param>
		virtual void SetScissor(const Rectangle& scissorRect) = 0;
	public:

		/**
		 * Executes a command list from a thread group.
		 * @param shader
		 * @param threadGroupCountX The number of groups dispatched in the x direction.
		 * @param threadGroupCountY The number of groups dispatched in the y direction.
		 * @param threadGroupCountZ The number of groups dispatched in the z direction.
		 */
		virtual void Dispatch(GPUShaderProgramCS* shader, uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ) = 0;

		/**
		 * Executes a command list from a thread group. Buffer must contain GPUDispatchIndirectArgs.
		 * @param shader The compute shader program to execute.
		 * @param bufferForArgs The buffer with drawing arguments.
		 * @param offsetForArgs The aligned byte offset for arguments.
		 */
		virtual void DispatchIndirect(GPUShaderProgramCS* shader, GPUBuffer* bufferForArgs, uint32 offsetForArgs) = 0;

		/**
		 * Resolves the multisampled texture by performing a copy of the resource into a non-multisampled resource.
		 * @param sourceMultisampleTexture The source multisampled texture. Must be multisampled.
		 * @param destTexture The destination texture. Must be single-sampled.
		 * @param sourceSubResource The source sub-resource index.
		 * @param destSubResource The destination sub-resource index.
		 * @param format The format. Indicates how the multisampled resource will be resolved to a single-sampled resource.
		 */
		virtual void ResolveMultisample(GPUTexture* sourceMultisampleTexture, GPUTexture* destTexture,
			int32 sourceSubResource = 0, int32 destSubResource = 0, PixelFormat format = PixelFormat::Undefined) = 0;

		/**
		 * Draws the fullscreen triangle (using single triangle). Use instance count parameter to render more than one instance of the triangle.
		 * @param instanceCount The instance count. Use SV_InstanceID in vertex shader to detect volume slice plane index.
		 */
		void DrawFullscreenTriangle();

		/**
		 * Draws the specified source texture to destination render target (using fullscreen triangle). Copies contents with resizing and format conversion support. Uses linear texture sampling.
		 * @param dst The destination texture.
		 * @param src The source texture.
		 */
		void Draw(GPUTexture* dst, GPUTexture* src);

		/**
		 * Draws the specified texture to render target (using fullscreen triangle). Copies contents with resizing and format conversion support. Uses linear texture sampling.
		 * @param rt The texture.
		 */
		void Draw(GPUTexture* rt);

		/**
		 * Draws the specified texture to render target (using fullscreen triangle). Copies contents with resizing and format conversion support. Uses linear texture sampling.
		 * @param rt The texture view.
		 */
		void Draw(GPUTextureView* rt);

		/**
		 * Draws non-indexed, non-instanced primitives.
		 * @param startVertex A value added to each index before reading a vertex from the vertex buffer.
		 * @param verticesCount The vertices count.
		 */
		inline void Draw(uint32 startVertex, uint32 verticesCount)
		{
			DrawInstanced(verticesCount, 1, 0, startVertex);
		}

		/**
		 * Draws the instanced primitives.
		 * @param verticesCount The vertices count.
		 * @param instanceCount Number of instances to draw.
		 * @param startInstance A value added to each index before reading per-instance data from a vertex buffer.
		 * @param startVertex A value added to each index before reading a vertex from the vertex buffer.
		 */
		virtual void DrawInstanced(uint32 verticesCount, uint32 instanceCount, int32 startInstance = 0, int32 startVertex = 0) = 0;

		/**
		 * Draws the indexed primitives.
		 * @param indicesCount The indices count.
		 * @param startVertex A value added to each index before reading a vertex from the vertex buffer.
		 * @param startIndex The location of the first index read by the GPU from the index buffer.
		 */
		inline void DrawIndexed(uint32 indicesCount, int32 startVertex = 0, int32 startIndex = 0)
		{
			DrawIndexedInstanced(indicesCount, 1, 0, startVertex, startIndex);
		}

		/**
		 * Draws the indexed, instanced primitives.
		 * @param indicesCount The indices count.
		 * @param instanceCount Number of instances to draw.
		 * @param startInstance A value added to each index before reading per-instance data from a vertex buffer.
		 * @param startVertex A value added to each index before reading a vertex from the vertex buffer.
		 * @param startIndex The location of the first index read by the GPU from the index buffer.
		 */
		virtual void DrawIndexedInstanced(uint32 indicesCount, uint32 instanceCount, int32 startInstance = 0, int32 startVertex = 0, int32 startIndex = 0) = 0;

		/**
		 * Draws the instanced GPU-generated primitives. Buffer must contain GPUDrawIndirectArgs.
		 * @param bufferForArgs The buffer with drawing arguments.
		 * @param offsetForArgs The aligned byte offset for arguments.
		 */
		virtual void DrawInstancedIndirect(GPUBuffer* bufferForArgs, uint32 offsetForArgs) = 0;

		/**
		 * Draws the instanced GPU-generated indexed primitives. Buffer must contain GPUDrawIndexedIndirectArgs.
		 * @param bufferForArgs The buffer with drawing arguments.
		 * @param offsetForArgs The aligned byte offset for arguments.
		 */
		virtual void DrawIndexedInstancedIndirect(GPUBuffer* bufferForArgs, uint32 offsetForArgs) = 0;

	public:
		/**
		 * 取消绑定的所有着色器资，并将更改与驱动程序刷新（用于防止驱动程序检测到资源危害，例如，在缩小纹理时）。
		 */
		virtual void ResetSR() = 0;

		/**
		 * 取消绑定的所有无序访问资，并将更改与驱动程序刷新（用于防止驱动程序检测到资源危害，例如，在缩小纹理时）。
		 */
		virtual void ResetUA() = 0;

		/**
		 * 取消绑定的所有常量缓冲区，并使用驱动程序刷新更改（用于防止驱动程序检测到资源危害，例如，在缩小纹理时）。
		 */
		virtual void ResetCB() = 0;

		/**
		 * 解除指定位置绑定的shader resource
		 * @param slot 位置
		 */
		inline void UnBindSR(int32 slot)
		{
			BindSR(slot, static_cast<GPUResourceView*>(nullptr));
		}

		/**
		 * 解除指定位置绑定的unordered access resource
		 * @param slot 位置
		 */
		inline void UnBindUA(int32 slot)
		{
			BindUA(slot, static_cast<GPUResourceView*>(nullptr));
		}

		/**
		 * 解除指定位置绑定的constant buffer
		 * @param slot 位置
		 */
		inline void UnBindCB(int32 slot)
		{
			BindCB(slot, nullptr);
		}

		/**
		 * 将texture作为shader资源绑定到指定位置
		 * @param slot 绑定位置
		 * @param texture 纹理
		 */
		void BindSR(int32 slot, GPUTexture* texture);

		/**
		 * 将resource view作为shader资源绑定到指定的位置
		 * @param slot 绑定位置
		 * @param view 资源view
		 */
		virtual void BindSR(int32 slot, GPUResourceView* view) = 0;

		/**
		 * 将resource view作为无序访问资源绑定到指定的位置
		 * @param slot 绑定位置
		 * @param view 资源view
		 */
		virtual void BindUA(int32 slot, GPUResourceView* view) = 0;

		/**
		 * 绑定constant buffer到指定绑定点
		 * @param slot 绑定位置
		 * @param cb constant buffer
		 */
		virtual void BindCB(int32 slot, GPUConstantBuffer* cb) = 0;


		/**
		 * 绑定多个顶点buffer
		 * @param vertexBuffers 顶点buffer数组
		 * @param vertexBuffersOffsets 从顶点缓冲区开始的字节偏移数组。当对多个几何对象重用相同的缓冲区分配时，可用于偏移顶点数据。
		 */
		virtual void BindVB(const Span<GPUBuffer*>& vertexBuffers, const uint32* vertexBuffersOffsets = nullptr) = 0;

		/**
		 * 绑定顶点buffer
		 * @param vertexBuffers 顶点buffer
		 * @param vertexBuffersOffsets 从顶点缓冲区开始的字节偏移数组。当对多个几何对象重用相同的缓冲区分配时，可用于偏移顶点数据。
		 */
		virtual void BindVB(GPUBuffer* vertexBuffers, const uint32 vertexBuffersOffsets = 0) = 0;

		/**
		 * 绑定顶点buffer
		 * @param indexBuffer 顶点buffer
		 */
		virtual void BindIB(GPUBuffer* indexBuffer) = 0;

		/**
		 * 绑定采样器到指定位置
		 * @param slot 绑定位置
		 * @param sampler 采样器
		 */
		virtual void BindSampler(int32 slot, GPUSampler* sampler) = 0;
	public:
		/// <summary>
		/// Clears texture surface with a color. Supports volumetric textures and texture arrays (including cube textures).
		/// </summary>
		/// <param name="rt">The target surface.</param>
		/// <param name="color">The clear color.</param>
		virtual void Clear(GPUTextureView* rt, const Color& color) = 0;

		/// <summary>
		/// Clears depth buffer.
		/// </summary>
		/// <param name="depthBuffer">The depth buffer to clear.</param>
		/// <param name="depthValue">The clear depth value.</param>
		virtual void ClearDepth(GPUTextureView* depthBuffer, float depthValue = 1.0f) = 0;

		/// <summary>
		/// Clears an unordered access buffer with a float value.
		/// </summary>
		/// <param name="buf">The buffer to clear.</param>
		/// <param name="value">The clear value.</param>
		virtual void ClearUA(GPUBuffer* buf, const Float4& value) = 0;

		/// <summary>
		/// Clears an unordered access buffer with a unsigned value.
		/// </summary>
		/// <param name="buf">The buffer to clear.</param>
		/// <param name="value">The clear value.</param>
		virtual void ClearUA(GPUBuffer* buf, const uint32 value[4]) = 0;

		/// <summary>
		/// Clears an unordered access texture with a unsigned value.
		/// </summary>
		/// <param name="texture">The texture to clear.</param>
		/// <param name="value">The clear value.</param>
		virtual void ClearUA(GPUTexture* texture, const uint32 value[4]) = 0;

		/// <summary>
		/// Clears an unordered access texture with a float value.
		/// </summary>
		/// <param name="texture">The texture to clear.</param>
		/// <param name="value">The clear value.</param>
		virtual void ClearUA(GPUTexture* texture, const Float4& value) = 0;

		/// <summary>
		/// Sets the graphics pipeline state.
		/// </summary>
		/// <param name="state">The state to bind.</param>
		virtual void SetState(GPUPipelineState* state) = 0;

		/// <summary>
		/// Gets the current pipeline state binded to the graphics pipeline.
		/// </summary>
		/// <returns>The current state.</returns>
		virtual GPUPipelineState* GetState() const = 0;

		/// <summary>
		/// Clears the context state.
		/// </summary>
		virtual void ClearState() = 0;

		/// <summary>
		/// Flushes the internal cached context state with a command buffer.
		/// </summary>
		virtual void FlushState() = 0;

		/// <summary>
		/// Flushes the command buffer (calls GPU execution).
		/// </summary>
		virtual void Flush() = 0;

	public:
		/// <summary>
		/// Updates the buffer data.
		/// </summary>
		/// <param name="buffer">The destination buffer to write to.</param>
		/// <param name="data">The pointer to the data.</param>
		/// <param name="size">The data size (in bytes) to write.</param>
		/// <param name="offset">The offset (in bytes) from the buffer start to copy data to.</param>
		virtual void UpdateBuffer(GPUBuffer* buffer, const void* data, uint32 size, uint32 offset = 0) = 0;

		/// <summary>
		/// Copies the buffer data.
		/// </summary>
		/// <param name="dstBuffer">The destination buffer to write to.</param>
		/// <param name="srcBuffer">The source buffer to read from.</param>
		/// <param name="size">The size of data to copy (in bytes).</param>
		/// <param name="dstOffset">The offset (in bytes) from the destination buffer start to copy data to.</param>
		/// <param name="srcOffset">The offset (in bytes) from the source buffer start to copy data from.</param>
		virtual void CopyBuffer(GPUBuffer* dstBuffer, GPUBuffer* srcBuffer, uint32 size, uint32 dstOffset = 0, uint32 srcOffset = 0) = 0;

		/// <summary>
		/// Updates the texture data.
		/// </summary>
		/// <param name="texture">The destination texture.</param>
		/// <param name="arrayIndex">The destination surface index in the texture array.</param>
		/// <param name="mipIndex">The absolute index of the mip map to update.</param>
		/// <param name="data">The pointer to the data.</param>
		/// <param name="rowPitch">The row pitch (in bytes) of the input data.</param>
		/// <param name="slicePitch">The slice pitch (in bytes) of the input data.</param>
		virtual void UpdateTexture(GPUTexture* texture, int32 arrayIndex, int32 mipIndex, const void* data, uint32 rowPitch, uint32 slicePitch) = 0;

		/// <summary>
		/// Copies region of the texture.
		/// </summary>
		/// <param name="dstResource">The destination resource.</param>
		/// <param name="dstSubresource">The destination subresource index.</param>
		/// <param name="dstX">The x-coordinate of the upper left corner of the destination region.</param>
		/// <param name="dstY">The y-coordinate of the upper left corner of the destination region.</param>
		/// <param name="dstZ">The z-coordinate of the upper left corner of the destination region.</param>
		/// <param name="srcResource">The source resource.</param>
		/// <param name="srcSubresource">The source subresource index.</param>
		virtual void CopyTexture(GPUTexture* dstResource, uint32 dstSubresource, uint32 dstX, uint32 dstY, uint32 dstZ, GPUTexture* srcResource, uint32 srcSubresource) = 0;

		/// <summary>
		/// Resets the counter buffer to zero (hidden by the driver).
		/// </summary>
		/// <param name="buffer">The buffer.</param>
		virtual void ResetCounter(GPUBuffer* buffer) = 0;

		/// <summary>
		/// Copies the counter buffer value.
		/// </summary>
		/// <param name="dstBuffer">The destination buffer.</param>
		/// <param name="dstOffset">The destination aligned byte offset.</param>
		/// <param name="srcBuffer">The source buffer.</param>
		virtual void CopyCounter(GPUBuffer* dstBuffer, uint32 dstOffset, GPUBuffer* srcBuffer) = 0;

		/// <summary>
		/// Copies the resource data (whole resource).
		/// </summary>
		/// <param name="dstResource">The destination resource.</param>
		/// <param name="srcResource">The source resource.</param>
		virtual void CopyResource(GPUResource* dstResource, GPUResource* srcResource) = 0;

		/// <summary>
		/// Copies the subresource data.
		/// </summary>
		/// <param name="dstResource">The destination resource.</param>
		/// <param name="dstSubresource">The destination subresource index.</param>
		/// <param name="srcResource">The source resource.</param>
		/// <param name="srcSubresource">The source subresource index.</param>
		virtual void CopySubresource(GPUResource* dstResource, uint32 dstSubresource, GPUResource* srcResource, uint32 srcSubresource) = 0;

		/// <summary>
		/// Updates the constant buffer data.
		/// </summary>
		/// <param name="cb">The constant buffer.</param>
		/// <param name="data">The pointer to the data.</param>
		virtual void UpdateCB(GPUConstantBuffer* cb, const void* data) = 0;
	public:
		
	};
}

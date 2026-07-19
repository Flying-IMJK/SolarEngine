#pragma once

#include "Runtime/Core/Types/Variable.h"
#include "Runtime/Core/Math/Vector2.h"
#include "Runtime/Core/Platform/Window.h"
#include "Runtime/Graphics/Base/PixelFormat.h"
#include "Base/GPUResource.h"

namespace SE
{
	class GPUContext;
	class GPUTexture;
	class GPUTextureView;
	class Task;
	class RenderTask;

	/// <summary>
	/// GPU swap chain object that provides rendering to native window backbuffer.
	/// </summary>
	class SE_API_RUNTIME GPUSwapChain : public GPUResource
	{
	protected:
		uint32 _width = 0;
		uint32 _height = 0;
		uint64 _presentCount = 0;
		PixelFormat _format = PixelFormat::Undefined;
		Window* _window = nullptr;
		Task* _downloadTask = nullptr;

		GPUSwapChain();

	public:
		/// <summary>
		/// Gets the linked window.
		/// </summary>
		Window* GetWindow() const
		{
			return _window;
		}

		/// <summary>
		/// The output backbuffer width (in pixels).
		/// </summary>
		inline uint32 GetWidth() const
		{
			return _width;
		}

		/// <summary>
		/// The output backbuffer height (in pixels).
		/// </summary>
		inline uint32 GetHeight() const
		{
			return _height;
		}

		/// <summary>
		/// The output backbuffer surface format.
		/// </summary>
		inline PixelFormat GetFormat() const
		{
			return _format;
		}

		/// <summary>
		/// The output backbuffer width and height (in pixels).
		/// </summary>
		inline Float2 GetSize() const
		{
			return Float2(static_cast<float>(_width), static_cast<float>(_height));
		}

		/// <summary>
		/// The output backbuffer aspect ratio.
		/// </summary>
		inline float GetAspectRatio() const
		{
			return static_cast<float>(_width) / _height;
		}

		/// <summary>
		/// Gets amount of backbuffer swaps.
		/// </summary>
		inline uint64 GetPresentCount() const
		{
			return _presentCount;
		}

		/// <summary>
		/// True if running in fullscreen mode.
		/// </summary>
		/// <returns>True if is in fullscreen mode, otherwise false</returns>
		virtual bool IsFullscreen() = 0;

		/// <summary>
		/// Set the fullscreen state.
		/// </summary>
		/// <param name="isFullscreen">Fullscreen mode to apply</param>
		virtual void SetFullscreen(bool isFullscreen) = 0;

		/// <summary>
		/// Gets the view for the output back buffer texture (for the current frame rendering).
		/// </summary>
		/// <returns>The output texture view to use.</returns>
		virtual GPUTextureView* GetBackBufferView() = 0;

		/// <summary>
		/// Copies the backbuffer contents to the destination texture.
		/// </summary>
		/// <param name="context">The GPU commands context.</param>
		/// <param name="dst">The destination texture. It must match the output dimensions and format. No staging texture support.</param>
		virtual void CopyBackBuffer(GPUContext* context, GPUTexture* dst) = 0;

		/// <summary>
		/// Checks if task is ready to render.
		/// </summary>
		/// <returns>True if is ready, otherwise false</returns>
		virtual bool IsReady() const
		{
			// Skip rendering for the hidden windows
			return GetWidth() > 0 && (_window != nullptr && (_window->IsVisible() || _window->m_ShowAfterFirstPaint));
		}

	public:
		/// <summary>
		/// Creates GPU async task that will gather render target data from the GPU.
		/// </summary>
		/// <param name="result">Result data</param>
		/// <returns>Download data task (not started yet)</returns>
		virtual Task* DownloadDataAsync(TextureData& result);

	public:
		/// <summary>
		/// Begin task rendering.
		/// </summary>
		/// <param name="task">Active task</param>
		virtual void Begin()
		{
		}

		/// <summary>
		/// End task rendering.
		/// </summary>
		/// <param name="task">Active task</param>
		virtual void End();

		/// <summary>
		/// Present back buffer to the output.
		/// </summary>
		/// <param name="vsync">True if use vertical synchronization to lock frame rate.</param>
		virtual void Present(bool vsync);

		/// <summary>
		/// Resize output back buffer.
		/// </summary>
		/// <param name="width">New output width (in pixels).</param>
		/// <param name="height">New output height (in pixels).</param>
		/// <returns>True if cannot resize the buffers, otherwise false.</returns>
		virtual bool Resize(uint32 width, uint32 height) = 0;

	public:
		// [GPUResource]
		String ToString() const override;
		GPUResourceType GetResType() const final override;
		void OnDeleteObject() override;

	protected:
		void OnReleaseGPU() override;

	public:

	};
}

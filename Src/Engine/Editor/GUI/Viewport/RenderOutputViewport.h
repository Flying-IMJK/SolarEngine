#pragma once

#include "Runtime/Graphics/Base/PixelFormat.h"
#include "Runtime/UI/GUI/ContainerControl.h"

namespace SE
{
	class GPUContext;
	class RenderTask;
	class SceneRenderTask;
    class GPUTexture;
}

namespace SE::Editor
{
	class RenderOutputViewport : public ContainerControl
	{
	protected:
	    /// <summary>
	    /// The task.
	    /// </summary>
	    SceneRenderTask* m_Task;

	    /// <summary>
	    /// The back buffer.
	    /// </summary>
	    GPUTexture* m_BackBuffer;

	private:
	    GPUTexture* m_BackBufferOld;
	    int _oldBackbufferLiveTimeLeft;
	    float m_ResizeTime;
	    Int2 _customResolution = Int2(0, 0);

	public:
        /// <summary>
        /// The default back buffer format used by the GUI controls presenting rendered frames.
        /// </summary>
	    static constexpr PixelFormat BackBufferFormat = PixelFormat::R8G8B8A8_UNorm;

        /// <summary>
        /// The resize check timeout (in seconds).
        /// </summary>
	    static constexpr float ResizeCheckTime = 0.9f;

        /// <summary>
        /// Gets the task.
        /// </summary>
	    SceneRenderTask* GetTask();

        /// <summary>
        /// Gets or sets a value indicating whether render to that output only if parent window exists, otherwise false.
        /// </summary>
	    bool RenderOnlyWithWindow = true;

        /// <summary>
        /// Gets or sets a value indicating whether use automatic task rendering skipping if output is too small or window is missing. Disable it to manually control <see cref="RenderTask.Enabled"/>.
        /// </summary>
	    bool UseAutomaticTaskManagement = true;

        /// <summary>
        /// Gets a value indicating whether keep aspect ratio of the backbuffer image, otherwise false.
        /// </summary>
	    bool KeepAspectRatio = false;

        /// <summary>
        /// Gets or sets the color of the tint used to color the backbuffer of the render output.
        /// </summary>
	    Color TintColor = Colors::White;

        /// <summary>
        /// Gets or sets the brightness of the output.
        /// </summary>
	    float Brightness = 1.0f;

        /// <summary>
        /// Gets or sets the rendering resolution scale. Can be used to upscale image or to downscale the rendering to save the performance.
        /// </summary>
	    float ResolutionScale = 1.0f;

        /// <summary>
        /// Gets or sets the custom resolution to use for the rendering.
        /// </summary>
        PRO(CustomResolution, RenderOutputViewport, Int2, __GetCustomResolution, __SetCustomResolution);

        /// <summary>
        /// Initializes a new instance of the <see cref="RenderOutputControl"/> class.
        /// </summary>
        /// <param name="task">The task. Cannot be null.</param>
        /// <exception cref="System.ArgumentNullException">Invalid task.</exception>
	    RenderOutputViewport(SceneRenderTask* task);

        /// <inheritdoc />
	    void Draw() override;

        /// <summary>
        /// Synchronizes size of the back buffer with the size of the control.
        /// </summary>
	    void SyncBackbufferSize();

        /// <inheritdoc />
		void OnDestroy() override;

	private:
		bool WalkTree(Control* c);

		void OnUpdate();

	protected:
        /// <summary>
        /// Performs a check if rendering a current frame can be skipped (if control size is too small, has missing data, etc.).
        /// </summary>
        /// <returns>True if skip rendering, otherwise false.</returns>
		virtual bool CanSkipRendering();

        /// <summary>
        /// Called when ask rendering ends.
        /// </summary>
        /// <param name="task">The task.</param>
        /// <param name="context">The GPU execution context.</param>
		virtual void OnEnd(RenderTask* task, GPUContext* context);

	private:
		Int2 __GetCustomResolution();
		void __SetCustomResolution(Int2 value);
	};

} // SE


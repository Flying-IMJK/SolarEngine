#pragma once

#include "RenderContext.h"
#include "RenderView.h"
#include "SceneRendering.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Types/Collections/List.h"
#include "Core/Types/Object.h"
#include "Core/Types/Delegate.h"

#include "Runtime/API.h"
#include "Runtime/Graphics/Viewport.h"

namespace SE
{
	struct RenderContext;
	class RenderBuffers;
	class GPUDevice;
	class GPUContext;
	class GPUTexture;
	class GPUTextureView;
	class GPUSwapChain;

	class SE_API_RUNTIME RenderTask : public Object
	{
	public:
		/// <summary>
		/// List with all registered tasks
		/// </summary>
		static List<RenderTask*> Tasks;

		/// <summary>
		/// Static locker for render tasks list
		/// </summary>
		static CriticalSection TasksLocker;

		/// <summary>
		/// Amount of tasks rendered during last frame
		/// </summary>
		static int32 TasksDoneLastFrame;

		/// <summary>
		/// The current task being executed.
		/// </summary>
		static RenderTask* CurrentTask;

		/// <summary>
		/// Draws all tasks. Called only during rendering by the graphics device.
		/// </summary>
		static void RenderAll();

	private:
		RenderTask* _prevTask = nullptr;

	public:
		RenderTask();

		/// <summary>
		/// Finalizes an instance of the <see cref="RenderTask"/> class.
		/// </summary>
		~RenderTask() override;

	public:
		/// <summary>
		/// Gets or sets a value indicating whether task is enabled.
		/// </summary>
		bool Enabled = true;

		/// <summary>
		/// The order of the task. Used for tasks rendering order. Lower first, higher later.
		/// </summary>
		int32 Order = 0;

		/// <summary>
		/// The amount of frames rendered by this task. It is auto incremented on task drawing.
		/// </summary>
		int32 FrameCount = 0;

		/// <summary>
		/// The output window swap chain. Optional, used only when rendering to native window backbuffer.
		/// </summary>
		GPUSwapChain* SwapChain = nullptr;

		/// <summary>
		/// The index of the frame when this task was last time rendered.
		/// </summary>
		uint64 LastUsedFrame = 0;

		/// <summary>
		/// Action fired on task rendering.
		/// </summary>
		Delegate<RenderTask*, GPUContext*> Render;

		/// <summary>
		/// Action fired on task rendering begin.
		/// </summary>
		Delegate<RenderTask*, GPUContext*> Begin;

		/// <summary>
		/// Action fired on task rendering end.
		/// </summary>
		Delegate<RenderTask*, GPUContext*> End;

		/// <summary>
		/// Action fired just after frame present.
		/// </summary>
		Delegate<RenderTask*> Present;

		/// <summary>
		/// Determines whether this task can be rendered.
		/// </summary>
		/// <returns><c>true</c> if this task can be rendered; otherwise, <c>false</c>.</returns>
		virtual bool CanDraw() const;

		/// <summary>
		/// Called by graphics device to draw this task. Can be used to invoke task rendering nested inside another task - use on own risk!
		/// </summary>
		virtual void OnRender();

		/// <summary>
		/// Called on task rendering begin.
		/// </summary>
		/// <param name="context">The GPU context.</param>
		virtual void OnBegin(GPUContext* context);

		/// <summary>
		/// Called on task rendering.
		/// </summary>
		/// <param name="context">The GPU context.</param>
		virtual void OnRender(GPUContext* context);

		/// <summary>
		/// Called on task rendering end.
		/// </summary>
		/// <param name="context">The GPU context.</param>
		virtual void OnEnd(GPUContext* context);

		/// <summary>
		/// Presents frame to the output.
		/// </summary>
		/// <param name="vsync">True if use vertical synchronization to lock frame rate.</param>
		virtual void OnPresent(bool vsync);

		/// <summary>
		/// Changes the buffers and output size. Does nothing if size won't change. Called by window or user to resize rendering buffers.
		/// </summary>
		/// <param name="width">The width.</param>
		/// <param name="height">The height.</param>
		/// <returns>True if cannot resize the buffers.</returns>
		virtual bool Resize(int32 width, int32 height);

	public:

		String ToString() const override;

		bool operator<(const RenderTask& other) const
		{
			return Order < other.Order;
		}
	};

	/// <summary>
	/// Defines actors to draw sources.
	/// </summary>
	enum class ActorsSources
	{
		/// <summary>
		/// The actors won't be rendered.
		/// </summary>
		None = 0,

		/// <summary>
		/// The actors from the loaded scenes.
		/// </summary>
		Scenes = 1,

		/// <summary>
		/// The actors from the custom collection.
		/// </summary>
		CustomActors = 2,

		/// <summary>
		/// The scenes from the custom collection.
		/// </summary>
		CustomScenes = 4,

		/// <summary>
		/// The actors from the loaded scenes and custom collection.
		/// </summary>
		ScenesAndCustomActors = Scenes | CustomActors,
	};

	typedef EnumFlags<ActorsSources> ActorsSourcesFlags;

	/// <summary>
	/// The Post Process effect rendering location within the rendering pipeline.
	/// </summary>
	enum class RenderingUpscaleLocation
	{
		/// <summary>
		/// The up-scaling happens directly to the output buffer (backbuffer) after post processing and anti-aliasing.
		/// </summary>
		AfterAntiAliasingPass = 0,

		/// <summary>
			/// The up-scaling happens before the post processing after scene rendering (after geometry, lighting, volumetrics, transparency and SSR/SSAO).
			/// </summary>
		BeforePostProcessingPass = 1,
	};

	/// <summary>
	/// Render task which draws scene actors into the output buffer.
	/// </summary>
	/// <seealso cref="FlaxEngine.RenderTask" />
	class SE_API_RUNTIME SceneRenderTask : public RenderTask
	{
	protected:
		// class SceneRendering* _customActorsScene = nullptr;

	public:
		SceneRenderTask();

		/// <summary>
		/// Finalizes an instance of the <see cref="SceneRenderTask"/> class.
		/// </summary>
		~SceneRenderTask() override;

	public:
		/// <summary>
		/// True if the current frame is after the camera cut. Used to clear the temporal effects history and prevent visual artifacts blended from the previous frames.
		/// </summary>
		bool IsCameraCut = true;

		/// <summary>
		/// True if the task is used for custom scene rendering and default scene drawing into output should be skipped. Enable it if you use Render event and draw scene manually.
		/// </summary>
		bool IsCustomRendering = false;

		/// <summary>
		/// Marks the next rendered frame as camera cut. Used to clear the temporal effects history and prevent visual artifacts blended from the previous frames.
		/// </summary>
		void CameraCut();

		/// <summary>
		/// The output texture (can be null if using rendering to window swap chain). Can be used to redirect the default scene rendering output to a texture.
		/// </summary>
		GPUTexture* Output = nullptr;

		/// <summary>
		/// The scene rendering buffers. Created and managed by the task.
		/// </summary>
		RenderBuffers* Buffers = nullptr;

		/// <summary>
		/// The scene rendering camera. Can be used to override the rendering view properties based on the current camera setup.
		/// </summary>
		// ScriptingObjectReference<Camera> Camera;

		/// <summary>
		/// The render view description.
		/// </summary>
		RenderView View;

		/// <summary>
		/// The actors source to use (configures what objects to render).
		/// </summary>
		ActorsSourcesFlags ActorsSource = ActorsSources::Scenes;

		/// <summary>
		/// The scale of the rendering resolution relative to the output dimensions. If lower than 1 the scene and postprocessing will be rendered at a lower resolution and upscaled to the output backbuffer.
		/// </summary>
		float RenderingPercentage = 1.0f;

		/// <summary>
		/// The image resolution upscale location within rendering pipeline. Unused if RenderingPercentage is 1.
		/// </summary>
		// RenderingUpscaleLocation UpscaleLocation = RenderingUpscaleLocation::AfterAntiAliasingPass;

	public:
		/*
		/// <summary>
		/// The custom set of actors to render. Used when ActorsSources::CustomActors flag is active.
		/// </summary>
		List<Actor*> CustomActors;

		/// <summary>
		/// The custom set of scenes to render. Used when ActorsSources::CustomScenes flag is active.
		/// </summary>
		List<Scene*> CustomScenes;

		/// <summary>
		/// Adds the custom actor to the rendering.
		/// </summary>
		/// <param name="actor">The actor.</param>
		void AddCustomActor(Actor* actor);

		/// <summary>
		/// Removes the custom actor from the rendering.
		/// </summary>
		/// <param name="actor">The actor.</param>
		void RemoveCustomActor(Actor* actor);

		/// <summary>
		/// Removes all the custom actors from the rendering.
		/// </summary>
		void ClearCustomActors();
		*/

	public:
/*		/// <summary>
		/// The custom set of postfx to render.
		/// </summary>
		List<PostProcessEffect*> CustomPostFx;

		/// <summary>
		/// Adds the custom postfx to the rendering.
		/// </summary>
		/// <param name="fx">The postfx script.</param>
		void AddCustomPostFx(PostProcessEffect* fx);

		/// <summary>
		/// Removes the custom postfx from the rendering.
		/// </summary>
		/// <param name="fx">The postfx script.</param>
		void RemoveCustomPostFx(PostProcessEffect* fx);

		/// <summary>
		/// True if allow using global custom PostFx when rendering this task.
		/// </summary>
		bool AllowGlobalCustomPostFx = true;

		/// <summary>
		/// The custom set of global postfx to render for all <see cref="SceneRenderTask"/> (applied to tasks that have <see cref="AllowGlobalCustomPostFx"/> turned on).
		/// </summary>
		static List<PostProcessEffect*> GlobalCustomPostFx;

		/// <summary>
		/// Adds the custom global postfx to the rendering.
		/// </summary>
		/// <param name="fx">The postfx script.</param>
		static void AddGlobalCustomPostFx(PostProcessEffect* fx);

		/// <summary>
		/// Removes the custom global postfx from the rendering.
		/// </summary>
		/// <param name="fx">The postfx script.</param>
		static void RemoveGlobalCustomPostFx(PostProcessEffect* fx);*/

	public:
		/// <summary>
		/// The action called on view rendering to collect draw calls. It allows to extend rendering pipeline and draw custom geometry non-existing in the scene or custom actors set.
		/// </summary>
		Delegate<RenderContext&> CollectDrawCalls;

		/// <summary>
		/// Calls collecting postFx volumes for rendering.
		/// </summary>
		/// <param name="renderContext">The rendering context.</param>
		virtual void CollectPostFxVolumes(RenderContext& renderContext);

		/// <summary>
		/// Calls drawing scene objects.
		/// </summary>
		/// <param name="renderContextBatch">The rendering context batch.</param>
		/// <param name="category">The actors category to draw (see SceneRendering::DrawCategory).</param>
		virtual void OnCollectDrawCalls(RenderContextBatch& renderContextBatch, SceneRendering::DrawCategory category = SceneRendering::DrawCategory::SceneDraw);

		/// <summary>
		/// The action called after scene rendering. Can be used to perform custom pre-rendering or to modify the render view.
		/// </summary>
		Delegate<RenderContext&> PreRender;

		/// <summary>
		/// Called before scene rendering. Can be used to perform custom pre-rendering or to modify the render view.
		/// </summary>
		/// <param name="context">The GPU commands context.</param>
		/// <param name="renderContext">The rendering context.</param>
		virtual void OnPreRender(RenderContext& renderContext);

		/// <summary>
		/// The action called after scene rendering. Can be used to render additional visual elements to the output.
		/// </summary>
		Delegate<RenderContext&> PostRender;

		/// <summary>
		/// Called after scene rendering. Can be used to render additional visual elements to the output.
		/// </summary>
		/// <param name="context">The GPU commands context.</param>
		/// <param name="renderContext">The rendering context.</param>
		virtual void OnPostRender(RenderContext& renderContext);

		/// <summary>
		/// The action called before any rendering to override/customize setup RenderSetup inside RenderList. Can be used to enable eg. Motion Vectors rendering.
		/// </summary>
		Delegate<RenderContext&> SetupRender;

	public:
		/// <summary>
		/// Gets the rendering render task viewport (before upsampling).
		/// </summary>
		Viewport GetViewport() const;

		/// <summary>
		/// Gets the rendering output viewport (after upsampling).
		/// </summary>
		Viewport GetOutputViewport() const;

		/// <summary>
		/// Gets the rendering output view.
		/// </summary>
		GPUTextureView* GetOutputView() const;

	public:
		// [RenderTask]
		bool Resize(int32 width, int32 height) override;
		bool CanDraw() const override;
		void OnBegin(GPUContext* context) override;
		void OnRender(GPUContext* context) override;
		void OnEnd(GPUContext* context) override;
	};

	class SE_API_RUNTIME MainRenderTask final : public SceneRenderTask
	{
	public:
		MainRenderTask();
		/// <summary>
		/// Finalizes an instance of the <see cref="MainRenderTask"/> class.
		/// </summary>
		~MainRenderTask() override;

	public:
		/// <summary>
		/// Gets the main game rendering task. Use it to plug custom rendering logic for your game.
		/// </summary>
		static MainRenderTask* Instance;

	public:
		// [SceneRenderTask]
		void OnBegin(GPUContext* context) override;
	};

} // SE

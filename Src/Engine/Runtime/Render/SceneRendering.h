#pragma once

#include "RenderContext.h"
#include "Core/Math/BoundingVolumes.h"
#include "Core/Platform/Win32/Win32CriticalSection.h"
#include "Core/Types/Delegate.h"

namespace SE
{
	class SE_API_RUNTIME IRender
	{
	public:
		virtual ~IRender() = default;
		int virtual RenderGetDrawCategory() = 0;
		int32 virtual RenderGetLayerMask() = 0;
		BoundingSphere virtual RenderGetSphere()  = 0;
		EnumFlags<StaticMask> virtual RenderGetStaticFlags() = 0;

		/// <summary>
		/// Draws this actor. Called by Scene Rendering service. This call is more optimized than generic Draw (eg. geometry is rendered during all pass types but other actors are drawn only during GBufferFill pass).
		/// </summary>
		/// <param name="context">The rendering context.</param>
		void virtual RenderDraw(RenderContext& context) { };

		/// <summary>
		/// Draws this actor. Called by Scene Rendering service. This call is more optimized than generic Draw (eg. geometry is rendered during all pass types but other actors are drawn only during GBufferFill pass).
		/// </summary>
		/// <param name="contextBatch">The rendering context batch (eg, main view and shadow projections).</param>
		void virtual RenderDraw(RenderContextBatch& contextBatch) {};
	};

	/// <summary>
	/// Interface for objects to plug into Scene Rendering and listen for its evens such as static actors changes which are relevant for drawing cache.
	/// </summary>
	/// <seealso cref="SceneRendering"/>
	class SE_API_RUNTIME ISceneRenderingListener
	{
		friend class SceneRendering;
	private:
		List<SceneRendering*, InlinedAllocation<8>> _scenes;
	public:
		virtual ~ISceneRenderingListener();

		// Starts listening to the scene rendering events.
		void ListenSceneRendering(SceneRendering* scene);

		// Events called by Scene Rendering
		virtual void OnSceneRenderingAddActor(IRender* a) = 0;
		virtual void OnSceneRenderingUpdateActor(IRender* a, const BoundingSphere& prevBounds) = 0;
		virtual void OnSceneRenderingRemoveActor(IRender* a) = 0;
		virtual void OnSceneRenderingClear(SceneRendering* scene) = 0;
	};

	class SceneRendering
	{
	public:
		struct RenderDraw
		{
			IRender* render;
			uint32 LayerMask;
			int8 NoCulling : 1;
			BoundingSphere Bounds;
		};

		/// <summary>
		/// Drawing categories for separate draw stages.
		/// </summary>
		enum DrawCategory
		{
			SceneDraw = 0,
			SceneDrawAsync,
			PreRender,
			PostRender,
			MAX
		};

		List<RenderDraw> Actors[MAX];
		// List<IPostFxSettingsProvider*> PostFxProviders;
		CriticalSection Locker;
	public:
		/// <summary>
		/// Draws the scene. Performs the optimized actors culling and draw calls submission for the current render pass (defined by the render view).
		/// </summary>
		/// <param name="renderContextBatch">The rendering context batch.</param>
		/// <param name="category">The actors category to draw.</param>
		void Draw(RenderContextBatch& renderContextBatch, DrawCategory category = SceneDraw);

		/// <summary>
		/// Clears this instance data.
		/// </summary>
		void Clear();

		void AddRender(IRender* render, int32& key);
		void UpdateRender(IRender* render, int32& key);
		void RemoveRender(IRender* render, int32& key);

	private:

		// Listener - some rendering systems cache state of the scene (eg. in RenderBuffers::CustomBuffer), this extensions allows those systems to invalidate cache and handle scene changes
		friend class ISceneRenderingListener;
		List<ISceneRenderingListener*, InlinedAllocation<8>> _listeners;

		List<BoundingFrustum> m_DrawFrustumsData;
		RenderDraw* m_DrawListData = nullptr;
		int64 m_DrawListSize = 0;
		volatile int64 m_DrawListIndex = 0;
		RenderContextBatch* m_DrawBatch = nullptr;

		void DrawActorsJob(int32 index);
	};
} // SE

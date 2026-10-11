#pragma once

#include "RenderEnum.h"
#include "Runtime/Core/Types/Variable.h"
#include "Runtime/Core/Math/Matrix.h"
#include "Runtime/Core/Math/BoundingVolumes.h"
#include "Runtime/Core/Types/BitFlags.h"

#include "Runtime/API.h"
#include "Utils/LayersMask.h"

namespace SE
{
	class Camera;
	struct Viewport;
	struct RenderContext;

	/// <summary>
	/// Rendering view description that defines how to render the objects (camera placement, rendering properties, etc.).
	/// </summary>
	SE_STRUCT(API())
	struct SE_API_RUNTIME RenderView
	{
		SCRIPTING_TYPE_MIN(RenderView);
		/// <summary>
		/// The position of the view origin (in world-units). Used for camera-relative rendering to achieve large worlds support with keeping 32-bit precision for coordinates in scene rendering.
		/// </summary>
		SE_FIELD(API())
		Float3 Origin = Float3::Zero;

		/// <summary>
		/// The global position of the view (Origin+Position).
		/// </summary>
		SE_FIELD(API())
		Float3 WorldPosition;

		/// <summary>
		/// The position of the view (relative to the origin).
		/// </summary>
		SE_FIELD(API())
		Float3 Position;

		/// <summary>
		/// The far plane.
		/// </summary>
		SE_FIELD(API())
		float Far = 10000.0f;

		/// <summary>
		/// The direction of the view.
		/// </summary>
		SE_FIELD(API())
		Float3 Direction;

		/// <summary>
		/// The near plane.
		/// </summary>
		SE_FIELD(API())
		float Near = 0.1f;

		/// <summary>
		/// The view matrix.
		/// </summary>
		SE_FIELD(API())
		Matrix View;

		/// <summary>
		/// The projection matrix.
		/// </summary>
		SE_FIELD(API())
		Matrix Projection;

		/// <summary>
		/// The projection matrix with no camera offset (no jittering). 
		/// For many temporal image effects, the camera that is currently rendering needs to be slightly offset from the default projection (that is, the camera is ‘jittered’). 
		/// If you use motion vectors and camera jittering together, use this property to keep the motion vectors stable between frames.
		/// </summary>
		SE_FIELD(API())
		Matrix NonJitteredProjection;

		/// <summary>
		/// The inverted view matrix.
		/// </summary>
		SE_FIELD(API())
		Matrix IV;

		/// <summary>
		/// The inverted projection matrix.
		/// </summary>
		SE_FIELD(API())
		Matrix IP;

		/// <summary>
		/// The inverted projection view matrix.
		/// </summary>
		SE_FIELD(API())
		Matrix IVP;

		/// <summary>
		/// The view frustum.
		/// </summary>
		SE_FIELD(API())
		BoundingFrustum frustum;

		/// <summary>
		/// The view frustum used for culling (can be different than Frustum in some cases e.g. cascaded shadow map rendering).
		/// </summary>
		SE_FIELD(API())
		BoundingFrustum cullingFrustum;

	public:
		/// <summary>
		/// The draw passes mask for the current view rendering.
		/// </summary>
		SE_FIELD(API())
		DrawPass Pass = DrawPass::None;

		/// <summary>
		/// Flag used by static, offline rendering passes (eg. reflections rendering, lightmap rendering etc.)
		/// </summary>
		SE_FIELD(API())
		bool DrawGizmos = false;

		/// <summary>
		/// Flag used by static, offline rendering passes (eg. reflections rendering, lightmap rendering etc.)
		/// </summary>
		SE_FIELD(API())
		bool IsOfflinePass = false;

		/// <summary>
		/// Flag used by single-frame rendering passes (eg. thumbnail rendering, model view caching) to reject LOD transitions animations and other temporal draw effects.
		/// </summary>
		SE_FIELD(API())
		bool IsSingleFrame = false;

		/// <summary>
		/// Flag used by custom passes to skip any object culling when drawing.
		/// </summary>
		SE_FIELD(API())
		bool IsCullingDisabled = false;

		/// <summary>
		/// True if TAA has been resolved when rendering view and frame doesn't contain jitter anymore. Rendering geometry after this point should not use jitter anymore (eg. editor gizmos or custom geometry as overlay).
		/// </summary>
		SE_FIELD(API())
		bool IsTaaResolved = false;

		/// <summary>
		/// The static flags mask used to hide objects that don't have a given static flags. Eg. use StaticFlags::Lightmap to render only objects that can use lightmap.
		/// </summary>
		SE_FIELD(API())
		StaticMask StaticFlagsMask = StaticMask::None;

		/// <summary>
		/// The view flags.
		/// </summary>
		// ViewFlags Flags = ViewFlags::DefaultGame;

		/// <summary>
		/// The view mode.
		/// </summary>
		SE_FIELD(API())
		ViewMode Mode = ViewMode::Default;

		/// <summary>
		/// Maximum allowed shadows quality for this view
		/// </summary>
		SE_FIELD(API())
		Quality MaxShadowsQuality = Quality::Ultra;

		/// <summary>
		/// The model LOD bias. Default is 0. Applied to all the objects in the render view.
		/// </summary>
		SE_FIELD(API())
		int32 ModelLODBias = 0;

		/// <summary>
		/// The model LOD distance scale factor. Default is 1. Applied to all the objects in the render view. Higher values increase LODs quality.
		/// </summary>
		SE_FIELD(API())
		float ModelLODDistanceFactor = 1.0f;

		/// <summary>
		/// The model LOD bias. Default is 0. Applied to all the objects in the shadow maps render views. Can be used to improve shadows rendering performance or increase quality.
		/// [Deprecated on 26.10.2022, expires on 26.10.2024]
		/// </summary>
		SE_FIELD(API())
		DEPRECATED int32 ShadowModelLODBias = 0;

		/// <summary>
		/// The model LOD distance scale factor. Default is 1. Applied to all the objects in the shadow maps render views. Higher values increase LODs quality. Can be used to improve shadows rendering performance or increase quality.
		/// [Deprecated on 26.10.2022, expires on 26.10.2024]
		/// </summary>
		SE_FIELD(API())
		DEPRECATED float ShadowModelLODDistanceFactor = 1.0f;

		/// <summary>
		/// Temporal Anti-Aliasing jitter frame index.
		/// </summary>
		SE_FIELD(API())
		int32 TaaFrameIndex = 0;

		/// <summary>
		/// The rendering mask for layers. Used to exclude objects from rendering.
		/// </summary>
		SE_FIELD(API())
		LayersMask RenderLayersMask;

	public:
		/// <summary>
		/// The view information vector with packed components to reconstruct linear depth and view position from the hardware depth buffer. Cached before rendering.
		/// </summary>
		SE_FIELD(API())
		Float4 ViewInfo;

		/// <summary>
		/// The screen size packed (x - width, y - height, zw - inv width, w - inv height). Cached before rendering.
		/// </summary>
		SE_FIELD(API())
		Float4 ScreenSize;

		/// <summary>
		/// The temporal AA jitter packed (xy - this frame jitter, zw - previous frame jitter). Cached before rendering. Zero if TAA is disabled. The value added to projection matrix (in clip space).
		/// </summary>
		SE_FIELD(API())
		Float4 TemporalAAJitter;

		/// <summary>
		/// The previous frame rendering view origin.
		/// </summary>
		SE_FIELD(API())
		Float3 PrevOrigin;

		/// <summary>
		/// The previous frame view matrix.
		/// </summary>
		SE_FIELD(API())
		Matrix PrevView;

		/// <summary>
		/// The previous frame projection matrix.
		/// </summary>
		SE_FIELD(API())
		Matrix PrevProjection;

		/// <summary>
		/// The previous frame view * projection matrix.
		/// </summary>
		SE_FIELD(API())
		Matrix PrevViewProjection;

		/// <summary>
		/// The main viewport view * projection matrix.
		/// </summary>
		SE_FIELD(API())
		Matrix MainViewProjection;

		/// <summary>
		/// The main viewport screen size packed (x - width, y - height, zw - inv width, w - inv height).
		/// </summary>
		SE_FIELD(API())
		Float4 MainScreenSize;

		/// <summary>
		/// Square of <see cref="ModelLODDistanceFactor"/>. Cached by rendering backend.
		/// </summary>
		SE_FIELD(API())
		float ModelLODDistanceFactorSqrt;

		/// <summary>
		/// Prepares view for rendering a scene. Called before rendering so other parts can reuse calculated value.
		/// </summary>
		/// <param name="renderContext">The rendering context.</param>
		void Prepare(RenderContext& renderContext);

		/// <summary>
		/// Prepares the cached data.
		/// </summary>
		/// <param name="renderContext">The rendering context.</param>
		/// <param name="width">The rendering width.</param>
		/// <param name="height">The rendering height.</param>
		/// <param name="temporalAAJitter">The temporal jitter for this frame.</param>
		/// <param name="mainView">The main rendering viewport. Use null if it's top level view; pass pointer to main view for sub-passes like shadow depths.</param>
		void PrepareCache(const RenderContext& renderContext, float width, float height, const Float2& temporalAAJitter, const RenderView* mainView = nullptr);

		/// <summary>
		/// Determines whether view is perspective projection or orthographic.
		/// </summary>
		FORCE_INLINE bool IsPerspectiveProjection() const
		{
			return Projection.M44 < 1.0f;
		}

		/// <summary>
		/// Determines whether view is orthographic projection or perspective.
		/// </summary>
		FORCE_INLINE bool IsOrthographicProjection() const
		{
			return Projection.M44 >= 1.0f;
		}

	public:
		// Ignore deprecation warnings in defaults
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		RenderView() = default;
		RenderView(const RenderView& other) = default;
		RenderView(RenderView&& other) = default;
		RenderView& operator=(const RenderView& other) = default;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS

		/// <summary>
		/// Updates the cached data for the view (inverse matrices, etc.).
		/// </summary>
		void UpdateCachedData();

		// Set up view with custom params
		// @param viewProjection View * Projection matrix
		void SetUp(const Matrix& viewProjection);

		// Set up view with custom params
		// @param view View matrix
		// @param projection Projection matrix
		void SetUp(const Matrix& view, const Matrix& projection);

		/// <summary>
		/// Set up view for cube rendering
		/// </summary>
		/// <param name="nearPlane">Near plane</param>
		/// <param name="farPlane">Far plane</param>
		/// <param name="position">Camera's position</param>
		void SetUpCube(float nearPlane, float farPlane, const Float3& position);

		/// <summary>
		/// Set up view for given face of the cube rendering
		/// </summary>
		/// <param name="faceIndex">Face index(0-5)</param>
		void SetFace(int32 faceIndex);

		/// <summary>
		/// Set up view for cube rendering
		/// </summary>
		/// <param name="nearPlane">Near plane</param>
		/// <param name="farPlane">Far plane</param>
		/// <param name="position">Camera's position</param>
		/// <param name="direction">Camera's direction vector</param>
		/// <param name="up">Camera's up vector</param>
		/// <param name="angle">Camera's FOV angle (in degrees)</param>
		void SetProjector(float nearPlane, float farPlane, const Float3& position, const Float3& direction, const Float3& up, float angle);

		/// <summary>
		/// Copies view data from camera to the view.
		/// </summary>
		/// <param name="camera">The camera to copy its data.</param>
		/// <param name="viewport">The custom viewport to use for view/projection matrices override.</param>
		void CopyFrom(const Camera* camera, const Viewport* viewport = nullptr);

	public:
		DrawPass GetShadowsDrawPassMask(ShadowsCastingMode shadowsMode) const
		{
			DrawPass flag = EnumRemoveFlags(Pass, DrawPass::Depth);

			if (EnumHasAllFlags(shadowsMode, ShadowsCastingMode::All))
			{
				return DrawPass::All;
			}else if (EnumHasAllFlags(shadowsMode, ShadowsCastingMode::DynamicOnly))
			{
				return IsOfflinePass ? flag : DrawPass::All;
			}else if (EnumHasAllFlags(shadowsMode, ShadowsCastingMode::StaticOnly))
			{
				return IsOfflinePass ? DrawPass::All : flag;
			}else if (shadowsMode == ShadowsCastingMode::None)
			{
				return flag;
			}

			return DrawPass::All;
		}

	public:
		// Camera's View * Projection matrix
		FORCE_INLINE const Matrix& ViewProjection() const
		{
			return frustum.GetMatrix();
		}

		// Camera's View * Projection matrix
		FORCE_INLINE Matrix ViewProjection()
		{
			return frustum.GetMatrix();
		}

		// Calculates the world matrix for the given transformation instance rendering.
		void GetWorldMatrix(const Transform& transform, Matrix& world) const;

		// Applies the render origin to the transformation instance matrix.
		FORCE_INLINE void GetWorldMatrix(Matrix& world) const
		{
			world.M41 -= static_cast<float>(Origin.X);
			world.M42 -= static_cast<float>(Origin.Y);
			world.M43 -= static_cast<float>(Origin.Z);
		}
	};

} // SE


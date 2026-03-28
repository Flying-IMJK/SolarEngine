#pragma once
#include "Runtime/Graphics/Viewport.h"
#include "Runtime/Level/Actor.h"
#include "Runtime/Render/RenderContext.h"
#include "Runtime/Render/Assets/Geometry/Model.h"
#include "Runtime/Render/Assets/Geometry/ModelInstanceEntry.h"
#include "Runtime/Resource/AssetRef.h"

namespace SE
{

/// <summary>
/// Describes the camera projection and view. Provides information about how to render scene (viewport location and direction, etc.).
/// </summary>
class SE_API_RUNTIME Camera final : public Actor
{
    SE_CLASS(Camera, Actor)

    // List with all created cameras actors on the scene
    static List<Camera*> Cameras;

    // The current cut-scene camera. Set by the Scene Animation Player to the current shot camera.
    static Camera* CutSceneCamera;

    // The overriden main camera.
    // static ScriptingObjectReference<Camera> OverrideMainCamera;

    // Gets the main camera.
    static Camera* GetMainCamera();

private:
    BoundingFrustum _frustum;
    Viewport _viewport;

    // Camera Settings
    bool _usePerspective;
    float _fov;
    float _customAspectRatio;
    float _near;
    float _far;
    float _orthoScale;

#if SE_EDITOR
    AssetRef<Model> _previewModel;
    ModelInstanceEntries _previewModelBuffer;
    BoundingBox _previewModelBox;
    int32 _sceneRenderingKey = -1;
#endif

public:
    /// <summary>
    /// Gets the frustum.
    /// </summary>
    FORCE_INLINE BoundingFrustum GetFrustum() const
    {
        return _frustum;
    }

public:

    Camera();

    /// <summary>
    /// Gets the value indicating if camera should use perspective rendering mode, otherwise it will use orthographic projection.
    /// </summary>
    // API_PROPERTY(Attributes="EditorOrder(10), DefaultValue(true), EditorDisplay(\"Camera\")")
    bool GetUsePerspective() const;

    /// <summary>
    /// Sets the value indicating if camera should use perspective rendering mode, otherwise it will use orthographic projection.
    /// </summary>
    void SetUsePerspective(bool value);

    /// <summary>
    /// Gets the camera's field of view (in degrees).
    /// </summary>
    // API_PROPERTY(Attributes="EditorOrder(20), DefaultValue(60.0f), Limit(0, 179), EditorDisplay(\"Camera\", \"Field Of View\"), VisibleIf(nameof(UsePerspective)), ValueCategory(Utils.ValueCategory.Angle)")
    float GetFieldOfView() const;

    /// <summary>
    /// Sets camera's field of view (in degrees).
    /// </summary>
    void SetFieldOfView(float value);

    /// <summary>
    /// Gets the custom aspect ratio. 0 if not use custom value.
    /// </summary>
    // API_PROPERTY(Attributes="EditorOrder(50), DefaultValue(0.0f), Limit(0, 10, 0.01f), EditorDisplay(\"Camera\"), VisibleIf(nameof(UsePerspective))")
    float GetCustomAspectRatio() const;

    /// <summary>
    /// Sets the custom aspect ratio. 0 if not use custom value.
    /// </summary>
    void SetCustomAspectRatio(float value);

    /// <summary>
    /// Gets camera's near plane distance.
    /// </summary>
    // API_PROPERTY(Attributes="EditorOrder(30), DefaultValue(10.0f), Limit(0, 1000, 0.05f), EditorDisplay(\"Camera\"), ValueCategory(Utils.ValueCategory.Distance)")
    float GetNearPlane() const;

    /// <summary>
    /// Sets camera's near plane distance.
    /// </summary>
    void SetNearPlane(float value);

    /// <summary>
    /// Gets camera's far plane distance.
    /// </summary>
    // API_PROPERTY(Attributes="EditorOrder(40), DefaultValue(40000.0f), Limit(0, float.MaxValue, 5), EditorDisplay(\"Camera\"), ValueCategory(Utils.ValueCategory.Distance)")
    float GetFarPlane() const;

    /// <summary>
    /// Sets camera's far plane distance.
    /// </summary>
    void SetFarPlane(float value);

    /// <summary>
    /// Gets the orthographic projection scale.
    /// </summary>
    // API_PROPERTY(Attributes="EditorOrder(60), DefaultValue(1.0f), Limit(0.0001f, 1000, 0.01f), EditorDisplay(\"Camera\"), VisibleIf(nameof(UsePerspective), true)")
    float GetOrthographicScale() const;

    /// <summary>
    /// Sets the orthographic projection scale.
    /// </summary>
    void SetOrthographicScale(float value);

    /// <summary>
    /// The layers mask used for rendering using this camera. Can be used to include or exclude specific actor layers from the drawing.
    /// </summary>
    LayersMask RenderLayersMask;

    /// <summary>
    /// Frame rendering flags used to switch between graphics features for this camera.
    /// </summary>
    // API_FIELD(Attributes = "EditorOrder(110), EditorDisplay(\"Camera\")")
    // ViewFlags RenderFlags = ViewFlags::DefaultGame;

    /// <summary>
    /// Describes frame rendering modes for this camera.
    /// </summary>
    // API_FIELD(Attributes = "EditorOrder(120), EditorDisplay(\"Camera\")")
    ViewMode RenderMode = ViewMode::Default;

public:
    /// <summary>
    /// Projects the point from 3D world-space to game window coordinates (in screen pixels for default viewport calculated from <see cref="Viewport"/>).
    /// </summary>
    /// <param name="worldSpaceLocation">The input world-space location (XYZ in world).</param>
    /// <param name="gameWindowSpaceLocation">The output game window coordinates (XY in screen pixels).</param>
    void ProjectPoint(const Float3& worldSpaceLocation, Float2& gameWindowSpaceLocation) const;

    /// <summary>
    /// Projects the point from 3D world-space to the camera viewport-space (in screen pixels for given viewport).
    /// </summary>
    /// <param name="worldSpaceLocation">The input world-space location (XYZ in world).</param>
    /// <param name="cameraViewportSpaceLocation">The output camera viewport-space location (XY in screen pixels).</param>
    /// <param name="viewport">The viewport.</param>
    void ProjectPoint(const Float3& worldSpaceLocation, Float2& cameraViewportSpaceLocation, const Viewport& viewport) const;

    /// <summary>
    /// Converts a game window-space point into a corresponding point in world space.
    /// </summary>
    /// <param name="gameWindowSpaceLocation">The input game window coordinates (XY in screen pixels).</param>
    /// <param name="depth">The input camera-relative depth position (eg. clipping plane).</param>
    /// <param name="worldSpaceLocation">The output world-space location (XYZ in world).</param>
    void UnProjectPoint(const Float2& gameWindowSpaceLocation, float depth, Float3& worldSpaceLocation) const;

    /// <summary>
    /// Converts a camera viewport-space point into a corresponding point in world space.
    /// </summary>
    /// <param name="cameraViewportSpaceLocation">The input camera viewport-space location (XY in screen pixels).</param>
    /// <param name="depth">The input camera-relative depth position (eg. clipping plane).</param>
    /// <param name="worldSpaceLocation">The output world-space location (XYZ in world).</param>
    /// <param name="viewport">The viewport.</param>
    void UnProjectPoint(const Float2& cameraViewportSpaceLocation, float depth, Float3& worldSpaceLocation, const Viewport& viewport) const;

    /// <summary>
    /// Checks if the 3d point of the world is in the camera's field of view.
    /// </summary>
    /// <param name="worldSpaceLocation">World Position (XYZ).</param>
    /// <returns>Returns true if the point is within the field of view.</returns>
    bool IsPointOnView(const Float3& worldSpaceLocation) const;

    /// <summary>
    /// Converts the mouse position to 3D ray.
    /// </summary>
    /// <param name="mousePosition">The mouse position.</param>
    /// <returns>Mouse ray</returns>
    Ray ConvertMouseToRay(const Float2& mousePosition) const;

    /// <summary>
    /// Converts the mouse position to 3D ray.
    /// </summary>
    /// <param name="mousePosition">The mouse position.</param>
    /// <param name="viewport">The viewport.</param>
    /// <returns>Mouse ray</returns>
    Ray ConvertMouseToRay(const Float2& mousePosition, const Viewport& viewport) const;

    /// <summary>
    /// Gets the camera viewport.
    /// </summary>
    Viewport GetViewport() const;

    void SetViewport(Viewport v);

    /// <summary>
    /// Calculates the view and the projection matrices for the camera.
    /// </summary>
    /// <param name="view">The result camera view matrix.</param>
    /// <param name="projection">The result camera projection matrix.</param>
    void GetMatrices(Matrix& view,Matrix& projection) const;

    /// <summary>
    /// Calculates the view and the projection matrices for the camera. Support using custom viewport.
    /// </summary>
    /// <param name="view">The result camera view matrix.</param>
    /// <param name="projection">The result camera projection matrix.</param>
    /// <param name="viewport">The custom output viewport.</param>
    void GetMatrices(Matrix& view,Matrix& projection, const Viewport& viewport) const;

    /// <summary>
    /// Calculates the view and the projection matrices for the camera. Support using custom viewport and view origin.
    /// </summary>
    /// <param name="view">The result camera view matrix.</param>
    /// <param name="projection">The result camera projection matrix.</param>
    /// <param name="viewport">The custom output viewport.</param>
    /// <param name="origin">The rendering view origin (for relative-to-camera rendering).</param>
    void GetMatrices(Matrix& view,Matrix& projection, const Viewport& viewport, const Float3& origin) const;

#if SE_EDITOR
    // Intersection check for editor picking the camera
    bool IntersectsItselfEditor(const Ray& ray, float& distance);

    // [Actor]
    // BoundingBox GetEditorBox() const override;
    // bool HasContentLoaded() const override;
    // void OnDebugDrawSelected() override;
#endif

    void Serialize(SerializeContext& context) override;
    void Deserialize(DeserializeContext& context) override;

protected:
    // [Actor]
    void OnEnable() override;
    void OnDisable() override;
    void OnTransformChanged() override;
#if SE_EDITOR
    // void BeginPlay(SceneBeginData* data) override;
#endif

private:
#if SE_EDITOR
    void OnPreviewModelLoaded();
#endif
    void UpdateCache();
};


} // SE



#include "Camera.h"

#include "Core/Math/Line.h"
#include "Core/Serialization/Serialization.h"
#include "Runtime/Engine.h"
#include "../Scene/Scene.h"
#include "Runtime/Render/RenderDrawCall.h"

namespace SE
{
    List<Camera*> Camera::Cameras;
    Camera* Camera::CutSceneCamera = nullptr;
    // ScriptingObjectReference<Camera> Camera::OverrideMainCamera;

    Camera* Camera::GetMainCamera()
    {
        /*Camera* overrideMainCamera = OverrideMainCamera.Get();
        if (overrideMainCamera)
            return overrideMainCamera;*/
        if (CutSceneCamera)
            return CutSceneCamera;
        return Cameras.HasItems() ? Cameras.First() : nullptr;
    }

    Camera::Camera() :
        _usePerspective(true)
        , _fov(60.0f)
        , _customAspectRatio(0.0f)
        , _near(10.0f)
        , _far(40000.0f)
        , _orthoScale(1.0f)
    {
#if SE_EDITOR
        _previewModel.Loaded.Bind<Camera, &Camera::OnPreviewModelLoaded>(this);
#endif
    }

    bool Camera::GetUsePerspective() const
    {
        return _usePerspective;
    }

    void Camera::SetUsePerspective(bool value)
    {
        if (_usePerspective != value)
        {
            _usePerspective = value;
            UpdateCache();
        }
    }

    float Camera::GetFieldOfView() const
    {
        return _fov;
    }

    void Camera::SetFieldOfView(float value)
    {
        value = Math::Clamp(value, 1.0f, 179.9f);
        if (!Math::IsNearEqual(_fov, value))
        {
            _fov = value;
            UpdateCache();
        }
    }

    float Camera::GetCustomAspectRatio() const
    {
        return _customAspectRatio;
    }

    void Camera::SetCustomAspectRatio(float value)
    {
        value = Math::Clamp(value, 0.0f, 100.0f);
        if (!Math::IsNearEqual(_customAspectRatio, value))
        {
            _customAspectRatio = value;
            UpdateCache();
        }
    }

    float Camera::GetNearPlane() const
    {
        return _near;
    }

    void Camera::SetNearPlane(float value)
    {
        value = Math::Clamp(value, 0.001f, _far - 1.0f);
        if (!Math::IsNearEqual(_near, value))
        {
            _near = value;
            UpdateCache();
        }
    }

    float Camera::GetFarPlane() const
    {
        return _far;
    }

    void Camera::SetFarPlane(float value)
    {
        value = Math::Max(value, _near + 1.0f);
        if (!Math::IsNearEqual(_far, value))
        {
            _far = value;
            UpdateCache();
        }
    }

    float Camera::GetOrthographicScale() const
    {
        return _orthoScale;
    }

    void Camera::SetOrthographicScale(float value)
    {
        value = Math::Clamp(value, 0.0001f, 1000000.0f);
        if (!Math::IsNearEqual(_orthoScale, value))
        {
            _orthoScale = value;
            UpdateCache();
        }
    }

    void Camera::ProjectPoint(const Float3& worldSpaceLocation, Float2& gameWindowSpaceLocation) const
    {
        ProjectPoint(worldSpaceLocation, gameWindowSpaceLocation, GetViewport());
    }

    void Camera::ProjectPoint(const Float3& worldSpaceLocation, Float2& cameraViewportSpaceLocation, const Viewport& viewport) const
    {
        Matrix v, p, vp;
        GetMatrices(v, p, viewport);
        Matrix::Multiply(v, p, vp);
        Float3 clipSpaceLocation;
        Float3::Transform(worldSpaceLocation, vp, clipSpaceLocation);
        viewport.Project(worldSpaceLocation, vp, clipSpaceLocation);
        cameraViewportSpaceLocation = Float2(clipSpaceLocation);
    }

    void Camera::UnProjectPoint(const Float2& gameWindowSpaceLocation, float depth, Float3& worldSpaceLocation) const
    {
        UnProjectPoint(gameWindowSpaceLocation, depth, worldSpaceLocation, GetViewport());
    }

    void Camera::UnProjectPoint(const Float2& cameraViewportSpaceLocation, float depth, Float3& worldSpaceLocation, const Viewport& viewport) const
    {
        Matrix v, p, ivp;
        GetMatrices(v, p, viewport);
        Matrix::Multiply(v, p, ivp);
        ivp.Invert();
        viewport.UnProject(Float3(cameraViewportSpaceLocation, depth), ivp, worldSpaceLocation);
    }

    bool Camera::IsPointOnView(const Float3& worldSpaceLocation) const
    {
        Float3 cameraUp = GetTransform().GetUp();
        Float3 cameraForward = GetTransform().GetForward();
        Float3 directionToPosition = (worldSpaceLocation - GetPosition()).GetNormalized();
        if (Float3::Dot(cameraForward, directionToPosition) < 0)
            return false;

        const Quaternion lookAt = Quaternion::LookRotation(directionToPosition, cameraUp);
        const Float3 lookAtDirection = lookAt * Float3::Forward;
        const Float3 newWorldLocation = GetPosition() + lookAtDirection;

        Float2 windowSpace;
        const Viewport viewport = GetViewport();
        ProjectPoint(newWorldLocation, windowSpace, viewport);

        return windowSpace.x >= 0 && windowSpace.x <= viewport.size.x && windowSpace.y >= 0 && windowSpace.y <= viewport.size.y;
    }

    Ray Camera::ConvertMouseToRay(const Float2& mousePosition) const
    {
        return ConvertMouseToRay(mousePosition, GetViewport());
    }

    Ray Camera::ConvertMouseToRay(const Float2& mousePosition, const Viewport& viewport) const
    {
#if 1
        // Gather camera properties
        Matrix v, p, ivp;
        GetMatrices(v, p, viewport);
        Matrix::Multiply(v, p, ivp);
        ivp.Invert();

        // Create near and far points
        Float3 nearPoint(mousePosition, 0.0f);
        Float3 farPoint(mousePosition, 1.0f);
        viewport.UnProject(nearPoint, ivp, nearPoint);
        viewport.UnProject(farPoint, ivp, farPoint);

        // Create direction vector
        Float3 direction = farPoint - nearPoint;
        direction.Normalize();

        return Ray(nearPoint, direction);
#else
        // Create near and far points
        Float3 nearPoint, farPoint;
        Matrix ivp;
        _frustum.GetInvMatrix(&ivp);
        viewport.Unproject(Float3(mousePosition, 0.0f), ivp, nearPoint);
        viewport.Unproject(Float3(mousePosition, 1.0f), ivp, farPoint);

        // Create direction vector
        Float3 direction = farPoint - nearPoint;
        direction.Normalize();

        // Return result
        return Ray(nearPoint, direction);
#endif
    }

    Viewport Camera::GetViewport() const
    {
        Viewport result = _viewport;

        // Fallback to the default value
        if (result.size.MinValue() <= Math::ZeroTolerance)
        {
            result.size = Float2(1280, 720);
        }

        return _viewport;

        /*Viewport result = Viewport(Float2::Zero);

#if SE_EDITOR
        // Editor
        if (Editor::Managed)
            result.Size = Editor::Managed->GetGameWindowSize();
#else
        // Game
        auto mainWin = Engine::MainWindow;
        if (mainWin)
        {
            const auto size = mainWin->GetClientSize();
            result.Size = size;
        }
#endif

        // Fallback to the default value
        if (result.size.MinValue() <= Math::ZeroTolerance)
        {
            result.size = Float2(1280, 720);
        }

        return result;*/
    }

    void Camera::SetViewport(Viewport v)
    {
        _viewport = v;
    }

    void Camera::GetMatrices(Matrix& view, Matrix& projection) const
    {
        GetMatrices(view, projection, GetViewport(), Float3::Zero);
    }

    void Camera::GetMatrices(Matrix& view, Matrix& projection, const Viewport& viewport) const
    {
        GetMatrices(view, projection, viewport, Float3::Zero);
    }

    void Camera::GetMatrices(Matrix& view, Matrix& projection, const Viewport& viewport, const Float3& origin) const
    {
        // Create projection matrix
        if (_usePerspective)
        {
            const float aspect = _customAspectRatio <= 0.0f ? viewport.GetAspectRatio() : _customAspectRatio;
            Matrix::PerspectiveFov(_fov * Math::DegreesToRadians, aspect, _near, _far, projection);
        }
        else
        {
            Matrix::Ortho(viewport.width * _orthoScale, viewport.height * _orthoScale, _near, _far, projection);
        }

        // Create view matrix
        const Float3 direction = GetForward();
        const Float3 position = m_Transform.Translation - origin;
        const Float3 target = position + direction;
        Float3 up;
        Float3::Transform(Float3::Up, GetOrientation(), up);
        Matrix::LookAt(position, target, up, view);
    }

#if SE_EDITOR

    void Camera::OnPreviewModelLoaded()
    {
        _previewModelBuffer.Setup(_previewModel.Get());

        UpdateCache();
    }

    /*void Camera::BeginPlay(SceneBeginData* data)
    {
        _previewModel = Content::LoadAsyncInternal<Model>(TEXT("Editor/Camera/O_Camera"));

        // Base
        Actor::BeginPlay(data);
    }*/

    /*BoundingBox Camera::GetEditorBox() const
    {
        const Float3 size(100);
        const Float3 pos = _transform.Translation + _transform.Orientation * Float3::Forward * 30.0f;
        return BoundingBox(pos - size, pos + size);
    }*/

    bool Camera::IntersectsItselfEditor(const Ray& ray, float& distance)
    {
        return _previewModelBox.Intersects(ray, distance);
    }

    /*bool Camera::HasContentLoaded() const
    {
        return _previewModel == nullptr || _previewModel->IsLoaded();
    }*/

    /*void Camera::Draw(RenderContext& renderContext)
    {
        /*if (EnumHasAnyFlags(renderContext.View.Flags, ViewFlags::EditorSprites)
            && _previewModel
            && _previewModel->IsLoaded())
        {
            Matrix rot, tmp, world;
            renderContext.view.GetWorldMatrix(_transform, tmp);
            Matrix::RotationY(Math::PI * -0.5f, rot);
            Matrix::Multiply(rot, tmp, world);
            GeometryDrawStateData drawState;
            Mesh::DrawInfo draw;
            draw.Buffer = &_previewModelBuffer;
            draw.World = &world;
            draw.DrawState = &drawState;
            // draw.Deformation = nullptr;
            draw.Lightmap = nullptr;
            draw.LightmapUVs = nullptr;
            draw.Flags = StaticMask::Transform;
            draw.DrawModes = renderContext.view.Pass;
            draw.DrawModes.SetMultipleFlags(DrawPass::Depth, DrawPass::GBuffer, DrawPass::Forward);

            BoundingSphere::FromBox(_previewModelBox, draw.Bounds);
            draw.Bounds.Center -= renderContext.view.Origin;
            draw.PerInstanceRandom = GetPerInstanceRandom();
            draw.LODBias = 0;
            draw.ForcedLOD = -1;
            draw.SortOrder = 0;
            draw.VertexColors = nullptr;
            if (!draw.DrawModes.Is(DrawPass::None))
            {
                _previewModel->Draw(renderContext, draw);
            }
        }
        // Load preview model if it doesnt exist. Ex: prefabs
        else if (EnumHasAnyFlags(renderContext.view.Flags, ViewFlags::EditorSprites) && !_previewModel)
        {
            _previewModel = Content::LoadAsyncInternal<Model>(TEXT("Editor/Camera/O_Camera"));
        }#1#
    }*/

/*#include "Engine/Debug/DebugDraw.h"

    void Camera::OnDebugDrawSelected()
    {
        DEBUG_DRAW_WIRE_FRUSTUM(_frustum, Color::White, 0, true);

        // Base
        Actor::OnDebugDrawSelected();
    }*/

#endif

    void Camera::UpdateCache()
    {
        // Calculate view and projection matrices
        Matrix view, projection;
        GetMatrices(view, projection);

        // Update frustum and bounding box
        _frustum.SetMatrix(view, projection);
        _frustum.GetBox(m_Box);
        BoundingSphere::FromBox(m_Box, m_Sphere);

#if SE_EDITOR

        // Update editor preview model cache
        Matrix rot, tmp, world;
        GetLocalToWorldMatrix(tmp);
        Matrix::RotationY(Math::PI * -0.5f, rot);
        Matrix::Multiply(rot, tmp, world);

        // Calculate snap box for preview model
        if (_previewModel && _previewModel->IsLoaded())
        {
            _previewModelBox = _previewModel->GetBox(world);
        }
        else
        {
            Float3 min(-10.0f), max(10.0f);
            min = Float3::Transform(min, world);
            max = Float3::Transform(max, world);
            _previewModelBox = BoundingBox(min, max);
        }

        // Extend culling bounding box
        BoundingBox::Merge(m_Box, _previewModelBox, m_Box);
        BoundingSphere::FromBox(m_Box, m_Sphere);

#endif
    }

    void Camera::Serialize(SerializeContext& context)
    {
        // Base
        Actor::Serialize(context);

        SERIALIZE_GET_OTHER_OBJ(Camera, context.otherObj);

        SERIALIZE_MEMBER(UsePerspective, _usePerspective);
        SERIALIZE_MEMBER(FOV, _fov);
        SERIALIZE_MEMBER(CustomAspectRatio, _customAspectRatio);
        SERIALIZE_MEMBER(Near, _near);
        SERIALIZE_MEMBER(Far, _far);
        SERIALIZE_MEMBER(OrthoScale, _orthoScale);
        /*SERIALIZE(RenderLayersMask);
        SERIALIZE(RenderFlags);*/
        SERIALIZE(RenderMode);
    }

    void Camera::Deserialize(DeserializeContext& context)
    {
        // Base
        Actor::Deserialize(context);

        DESERIALIZE_MEMBER(UsePerspective, _usePerspective);
        DESERIALIZE_MEMBER(FOV, _fov);
        DESERIALIZE_MEMBER(CustomAspectRatio, _customAspectRatio);
        DESERIALIZE_MEMBER(Near, _near);
        DESERIALIZE_MEMBER(Far, _far);
        DESERIALIZE_MEMBER(OrthoScale, _orthoScale);
        /*DESERIALIZE(RenderLayersMask);
        DESERIALIZE(RenderFlags);*/
        DESERIALIZE(RenderMode);
    }

    void Camera::OnEnable()
    {
        Cameras.Add(this);
        // Base
        Actor::OnEnable();
    }

    void Camera::OnDisable()
    {
        Cameras.Remove(this);
        if (CutSceneCamera == this)
        {
            CutSceneCamera = nullptr;
        }

        // Base
        Actor::OnDisable();
    }

    void Camera::OnTransformChanged()
    {
        // Base
        Actor::OnTransformChanged();

        UpdateCache();
    }
} // SE
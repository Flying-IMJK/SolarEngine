

#include "FPSCamera.h"

#include "Editor/Viewport/EditorViewport.h"
#include "Runtime/Utilities/Time.h"

namespace SE::Editor
{
	bool FPSCamera::GetIsAnimatingMove()
	{
		return _moveStartTime > Math::EPSILON;
	}

	void FPSCamera::SetView(Float3 position, Float3 direction)
	{
		if (GetIsAnimatingMove())
			return;

		// Rotate and move
		GetViewport()->ViewPosition = position;
		GetViewport()->ViewDirection = direction;
	}

	void FPSCamera::SetView(Float3 position, Quaternion orientation)
	{
		if (GetIsAnimatingMove())
			return;

		// Rotate and move
		GetViewport()->ViewPosition = position;
		GetViewport()->ViewOrientation = orientation;
	}

	void FPSCamera::MoveViewport(Float3 position, Quaternion orientation)
	{
		MoveViewport(Transform(position, orientation));
	}

	void FPSCamera::MoveViewport(Transform target)
	{
		_startMove = GetViewport()->ViewTransform;
		_endMove = target;
		_moveStartTime = Time::GetUnscaledGameTime();
	}

	void FPSCamera::ShowSphere(BoundingSphere& sphere, Quaternion& orientation)
	{
		Float3 position;
		if (GetViewport()->UseOrthographicProjection)
		{
			position = sphere.Center + orientation * Float3::Backward * (sphere.Radius * 5.0f);
			GetViewport()->OrthographicScale = Float3::Distance(position, sphere.Center) / 1000;
		}
		else
		{
			// calculate the min. distance so that the sphere fits roughly 70% in FOV
			// clip to far plane as a disappearing big object might be confusing
			float distance = Math::Min(1.4f * sphere.Radius / Math::Tan(Math::DegreesToRadians * GetViewport()->FieldOfView / 2), GetViewport()->FarPlane);
			position = sphere.Center - orientation * Float3::Forward * distance;
		}
		TargetPoint = sphere.Center;
		MoveViewport(position, orientation);
	}

	void FPSCamera::SetArcBallView(Quaternion orientation, Float3 orbitCenter, float orbitRadius)
	{
		ViewportCamera::SetArcBallView(orientation, orbitCenter, orbitRadius);

		TargetPoint = orbitCenter;
	}

	void FPSCamera::Update(float deltaTime)
	{
		// Update animated movement
		if (GetIsAnimatingMove())
		{
			// Calculate linear progress
			float animationDuration = 0.5f;
			float time = Time::GetUnscaledGameTime();
			float progress = (time - _moveStartTime) / animationDuration;

			// Check for end
			if (progress >= 1.0f)
			{
				// Animation has been finished
				_moveStartTime = -1;
			}

			// Animate camera
			float a = Math::Saturate(progress);
			a = a * a * a;
			Transform targetTransform = Transform::Lerp(_startMove, _endMove, a);
			targetTransform.Scale = Float3::Zero;

			EditorViewport* viewport = GetViewport();
			viewport->ViewPosition = targetTransform.Translation;
			viewport->ViewOrientation = targetTransform.Orientation;
		}
	}
	
	void FPSCamera::UpdateView(float dt, const Float3& moveDelta, const Float2& mouseDelta, bool& centerMouse)
	{
		centerMouse = true;

		if (GetIsAnimatingMove())
			return;

		EditorViewport* viewport = GetViewport();

		EditorViewport::Input input = viewport->GetInput();
		EditorViewport::Input prevInput = viewport->GetPrevInput();
		// TransformGizmoBase transformGizmo = (Viewport as EditorGizmoViewport)?.Gizmos.Active as TransformGizmoBase;
		bool isUsingGizmo = false; //transformGizmo != nullptr && transformGizmo.ActiveAxis != TransformGizmoBase.Axis.None;

		// Get current view properties
		float yaw = viewport->Yaw;
		float pitch = viewport->Pitch;
		Float3 position = viewport->ViewPosition;
		Quaternion rotation = viewport->ViewOrientation;

		// Compute base vectors for camera movement
		Float3 forward = rotation * Float3::Forward;
		Float3 up = rotation * Float3::Up;
		Float3 right = Float3::Cross(forward, up);

		// Dolly
		if (input.IsPanning || input.IsMoving || input.IsRotating)
		{
			Float3 move;
			Float3::Transform(moveDelta, rotation, move);
			position += move;
		}

		// Pan
		if (input.IsPanning)
		{
			float panningSpeed = (viewport->RelativePanning)
				? Math::Abs((position - TargetPoint).Length()) * 0.005f
				: viewport->PanningSpeed;

			if (viewport->InvertPanning)
			{
				position += up * (mouseDelta.y * panningSpeed);
				position += right * (mouseDelta.x * panningSpeed);
			}
			else
			{
				position -= right * (mouseDelta.x * panningSpeed);
				position -= up * (mouseDelta.y * panningSpeed);
			}
		}

		// Move
		if (input.IsMoving)
		{
			// Move camera over XZ plane
			Float3 projectedForward = Float3::Normalize(Float3(forward.x, 0, forward.z));
			position -= projectedForward * mouseDelta.y;
			yaw += mouseDelta.x;
		}

		// Rotate or orbit
		if (input.IsRotating || (input.IsOrbiting && !isUsingGizmo && prevInput.IsOrbiting))
		{
			yaw += mouseDelta.x;
			pitch += mouseDelta.y;
		}

		// Zoom in/out
		if (input.IsZooming && !input.IsRotating)
		{
			position += forward * (viewport->MouseWheelZoomSpeedFactor * input.MouseWheelDelta * 25.0f);
			if (input.IsAltDown)
			{
				position += forward * (viewport->MouseSpeed * 40 * viewport->MousePositionDelta.Get());
			}
		}

		// Move camera with the gizmo
		if (input.IsOrbiting && isUsingGizmo)
		{
			centerMouse = false;
			// GetViewport()->ViewPosition += transformGizmo.LastDelta.Translation;
			return;
		}

		// Update view
		viewport->Yaw = yaw;
		viewport->Pitch = pitch;
		if (input.IsOrbiting)
		{
			float orbitRadius = Math::Max(Float3::Distance(position, TargetPoint), 0.0001f);
			Float3 localPosition = viewport->ViewDirection.Get() * (-1 * orbitRadius);
			viewport->ViewPosition = TargetPoint + localPosition;
		}
		else
		{
			TargetPoint += position - viewport->ViewPosition;
			viewport->ViewPosition = position;
		}
	}
} // SE
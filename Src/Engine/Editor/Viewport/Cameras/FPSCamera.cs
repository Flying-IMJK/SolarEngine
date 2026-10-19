#nullable disable annotations

namespace SE.Editor
{
    /// <summary>
    /// First-person camera used by the editor viewport.
    /// </summary>
    public class FPSCamera : ViewportCamera
    {
        private Transform m_StartMove;
        private Transform m_EndMove;
        private float m_MoveStartTime = -1;

        public Float3 TargetPoint = new Float3(-200);

        public bool GetIsAnimatingMove()
        {
            return m_MoveStartTime > Mathf.Epsilon;
        }

        public void SetView(Float3 position, Float3 direction)
        {
            if (GetIsAnimatingMove())
            {
                return;
            }
            GetViewport().ViewPosition = position;
            GetViewport().ViewDirection = direction;
        }

        public void SetView(Float3 position, Quaternion orientation)
        {
            if (GetIsAnimatingMove())
            {
                return;
            }
            GetViewport().ViewPosition = position;
            GetViewport().ViewOrientation = orientation;
        }

        public void MoveViewport(Float3 position, Quaternion orientation)
        {
            MoveViewport(new Transform(position, orientation));
        }

        public void MoveViewport(Transform target)
        {
            m_StartMove = GetViewport().ViewTransform;
            m_EndMove = target;
            m_MoveStartTime = Time.GetUnscaledGameTime();
        }

        public override void ShowSphere(ref BoundingSphere sphere, ref Quaternion orientation)
        {
            EditorViewport viewport = GetViewport();
            Float3 position;
            if (viewport.UseOrthographicProjection)
            {
                position = sphere.Center + Float3.Backward * orientation * (sphere.Radius * 5.0f);
                viewport.OrthographicScale = Float3.Distance(position, sphere.Center) / 1000;
            }
            else
            {
                float distance = Mathf.Min(1.4f * sphere.Radius /
                    Mathf.Tan(Mathf.DegreesToRadians * viewport.FieldOfView / 2), viewport.FarPlane);
                position = sphere.Center - Float3.Forward * orientation * distance;
            }
            TargetPoint = sphere.Center;
            MoveViewport(position, orientation);
        }

        public override void SetArcBallView(Quaternion orientation, Float3 orbitCenter, float orbitRadius)
        {
            base.SetArcBallView(orientation, orbitCenter, orbitRadius);
            TargetPoint = orbitCenter;
        }

        public override void Update(float deltaTime)
        {
            if (GetIsAnimatingMove())
            {
                float animationDuration = 0.5f;
                float time = Time.GetUnscaledGameTime();
                float progress = (time - m_MoveStartTime) / animationDuration;
                if (progress >= 1.0f)
                {
                    m_MoveStartTime = -1;
                }

                float a = Mathf.Saturate(progress);
                a = a * a * a;
                Transform targetTransform = Transform.Lerp(m_StartMove, m_EndMove, a);
                targetTransform.Scale = Float3.Zero;

                EditorViewport viewport = GetViewport();
                viewport.ViewPosition = targetTransform.Translation;
                viewport.ViewOrientation = targetTransform.Orientation;
            }
        }

        public override void UpdateView(float dt, in Float3 moveDelta, in Float2 mouseDelta, ref bool centerMouse)
        {
            centerMouse = true;
            if (GetIsAnimatingMove())
            {
                return;
            }

            EditorViewport viewport = GetViewport();
            EditorViewport.Input input = viewport.GetInput();
            EditorViewport.Input prevInput = viewport.GetPrevInput();
            // TransformGizmoBase transformGizmo = (Viewport as EditorGizmoViewport)?.Gizmos.Active as
            // TransformGizmoBase;
            bool isUsingGizmo =
                false; // transformGizmo != null && transformGizmo.ActiveAxis != TransformGizmoBase.Axis.None;

            float yaw = viewport.Yaw;
            float pitch = viewport.Pitch;
            Float3 position = viewport.ViewPosition;
            Quaternion rotation = viewport.ViewOrientation;

            Float3 forward = Float3.Forward * rotation;
            Float3 up = Float3.Up * rotation;
            Float3 right = Float3.Cross(forward, up);

            if (input.IsPanning || input.IsMoving || input.IsRotating)
            {
                Float3 move = Float3.Transform(moveDelta, rotation);
                position += move;
            }

            if (input.IsPanning)
            {
                float panningSpeed = viewport.RelativePanning
                    ? Mathf.Abs((position - TargetPoint).Length) * 0.005f
                    : viewport.PanningSpeed;
                if (viewport.InvertPanning)
                {
                    position += up * (mouseDelta.Y * panningSpeed);
                    position += right * (mouseDelta.X * panningSpeed);
                }
                else
                {
                    position -= right * (mouseDelta.X * panningSpeed);
                    position -= up * (mouseDelta.Y * panningSpeed);
                }
            }

            if (input.IsMoving)
            {
                Float3 projectedForward = Float3.Normalize(new Float3(forward.X, 0, forward.Z));
                position -= projectedForward * mouseDelta.Y;
                yaw += mouseDelta.X;
            }

            if (input.IsRotating || (input.IsOrbiting && !isUsingGizmo && prevInput.IsOrbiting))
            {
                yaw += mouseDelta.X;
                pitch += mouseDelta.Y;
            }

            if (input.IsZooming && !input.IsRotating)
            {
                position += forward * (viewport.MouseWheelZoomSpeedFactor * input.MouseWheelDelta * 25.0f);
                if (input.IsAltDown)
                {
                    // The native expression multiplies Float3 by Float2; use vertical drag as the dolly distance.
                    position += forward * (viewport.MouseSpeed * 40.0f * viewport.MousePositionDelta.Y);
                }
            }

            if (input.IsOrbiting && isUsingGizmo)
            {
                centerMouse = false;
                // viewport.ViewPosition += transformGizmo.LastDelta.Translation;
                return;
            }

            viewport.Yaw = yaw;
            viewport.Pitch = pitch;
            if (input.IsOrbiting)
            {
                float orbitRadius = Mathf.Max(Float3.Distance(position, TargetPoint), 0.0001f);
                Float3 localPosition = viewport.ViewDirection * (-orbitRadius);
                viewport.ViewPosition = TargetPoint + localPosition;
            }
            else
            {
                TargetPoint += position - viewport.ViewPosition;
                viewport.ViewPosition = position;
            }
        }
    }
}


using System;
using System.Collections.Generic;
using SE.Editor.SceneGraph;

namespace SE.Editor.Gizmo
{
    /// <summary>
    /// Shared transform tool interaction. Rendering and actor lifetime remain native.
    /// Drag deltas are applied to the initial transforms to avoid cumulative rounding errors.
    /// </summary>
    public abstract partial class TransformGizmoBase : GizmoBase
    {
        protected readonly List<Transform> m_StartTransforms = new();
        private Transform m_GizmoWorld = Transform.Identity;
        private Float3 m_DragNormal;
        private Float3 m_DragStart;
        private Float3 m_RotationVector;
        private Float2 m_StartMousePosition;
        private Float3 m_LastTranslation;
        private float m_RotationAngle;
        private bool m_HasChange;

        public Float3 Position => m_GizmoWorld.Translation;
        public bool IsTransforming { get; private set; }
        public override bool IsControllingMouse => IsTransforming;
        public event Action TransformingStarted;
        public event Action TransformingEnded;

        protected abstract int SelectionCount { get; }
        protected abstract ScenesGraphNode GetSelectedObject(int index);
        protected abstract Transform GetSelectedTransform(int index);
        protected abstract void SetSelectedTransform(int index, Transform value);

        protected TransformGizmoBase(IGizmoOwner owner) : base(owner) { }

        private bool UpdateGizmoPosition()
        {
            if (SelectionCount == 0 || Owner.Viewport.Height <= 0.0f)
            {
                ActiveAxis = Axis.None;
                return false;
            }
            if (IsTransforming)
            {
                return true;
            }

            Float3 position = Float3.Zero;
            switch (ActivePivot)
            {
                case PivotType.ObjectCenter:
                    position = GetSelectedTransform(0).Translation;
                    break;
                case PivotType.SelectionCenter:
                    for (int i = 0; i < SelectionCount; i++)
                    {
                        position += GetSelectedTransform(i).Translation;
                    }
                    position /= SelectionCount;
                    break;
            }

            EditorGizmoViewport viewport = Owner.Viewport;
            float depth = Float3.Dot(position - viewport.ViewPosition, viewport.ViewDirection);
            if (!viewport.UseOrthographicProjection && depth <= viewport.NearPlane)
            {
                ActiveAxis = Axis.None;
                return false;
            }
            // Keep the one-unit FBX axis approximately 96 screen pixels long.
            float scale = viewport.UseOrthographicProjection
                ? viewport.OrthographicScale * 96.0f
                : 192.0f * depth * Mathf.Tan(viewport.FieldOfView * Mathf.DegreesToRadians * 0.5f) / viewport.Height;
            Quaternion orientation = ActiveTransformSpace == TransformSpace.Local
                ? GetSelectedTransform(0).Orientation : Quaternion.Identity;
            m_GizmoWorld = new Transform(position, orientation, new Float3(Mathf.Max(scale, 0.0001f)));
            return true;
        }

        public override void Update(float deltaTime)
        {
            if (!IsActive || !Visible || !UpdateGizmoPosition())
            {
                return;
            }
            if (IsTransforming)
            {
                for (int i = 0; i < SelectionCount; i++)
                {
                    if (GetSelectedObject(i).IsDisposed)
                    {
                        CancelTransforming();
                        return;
                    }
                }
                UpdateTransformation();
            }
            else
            {
                ActiveAxis = Owner.Viewport.IsPointerOverScene ? HitTest(Owner.MouseRay) : Axis.None;
            }
        }

        public bool StartTransforming()
        {
            if (IsTransforming || !IsActive || !Visible || !UpdateGizmoPosition())
            {
                return false;
            }
            ActiveAxis = HitTest(Owner.MouseRay);
            if (ActiveAxis == Axis.None)
            {
                return false;
            }

            Float3 axis = GetAxisDirection(ActiveAxis);
            if (ActiveMode == Mode.Rotate)
            {
                m_DragNormal = axis;
            }
            else if (ActiveAxis == Axis.Center)
            {
                m_DragNormal = Owner.Viewport.ViewDirection;
            }
            else if (ActiveAxis == Axis.XY || ActiveAxis == Axis.ZX || ActiveAxis == Axis.YZ)
            {
                m_DragNormal = GetAxisDirection(ActiveAxis == Axis.XY ? Axis.Z : ActiveAxis == Axis.ZX ? Axis.Y : Axis.X);
            }
            else
            {
                Float3 toView = Owner.Viewport.UseOrthographicProjection
                    ? -Owner.Viewport.ViewDirection : Owner.Viewport.ViewPosition - Position;
                m_DragNormal = toView - axis * Float3.Dot(toView, axis);
            }
            if (m_DragNormal.LengthSquared < 0.000001f)
            {
                ActiveAxis = Axis.None;
                return false;
            }
            m_DragNormal.Normalize();
            if (!IntersectPlane(Owner.MouseRay, Position, m_DragNormal, out m_DragStart))
            {
                ActiveAxis = Axis.None;
                return false;
            }

            m_StartTransforms.Clear();
            for (int i = 0; i < SelectionCount; i++)
            {
                m_StartTransforms.Add(GetSelectedTransform(i));
            }
            m_StartMousePosition = Owner.MousePosition;
            m_RotationVector = Float3.Normalize(m_DragStart - Position);
            m_RotationAngle = 0.0f;
            m_LastTranslation = Float3.Zero;
            m_HasChange = false;
            IsTransforming = true;
            OnStartTransforming();
            TransformingStarted?.Invoke();
            return true;
        }

        public void EndTransforming()
        {
            if (!IsTransforming)
            {
                return;
            }
            IsTransforming = false;
            OnEndTransforming(false, m_HasChange);
            m_StartTransforms.Clear();
            ActiveAxis = Axis.None;
            TransformingEnded?.Invoke();
        }

        public void CancelTransforming()
        {
            if (!IsTransforming)
            {
                return;
            }
            for (int i = 0; i < SelectionCount && i < m_StartTransforms.Count; i++)
            {
                if (!GetSelectedObject(i).IsDisposed)
                {
                    SetSelectedTransform(i, m_StartTransforms[i]);
                }
            }
            IsTransforming = false;
            OnEndTransforming(true, m_HasChange);
            m_StartTransforms.Clear();
            ActiveAxis = Axis.None;
            TransformingEnded?.Invoke();
        }

        protected virtual void OnStartTransforming() { }
        protected virtual void OnEndTransforming(bool cancelled, bool changed) { }

        private void UpdateTransformation()
        {
            if (!IntersectPlane(Owner.MouseRay, Position, m_DragNormal, out Float3 point))
            {
                return;
            }
            Float3 translation = Float3.Zero;
            Float3 scale = Float3.Zero;
            Quaternion rotation = Quaternion.Identity;
            Quaternion inverse = Quaternion.Invert(m_GizmoWorld.Orientation);

            if (ActiveMode == Mode.Rotate)
            {
                Float3 vector = point - Position;
                if (vector.LengthSquared < 0.000001f)
                {
                    return;
                }
                vector.Normalize();
                m_RotationAngle += Mathf.Atan2(Float3.Dot(m_DragNormal, Float3.Cross(m_RotationVector, vector)),
                    Float3.Dot(m_RotationVector, vector));
                m_RotationVector = vector;
                float angle = m_RotationAngle;
                if ((RotationSnapEnabled || Owner.UseSnapping) && RotationSnapValue > 0.0f)
                {
                    float step = RotationSnapValue * Mathf.DegreesToRadians;
                    angle = Mathf.Round(angle / step) * step;
                }
                rotation = Quaternion.RotationAxis(m_DragNormal, angle);
            }
            else
            {
                Float3 delta = Float3.Transform(point - m_DragStart, inverse);
                if (ActiveAxis != Axis.Center)
                {
                    delta.X = (ActiveAxis & Axis.X) != 0 ? delta.X : 0.0f;
                    delta.Y = (ActiveAxis & Axis.Y) != 0 ? delta.Y : 0.0f;
                    delta.Z = (ActiveAxis & Axis.Z) != 0 ? delta.Z : 0.0f;
                }
                if (ActiveMode == Mode.Scale)
                {
                    delta /= m_GizmoWorld.Scale.X;
                    if (ActiveAxis == Axis.Center)
                    {
                        delta = new Float3((m_StartMousePosition.Y - Owner.MousePosition.Y) / 96.0f);
                    }
                }
                float step = ActiveMode == Mode.Scale ? ScaleSnapValue : TranslationSnapValue;
                bool snapEnabled = ActiveMode == Mode.Scale ? ScaleSnapEnabled : TranslationSnapEnable;
                if ((snapEnabled || Owner.UseSnapping) && step > 0.0f)
                {
                    delta = new Float3(Mathf.Round(delta.X / step), Mathf.Round(delta.Y / step), Mathf.Round(delta.Z / step)) * step;
                }
                if (ActiveMode == Mode.Translate)
                {
                    translation = Float3.Transform(delta, m_GizmoWorld.Orientation);
                }
                else
                {
                    // Flax applies additive scale deltas to each selected object's local scale.
                    scale = delta;
                }
            }

            m_HasChange = false;
            for (int i = 0; i < SelectionCount; i++)
            {
                Transform start = m_StartTransforms[i];
                Transform value = start;
                if (ActiveMode == Mode.Rotate && rotation != Quaternion.Identity)
                {
                    value.Orientation = Quaternion.Normalize(rotation * start.Orientation);
                    if (ActivePivot != PivotType.ObjectCenter)
                    {
                        value.Translation = Position + Float3.Transform(start.Translation - Position, rotation);
                    }
                }
                value.Translation += translation;
                if (scale != Float3.Zero)
                {
                    value.Scale = Float3.Clamp(value.Scale + scale, new Float3(-99_999_999.0f), new Float3(99_999_999.0f));
                }
                m_HasChange |= value != start;
                SetSelectedTransform(i, value);
            }
            m_LastTranslation = translation;
        }

        private Float3 GetAxisDirection(Axis axis)
        {
            Float3 direction = axis == Axis.X ? Float3.UnitX : axis == Axis.Y ? Float3.UnitY : Float3.UnitZ;
            return Float3.Transform(direction, m_GizmoWorld.Orientation);
        }

        private static bool IntersectPlane(Ray ray, Float3 position, Float3 normal, out Float3 point)
        {
            float denominator = Float3.Dot(ray.Direction, normal);
            if (Mathf.Abs(denominator) < 0.00001f)
            {
                point = Float3.Zero;
                return false;
            }
            float distance = Float3.Dot(position - ray.Position, normal) / denominator;
            point = ray.GetPoint(distance);
            return distance >= 0.0f;
        }

        public override void OnDeactivated()
        {
            CancelTransforming();
            ActiveAxis = Axis.None;
        }

        public override void Destroy()
        {
            CancelTransforming();
            base.Destroy();
        }
    }
}

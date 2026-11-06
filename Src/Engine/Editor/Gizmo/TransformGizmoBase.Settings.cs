
using System;

namespace SE.Editor.Gizmo
{
    public abstract partial class TransformGizmoBase
    {
        private Mode m_ActiveMode;
        private TransformSpace m_ActiveTransformSpace = TransformSpace.World;
        private PivotType m_ActivePivot = PivotType.SelectionCenter;

        public event Action ModeChanged;
        public event Action TransformSpaceChanged;
        public event Action PivotChanged;

        public bool TranslationSnapEnable;
        public bool RotationSnapEnabled;
        public bool ScaleSnapEnabled;
        public float TranslationSnapValue = 10.0f;
        public float RotationSnapValue = 15.0f;
        public float ScaleSnapValue = 0.1f;

        public Axis ActiveAxis { get; protected set; }

        public Mode ActiveMode
        {
            get => m_ActiveMode;
            set
            {
                if (m_ActiveMode != value)
                {
                    CancelTransforming();
                    m_ActiveMode = value;
                    ActiveAxis = Axis.None;
                    ModeChanged?.Invoke();
                }
            }
        }

        public TransformSpace ActiveTransformSpace
        {
            get => m_ActiveTransformSpace;
            set
            {
                if (m_ActiveTransformSpace != value)
                {
                    CancelTransforming();
                    m_ActiveTransformSpace = value;
                    ActiveAxis = Axis.None;
                    TransformSpaceChanged?.Invoke();
                }
            }
        }

        public PivotType ActivePivot
        {
            get => m_ActivePivot;
            set
            {
                if (m_ActivePivot != value)
                {
                    CancelTransforming();
                    m_ActivePivot = value;
                    ActiveAxis = Axis.None;
                    PivotChanged?.Invoke();
                }
            }
        }

        public void ToggleTransformSpace()
        {
            ActiveTransformSpace = ActiveTransformSpace == TransformSpace.World ? TransformSpace.Local : TransformSpace.World;
        }
    }
}

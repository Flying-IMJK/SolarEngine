
using System.Collections.Generic;
using SE.Editor.GUI;
using SE.Editor.Gizmo;
using SE.Editor.SceneGraph;
using SE.GUI;

namespace SE.Editor
{
    /// <summary>
    /// Editor viewport with scene gizmos.
    /// </summary>
    public partial class EditorGizmoViewport : EditorViewport, IGizmoOwner
    {
        private readonly SceneModule m_Scene;
        private Float2 m_ClickPosition;
        private bool m_CanPick;
        private bool m_WaitForGizmoMouseRelease;

        public GizmosCollection Gizmos { get; }
        public TransformGizmo TransformGizmo { get; }
        public GizmoRenderer Renderer { get; }
        public RootGraphNode SceneGraphRoot => m_Scene.Root;
        public SceneRenderTask RenderTask => m_Task;
        public Float2 MousePosition => m_ViewMousePos;
        public bool IsControlDown => Root?.GetKey(KeyboardKeys.Control) ?? false;
        public bool UseSnapping => IsControlDown;
        EditorGizmoViewport IGizmoOwner.Viewport => this;

        public bool IsPointerOverScene => m_ViewMousePos.X >= 0.0f && m_ViewMousePos.Y >= 0.0f &&
            m_ViewMousePos.X < Width && m_ViewMousePos.Y < Height &&
            GetChildAt(m_ViewMousePos, control => control.Visible) == null;

        public Ray MouseRay
        {
            get
            {
                CreateViewMatrix(ViewPosition, out Matrix view);
                CreateProjectionMatrix(out Matrix projection);
                Matrix vp = view * projection;
                SE.Viewport viewport = new SE.Viewport(0, 0, Width, Height);
                return Ray.GetPickRay(m_ViewMousePos.X, m_ViewMousePos.Y, ref viewport, ref vp);
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="EditorGizmoViewport"/> class.
        /// </summary>
        /// <param name="task">The task.</param>
        /// <param name="scene">The scene and selection owner.</param>
        /// <param name="toolStrip">Window-owned toolbar populated by this viewport.</param>
        public EditorGizmoViewport(SceneRenderTask task, SceneModule scene, ToolStrip toolStrip)
            : base(task, new FPSCamera(), true)
        {
            m_Scene = scene;
            Renderer = new GizmoRenderer();
            // Renderer.TransformRestored += OnTransformRestored;
            Gizmos = new GizmosCollection(this);
            TransformGizmo = new TransformGizmo(this);
            Gizmos.AddMode(new TransformGizmoMode());
            Gizmos.SetActiveMode<TransformGizmoMode>();

            m_Scene.SelectionChanged += OnSelectionChanged;
            m_Task.End += OnDrawGizmos;
            OnSelectionChanged();

            BuildToolStrip(toolStrip);
            TransformGizmo.ModeChanged += UpdateToolStrip;
            TransformGizmo.TransformSpaceChanged += UpdateToolStrip;
            TransformGizmo.PivotChanged += UpdateToolStrip;
            TransformGizmo.TransformingEnded += OnTransformingEnded;
        }

        private void CyclePivot()
        {
            TransformGizmo.ActivePivot = TransformGizmo.ActivePivot == TransformGizmoBase.PivotType.SelectionCenter
                ? TransformGizmoBase.PivotType.ObjectCenter
                : TransformGizmo.ActivePivot == TransformGizmoBase.PivotType.ObjectCenter
                    ? TransformGizmoBase.PivotType.WorldOrigin : TransformGizmoBase.PivotType.SelectionCenter;
        }

        public void Select(List<ScenesGraphNode> nodes)
        {
            m_Scene.Select(nodes);
        }

        private void OnSelectionChanged()
        {
            m_CanPick = false;
            foreach (GizmoBase gizmo in Gizmos)
            {
                gizmo.OnSelectionChanged(m_Scene.Selection);
            }
        }

        public override void Update(float deltaTime)
        {
            if (Root is not WindowRootControl root)
            {
                return;
            }
            if (root.Window != null && !root.Window.GetMouseButton(MouseButton.Left))
            {
                m_WaitForGizmoMouseRelease = false;
            }
            base.Update(deltaTime);
            foreach (GizmoBase gizmo in Gizmos)
            {
                gizmo.Update(deltaTime);
            }
            UpdateToolStrip();
        }

        protected override bool GetIsControllingMouse() => Gizmos.Active?.IsControllingMouse ?? false;

        protected override void OnLeftMouseButtonDown()
        {
            base.OnLeftMouseButtonDown();
            m_ClickPosition = m_ViewMousePos;
            m_CanPick = !m_WaitForGizmoMouseRelease && !m_Input.GetIsControllingMouse() && IsPointerOverScene;
            if (m_CanPick && Gizmos.Active == TransformGizmo && TransformGizmo.StartTransforming())
            {
                m_CanPick = false;
            }
        }

        protected override void OnLeftMouseButtonUp()
        {
            if (TransformGizmo.IsTransforming)
            {
                TransformGizmo.Update(0.0f);
                TransformGizmo.EndTransforming();
            }
            else if (m_CanPick && (m_ViewMousePos - m_ClickPosition).LengthSquared <= 16.0f)
            {
                Gizmos.Active?.Pick();
            }
            m_CanPick = false;
            base.OnLeftMouseButtonUp();
        }

        protected override void OnControlMouseBegin(Window window)
        {
            m_CanPick = false;
            TransformGizmo.CancelTransforming();
            base.OnControlMouseBegin(window);
        }

        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (key == KeyboardKeys.Escape && TransformGizmo.IsTransforming)
            {
                TransformGizmo.CancelTransforming();
                return true;
            }
            if (!m_Input.GetIsControllingMouse() && !IsControlDown)
            {
                switch (key)
                {
                    case KeyboardKeys.W: TransformGizmo.ActiveMode = TransformGizmoBase.Mode.Translate; return true;
                    case KeyboardKeys.E: TransformGizmo.ActiveMode = TransformGizmoBase.Mode.Rotate; return true;
                    case KeyboardKeys.R: TransformGizmo.ActiveMode = TransformGizmoBase.Mode.Scale; return true;
                    case KeyboardKeys.X: TransformGizmo.ToggleTransformSpace(); return true;
                    case KeyboardKeys.P: CyclePivot(); return true;
                }
            }
            // UndoStack keyboard integration is intentionally disabled.
            // if (!TransformGizmo.IsTransforming && IsControlDown && key == KeyboardKeys.Z) { Renderer.Undo(); return true; }
            // if (!TransformGizmo.IsTransforming && IsControlDown && key == KeyboardKeys.Y) { Renderer.Redo(); return true; }
            return base.OnKeyDown(key);
        }

        // private void OnTransformRestored(Actor actor)
        // {
        //     SceneGraphNode scene = m_Scene.GetActorNode(actor)?.ParentScene;
        //     if (scene != null)
        //     {
        //         scene.IsEdited = true;
        //     }
        // }

        public override void OnLostFocus()
        {
            m_CanPick = false;
            TransformGizmo.CancelTransforming();
            base.OnLostFocus();
        }

        public override void OnEndMouseCapture()
        {
            m_CanPick = false;
            TransformGizmo.CancelTransforming();
            base.OnEndMouseCapture();
        }

        private void OnDrawGizmos(RenderTask task, GPUContext context)
        {
            foreach (GizmoBase gizmo in Gizmos)
            {
                if (gizmo.Visible)
                {
                    gizmo.Draw(context);
                }
            }
        }

        private void OnTransformingEnded()
        {
            // A cancelled drag must not restart while its original mouse button is still held.
            m_WaitForGizmoMouseRelease = Root is WindowRootControl root && root.Window != null && root.Window.GetMouseButton(MouseButton.Left);
        }

        public override void OnDestroy()
        {
            if (IsDisposed)
            {
                return;
            }
            TransformGizmo.ModeChanged -= UpdateToolStrip;
            TransformGizmo.TransformSpaceChanged -= UpdateToolStrip;
            TransformGizmo.PivotChanged -= UpdateToolStrip;
            TransformGizmo.TransformingEnded -= OnTransformingEnded;
            DestroyToolStripItems();
            m_Scene.SelectionChanged -= OnSelectionChanged;
            m_Task.End -= OnDrawGizmos;
            // Renderer.TransformRestored -= OnTransformRestored;
            Gizmos.Clear();
            Object.DestroyNow(Renderer);
            base.OnDestroy();
        }
    }
}

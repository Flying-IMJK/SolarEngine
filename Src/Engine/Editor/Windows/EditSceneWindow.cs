using System.Collections.Generic;
using SE.GUI;

namespace SE.Editor
{
    /// <summary>
    /// Managed scene editing window.
    /// </summary>
    public sealed class EditSceneWindow : SceneEditorWindow
    {
        /// <summary>
        /// Camera preview output control.
        /// </summary>
        public sealed class CameraPreview : GUI.RenderOutputViewport
        {
            private bool m_IsPinned;
            private Button m_PinButton;

            public bool IsPinned
            {
                get => m_IsPinned;
                set
                {
                    if (m_IsPinned != value)
                    {
                        m_IsPinned = value;
                        UpdatePinButton();
                    }
                }
            }

            public Camera Camera
            {
                get => null; // m_Task.Camera;
                set { /* m_Task.Camera = value; */ }
            }

            public CameraPreview() : base(new SceneRenderTask())
            {
                AutoFocus = false;

                const float pinSize = 12.0f;
                const float pinMargin = 2.0f;
                m_PinButton = new Button
                {
                    AnchorPreset = AnchorPresets.TopRight,
                    Parent = this,
                    Bounds = new Rectangle(Width - pinSize - pinMargin, pinMargin, pinSize, pinSize)
                };
                m_PinButton.Clicked += () => IsPinned = !IsPinned;
                UpdatePinButton();

                m_Task.Begin += OnTaskBegin;
            }

            private void OnTaskBegin(RenderTask task, GPUContext context)
            {
                SceneRenderTask sceneTask = (SceneRenderTask)task;
                // Copy camera view parameters for the scene rendering
                RenderView view = sceneTask.View;
                Float2 size = sceneTask.GetBuffersSize();
                SE.Viewport viewport = new SE.Viewport(0, 0, size.X, size.Y);
                view.CopyFrom(Camera, ref viewport);
                sceneTask.View = view;
            }

            private void UpdatePinButton()
            {
                if (m_IsPinned)
                {
                    m_PinButton.Text = "-";
                    m_PinButton.TooltipText = "Unpin preview";
                }
                else
                {
                    m_PinButton.Text = "+";
                    m_PinButton.TooltipText = "Pin preview";
                }
            }

            public override void Draw()
            {
                base.Draw();
                Render2D.DrawRectangle(new Rectangle(Float2.Zero, Size), Color.Black);
            }

            public override void OnDestroy()
            {
                IsPinned = false;
                Camera = null;
                m_PinButton = null;
                base.OnDestroy();
            }
        }

        private readonly List<CameraPreview> m_Previews = new();
        private Actor m_PilotActor;
        private BoundingBox m_PilotBounds;
        private Transform m_PilotStart;

        public EditorGizmoViewport Viewport;

        public EditSceneWindow(Editor editor) : base(editor)
        {
            Title = "Scene";
            Icon = EditorIcons.SceneWindow32;

            Viewport = new EditorGizmoViewport(new SceneRenderTask(), editor.Scene.Root);
            Viewport.Parent = this;
            Viewport.NearPlane = 0.01f;
            Viewport.FarPlane = 20000.0f;
            // Viewport.GetTask().ViewFlags = ViewFlags.DefaultEditor;
            // editor.Scene.SelectionChanged += OnSelectionChanged;
            // editor.Scene.ActorRemoved += SceneOnActorRemoved;
        }

        public override void OnEditorStateChanged()
        {
            base.OnEditorStateChanged();
            UpdateCameraPreview();
        }

        public override void OnExit()
        {
            Viewport.Dispose();
        }

        private void OnSelectionChanged()
        {
            UpdateCameraPreview();
        }

        private void UpdateCameraPreview()
        {
            /*// Disable rendering preview during GI baking
            if (Editor.StateMachine.CurrentState.IsPerformanceHeavy)
            {
                HideAllCameraPreviews();
                return;
            }

            var selection = Editor.SceneEditing.Selection;

            // Hide unpinned previews for which camera being previews is not selected
            for (int i = 0; i < _previews.Count; i++)
            {
                if (_previews[i].IsPinned)
                    continue;
                var camera = _previews[i].Camera;
                var cameraNode = Editor.Scene.GetActorNode(camera);
                if (cameraNode == null || !selection.Contains(cameraNode))
                {
                    HideCameraPreview(_previews[i--]);
                }
            }

            if (Editor.Options.Options.Interface.ShowSelectedCameraPreview)
            {
                for (int i = 0; i < selection.Count; i++)
                {
                    if (selection[i] is CameraNode cameraNode)
                    {
                        if (_previews.Count >= 8)
                            break;
                        var camera = (Camera)cameraNode.Actor;
                        var preview = _previews.Find(x => x.Camera == camera);
                        if (preview == null)
                        {
                            preview = new CameraPreview { Camera = camera, Parent = this };
                            _previews.Add(preview);
                        }
                    }
                }
            }

            int count = _previews.Count;
            if (count > 0)
            {
                const float aspectRatio = 16.0f / 9.0f;
                const float minHeight = 20;
                const float minWidth = minHeight * aspectRatio;
                const float maxHeight = 150;
                const float maxWidth = maxHeight * aspectRatio;
                const float viewSpaceMaxPercentage = 0.7f;
                const float margin = 10;
                var totalSize = Size * viewSpaceMaxPercentage - margin;
                var singleSize = totalSize / count - count * margin;
                float sizeX = Mathf.Clamp(singleSize.X, minWidth, maxWidth);
                float sizeY = sizeX / aspectRatio;
                singleSize = new Float2(sizeX, sizeY);
                int countPerX = Mathf.FloorToInt(totalSize.X / singleSize.X);
                int countPerY = Mathf.FloorToInt(totalSize.Y / singleSize.Y);
                int index = 0;
                for (int y = 1; y <= countPerY; y++)
                {
                    for (int x = 1; x <= countPerX; x++)
                    {
                        if (index == count)
                            break;
                        var pos = Size - (singleSize + margin) * new Float2(x, y);
                        _previews[index++].Bounds = new Rectangle(pos, singleSize);
                    }
                    if (index == count)
                        break;
                }
            }*/
        }
    }
}

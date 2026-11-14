using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Renders a model in an isolated viewport with a directional light.
    /// The preview owns its task and actors; the model asset is borrowed.
    /// </summary>
    public class ModelPreview : EditorViewport
    {
        private StaticModel m_PreviewModel;
        private DirectionalLight m_PreviewLight;
        private ContextMenuButton m_ScaleToFitButton;
        private bool m_ScaleToFit = true;
        private bool m_FrameModel;

        /// <summary>
        /// Normalizes the preview instance to a longest side of 50 and centers it at the origin.
        /// Disabling this restores the instance's identity transform; the asset is never modified.
        /// </summary>
        public bool ScaleToFit
        {
            get => m_ScaleToFit;
            set
            {
                if (m_ScaleToFit == value)
                {
                    return;
                }
                m_ScaleToFit = value;
                if (m_ScaleToFitButton != null)
                {
                    m_ScaleToFitButton.Checked = value;
                }
                m_FrameModel = Model != null;
                if (m_FrameModel)
                {
                    ResetCamera();
                }
            }
        }

        public Model Model
        {
            get => m_PreviewModel.Model;
            set
            {
                if (m_PreviewModel.Model == value)
                {
                    return;
                }
                m_PreviewModel.Model = value;
                m_FrameModel = value != null;
                m_Task.IsCameraCut = true;
            }
        }

        public ModelPreview(bool useWidgets) : base(new SceneRenderTask(), new FPSCamera(), useWidgets)
        {
            m_Task.Enabled = false;
            m_Task.ActorsSource = ActorsSources.CustomActors;
            BackgroundColor = Color.Black;
            NearPlane = 0.01f;
            FarPlane = 1000.0f;
            RelativePanning = true;

            m_PreviewModel = Object.New<StaticModel>();
            m_PreviewLight = Object.New<DirectionalLight>();
            m_PreviewLight.Brightness = 8.0f;
            m_PreviewLight.ShadowsMode = ShadowsCastingMode.None;
            m_PreviewLight.Orientation = Quaternion.Euler(new Float3(52.1477f, -109.109f, -111.739f));

            m_Task.AddCustomActor(m_PreviewModel);
            m_Task.AddCustomActor(m_PreviewLight);
            m_Task.Begin += OnPreviewRenderBegin;

            if (useWidgets)
            {
                ContextMenu menu = _cameraButton.ContextMenu;
                menu.AddSeparator();
                menu.AddButton("Frame Model", "F", ResetCamera);
                m_ScaleToFitButton = menu.AddButton("Scale to Fit", () => ScaleToFit = !ScaleToFit);
                m_ScaleToFitButton.Checked = ScaleToFit;
                m_ScaleToFitButton.CloseMenuOnClick = false;
            }
        }

        /// <summary>
        /// Applies the preview size mode and frames the loaded model without waiting for streaming.
        /// </summary>
        public void ResetCamera()
        {
            Model model = Model;
            if (model == null || !model.IsLoaded() || model.GetLODsCount() == 0 || Width <= 0.0f || Height <= 0.0f)
            {
                return;
            }

            BoundingBox box = model.GetBox();
            float maxSize = box.Size.MaxValue;
            float scale = m_ScaleToFit && maxSize > 0.0f ? 50.0f / maxSize : 1.0f;
            Float3 position = m_ScaleToFit ? box.Center * -scale : Float3.Zero;
            m_PreviewModel.Transform = new Transform(position, Quaternion.Identity, new Float3(scale));

            BoundingSphere sphere = BoundingSphere.FromBox(m_PreviewModel.Box);
            sphere.Radius = Mathf.Max(0.000001f, sphere.Radius);
            float halfFov = FieldOfView * Mathf.DegreesToRadians * 0.5f;
            float horizontalHalfFov = Mathf.Atan(Mathf.Tan(halfFov) * Width / Height);
            float distance = sphere.Radius * 1.1f / Mathf.Sin(Mathf.Min(halfFov, horizontalHalfFov));
            // Keep the light bounds inside the view when the asset is authored away from its origin.
            m_PreviewLight.Position = sphere.Center;
            NearPlane = Mathf.Max(0.0000001f, Mathf.Min(10.0f, sphere.Radius * 0.25f));
            FarPlane = Mathf.Max(1000.0f, distance + sphere.Radius * 2.0f);
            if (UseOrthographicProjection)
            {
                OrthographicScale = sphere.Radius * 2.2f / Mathf.Min(Width, Height);
            }
            ViewportCamera.SetArcBallView(sphere, distance / sphere.Radius);
            m_Task.IsCameraCut = true;
            m_FrameModel = false;
        }

        public override void Update(float deltaTime)
        {
            if (IsDisposing)
            {
                return;
            }
            if (m_FrameModel)
            {
                ResetCamera();
            }

            // Offscreen thumbnails have no window input; a mounted preview uses the existing FPS controls.
            if (Root is WindowRootControl)
            {
                base.Update(deltaTime);
            }
        }

        protected override void UpdateView(float dt, in Float3 moveDelta, in Float2 mouseDelta, ref bool centerMouse)
        {
            FPSCamera camera = (FPSCamera)ViewportCamera;
            float orbitRadius = Float3.Distance(ViewPosition, camera.TargetPoint);
            base.UpdateView(dt, in moveDelta, in mouseDelta, ref centerMouse);
            if (m_Input.IsOrbiting && orbitRadius > 0.0f)
            {
                // FPSCamera's absolute minimum radius must not enlarge an original-size tiny model's orbit.
                ViewPosition = camera.TargetPoint - ViewDirection * orbitRadius;
            }
        }

        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (base.OnKeyDown(key))
            {
                return true;
            }
            if (key == KeyboardKeys.F)
            {
                ResetCamera();
                return true;
            }
            return false;
        }

        public override bool OnMouseWheel(Float2 location, float delta)
        {
            // Consume preview zoom here: the base handler reserves RMB + wheel for movement-speed changes.
            Focus();
            if (m_FrameModel)
            {
                ResetCamera();
            }
            Model model = Model;
            if (model == null || !model.IsLoaded() || m_FrameModel)
            {
                return true;
            }

            float radius = Mathf.Max(0.000001f, m_PreviewModel.Sphere.Radius);
            float zoom = Mathf.Pow(0.8f, delta);
            if (UseOrthographicProjection)
            {
                float minimumScale = radius * 0.0001f / Mathf.Max(1.0f, Mathf.Max(Width, Height));
                OrthographicScale = Mathf.Max(minimumScale, OrthographicScale * zoom);
                return true;
            }

            FPSCamera camera = (FPSCamera)ViewportCamera;
            Float3 offset = ViewPosition - camera.TargetPoint;
            float distance = offset.Length;
            float newDistance = Mathf.Max(radius * 0.01f, distance * zoom);
            ViewPosition = distance > 0.0f
                ? camera.TargetPoint + offset * (newDistance / distance)
                : camera.TargetPoint - ViewDirection * newDistance;
            NearPlane = Mathf.Max(0.0000001f, Mathf.Min(10.0f, Mathf.Min(radius, newDistance) * 0.25f));
            FarPlane = Mathf.Max(1000.0f, newDistance + radius * 2.0f);
            return true;
        }

        private void OnPreviewRenderBegin(RenderTask task, GPUContext context)
        {
            // Directional lighting is position-independent; keep its point bounds visible after panning or rotating.
            float distance = NearPlane + (FarPlane - NearPlane) * 0.5f;
            m_PreviewLight.Position = ViewPosition + ViewDirection * distance;
        }

        public override bool GetHasLoadedAssets()
        {
            return base.GetHasLoadedAssets() && m_PreviewModel.HasContentLoaded();
        }

        protected override bool CanSkipRendering()
        {
            Model model = Model;
            return base.CanSkipRendering() || model == null || !model.IsLoaded() || model.GetLoadedLODs() == 0 || !GetHasLoadedAssets();
        }

        public override void OnDestroy()
        {
            if (IsDisposing)
            {
                return;
            }

            m_Task.Enabled = false;
            m_Task.Begin -= OnPreviewRenderBegin;
            m_Task.ClearCustomActors();
            ContextMenu menu = _cameraButton?.ContextMenu;
            menu?.Hide();

            // Destroy the task and its cached draw references before releasing the actors.
            base.OnDestroy();
            menu?.Dispose();
            m_ScaleToFitButton = null;
            m_PreviewModel.Model = null;
            Object.DestroyNow(m_PreviewModel);
            Object.DestroyNow(m_PreviewLight);
            m_PreviewModel = null;
            m_PreviewLight = null;
        }
    }
}

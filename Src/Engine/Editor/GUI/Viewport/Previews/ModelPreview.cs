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
                m_Task.IsCameraCut = true;
            }
        }

        public ModelPreview(bool useWidgets)
            : base(new SceneRenderTask(), new FPSCamera(), useWidgets)
        {
            m_Task.Enabled = false;
            m_Task.ActorsSource = ActorsSources.CustomActors;
            BackgroundColor = Color.Black;
            NearPlane = 0.01f;
            FarPlane = 1000.0f;

            m_PreviewModel = new StaticModel();
            m_PreviewLight = new DirectionalLight
            {
                Brightness = 8.0f,
                ShadowsMode = ShadowsCastingMode.None,
                Orientation = Quaternion.Euler(new Float3(52.1477f, -109.109f, -111.739f)),
            };
            m_Task.AddCustomActor(m_PreviewModel);
            m_Task.AddCustomActor(m_PreviewLight);
        }

        /// <summary>
        /// Frames the loaded model without changing its transform or waiting for streaming.
        /// </summary>
        public void ResetCamera()
        {
            Model model = Model;
            if (model == null || !model.IsLoaded() || model.GetLODsCount() == 0)
            {
                return;
            }

            BoundingSphere sphere = BoundingSphere.FromBox(model.GetBox());
            sphere.Radius = Mathf.Max(0.001f, sphere.Radius);
            // Keep the light bounds inside the view when the asset is authored away from its origin.
            m_PreviewLight.Position = sphere.Center;
            NearPlane = Mathf.Min(10.0f, sphere.Radius * 0.25f);
            FarPlane = Mathf.Max(1000.0f, sphere.Radius * 4.0f + 100.0f);
            ViewportCamera.SetArcBallView(sphere);
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
            m_Task.ClearCustomActors();

            // Destroy the task and its cached draw references before releasing the actors.
            base.OnDestroy();
            m_PreviewModel.Model = null;
            Object.DestroyNow(m_PreviewModel);
            Object.DestroyNow(m_PreviewLight);
            m_PreviewModel = null;
            m_PreviewLight = null;
        }
    }
}

namespace SE.Editor.GUI
{
    public sealed class FloatWindowDockPanel : DockPanel
    {
        private SE.GUI.WindowRootControl? _hostRoot;

        public FloatWindowDockPanel(MasterDockPanel masterPanel)
            : base(null)
        {
            MasterPanel = masterPanel;
        }

        public MasterDockPanel MasterPanel { get; }
        public bool IsDragging { get; private set; }
        public override bool IsFloating => true;
        public SE.Window? HostWindow { get; private set; }

        public void BeginDrag()
        {
            IsDragging = true;
        }

        public void EndDrag()
        {
            IsDragging = false;
        }

        public override DockState TryGetDockState(out float splitterValue)
        {
            splitterValue = DefaultSplitterValue;
            return DockState.Float;
        }

        internal void AttachHostWindow(SE.Window window)
        {
            HostWindow = window;
            _hostRoot = window.GUI;
            _hostRoot.SizeChanged += OnHostRootSizeChanged;
        }

        protected override void OnLastTabRemoved()
        {
            MasterPanel.FloatingPanels.Remove(this);
            if (HostWindow != null)
                HostWindow.Close();
            else
                Dispose();
        }

        protected override void OnSelectedTabChanged()
        {
            if (SelectedTab != null)
            {
                SelectedTab.Visible = true;
                if (HostWindow != null)
                    HostWindow.Title = SelectedTab.Title;
            }
        }

        protected override void OnDispose()
        {
            if (_hostRoot != null)
            {
                _hostRoot.SizeChanged -= OnHostRootSizeChanged;
                _hostRoot = null;
            }

            MasterPanel.FloatingPanels.Remove(this);
            base.OnDispose();
        }

        private void OnHostRootSizeChanged(SE.GUI.Control root)
        {
            SetBounds(0.0f, 0.0f, root.Width, root.Height);
        }
    }
}

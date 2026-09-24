namespace SE.Editor.GUI
{
    public sealed class FloatWindowDockPanel : DockPanel
    {
        private SE.GUI.WindowRootControl? m_HostRoot;

        public FloatWindowDockPanel(MasterDockPanel masterPanel)
            : base(null)
        {
            MasterPanel = masterPanel;
        }

        public FloatWindowDockPanel(MasterDockPanel masterPanel, SE.GUI.WindowRootControl window)
            : this(masterPanel)
        {
            MasterPanel.FloatingPanels.Add(this);
            SE.Window host = window.Window;
            HostWindow = host;
            m_HostRoot = window;
            window.AddChild(this);
            m_HostRoot.SizeChanged += OnHostRootSizeChanged;
            host.Closing += OnHostWindowClosing;
            host.LeftButtonHit = OnLeftButtonHit;
        }

        public MasterDockPanel MasterPanel { get; }
        public SE.GUI.WindowRootControl? Window => m_HostRoot;
        public override bool IsFloating => true;
        public SE.Window? HostWindow { get; private set; }

        public void BeginDrag()
        {
            if (HostWindow != null)
                DockHintWindow.Create(this);
        }

        public static SE.Window CreateFloatWindow(
            SE.GUI.RootControl? parent,
            Float2 location,
            Float2 size,
            WindowStartPosition startPosition,
            string title)
        {
            // Native code currently clears the candidate parent before creating the window.
            _ = parent;
            CreateWindowSettings settings = SE.Window.CreateDefaultSettings();
            settings.Title = title;
            settings.Size = size;
            settings.Position = location;
            settings.MinimumSize = Float2.One;
            settings.MaximumSize = Float2.Zero;
            settings.Fullscreen = false;
            settings.HasBorder = true;
            settings.SupportsTransparency = false;
            settings.ActivateWhenFirstShown = true;
            settings.AllowInput = true;
            settings.AllowMinimize = true;
            settings.AllowMaximize = true;
            settings.AllowDragAndDrop = true;
            settings.IsTopmost = false;
            settings.IsRegularWindow = true;
            settings.HasSizingFrame = true;
            settings.ShowAfterFirstPaint = false;
            settings.ShowInTaskbar = true;
            settings.StartPosition = (SE.WindowStartPosition)(int)startPosition;
            return SE.Window.Create(settings);
        }

        public override DockState TryGetDockState(out float splitterValue)
        {
            splitterValue = 0.5f;
            return DockState.Float;
        }

        internal void AttachHostWindow(SE.Window window)
        {
            HostWindow = window;
            m_HostRoot = window.GUI;
            m_HostRoot.SizeChanged += OnHostRootSizeChanged;
            window.Closing += OnHostWindowClosing;
            window.LeftButtonHit = OnLeftButtonHit;
        }

        protected override void OnLastTabRemoved()
        {
            if (HostWindow != null)
                HostWindow.Close();
        }

        protected override void OnSelectedTabChanged()
        {
            base.OnSelectedTabChanged();
            if (SelectedTab != null)
            {
                if (HostWindow != null)
                    HostWindow.Title = SelectedTab.Title;
            }
        }

        protected override void OnDispose()
        {
            if (m_HostRoot != null)
            {
                m_HostRoot.SizeChanged -= OnHostRootSizeChanged;
                m_HostRoot = null;
            }

            if (HostWindow != null)
            {
                HostWindow.Closing -= OnHostWindowClosing;
                HostWindow.LeftButtonHit = null;
                HostWindow = null;
            }

            MasterPanel.FloatingPanels.Remove(this);
            base.OnDispose();
        }

        private void OnHostRootSizeChanged(SE.GUI.Control root)
        {
            SetBounds(0.0f, 0.0f, root.Width, root.Height);
        }

        private bool OnLeftButtonHit(WindowHitCodes hit)
        {
            if (hit != WindowHitCodes.Caption)
                return false;

            BeginDrag();
            return true;
        }

        private void OnHostWindowClosing(SE.ClosingReason reason, ref bool cancel)
        {
            ClosingReason dockReason = (ClosingReason)(int)reason;
            while (TabsCount > 0)
            {
                if (!Tabs[0].Close(dockReason))
                {
                    cancel = true;
                    return;
                }
            }

            SE.Window? window = HostWindow;
            if (window != null)
            {
                window.Closing -= OnHostWindowClosing;
                window.LeftButtonHit = null;
            }
            HostWindow = null;
            Dispose();
        }
    }
}

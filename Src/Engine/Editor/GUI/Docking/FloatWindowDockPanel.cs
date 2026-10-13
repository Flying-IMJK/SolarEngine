#nullable disable annotations

using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Floating Window Dock Panel control.
    /// </summary>
    /// <seealso cref="DockPanel" />
    public class FloatWindowDockPanel : DockPanel
    {
        private MasterDockPanel _masterPanel;
        private WindowRootControl _window;

        private bool OnLeftButtonHit(WindowHitCodes hitTest)
        {
            if (hitTest == WindowHitCodes.Caption)
            {
                BeginDrag();
                return true;
            }
            else
            {
                return false;
            }
        }

        private void OnClosing(SE.ClosingReason reason, ref bool cancel)
        {
            // Close all docked windows
            while (Tabs.Count > 0)
            {
                if (!Tabs[0].Close((ClosingReason)(int)reason))
                {
                    // Cancel
                    cancel = true;
                    return;
                }
            }

            // Unlink
            _window.Window.Closing -= OnClosing;
            _window.Window.LeftButtonHit = null;
            _window = null;

            // Remove object
            SE.Debug.Assert(TabsCount == 0 && ChildPanelsCount == 0,
                "Floating dock panel still contains tabs or child panels.");
            Dispose();
        }

        /// <summary>
        /// Gets the master panel.
        /// </summary>
        public MasterDockPanel MasterPanel => _masterPanel;

        /// <summary>
        /// Gets the window.
        /// </summary>
        public WindowRootControl Window => _window;

        /// <summary>
        /// Initializes a new instance of the <see cref="FloatWindowDockPanel"/> class.
        /// </summary>
        /// <param name="masterPanel">The master panel.</param>
        /// <param name="window">The window.</param>
        public FloatWindowDockPanel(MasterDockPanel masterPanel, RootControl window) : base(null)
        {
            _masterPanel = masterPanel;
            _window = (WindowRootControl)window;

            // Link
            _masterPanel.FloatingPanels.Add(this);
            Parent = window;
            _window.Window.Closing += OnClosing;
            _window.Window.LeftButtonHit = OnLeftButtonHit;
        }

        /// <summary>
        /// Begin drag operation on the window.
        /// </summary>
        public void BeginDrag()
        {
            // Filter invalid events
            if (_window == null)
            {
                return;
            }

            // Create docking hint window
            DockHintWindow.Create(this);
        }

        /// <summary>
        /// Creates a floating window.
        /// </summary>
        /// <param name="parent">Source docking root; it does not own the floating window.</param>
        /// <param name="location">Client area location.</param>
        /// <param name="size">Window client area size.</param>
        /// <param name="startPosition">Window start position.</param>
        /// <param name="title">Initial window title.</param>
        public static SE.Window CreateFloatWindow(RootControl parent, Float2 location, Float2 size, WindowStartPosition startPosition, string title)
        {
            // Setup initial window settings
            CreateWindowSettings settings = SE.Window.CreateDefaultSettings();
            // Keep floating docks as independent top-level windows for taskbar activation.
            settings.Parent = null;

            settings.Title = title;
            settings.Size = size;
            settings.Position = location;
            settings.MinimumSize = Float2.One;
            settings.MaximumSize = Float2.Zero; // Unlimited size
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

            // Create window
            return SE.Window.Create(settings);
        }

        /// <inheritdoc />
        public override DockState TryGetDockState(out float splitterValue)
        {
            splitterValue = 0.5f;
            return DockState.Float;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (_masterPanel != null)
            {
                _masterPanel.FloatingPanels.Remove(this);
            }

            base.OnDestroy();
        }

        /// <inheritdoc />
        protected override void OnLastTabRemoved()
        {
            // Close window
            if (_window != null)
            {
                _window.Close();
            }
        }

        /// <inheritdoc />
        protected override void OnSelectedTabChanged()
        {
            base.OnSelectedTabChanged();

            if (_window != null && SelectedTab != null)
            {
                _window.SetTitle(SelectedTab.Title);
            }
        }

        /// <inheritdoc />
        public override bool IsFloating => true;
    }
}

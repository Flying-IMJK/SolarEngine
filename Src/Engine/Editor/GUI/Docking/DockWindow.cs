#nullable disable annotations

using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Dockable window UI control.
    /// </summary>
    public class DockWindow : Panel
    {
        private string m_Title = string.Empty;
        private Float2 m_TitleSize;

        /// <summary>
        /// The master panel.
        /// </summary>
        protected MasterDockPanel _masterPanel;

        /// <summary>
        /// The parent panel.
        /// </summary>
        protected DockPanel _dockedTo;

        /// <summary>
        /// Called when window is unlinked from the master panel.
        /// </summary>
        protected virtual void OnUnlink()
        {
            _masterPanel = null;
        }

        /// <summary>
        /// Undocks this window.
        /// </summary>
        protected virtual void Undock()
        {
            // Defocus itself
            if (ContainsFocus)
            {
                Focus();
            }
            Defocus();

            // Call undock
            if (_dockedTo != null)
            {
                _dockedTo.UndockWindowInternal(this);
                SE.Debug.Assert(_dockedTo == null, "Window is still docked.");
            }
        }

        /// <summary>
        /// Called when window is closing. Operation can be cancelled.
        /// </summary>
        /// <param name="reason">The reason.</param>
        /// <returns>True if cancel, otherwise false to allow.</returns>
        protected virtual bool OnClosing(ClosingReason reason)
        {
            // Allow
            return false;
        }

        /// <summary>
        /// Called when window is closed.
        /// </summary>
        protected virtual void OnClose()
        {
            // Nothing to do
        }

        /// <summary>
        /// Called when window shows.
        /// </summary>
        protected virtual void OnShow()
        {
            // Nothing to do
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            // Cache window title dimensions
            if (m_TitleSize.X <= 0)
            {
                Style style = Style.Current;
                if (style != null && style.FontMedium != null)
                {
                    m_TitleSize = style.FontMedium.MeasureText(m_Title, TextLayoutOptions.Default);
                }
            }

            base.PerformLayoutBeforeChildren();
        }

        /// <summary>
        /// Gets or sets a value indicating whether hide window on close.
        /// </summary>
        public bool HideOnClose;

        /// <summary>
        /// Gets the master panel.
        /// </summary>
        public MasterDockPanel MasterPanel => _masterPanel;

        /// <summary>
        /// Gets the parent dock panel.
        /// </summary>
        public DockPanel ParentDockPanel
        {
            get => _dockedTo;
            set => _dockedTo = value;
        }

        /// <summary>
        /// Gets a value indicating whether this window is docked.
        /// </summary>
        public bool IsDocked => _dockedTo != null;

        /// <summary>
        /// Gets a value indicating whether this window is selected.
        /// </summary>
        public bool IsSelected
        {
            get
            {
                if (_dockedTo != null)
                {
                    return _dockedTo.SelectedTab == this;
                }
                return false;
            }
        }

        /// <summary>
        /// Gets a value indicating whether this window is hidden from the user (eg. not shown or hidden on closed).
        /// </summary>
        public bool IsHidden => !Visible || _dockedTo == null;

        /// <summary>
        /// Gets the default window size (in UI units, unscaled by DPI which is handled by windowing system).
        /// </summary>
        public virtual Float2 DefaultSize => new Float2(900, 580);

        /// <summary>
        /// Gets the serialization typename.
        /// </summary>
        public virtual string SerializationTypename => "::" + GetType().Name;

        /// <summary>
        /// Gets or sets the window title.
        /// </summary>
        public string Title
        {
            get => m_Title;
            set
            {
                m_Title = value;

                // Invalidate cached title size
                m_TitleSize = new Float2(-1);
                PerformLayoutBeforeChildren();

                // Check if is docked to the floating window and is selected so update window title
                if (IsSelected && _dockedTo is FloatWindowDockPanel floatPanel)
                {
                    floatPanel.Window.SetTitle(Title);
                }
            }
        }

        /// <summary>
        /// Gets or sets the window icon.
        /// </summary>
        public SpriteHandle Icon;

        /// <summary>
        /// Gets the size of the title.
        /// </summary>
        public Float2 TitleSize => m_TitleSize;

        /// <summary>
        /// Initializes a new instance of the <see cref="DockWindow"/> class.
        /// </summary>
        /// <param name="masterPanel">The master docking panel.</param>
        /// <param name="hideOnClose">True if hide window on closing, otherwise it will be destroyed.</param>
        /// <param name="scrollBars">The scroll bars.</param>
        public DockWindow(MasterDockPanel masterPanel, bool hideOnClose, ScrollBars scrollBars) : base(scrollBars)
        {
            _masterPanel = masterPanel;
            HideOnClose = hideOnClose;
            AnchorPreset = AnchorPresets.StretchAll;
            Offsets = Margin.Zero;

            // Bind navigation shortcuts
            // Native InputActions registration is commented out.

            // Link to the master panel
            if (_masterPanel != null)
            {
                _masterPanel.LinkWindow(this);
            }
        }

        /// <summary>
        /// Shows the window in a floating state.
        /// </summary>
        public void ShowFloating()
        {
            ShowFloating(Float2.Zero);
        }

        /// <summary>
        /// Shows the window in a floating state.
        /// </summary>
        /// <param name="position">Window location.</param>
        public void ShowFloating(WindowStartPosition position)
        {
            ShowFloating(Float2.Zero, position);
        }

        /// <summary>
        /// Shows the window in a floating state.
        /// </summary>
        /// <param name="size">Window size, set <see cref="Float2.Zero"/> to use default.</param>
        /// <param name="position">Window location.</param>
        public void ShowFloating(Float2 size, WindowStartPosition position = WindowStartPosition.CenterParent)
        {
            ShowFloating(new Float2(200, 200), size, position);
        }

        /// <summary>
        /// Shows the window in a floating state.
        /// </summary>
        /// <param name="location">Window location.</param>
        /// <param name="size">Window size, set <see cref="Float2.Zero"/> to use default.</param>
        /// <param name="position">Window location.</param>
        public void ShowFloating(Float2 location, Float2 size, WindowStartPosition position = WindowStartPosition
            .CenterParent)
        {
            Undock();

            // Create window
            Float2 winSize = size.LengthSquared > 4 ? size : DefaultSize;
            SE.Window window = FloatWindowDockPanel.CreateFloatWindow(_masterPanel.Root, location, winSize,
                position, m_Title);
            WindowRootControl windowGUI = window.GUI;

            // Create dock panel for the window
            FloatWindowDockPanel dockPanel = new FloatWindowDockPanel(_masterPanel, windowGUI);
            dockPanel.DockWindowInternal(DockState.DockFill, this);

            // Perform layout
            Visible = true;
            windowGUI.UnlockChildrenRecursive();
            windowGUI.PerformLayout();

            // Show
            window.Show();
            window.BringToFront();
            window.Focus();
            OnShow();

            // Perform layout again
            windowGUI.PerformLayout();
        }

        /// <summary>
        /// Shows the window.
        /// </summary>
        /// <param name="state">Initial window state.</param>
        /// <param name="toDock">Panel to dock to it.</param>
        /// <param name="autoSelect"> Only used if <paramref name="toDock"/> is set. If true the window will be selected
        /// after docking it.</param>
        /// <param name="splitterValue"> Only used if <paramref name="toDock"/> is set. The splitter value to use. If
        /// not specified, a default value will be used.</param>
        public void Show(DockState state = DockState.Float, DockPanel toDock = null, bool autoSelect = true,
            float splitterValue = 0)
        {
            if (state == DockState.Hidden)
            {
                Hide();
            }
            else if (state == DockState.Float)
            {
                ShowFloating();
            }
            else
            {
                Visible = true;

                // Undock first
                Undock();

                // Then dock
                if (toDock != null)
                {
                    toDock.DockWindowInternal(state, this, autoSelect, splitterValue);
                }
                else
                {
                    _masterPanel.DockWindowInternal(state, this, autoSelect, splitterValue);
                }

                OnShow();
                PerformLayout();
            }
        }

        /// <summary>
        /// Shows the window.
        /// </summary>
        /// <param name="state">Initial window state.</param>
        /// <param name="toDock">Window to dock to it.</param>
        public void Show(DockState state, DockWindow toDock)
        {
            DockPanel panel = null;
            if (toDock != null)
            {
                panel = toDock.ParentDockPanel;
            }

            Show(state, panel);
        }

        /// <summary>
        /// Focuses or shows the window.
        /// </summary>
        public void FocusOrShow()
        {
            FocusOrShow(DockState.DockFill);
        }

        /// <summary>
        /// Focuses or shows the window.
        /// </summary>
        /// <param name="state">The state.</param>
        public void FocusOrShow(DockState state)
        {
            if (Visible)
            {
                SelectTab();
                Focus();
            }
            else
            {
                Show(state);
            }
        }

        /// <summary>
        /// Hides the window.
        /// </summary>
        public void Hide()
        {
            // Undock
            Undock();

            Visible = false;

            // Ensure that dock panel has no parent
            SE.Debug.Assert(HasParent, "Dock window has no parent.");
        }

        /// <summary>
        /// Closes the window.
        /// </summary>
        /// <param name="reason">Window closing reason.</param>
        /// <returns>True if action has been cancelled (due to window internal logic).</returns>
        public bool Close(ClosingReason reason = ClosingReason.CloseEvent)
        {
            // Fire events
            if (!OnClosing(reason))
            {
                return false;
            }
            OnClose();

            // Check if window should be hidden on close event
            if (HideOnClose)
            {
                // Hide
                Hide();
            }
            else
            {
                // Undock
                Undock();

                // Delete itself
                Dispose();
            }

            // Done
            return true;
        }

        /// <summary>
        /// Selects this tab page.
        /// </summary>
        /// <param name="autoFocus">True if focus tab after selection change.</param>
        public void SelectTab(bool autoFocus = true)
        {
            if (_dockedTo != null)
            {
                _dockedTo.SelectTab(this, autoFocus);
            }
        }

        /// <summary>
        /// Brings the window to the front of the Z order.
        /// </summary>
        public void BringToFront()
        {
            if (_dockedTo != null && _dockedTo.RootWindow() != null)
            {
                _dockedTo.RootWindow().Window.BringToFront();
            }
        }

        /// <summary>
        /// Gets a value indicating whether window uses custom layout data.
        /// </summary>
        public virtual bool UseLayoutData => false;

        /// <summary>
        /// Called when during windows layout deserialization if window has no layout data to load. Can be used to
        /// restore default UI layout.
        /// </summary>
        public virtual void OnLayoutDeserialize()
        {
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            // Auto undock from non-disposing parent (user wants to remove only the dock window)
            if (HasParent && !Parent.IsDisposing)
            {
                Undock();
            }

            // Unlink from the master panel
            if (_masterPanel != null)
            {
                _masterPanel.UnlinkWindow(this);
            }

            base.OnDestroy();
        }

        /// <inheritdoc />
        public override void Focus()
        {
            base.Focus();

            SelectTab();
            BringToFront();
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            // Base
            if (base.OnKeyDown(key))
            {
                return true;
            }

            // Custom input events
            return false;
        }

        /// <summary>
        /// Called when dock panel wants to show the context menu for this window. Can be used to inject custom buttons
        /// and items to the context menu (on top).
        /// </summary>
        /// <param name="menu">The menu.</param>
        public virtual void OnShowContextMenu(ContextMenu menu)
        {
        }

        internal void NotifyUnlinked()
        {
            OnUnlink();
        }
    }
}

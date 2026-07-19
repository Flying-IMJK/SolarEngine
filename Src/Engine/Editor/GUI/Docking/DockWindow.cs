using System;
using SE.Editor.GUI;
using SE.GUI;

namespace SE.Editor.GUI
{
    public enum ClosingReason
    {
        CloseEvent,
        User,
    }

    public enum ScrollBars
    {
        None,
        Horizontal,
        Vertical,
        Both,
    }

    public enum WindowStartPosition
    {
        Manual,
        CenterParent,
    }

    public class DockWindow : Panel
    {
        private string _title = string.Empty;
        private GuiSize _titleSize = GuiSize.Zero;

        public DockWindow(MasterDockPanel masterPanel, bool hideOnClose = true, ScrollBars scrollBars = ScrollBars.None)
            : base(new Rectangle(0, 0, 300, 200))
        {
            MasterPanel = masterPanel;
            HideOnClose = hideOnClose;
            ScrollBars = scrollBars;
            base.ScrollBars = scrollBars switch
            {
                ScrollBars.Horizontal => SE.GUI.ScrollBars.Horizontal,
                ScrollBars.Vertical => SE.GUI.ScrollBars.Vertical,
                ScrollBars.Both => SE.GUI.ScrollBars.Both,
                _ => SE.GUI.ScrollBars.None,
            };
            MasterPanel.LinkWindow(this);
        }

        public bool HideOnClose { get; set; }
        public MasterDockPanel MasterPanel { get; }
        public DockPanel? ParentDockPanel { get; internal set; }
        public bool IsDocked => ParentDockPanel != null;
        public bool IsSelected => ParentDockPanel?.SelectedTab == this;
        public bool IsHidden => !Visible || ParentDockPanel == null;
        public virtual GuiSize DefaultSize => new GuiSize(900, 580);
        public virtual string SerializationTypename => GetType().Name;
        /// <summary>
        /// Gets the docking-layer scrollbar configuration used to construct this window.
        /// </summary>
        public new ScrollBars ScrollBars { get; }

        public string Title
        {
            get => _title;
            set
            {
                _title = value;
                _titleSize = new GuiSize(value.Length * 7.0f, DockPanel.DefaultHeaderHeight);
            }
        }

        public GuiSize TitleSize => _titleSize;

        public void ShowFloating()
        {
            ShowFloating(GuiPoint.Zero, DefaultSize, WindowStartPosition.CenterParent);
        }

        public void ShowFloating(WindowStartPosition position)
        {
            ShowFloating(GuiPoint.Zero, DefaultSize, position);
        }

        public void ShowFloating(GuiSize size, WindowStartPosition position = WindowStartPosition.CenterParent)
        {
            ShowFloating(new GuiPoint(200, 200), size, position);
        }

        public void ShowFloating(GuiPoint location, GuiSize size, WindowStartPosition position = WindowStartPosition.CenterParent)
        {
            Undock();
            FloatWindowDockPanel floatingPanel = MasterPanel.CreateFloatingPanel(location, size, Title);
            floatingPanel.DockWindowInternal(DockState.Float, this);
            Visible = true;
            OnShow();
        }

        public void Show(DockState state = DockState.Float, DockPanel? toDock = null, bool autoSelect = true, float splitterValue = 0)
        {
            if (state == DockState.Hidden)
            {
                Hide();
                return;
            }
            if (state == DockState.Float)
            {
                ShowFloating();
                return;
            }

            Visible = true;
            Undock();
            DockPanel target = toDock ?? MasterPanel;
            target.DockWindowInternal(state, this, autoSelect, splitterValue);
            OnShow();
        }

        public void Show(DockState state, DockWindow toDock)
        {
            Show(state, toDock.ParentDockPanel);
        }

        public void FocusOrShow()
        {
            if (IsDocked)
                SelectTab();
            else
                Show();
        }

        public void FocusOrShow(DockState state)
        {
            if (IsDocked)
                SelectTab();
            else
                Show(state);
        }

        public void Hide()
        {
            Undock();
            Visible = false;
        }

        public bool Close(ClosingReason reason = ClosingReason.CloseEvent)
        {
            if (OnClosing(reason))
                return true;

            if (HideOnClose)
            {
                Hide();
            }
            else
            {
                ParentDockPanel?.UndockWindowInternal(this);
                MasterPanel.UnlinkWindow(this);
                Dispose();
            }

            OnClose();
            return false;
        }

        public void SelectTab(bool autoFocus = true)
        {
            ParentDockPanel?.SelectTab(this, autoFocus);
        }

        public void BringToFront()
        {
            SelectTab(false);
        }

        public virtual void Focus()
        {
            SelectTab(false);
        }

        public override bool OnKeyDown(int key)
        {
            return false;
        }

        public virtual void OnShowContextMenu(ContextMenu menu)
        {
        }

        public virtual void OnLayoutDeserialize()
        {
        }

        protected virtual void OnUnlink()
        {
        }

        protected virtual void Undock()
        {
            ParentDockPanel?.UndockWindowInternal(this);
        }

        protected virtual bool OnClosing(ClosingReason reason)
        {
            return false;
        }

        protected virtual void OnClose()
        {
        }

        protected virtual void OnShow()
        {
        }

        protected override void OnDispose()
        {
            if (IsDisposed)
                return;

            MasterPanel.UnlinkWindow(this);
            base.OnDispose();
        }

        internal void NotifyUnlinked()
        {
            OnUnlink();
        }
    }
}

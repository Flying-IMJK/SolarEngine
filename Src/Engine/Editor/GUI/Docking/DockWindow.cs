using System;
using SE.GUI;

namespace SE.Editor.GUI
{
    public enum ClosingReason
    {
        Unknown = 0,
        User = 1,
        EngineExit = 2,
        CloseEvent = 3,
    }

    public enum WindowStartPosition
    {
        CenterParent = 0,
        CenterScreen = 1,
        Manual = 2,
    }

    public class DockWindow : Panel
    {
        private MasterDockPanel? m_MasterPanel;
        private string m_Title = string.Empty;
        private Float2 m_TitleSize = new Float2(-1.0f);

        public DockWindow(MasterDockPanel masterPanel, bool hideOnClose = true, ScrollBars scrollBars = ScrollBars.None)
            : base(new Rectangle(0, 0, 300, 200))
        {
            AnchorMin = Float2.Zero;
            AnchorMax = Float2.One;
            Offsets = Margin.Zero;
            m_MasterPanel = masterPanel;
            HideOnClose = hideOnClose;
            ScrollBars = scrollBars;
            base.ScrollBars = scrollBars;
            masterPanel.LinkWindow(this);

            // Flax registers CloseTab, PreviousTab, and NextTab through Editor InputActions here.
            // Keep those bindings disabled until the managed Editor input-action API is available.
        }

        public bool HideOnClose { get; set; }
        public MasterDockPanel? MasterPanel => m_MasterPanel;
        public DockPanel? ParentDockPanel { get; internal set; }
        public bool IsDocked => ParentDockPanel != null;
        public bool IsSelected => ParentDockPanel?.SelectedTab == this;
        public bool IsHidden => !Visible || ParentDockPanel == null;
        public virtual Float2 DefaultSize => new Float2(900, 580);
        public virtual string SerializationTypename => "::" + GetType().Name;
        /// <summary>
        /// Gets the docking-layer scrollbar configuration used to construct this window.
        /// </summary>
        public new ScrollBars ScrollBars { get; }

        public string Title
        {
            get => m_Title;
            set
            {
                m_Title = value;
                m_TitleSize = new Float2(-1.0f);
                PerformLayout();
            }
        }

        public Float2 TitleSize => m_TitleSize;
        public SpriteHandle Icon;

        public void ShowFloating()
        {
            ShowFloating(Float2.Zero, DefaultSize, WindowStartPosition.CenterParent);
        }

        public void ShowFloating(WindowStartPosition position)
        {
            ShowFloating(Float2.Zero, DefaultSize, position);
        }

        public void ShowFloating(Float2 size, WindowStartPosition position = WindowStartPosition.CenterParent)
        {
            ShowFloating(new Float2(200, 200), size, position);
        }

        public void ShowFloating(Float2 location, Float2 size, WindowStartPosition position = WindowStartPosition.CenterParent)
        {
            Undock();
            Float2 windowSize = size.X * size.X + size.Y * size.Y > 4.0f ? size : DefaultSize;
            FloatWindowDockPanel floatingPanel = m_MasterPanel!.CreateFloatingPanel(location, windowSize, position, Title);
            floatingPanel.DockWindowInternal(DockState.DockFill, this);
            Visible = true;
            SE.Window? window = floatingPanel.HostWindow;
            if (window != null)
            {
                window.GUI.UnlockChildrenRecursive();
                window.GUI.PerformLayout();
                window.Show();
                window.BringToFront();
                window.Focus();
            }
            OnShow();
            window?.GUI.PerformLayout();
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
            DockPanel target = toDock ?? m_MasterPanel!;
            target.DockWindowInternal(state, this, autoSelect, splitterValue);
            OnShow();
            PerformLayout();
        }

        public void Show(DockState state, DockWindow? toDock)
        {
            Show(state, toDock?.ParentDockPanel);
        }

        public void FocusOrShow()
        {
            FocusOrShow(DockState.DockFill);
        }

        public void FocusOrShow(DockState state)
        {
            if (Visible)
            {
                SelectTab();
                Focus();
            }
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
            // Native active code treats false as cancellation despite the opposite header comment.
            if (!OnClosing(reason))
                return false;

            OnClose();

            if (HideOnClose)
            {
                Hide();
            }
            else
            {
                Undock();
                Dispose();
            }

            return true;
        }

        public void SelectTab(bool autoFocus = true)
        {
            ParentDockPanel?.SelectTab(this, autoFocus);
        }

        public void BringToFront()
        {
            if (Root is WindowRootControl root)
                root.Window.BringToFront();
        }

        public override void Focus()
        {
            base.Focus();
            SelectTab();
            BringToFront();
        }

        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (base.OnKeyDown(key))
                return true;

            // Flax forwards unhandled keys to InputActions.Process(Editor.Instance, this, key).
            // Keep the Editor shortcut route disabled; no replacement shortcut behavior is introduced.
            return false;
        }

        public override void PerformLayout(bool force = false)
        {
            if (m_TitleSize.X <= 0.0f)
            {
                Font? font = Style.Current.FontMedium;
                if (font != null)
                    m_TitleSize = font.MeasureText(m_Title, new TextLayoutOptions());
            }
            base.PerformLayout(force);
        }

        public virtual void OnShowContextMenu(ContextMenu menu)
        {
        }

        public virtual bool UseLayoutData => false;

        // Native XML persistence overloads remain disabled. The active no-argument hook below is a no-op,
        // matching the current C++ surface without introducing a managed layout format.
        /*
        public virtual void OnLayoutSerialize(System.Xml.XmlWriter writer)
        {
        }

        public virtual void OnLayoutDeserialize(System.Xml.XmlElement node)
        {
        }

        protected void LayoutSerializeSplitter(System.Xml.XmlWriter writer, string name, SplitPanel splitter)
        {
            writer.WriteAttributeString(name, splitter.SplitterValue.ToString(System.Globalization.CultureInfo.InvariantCulture));
        }

        protected void LayoutDeserializeSplitter(System.Xml.XmlElement node, string name, SplitPanel splitter)
        {
            if (float.TryParse(node.GetAttribute(name), System.Globalization.CultureInfo.InvariantCulture, out float value) && value > 0.01f && value < 0.99f)
                splitter.SplitterValue = value;
        }
        */

        public virtual void OnLayoutDeserialize()
        {
        }

        protected virtual void OnUnlink()
        {
            m_MasterPanel = null;
        }

        protected virtual void Undock()
        {
            if (ContainsFocus)
                Focus();
            Defocus();

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
            if (Parent != null && !Parent.IsDisposing)
                Undock();
            m_MasterPanel?.UnlinkWindow(this);
            base.OnDispose();
        }

        internal void NotifyUnlinked()
        {
            OnUnlink();
        }
    }
}

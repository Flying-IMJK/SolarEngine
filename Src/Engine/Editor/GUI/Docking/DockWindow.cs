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
        private string m_Title = string.Empty;
        private Float2 m_TitleSize = Float2.Zero;

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

            // Flax registers CloseTab, PreviousTab, and NextTab through Editor InputActions here.
            // Keep those bindings disabled until the managed Editor input-action API is available.
        }

        public bool HideOnClose { get; set; }
        public MasterDockPanel MasterPanel { get; }
        public DockPanel? ParentDockPanel { get; internal set; }
        public bool IsDocked => ParentDockPanel != null;
        public bool IsSelected => ParentDockPanel?.SelectedTab == this;
        public bool IsHidden => !Visible || ParentDockPanel == null;
        public virtual Float2 DefaultSize => new Float2(900, 580);
        public virtual string SerializationTypename => GetType().Name;
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
                m_TitleSize = new Float2(value.Length * 7.0f, DockPanel.DefaultHeaderHeight);
            }
        }

        public Float2 TitleSize => m_TitleSize;

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
            // Flax obtains the default dock state from Editor.Options.Interface.NewWindowLocation.
            // That Editor option is intentionally disabled until it is exposed to managed code.
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

        public override bool OnKeyDown(KeyboardKeys key)
        {
            // Flax forwards unhandled keys to InputActions.Process(Editor.Instance, this, key).
            // Keep the Editor shortcut route disabled; no replacement shortcut behavior is introduced.
            return false;
        }

        public virtual void OnShowContextMenu(ContextMenu menu)
        {
        }

        // Flax window-layout serialization is permitted for this migration, but must remain disabled
        // until its layout coordinator and XML persistence path are migrated as a single unit.
        // No replacement serialization format or compatibility layer is introduced here.
        /*
        public virtual string SerializationTypename => "::" + GetType().FullName;

        public virtual bool UseLayoutData => false;

        public virtual void OnLayoutSerialize(System.Xml.XmlWriter writer)
        {
        }

        public virtual void OnLayoutDeserialize(System.Xml.XmlElement node)
        {
        }

        public virtual void OnLayoutDeserialize()
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

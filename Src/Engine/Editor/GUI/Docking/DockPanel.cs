using System;
using System.Collections.Generic;
using SE.Editor.GUI;
using SE.GUI;

namespace SE.Editor.GUI
{
    public class DockPanel : ContainerControl
    {
        public const float DefaultHeaderHeight = 25.0f;
        public const float DefaultTextMargin = 2.0f;
        public const float DefaultButtonsSize = 12.0f;
        public const float DefaultButtonsMargin = 4.0f;
        public const float DefaultSplitterValue = 0.25f;

        private readonly List<DockPanel> m_ChildPanels = new List<DockPanel>();
        private readonly List<DockWindow> m_Tabs = new List<DockWindow>();
        private DockWindow? m_SelectedTab;
        private DockPanelProxy? m_TabsProxy;
        private DockState m_DockStateInParent = DockState.DockFill;
        private float m_SplitterValue = DefaultSplitterValue;
        private Rectangle m_TabAreaBounds;

        public DockPanel(DockPanel? parentPanel = null)
            : base(new Rectangle(0, 0, 300, 200))
        {
            ParentDockPanel = parentPanel;
            parentPanel?.m_ChildPanels.Add(this);
            SetBounds(0, 0, 300, 200);
        }

        public virtual bool IsMaster => false;
        public virtual bool IsFloating => false;
        public Rectangle DockAreaBounds => m_TabAreaBounds;
        public IReadOnlyList<DockPanel> ChildPanels => m_ChildPanels;
        public int ChildPanelsCount => m_ChildPanels.Count;
        public IReadOnlyList<DockWindow> Tabs => m_Tabs;
        public int TabsCount => m_Tabs.Count;
        public DockWindow? SelectedTab => m_SelectedTab;
        public DockWindow? FirstTab => m_Tabs.Count > 0 ? m_Tabs[0] : null;
        public DockWindow? LastTab => m_Tabs.Count > 0 ? m_Tabs[m_Tabs.Count - 1] : null;
        public DockPanel? ParentDockPanel { get; }
        public DockPanelProxy TabsProxy => m_TabsProxy ??= CreateTabsProxy();

        public int SelectedTabIndex
        {
            get => m_SelectedTab != null ? m_Tabs.IndexOf(m_SelectedTab) : -1;
            set => SelectTab(value);
        }

        public bool CloseAll(ClosingReason reason = ClosingReason.CloseEvent)
        {
            bool cancelled = false;
            for (int i = m_Tabs.Count - 1; i >= 0; i--)
            {
                cancelled |= m_Tabs[i].Close(reason);
            }

            return cancelled;
        }

        public DockWindow GetTab(int tabIndex)
        {
            return m_Tabs[tabIndex];
        }

        public int GetTabIndex(DockWindow tab)
        {
            return m_Tabs.IndexOf(tab);
        }

        public bool ContainsTab(DockWindow tab)
        {
            return m_Tabs.Contains(tab);
        }

        public void SelectTab(int tabIndex)
        {
            if (tabIndex < 0 || tabIndex >= m_Tabs.Count)
                return;

            SelectTab(m_Tabs[tabIndex]);
        }

        public void SelectTab(DockWindow tab, bool autoFocus = true)
        {
            if (!m_Tabs.Contains(tab))
                return;

            if (m_SelectedTab == tab)
                return;

            if (m_SelectedTab != null)
                m_SelectedTab.Visible = false;

            m_SelectedTab = tab;
            m_SelectedTab.Visible = true;
            PerformLayout();
            OnSelectedTabChanged();

            if (autoFocus)
                tab.Focus();
        }

        public override DockPanel? HitTest(Float2 position)
        {
            for (int i = m_ChildPanels.Count - 1; i >= 0; i--)
            {
                DockPanel? hit = m_ChildPanels[i].HitTest(position);
                if (hit != null)
                    return hit;
            }

            Float2 screenPosition = ScreenPos;
            Float2 localPosition = new Float2(position.X - screenPosition.X, position.Y - screenPosition.Y);
            return DockAreaBounds.Contains(localPosition) ? this : null;
        }

        public virtual DockState TryGetDockState(out float splitterValue)
        {
            splitterValue = DefaultSplitterValue;
            return IsFloating ? DockState.Float : DockState.DockFill;
        }

        public DockPanel CreateChildPanel(DockState state, float splitterValue)
        {
            DockPanel child = new DockPanel(this);
            child.SetDockPlacement(state, splitterValue);
            AddChild(child);
            PerformLayout();
            return child;
        }

        public virtual void DockWindowInternal(DockState state, DockWindow window, bool autoSelect = true, float splitterValue = 0)
        {
            DockWindow(state, window, autoSelect, splitterValue);
        }

        public void RemoveIt()
        {
            OnLastTabRemoved();
        }

        public void UndockWindowInternal(DockWindow window)
        {
            UndockWindow(window);
        }

        public void MoveTabLeft(int index)
        {
            if (index <= 0 || index >= m_Tabs.Count)
                return;

            DockWindow tab = m_Tabs[index];
            m_Tabs.RemoveAt(index);
            m_Tabs.Insert(index - 1, tab);
            PerformLayout();
        }

        public void MoveTabRight(int index)
        {
            if (index < 0 || index >= m_Tabs.Count - 1)
                return;

            DockWindow tab = m_Tabs[index];
            m_Tabs.RemoveAt(index);
            m_Tabs.Insert(index + 1, tab);
            PerformLayout();
        }

        protected virtual void OnLastTabRemoved()
        {
            if (ParentDockPanel != null)
            {
                ParentDockPanel.m_ChildPanels.Remove(this);
                Dispose();
            }
        }

        protected virtual void DockWindow(DockState state, DockWindow window, bool autoSelect = true, float splitterValue = 0)
        {
            if (state == DockState.Hidden)
            {
                window.Hide();
                return;
            }

            if (state != DockState.DockFill && state != DockState.Float && state != DockState.Unknown)
            {
                DockPanel child = CreateChildPanel(state, splitterValue <= 0 ? DefaultSplitterValue : splitterValue);
                child.AddTab(window, autoSelect);
                return;
            }

            AddTab(window, autoSelect);
        }

        protected virtual void UndockWindow(DockWindow window)
        {
            int index = m_Tabs.IndexOf(window);
            if (index < 0)
                return;

            bool wasSelected = m_SelectedTab == window;
            m_Tabs.RemoveAt(index);
            if (wasSelected)
                m_SelectedTab = null;

            window.ParentDockPanel = null;
            RemoveChild(window);

            if (wasSelected && m_Tabs.Count > 0)
                SelectTab(m_Tabs[index == 0 ? 0 : index - 1]);
            if (m_Tabs.Count == 0)
                OnLastTabRemoved();
        }

        protected virtual void AddTab(DockWindow window, bool autoSelect = true)
        {
            window.ParentDockPanel?.UndockWindowInternal(window);
            if (m_Tabs.Contains(window))
                return;

            m_Tabs.Add(window);
            window.ParentDockPanel = this;
            AddChild(window);
            window.Visible = false;

            if (autoSelect || m_SelectedTab == null)
                SelectTab(window);
            else
                PerformLayout();
        }

        protected virtual void OnSelectedTabChanged()
        {
        }

        protected override void OnLayoutChildren()
        {
            Rectangle remaining = new Rectangle(0, 0, Width, Height);
            foreach (DockPanel childPanel in m_ChildPanels)
            {
                float splitter = Math.Clamp(childPanel.m_SplitterValue, 0.05f, 0.95f);
                switch (childPanel.m_DockStateInParent)
                {
                case DockState.DockTop:
                {
                    float size = remaining.Height * splitter;
                    childPanel.SetBounds(remaining.X, remaining.Y, remaining.Width, size);
                    remaining = new Rectangle(remaining.X, remaining.Y + size, remaining.Width, Math.Max(0.0f, remaining.Height - size));
                    break;
                }
                case DockState.DockBottom:
                {
                    float size = remaining.Height * splitter;
                    childPanel.SetBounds(remaining.X, remaining.Bottom - size, remaining.Width, size);
                    remaining = new Rectangle(remaining.X, remaining.Y, remaining.Width, Math.Max(0.0f, remaining.Height - size));
                    break;
                }
                case DockState.DockLeft:
                {
                    float size = remaining.Width * splitter;
                    childPanel.SetBounds(remaining.X, remaining.Y, size, remaining.Height);
                    remaining = new Rectangle(remaining.X + size, remaining.Y, Math.Max(0.0f, remaining.Width - size), remaining.Height);
                    break;
                }
                case DockState.DockRight:
                {
                    float size = remaining.Width * splitter;
                    childPanel.SetBounds(remaining.Right - size, remaining.Y, size, remaining.Height);
                    remaining = new Rectangle(remaining.X, remaining.Y, Math.Max(0.0f, remaining.Width - size), remaining.Height);
                    break;
                }
                default:
                    childPanel.SetBounds(remaining.X, remaining.Y, remaining.Width, remaining.Height);
                    break;
                }
            }

            m_TabAreaBounds = new Rectangle(
                remaining.X,
                remaining.Y + DefaultHeaderHeight,
                remaining.Width,
                Math.Max(0.0f, remaining.Height - DefaultHeaderHeight));
            TabsProxy.SetBounds(remaining.X, remaining.Y, remaining.Width, Math.Min(DefaultHeaderHeight, remaining.Height));
            foreach (DockWindow tab in m_Tabs)
            {
                tab.SetBounds(m_TabAreaBounds.X, m_TabAreaBounds.Y, m_TabAreaBounds.Width, m_TabAreaBounds.Height);
            }
        }

        protected override void OnDispose()
        {
            if (IsDisposed)
                return;

            foreach (DockWindow tab in m_Tabs.ToArray())
            {
                tab.ParentDockPanel = null;
            }

            m_Tabs.Clear();
            m_ChildPanels.Clear();
            base.OnDispose();
        }

        private DockPanelProxy CreateTabsProxy()
        {
            DockPanelProxy proxy = new DockPanelProxy(this);
            m_TabsProxy = proxy;
            AddChild(proxy);
            return proxy;
        }

        private void SetDockPlacement(DockState state, float splitterValue)
        {
            m_DockStateInParent = state;
            m_SplitterValue = splitterValue > 0.0f ? splitterValue : DefaultSplitterValue;
        }
    }
}

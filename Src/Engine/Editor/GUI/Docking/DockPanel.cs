using System;
using System.Collections.Generic;
using System.Linq;
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

        public DockPanel(DockPanel? parentPanel = null)
            : base(new Rectangle(0, 0, 300, 200))
        {
            AutoFocus = false;
            AnchorMin = Float2.Zero;
            AnchorMax = Float2.One;
            Offsets = Margin.Zero;
            ParentDockPanel = parentPanel;
            parentPanel?.m_ChildPanels.Add(this);
        }

        public virtual bool IsMaster => false;
        public virtual bool IsFloating => false;
        public Rectangle DockAreaBounds
        {
            get
            {
                Control control = m_TabsProxy ?? this;
                Rectangle bounds = control.ScreenBounds;
                if (Root is WindowRootControl root)
                {
                    return new Rectangle(
                        root.Window.ClientToScreen(bounds.Location * root.DpiScale),
                        bounds.Size * root.DpiScale);
                }
                return bounds;
            }
        }
        public IReadOnlyList<DockPanel> ChildPanels => m_ChildPanels;
        public int ChildPanelsCount => m_ChildPanels.Count;
        public IReadOnlyList<DockWindow> Tabs => m_Tabs;
        public int TabsCount => m_Tabs.Count;
        public DockWindow? SelectedTab => m_SelectedTab;
        public DockWindow? FirstTab => m_Tabs.Count > 0 ? m_Tabs[0] : null;
        public DockWindow? LastTab => m_Tabs.Count > 0 ? m_Tabs[m_Tabs.Count - 1] : null;
        public DockPanel? ParentDockPanel { get; }
        public DockPanelProxy? TabsProxy => m_TabsProxy;

        public int SelectedTabIndex
        {
            get => m_SelectedTab != null ? m_Tabs.IndexOf(m_SelectedTab) : -1;
            set => SelectTab(value);
        }

        public bool CloseAll(ClosingReason reason = ClosingReason.CloseEvent)
        {
            // Deliberately follows the native active control flow and return-value contract.
            while (m_Tabs.Count > 0)
            {
                if (!m_Tabs[0].Close(reason))
                    return false;
            }
            return true;
        }

        public DockWindow GetTab(int tabIndex) => m_Tabs[tabIndex];
        public int GetTabIndex(DockWindow tab) => m_Tabs.IndexOf(tab);
        public bool ContainsTab(DockWindow tab) => m_Tabs.Contains(tab);

        public void SelectTab(int tabIndex)
        {
            DockWindow? tab = tabIndex >= 0 && tabIndex < m_Tabs.Count ? m_Tabs[tabIndex] : null;
            SelectTab(tab);
        }

        public void SelectTab(DockWindow? tab, bool autoFocus = true)
        {
            if (!ReferenceEquals(m_SelectedTab, tab))
            {
                ContainerControl proxy;
                if (m_SelectedTab != null)
                {
                    proxy = m_SelectedTab.Parent!;
                    proxy.RemoveChild(m_SelectedTab);
                }
                else
                {
                    proxy = CreateTabsProxy();
                }

                m_SelectedTab = tab;
                if (m_SelectedTab != null)
                {
                    m_SelectedTab.UnlockChildrenRecursive();
                    proxy.AddChild(m_SelectedTab);
                    if (autoFocus)
                        m_SelectedTab.Focus();
                }
                OnSelectedTabChanged();
            }
            else if (autoFocus && m_SelectedTab != null && !m_SelectedTab.ContainsFocus)
            {
                m_SelectedTab.Focus();
            }
        }

        public override DockPanel? HitTest(Float2 position)
        {
            Float2 rootPosition = position;
            if (Root is WindowRootControl root)
                rootPosition = root.Window.ScreenToClient(position) / Math.Max(root.DpiScale, 0.001f);
            if (!ScreenBounds.Contains(rootPosition))
                return null;

            DockPanel? result = null;
            float smallestSize = float.MaxValue;
            foreach (DockPanel child in m_ChildPanels)
            {
                DockPanel? hit = child.HitTest(position);
                if (hit == null)
                    continue;
                float size = hit.Width * hit.Width + hit.Height * hit.Height;
                if (size < smallestSize)
                {
                    smallestSize = size;
                    result = hit;
                }
            }
            return result ?? this;
        }

        public virtual DockState TryGetDockState(out float splitterValue)
        {
            splitterValue = DefaultSplitterValue;
            if (Parent is Panel host && host.Parent is SplitPanel splitter)
            {
                splitterValue = splitter.SplitterValue;
                if (ReferenceEquals(host, splitter.Panel1))
                    return splitter.Orientation == Orientation.Horizontal ? DockState.DockLeft : DockState.DockTop;

                splitterValue = 1.0f - splitterValue;
                return splitter.Orientation == Orientation.Horizontal ? DockState.DockRight : DockState.DockBottom;
            }
            return DockState.Unknown;
        }

        public DockPanel CreateChildPanel(DockState state, float splitterValue)
        {
            DockPanelProxy tabsProxy = CreateTabsProxy();
            ContainerControl parent = tabsProxy.Parent ?? this;
            DockPanel child = new DockPanel(this);
            Control first;
            Control second;
            Orientation orientation;

            switch (state)
            {
            case DockState.DockTop:
                orientation = Orientation.Vertical;
                first = child;
                second = tabsProxy;
                break;
            case DockState.DockBottom:
                splitterValue = 1.0f - splitterValue;
                orientation = Orientation.Vertical;
                first = tabsProxy;
                second = child;
                break;
            case DockState.DockLeft:
                orientation = Orientation.Horizontal;
                first = child;
                second = tabsProxy;
                break;
            case DockState.DockRight:
                splitterValue = 1.0f - splitterValue;
                orientation = Orientation.Horizontal;
                first = tabsProxy;
                second = child;
                break;
            default:
                throw new ArgumentOutOfRangeException(nameof(state));
            }

            SplitPanel splitter = new SplitPanel(orientation) { SplitterValue = splitterValue };
            splitter.AnchorMin = Float2.Zero;
            splitter.AnchorMax = Float2.One;
            splitter.Offsets = Margin.Zero;
            splitter.Panel1.AddChild(first);
            splitter.Panel2.AddChild(second);
            parent.AddChild(splitter);
            splitter.UnlockChildrenRecursive();
            splitter.PerformLayout();
            return child;
        }

        public virtual void DockWindowInternal(DockState state, DockWindow window, bool autoSelect = true, float splitterValue = 0)
            => DockWindow(state, window, autoSelect, splitterValue);
        public void RemoveIt() => OnLastTabRemoved();
        public void UndockWindowInternal(DockWindow window) => UndockWindow(window);

        public void MoveTabLeft(int index)
        {
            if (index <= 0 || index >= m_Tabs.Count)
                return;
            DockWindow tab = m_Tabs[index];
            m_Tabs.RemoveAt(index);
            m_Tabs.Insert(index - 1, tab);
        }

        public void MoveTabRight(int index)
        {
            // Native currently stops one slot before the final tab; preserve that active behavior.
            if (index < 0 || index >= m_Tabs.Count - 2)
                return;
            DockWindow tab = m_Tabs[index];
            m_Tabs.RemoveAt(index);
            m_Tabs.Insert(index + 1, tab);
        }

        protected virtual void OnLastTabRemoved()
        {
            if (Parent is not Panel host || host.Parent is not SplitPanel splitter)
                return;

            if (m_ChildPanels.Count > 0)
            {
                DockWindow? selected = null;
                foreach (DockPanel child in m_ChildPanels.ToArray())
                {
                    // Native code walks the live list forward while UndockWindow mutates it.
                    for (int index = 0; index < child.m_Tabs.Count; index++)
                    {
                        DockWindow tab = child.m_Tabs[index];
                        if (selected == null && tab.IsSelected)
                            selected = tab;
                        child.UndockWindow(tab);
                        AddTab(tab, false);
                    }
                }
                if (selected != null)
                    SelectTab(selected);
                return;
            }

            ContainerControl? splitterParent = splitter.Parent;
            if (splitterParent == null)
                return;
            Panel source = ReferenceEquals(host, splitter.Panel2) ? splitter.Panel1 : splitter.Panel2;
            Control[] remaining = source.Children
                .Where(control => !ReferenceEquals(control, source.VerticalScrollBar) &&
                                  !ReferenceEquals(control, source.HorizontalScrollBar))
                .ToArray();
            splitterParent.RemoveChild(splitter);
            for (int index = remaining.Length - 1; index >= 0; index--)
                splitterParent.AddChild(remaining[index]);
            splitter.Dispose();
        }

        protected virtual void DockWindow(DockState state, DockWindow window, bool autoSelect = true, float splitterValue = 0)
        {
            CreateTabsProxy();
            if (state == DockState.DockFill)
            {
                AddTab(window, autoSelect);
            }
            else
            {
                DockPanel child = CreateChildPanel(state, splitterValue != 0.0f ? splitterValue : DefaultSplitterValue);
                child.DockWindow(DockState.DockFill, window);
            }
        }

        protected virtual void UndockWindow(DockWindow window)
        {
            int index = GetTabIndex(window);
            if (index < 0)
                return;

            if (ReferenceEquals(window, m_SelectedTab))
                SelectTab(index == 0 && m_Tabs.Count > 1 ? 1 : index - 1);

            m_Tabs.RemoveAt(index);
            window.ParentDockPanel = null;
            if (m_Tabs.Count == 0)
                OnLastTabRemoved();
            else
                PerformLayout();
        }

        protected virtual void AddTab(DockWindow window, bool autoSelect = true)
        {
            m_Tabs.Add(window);
            window.ParentDockPanel = this;
            if (autoSelect)
                SelectTab(window);
        }

        protected virtual void OnSelectedTabChanged()
        {
        }

        protected override void OnLayoutChildren()
        {
            m_TabsProxy?.SetBounds(0.0f, 0.0f, Width, Height);
        }

        protected override void OnDispose()
        {
            if (ParentDockPanel != null)
                ParentDockPanel.m_ChildPanels.Remove(this);

            base.OnDispose();

            // Native DockPanel destroys tabs that were not attached to the active proxy view.
            for (int index = 0; index < m_Tabs.Count; index++)
            {
                DockWindow tab = m_Tabs[index];
                tab.ParentDockPanel = null;
                if (!tab.IsDisposed)
                    tab.Dispose();
            }
            m_Tabs.Clear();
            m_ChildPanels.Clear();
            m_SelectedTab = null;
            m_TabsProxy = null;
        }

        private DockPanelProxy CreateTabsProxy()
        {
            if (m_TabsProxy == null)
            {
                m_TabsProxy = new DockPanelProxy(this);
                AddChild(m_TabsProxy);
                m_TabsProxy.UnlockChildrenRecursive();
            }
            return m_TabsProxy;
        }
    }
}

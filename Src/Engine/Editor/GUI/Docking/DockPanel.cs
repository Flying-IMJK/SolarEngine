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

        private readonly List<DockPanel> _childPanels = new List<DockPanel>();
        private readonly List<DockWindow> _tabs = new List<DockWindow>();
        private DockWindow? _selectedTab;
        private DockPanelProxy? _tabsProxy;
        private DockState _dockStateInParent = DockState.DockFill;
        private float _splitterValue = DefaultSplitterValue;
        private GuiRect _tabAreaBounds;

        public DockPanel(DockPanel? parentPanel = null)
            : base(new Rectangle(0, 0, 300, 200))
        {
            ParentDockPanel = parentPanel;
            parentPanel?._childPanels.Add(this);
            SetBounds(0, 0, 300, 200);
        }

        public virtual bool IsMaster => false;
        public virtual bool IsFloating => false;
        public GuiRect DockAreaBounds => _tabAreaBounds;
        public IReadOnlyList<DockPanel> ChildPanels => _childPanels;
        public int ChildPanelsCount => _childPanels.Count;
        public IReadOnlyList<DockWindow> Tabs => _tabs;
        public int TabsCount => _tabs.Count;
        public DockWindow? SelectedTab => _selectedTab;
        public DockWindow? FirstTab => _tabs.Count > 0 ? _tabs[0] : null;
        public DockWindow? LastTab => _tabs.Count > 0 ? _tabs[_tabs.Count - 1] : null;
        public DockPanel? ParentDockPanel { get; }
        public DockPanelProxy TabsProxy => _tabsProxy ??= CreateTabsProxy();

        public int SelectedTabIndex
        {
            get => _selectedTab != null ? _tabs.IndexOf(_selectedTab) : -1;
            set => SelectTab(value);
        }

        public bool CloseAll(ClosingReason reason = ClosingReason.CloseEvent)
        {
            bool cancelled = false;
            for (int i = _tabs.Count - 1; i >= 0; i--)
            {
                cancelled |= _tabs[i].Close(reason);
            }

            return cancelled;
        }

        public DockWindow GetTab(int tabIndex)
        {
            return _tabs[tabIndex];
        }

        public int GetTabIndex(DockWindow tab)
        {
            return _tabs.IndexOf(tab);
        }

        public bool ContainsTab(DockWindow tab)
        {
            return _tabs.Contains(tab);
        }

        public void SelectTab(int tabIndex)
        {
            if (tabIndex < 0 || tabIndex >= _tabs.Count)
                return;

            SelectTab(_tabs[tabIndex]);
        }

        public void SelectTab(DockWindow tab, bool autoFocus = true)
        {
            if (!_tabs.Contains(tab))
                return;

            if (_selectedTab == tab)
                return;

            if (_selectedTab != null)
                _selectedTab.Visible = false;

            _selectedTab = tab;
            _selectedTab.Visible = true;
            PerformLayout();
            OnSelectedTabChanged();

            if (autoFocus)
                tab.Focus();
        }

        public DockPanel? HitTest(GuiPoint position)
        {
            for (int i = _childPanels.Count - 1; i >= 0; i--)
            {
                DockPanel? hit = _childPanels[i].HitTest(position);
                if (hit != null)
                    return hit;
            }

            Float2 screenPosition = ScreenPos;
            GuiPoint localPosition = new GuiPoint(position.X - screenPosition.X, position.Y - screenPosition.Y);
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
            if (index <= 0 || index >= _tabs.Count)
                return;

            DockWindow tab = _tabs[index];
            _tabs.RemoveAt(index);
            _tabs.Insert(index - 1, tab);
            PerformLayout();
        }

        public void MoveTabRight(int index)
        {
            if (index < 0 || index >= _tabs.Count - 1)
                return;

            DockWindow tab = _tabs[index];
            _tabs.RemoveAt(index);
            _tabs.Insert(index + 1, tab);
            PerformLayout();
        }

        protected virtual void OnLastTabRemoved()
        {
            if (ParentDockPanel != null)
            {
                ParentDockPanel._childPanels.Remove(this);
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
            int index = _tabs.IndexOf(window);
            if (index < 0)
                return;

            bool wasSelected = _selectedTab == window;
            _tabs.RemoveAt(index);
            if (wasSelected)
                _selectedTab = null;

            window.ParentDockPanel = null;
            RemoveChild(window);

            if (wasSelected && _tabs.Count > 0)
                SelectTab(_tabs[index == 0 ? 0 : index - 1]);
            if (_tabs.Count == 0)
                OnLastTabRemoved();
        }

        protected virtual void AddTab(DockWindow window, bool autoSelect = true)
        {
            window.ParentDockPanel?.UndockWindowInternal(window);
            if (_tabs.Contains(window))
                return;

            _tabs.Add(window);
            window.ParentDockPanel = this;
            AddChild(window);
            window.Visible = false;

            if (autoSelect || _selectedTab == null)
                SelectTab(window);
            else
                PerformLayout();
        }

        protected virtual void OnSelectedTabChanged()
        {
        }

        protected override void OnLayoutChildren()
        {
            GuiRect remaining = new GuiRect(0, 0, Width, Height);
            foreach (DockPanel childPanel in _childPanels)
            {
                float splitter = Math.Clamp(childPanel._splitterValue, 0.05f, 0.95f);
                switch (childPanel._dockStateInParent)
                {
                case DockState.DockTop:
                {
                    float size = remaining.Height * splitter;
                    childPanel.SetBounds(remaining.X, remaining.Y, remaining.Width, size);
                    remaining = new GuiRect(remaining.X, remaining.Y + size, remaining.Width, Math.Max(0.0f, remaining.Height - size));
                    break;
                }
                case DockState.DockBottom:
                {
                    float size = remaining.Height * splitter;
                    childPanel.SetBounds(remaining.X, remaining.Bottom - size, remaining.Width, size);
                    remaining = new GuiRect(remaining.X, remaining.Y, remaining.Width, Math.Max(0.0f, remaining.Height - size));
                    break;
                }
                case DockState.DockLeft:
                {
                    float size = remaining.Width * splitter;
                    childPanel.SetBounds(remaining.X, remaining.Y, size, remaining.Height);
                    remaining = new GuiRect(remaining.X + size, remaining.Y, Math.Max(0.0f, remaining.Width - size), remaining.Height);
                    break;
                }
                case DockState.DockRight:
                {
                    float size = remaining.Width * splitter;
                    childPanel.SetBounds(remaining.Right - size, remaining.Y, size, remaining.Height);
                    remaining = new GuiRect(remaining.X, remaining.Y, Math.Max(0.0f, remaining.Width - size), remaining.Height);
                    break;
                }
                default:
                    childPanel.SetBounds(remaining.X, remaining.Y, remaining.Width, remaining.Height);
                    break;
                }
            }

            _tabAreaBounds = new GuiRect(
                remaining.X,
                remaining.Y + DefaultHeaderHeight,
                remaining.Width,
                Math.Max(0.0f, remaining.Height - DefaultHeaderHeight));
            TabsProxy.SetBounds(remaining.X, remaining.Y, remaining.Width, Math.Min(DefaultHeaderHeight, remaining.Height));
            foreach (DockWindow tab in _tabs)
            {
                tab.SetBounds(_tabAreaBounds.X, _tabAreaBounds.Y, _tabAreaBounds.Width, _tabAreaBounds.Height);
            }
        }

        protected override void OnDispose()
        {
            if (IsDisposed)
                return;

            foreach (DockWindow tab in _tabs.ToArray())
            {
                tab.ParentDockPanel = null;
            }

            _tabs.Clear();
            _childPanels.Clear();
            base.OnDispose();
        }

        private DockPanelProxy CreateTabsProxy()
        {
            DockPanelProxy proxy = new DockPanelProxy(this);
            _tabsProxy = proxy;
            AddChild(proxy);
            return proxy;
        }

        private void SetDockPlacement(DockState state, float splitterValue)
        {
            _dockStateInParent = state;
            _splitterValue = splitterValue > 0.0f ? splitterValue : DefaultSplitterValue;
        }
    }
}

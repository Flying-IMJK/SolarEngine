#nullable disable annotations

using System.Collections.Generic;
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Dockable panel control.
    /// </summary>
    /// <seealso cref="ContainerControl" />
    public class DockPanel : ContainerControl
    {
        /// <summary>
        /// The default dock tabs header height.
        /// </summary>
        public const float DefaultHeaderHeight = 25.0f;

        /// <summary>
        /// The default tabs header text left margin.
        /// </summary>
        public const float DefaultTextMargin = 2.0f;

        /// <summary>
        /// The default tabs header buttons size.
        /// </summary>
        public const float DefaultButtonsSize = 12.0f;

        /// <summary>
        /// The default tabs header buttons margin.
        /// </summary>
        public const float DefaultButtonsMargin = 4.0f;

        /// <summary>
        /// The default splitters value.
        /// </summary>
        public const float DefaultSplitterValue = 0.25f;

        private DockPanel _parentPanel;
        private readonly List<DockPanel> _childPanels = new List<DockPanel>();
        private readonly List<DockWindow> _tabs = new List<DockWindow>();
        private DockWindow _selectedTab;
        private DockPanelProxy _tabsProxy;

        /// <summary>
        /// Returns true if this panel is a master panel.
        /// </summary>
        public virtual bool IsMaster => false;

        /// <summary>
        /// Returns true if this panel is a floating window panel.
        /// </summary>
        public virtual bool IsFloating => false;

        /// <summary>
        /// Gets docking area bounds (tabs rectangle) in a screen space.
        /// </summary>
        public Rectangle DockAreaBounds
        {
            get
            {
                RootControl parentWin = Root;
                SE.Debug.Assert(parentWin != null, "Missing parent window.");

                Control control = _tabsProxy != null ? (Control)_tabsProxy : this;
                Float2 clientPos = control.PointToWindow(Float2.Zero);
                return new Rectangle(parentWin.PointToScreen(clientPos), new Float2(GetDpiScale()) * control.Size);
            }
        }

        /// <summary>
        /// Gets the child panels.
        /// </summary>
        public List<DockPanel> ChildPanels => _childPanels;

        /// <summary>
        /// Gets the child panels count.
        /// </summary>
        public int ChildPanelsCount => _childPanels.Count;

        /// <summary>
        /// Gets the tabs.
        /// </summary>
        public List<DockWindow> Tabs => _tabs;

        /// <summary>
        /// Gets amount of the tabs in a dock panel.
        /// </summary>
        public int TabsCount => _tabs.Count;

        /// <summary>
        /// Gets or sets the index of the selected tab.
        /// </summary>
        public int SelectedTabIndex
        {
            get => _tabs.IndexOf(_selectedTab);
            set => SelectTab(value);
        }

        /// <summary>
        /// Gets the selected tab.
        /// </summary>
        public DockWindow SelectedTab => _selectedTab;

        /// <summary>
        /// Gets the first tab.
        /// </summary>
        public DockWindow FirstTab => _tabs.Count > 0 ? _tabs[0] : null;

        /// <summary>
        /// Gets the last tab.
        /// </summary>
        public DockWindow LastTab => _tabs.Count > 0 ? _tabs[_tabs.Count - 1] : null;

        /// <summary>
        /// Gets the parent panel.
        /// </summary>
        public DockPanel ParentDockPanel => _parentPanel;

        /// <summary>
        /// Gets the tabs header proxy.
        /// </summary>
        public DockPanelProxy TabsProxy => _tabsProxy;

        /// <summary>
        /// Initializes a new instance of the <see cref="DockPanel"/> class.
        /// </summary>
        /// <param name="parentPanel">The parent panel.</param>
        public DockPanel(DockPanel parentPanel)
        {
            AutoFocus = false;

            _parentPanel = parentPanel;
            if (_parentPanel != null)
            {
                _parentPanel._childPanels.Add(this);
            }

            AnchorPreset = AnchorPresets.StretchAll;
            Offsets = Margin.Zero;
        }

        /// <summary>
        /// Closes all the windows.
        /// </summary>
        /// <param name="reason">Window closing reason.</param>
        /// <returns>True if action has been cancelled (due to window internal logic).</returns>
        public bool CloseAll(ClosingReason reason = ClosingReason.CloseEvent)
        {
            while (_tabs.Count > 0)
            {
                if (!_tabs[0].Close(reason))
                {
                    return false;
                }
            }

            return true;
        }

        /// <summary>
        /// Gets tab at the given index.
        /// </summary>
        /// <param name="tabIndex">The index of the tab page.</param>
        /// <returns>The tab.</returns>
        public DockWindow GetTab(int tabIndex)
        {
            return _tabs[tabIndex];
        }

        /// <summary>
        /// Gets tab at the given index.
        /// </summary>
        /// <param name="tab">The tab page.</param>
        /// <returns>The index of the given tab.</returns>
        public int GetTabIndex(DockWindow tab)
        {
            return _tabs.IndexOf(tab);
        }

        /// <summary>
        /// Determines whether panel contains the specified tab.
        /// </summary>
        /// <param name="tab">The tab.</param>
        /// <returns><c>true</c> if panel contains the specified tab; otherwise, <c>false</c>.</returns>
        public bool ContainsTab(DockWindow tab)
        {
            return _tabs.Contains(tab);
        }

        /// <summary>
        /// Selects the tab page.
        /// </summary>
        /// <param name="tabIndex">The index of the tab page to select.</param>
        public void SelectTab(int tabIndex)
        {
            DockWindow tab = null;
            if (tabIndex >= 0 && _tabs.Count > tabIndex && _tabs.Count > 0)
            {
                tab = _tabs[tabIndex];
            }
            SelectTab(tab);
        }

        /// <summary>
        /// Selects the tab page.
        /// </summary>
        /// <param name="tab">The tab page to select.</param>
        /// <param name="autoFocus">True if focus tab after selection change.</param>
        public void SelectTab(DockWindow tab, bool autoFocus = true)
        {
            // Check if tab will change
            if (_selectedTab != tab)
            {
                // Change
                ContainerControl proxy;
                if (_selectedTab != null)
                {
                    proxy = _selectedTab.Parent;
                    _selectedTab.Parent = null;
                }
                else
                {
                    CreateTabsProxy();
                    proxy = _tabsProxy;
                }

                _selectedTab = tab;
                if (_selectedTab != null)
                {
                    _selectedTab.UnlockChildrenRecursive();
                    _selectedTab.Parent = proxy;
                    if (autoFocus)
                    {
                        _selectedTab.Focus();
                    }
                }
                OnSelectedTabChanged();
            }
            else if (autoFocus && _selectedTab != null && !_selectedTab.ContainsFocus)
            {
                _selectedTab.Focus();
            }
        }

        /// <summary>
        /// Performs hit test over dock panel.
        /// </summary>
        /// <param name="position">Screen space position to test.</param>
        /// <returns>Dock panel that has been hit or null if nothing found.</returns>
        public DockPanel HitTest(Float2 position)
        {
            // Get parent window and transform point position into local coordinates system
            RootControl parentWin = Root;
            if (parentWin == null)
            {
                return null;
            }
            Float2 clientPos = parentWin.PointFromScreen(position);
            Float2 localPos = PointFromWindow(clientPos);

            // Early out
            if (localPos.X < 0 || localPos.Y < 0)
            {
                return null;
            }
            if (localPos.X > Width || localPos.Y > Height)
            {
                return null;
            }

            // Test all docked controls (find the smallest one)
            float sizeLengthSquared = float.MaxValue;
            DockPanel result = null;
            for (int i = 0; i < _childPanels.Count; i++)
            {
                DockPanel panel = _childPanels[i].HitTest(position);
                if (panel != null)
                {
                    float sizeLen = panel.Size.LengthSquared;
                    if (sizeLen < sizeLengthSquared)
                    {
                        sizeLengthSquared = sizeLen;
                        result = panel;
                    }
                }
            }
            if (result != null)
            {
                return result;
            }

            // Itself
            return this;
        }

        /// <summary>
        /// Try get panel dock state.
        /// </summary>
        /// <param name="splitterValue">Splitter value.</param>
        /// <returns>Dock State.</returns>
        public virtual DockState TryGetDockState(out float splitterValue)
        {
            DockState result = DockState.Unknown;
            splitterValue = DefaultSplitterValue;

            if (HasParent)
            {
                if (Parent.Parent is SplitPanel splitter)
                {
                    splitterValue = splitter.SplitterValue;
                    if (Parent == splitter.Panel1)
                    {
                        if (splitter.Orientation == Orientation.Horizontal)
                        {
                            result = DockState.DockLeft;
                        }
                        else
                        {
                            result = DockState.DockTop;
                        }
                    }
                    else
                    {
                        if (splitter.Orientation == Orientation.Horizontal)
                        {
                            result = DockState.DockRight;
                        }
                        else
                        {
                            result = DockState.DockBottom;
                        }
                        splitterValue = 1.0f - splitterValue;
                    }
                }
            }

            return result;
        }

        /// <summary>
        /// Create child dock panel.
        /// </summary>
        /// <param name="state">Dock panel state.</param>
        /// <param name="splitterValue">Initial splitter value.</param>
        /// <returns>Child panel.</returns>
        public DockPanel CreateChildPanel(DockState state, float splitterValue)
        {
            CreateTabsProxy();

            // Create child dock panel
            DockPanel dockPanel = new DockPanel(this);

            // Switch dock mode
            Control c1 = null;
            Control c2 = null;
            Orientation orientation = default;
            switch (state)
            {
                case DockState.DockTop:
                {
                    orientation = Orientation.Vertical;
                    c1 = dockPanel;
                    c2 = _tabsProxy;
                    break;
                }
                case DockState.DockBottom:
                {
                    splitterValue = 1.0f - splitterValue;
                    orientation = Orientation.Vertical;
                    c1 = _tabsProxy;
                    c2 = dockPanel;
                    break;
                }
                case DockState.DockLeft:
                {
                    orientation = Orientation.Horizontal;
                    c1 = dockPanel;
                    c2 = _tabsProxy;
                    break;
                }
                case DockState.DockRight:
                {
                    splitterValue = 1.0f - splitterValue;
                    orientation = Orientation.Horizontal;
                    c1 = _tabsProxy;
                    c2 = dockPanel;
                    break;
                }
            }

            // Create splitter and link controls
            ContainerControl parent = _tabsProxy.Parent;
            SplitPanel splitter = new SplitPanel(orientation, ScrollBars.None, ScrollBars.None);
            splitter.AnchorPreset = AnchorPresets.StretchAll;
            splitter.Offsets = Margin.Zero;
            splitter.SplitterValue = splitterValue;

            splitter.Panel1.AddChild(c1);
            splitter.Panel2.AddChild(c2);
            parent.AddChild(splitter);

            // Update
            splitter.UnlockChildrenRecursive();
            splitter.PerformLayout();

            return dockPanel;
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
            if (index > 0)
            {
                DockWindow tab = _tabs[index];
                _tabs.RemoveAt(index);
                _tabs.Insert(index - 1, tab);
            }
        }

        public void MoveTabRight(int index)
        {
            if (index < _tabs.Count - 2)
            {
                DockWindow tab = _tabs[index];
                _tabs.RemoveAt(index);
                _tabs.Insert(index + 1, tab);
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (_parentPanel != null)
            {
                _parentPanel._childPanels.Remove(this);
            }

            base.OnDestroy();

            // Clear other tabs (not in the view)
            for (int i = 0; i < _tabs.Count; i++)
            {
                if (!_tabs[i].IsDisposing)
                {
                    _tabs[i].Dispose();
                }
            }
        }

        /// <summary>
        /// Called when last tab gets removed.
        /// </summary>
        protected virtual void OnLastTabRemoved()
        {
            // Check if dock panel is linked to the split panel
            if (HasParent)
            {
                if (Parent.Parent is SplitPanel splitter)
                {
                    // Check if there is another nested dock panel inside this dock panel and extract it here
                    List<DockPanel> childPanels = new List<DockPanel>(_childPanels);
                    if (childPanels.Count != 0)
                    {
                        // Move tabs from child panels into this one
                        DockWindow selectedTab = null;
                        foreach (DockPanel childPanel in childPanels)
                        {
                            List<DockWindow> childPanelTabs = childPanel.Tabs;
                            for (int i = 0; i < childPanelTabs.Count; i++)
                            {
                                DockWindow childPanelTab = childPanelTabs[i];
                                if (selectedTab == null && childPanelTab.IsSelected)
                                {
                                    selectedTab = childPanelTab;
                                }
                                childPanel.UndockWindow(childPanelTab);
                                AddTab(childPanelTab, false);
                            }
                        }
                        if (selectedTab != null)
                        {
                            SelectTab(selectedTab);
                        }
                    }
                    else
                    {
                        // Unlink splitter
                        ContainerControl splitterParent = splitter.Parent;
                        SE.Debug.Assert(splitterParent != null, "Missing splitter parent.");
                        splitter.Parent = null;

                        // Move controls from second split panel to the split panel parent
                        Panel scrPanel = Parent == splitter.Panel2 ? splitter.Panel1 : splitter.Panel2;
                        int srcPanelChildrenCount = scrPanel.ChildrenCount;
                        for (int i = srcPanelChildrenCount - 1; i >= 0 && scrPanel.ChildrenCount > 0; i--)
                        {
                            scrPanel.GetChild(i).Parent = splitterParent;
                        }

                        SE.Debug.Assert(scrPanel.ChildrenCount == 0, "Source panel still has children.");
                        SE.Debug.Assert(splitterParent.ChildrenCount == srcPanelChildrenCount, "Splitter parent child count differs.");

                        // Delete
                        splitter.Dispose();
                    }
                }
                else if (!IsMaster)
                {
                    SE.Debug.Assert(false, "Dock panel has no split panel parent.");
                }
            }
        }

        /// <summary>
        /// Docks the window.
        /// </summary>
        /// <param name="state">The state.</param>
        /// <param name="window">The window.</param>
        /// <param name="autoSelect">Whether or not to automatically select the window after docking it.</param>
        /// <param name="splitterValue">The splitter value to use when docking to window.</param>
        protected virtual void DockWindow(DockState state, DockWindow window, bool autoSelect = true, float splitterValue = 0)
        {
            CreateTabsProxy();

            // Check if dock like a tab or not
            if (state == DockState.DockFill)
            {
                // Add tab
                AddTab(window, autoSelect);
            }
            else
            {
                // Create child panel
                DockPanel dockPanel = CreateChildPanel(state, splitterValue != 0.0f ? splitterValue : DefaultSplitterValue);

                // Dock window as a tab in a child panel
                dockPanel.DockWindow(DockState.DockFill, window);
            }
        }

        /// <summary>
        /// Undocks the window.
        /// </summary>
        /// <param name="window">The window.</param>
        protected virtual void UndockWindow(DockWindow window)
        {
            int index = GetTabIndex(window);
            SE.Debug.Assert(index >= 0, "Window is not a tab of this panel.");

            // Check if tab was selected
            if (window == SelectedTab)
            {
                // Change selection
                if (index == 0 && TabsCount > 1)
                {
                    SelectTab(1);
                }
                else
                {
                    SelectTab(index - 1);
                }
            }

            // Undock
            _tabs.RemoveAt(index);
            window.ParentDockPanel = null;

            // Check if has no more tabs
            if (_tabs.Count == 0)
            {
                OnLastTabRemoved();
            }
            else
            {
                // Update
                PerformLayout();
            }
        }

        /// <summary>
        /// Adds the tab.
        /// </summary>
        /// <param name="window">The window to insert as a tab.</param>
        /// <param name="autoSelect">True if auto-select newly added tab.</param>
        protected virtual void AddTab(DockWindow window, bool autoSelect = true)
        {
            _tabs.Add(window);
            window.ParentDockPanel = this;
            if (autoSelect)
            {
                SelectTab(window);
            }
        }

        /// <summary>
        /// Called when selected tab gets changed.
        /// </summary>
        protected virtual void OnSelectedTabChanged()
        {
        }

        private void CreateTabsProxy()
        {
            if (_tabsProxy == null)
            {
                // Create proxy and make set simple full dock
                _tabsProxy = new DockPanelProxy(this);
                _tabsProxy.Parent = this;

                _tabsProxy.UnlockChildrenRecursive();
            }
        }
    }
}

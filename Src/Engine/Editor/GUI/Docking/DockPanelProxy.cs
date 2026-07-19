using System.Linq;
using SE.GUI;

namespace SE.Editor.GUI
{
    public sealed class DockPanelProxy : ContainerControl
    {
        private readonly DockPanel _panel;
        private readonly Label _title;

        public DockPanelProxy(DockPanel panel)
            : base(new Rectangle(0, 0, 300, DockPanel.DefaultHeaderHeight))
        {
            _panel = panel;
            BackgroundColor = SE.GUI.Style.Current.BackgroundNormal;
            _title = new Label(new Rectangle(4, 0, 292, DockPanel.DefaultHeaderHeight), string.Empty)
            {
                AutoFocus = false,
                Enabled = false,
            };
            AddChild(_title);
        }

        public bool IsMouseLeftButtonDown { get; set; }
        public bool IsMouseRightButtonDown { get; set; }
        public bool IsMouseMiddleButtonDown { get; set; }
        public bool IsMouseDownOverCross { get; set; }
        public DockWindow? MouseDownWindow { get; set; }
        public GuiPoint MousePosition { get; set; }
        public DockWindow? StartDragAsyncWindow { get; set; }

        public DockWindow? GetTabAt(GuiPoint position)
        {
            return GetTabAt(position, out _);
        }

        public override bool OnMouseDown(Float2 location, int button)
        {
            MousePosition = new GuiPoint(location.X, location.Y);
            MouseDownWindow = GetTabAt(MousePosition, out bool isOverCross);
            IsMouseDownOverCross = isOverCross;

            switch (button)
            {
            case 1:
                IsMouseLeftButtonDown = true;
                Root?.StartTrackingMouse(this);
                if (!IsMouseDownOverCross && MouseDownWindow != null)
                    _panel.SelectTab(MouseDownWindow);
                return MouseDownWindow != null;
            case 3:
                IsMouseRightButtonDown = true;
                if (MouseDownWindow != null)
                    _panel.SelectTab(MouseDownWindow, autoFocus: false);
                return MouseDownWindow != null;
            case 2:
                IsMouseMiddleButtonDown = true;
                return MouseDownWindow != null;
            default:
                return base.OnMouseDown(location, button);
            }
        }

        public override bool OnMouseUp(Float2 location, int button)
        {
            MousePosition = new GuiPoint(location.X, location.Y);
            DockWindow? tab = GetTabAt(MousePosition, out bool isOverCross);
            switch (button)
            {
            case 1 when IsMouseLeftButtonDown:
                IsMouseLeftButtonDown = false;
                Root?.EndTrackingMouse();
                if (tab != null && ReferenceEquals(tab, MouseDownWindow) && IsMouseDownOverCross && isOverCross)
                    tab.Close(ClosingReason.User);
                MouseDownWindow = null;
                return true;
            case 3 when IsMouseRightButtonDown:
                IsMouseRightButtonDown = false;
                if (tab != null)
                    ShowContextMenu(tab);
                MouseDownWindow = null;
                return tab != null;
            case 2 when IsMouseMiddleButtonDown:
                IsMouseMiddleButtonDown = false;
                if (tab != null)
                    tab.Close(ClosingReason.User);
                MouseDownWindow = null;
                return tab != null;
            default:
                return base.OnMouseUp(location, button);
            }
        }

        public override void OnMouseMove(Float2 location)
        {
            MousePosition = new GuiPoint(location.X, location.Y);
            if (!IsMouseLeftButtonDown || MouseDownWindow == null)
            {
                base.OnMouseMove(location);
                return;
            }

            GuiRect header = new GuiRect(0, 0, Width, Height);
            if (!header.Contains(MousePosition))
            {
                DockWindow draggingWindow = MouseDownWindow;
                IsMouseLeftButtonDown = false;
                MouseDownWindow = null;
                Root?.EndTrackingMouse();
                Float2 rootPosition = PointToRoot(location);
                draggingWindow.ShowFloating(new GuiPoint(rootPosition.X, rootPosition.Y), draggingWindow.DefaultSize);
                return;
            }

            if (!IsMouseDownOverCross && _panel.TabsCount > 1)
            {
                GuiRect currentTab = GetTabBounds(MouseDownWindow);
                if (!currentTab.Contains(MousePosition))
                {
                    int index = _panel.GetTabIndex(MouseDownWindow);
                    if (MousePosition.X < currentTab.X)
                        _panel.MoveTabLeft(index);
                    else
                        _panel.MoveTabRight(index);
                }
            }
        }

        public override SE.GUI.DragDropEffect OnDragEnter(Float2 location, SE.GUI.DragData data)
        {
            return SelectTabUnderPointer(location);
        }

        public override SE.GUI.DragDropEffect OnDragMove(Float2 location, SE.GUI.DragData data)
        {
            return SelectTabUnderPointer(location);
        }

        public override void OnDragLeave()
        {
            base.OnDragLeave();
        }

        public override void ClearState()
        {
            IsMouseLeftButtonDown = false;
            IsMouseRightButtonDown = false;
            IsMouseMiddleButtonDown = false;
            IsMouseDownOverCross = false;
            MouseDownWindow = null;
            base.ClearState();
        }

        private DockWindow? GetTabAt(GuiPoint position, out bool isOverCross)
        {
            isOverCross = false;
            if (_panel.TabsCount == 1 && new GuiRect(0, 0, Width, Height).Contains(position))
            {
                DockWindow tab = _panel.FirstTab!;
                GuiRect closeButton = GetCloseButtonBounds(0, Width);
                isOverCross = closeButton.Contains(position);
                return tab;
            }

            float x = 0.0f;
            foreach (DockWindow tab in _panel.Tabs)
            {
                float width = GetTabWidth(tab);
                GuiRect rect = new GuiRect(x, 0, width, Height);
                if (rect.Contains(position))
                {
                    isOverCross = GetCloseButtonBounds(x, width).Contains(position);
                    return tab;
                }
                x += width;
            }

            return null;
        }

        public void ShowContextMenu(DockWindow tab)
        {
            ContextMenu menu = new ContextMenu();
            tab.OnShowContextMenu(menu);
            menu.AddButton("Close", () => tab.Close(ClosingReason.User));
            menu.AddButton("Close All", () => _panel.CloseAll(ClosingReason.User));
            menu.AddButton("Close All But This", () => CloseAllBut(tab));
            int tabIndex = _panel.GetTabIndex(tab);
            if (tabIndex >= 0 && tabIndex < _panel.TabsCount - 1)
                menu.AddButton("Close All To The Right", () => CloseAllToTheRight(tabIndex));
            if (!_panel.IsFloating)
            {
                menu.AddSeparator();
                menu.AddButton("Undock", tab.ShowFloating);
            }
            menu.Show(this, MousePosition.X, MousePosition.Y);
        }

        protected override void OnLayoutChildren()
        {
            _title.Text = _panel.SelectedTab?.Title ?? string.Empty;
            _title.SetBounds(4, 0, Width - 8, Height);
        }

        private GuiRect GetTabBounds(DockWindow tab)
        {
            if (_panel.TabsCount == 1 && ReferenceEquals(_panel.FirstTab, tab))
                return new GuiRect(0, 0, Width, Height);

            float x = 0.0f;
            foreach (DockWindow candidate in _panel.Tabs)
            {
                float width = GetTabWidth(candidate);
                if (ReferenceEquals(candidate, tab))
                    return new GuiRect(x, 0, width, Height);
                x += width;
            }

            return new GuiRect(0, 0, 0, 0);
        }

        private static float GetTabWidth(DockWindow tab)
        {
            return tab.TitleSize.Width + DockPanel.DefaultButtonsSize + DockPanel.DefaultButtonsMargin * 2.0f + DockPanel.DefaultTextMargin * 2.0f;
        }

        private static GuiRect GetCloseButtonBounds(float tabX, float tabWidth)
        {
            return new GuiRect(
                tabX + tabWidth - DockPanel.DefaultButtonsSize - DockPanel.DefaultButtonsMargin,
                (DockPanel.DefaultHeaderHeight - DockPanel.DefaultButtonsSize) * 0.5f,
                DockPanel.DefaultButtonsSize,
                DockPanel.DefaultButtonsSize);
        }

        private SE.GUI.DragDropEffect SelectTabUnderPointer(Float2 location)
        {
            DockWindow? tab = GetTabAt(new GuiPoint(location.X, location.Y));
            if (tab == null)
                return SE.GUI.DragDropEffect.None;

            _panel.SelectTab(tab);
            return SE.GUI.DragDropEffect.Move;
        }

        private void CloseAllBut(DockWindow tab)
        {
            foreach (DockWindow candidate in _panel.Tabs.ToArray())
            {
                if (!ReferenceEquals(candidate, tab))
                    candidate.Close(ClosingReason.User);
            }
        }

        private void CloseAllToTheRight(int tabIndex)
        {
            for (int index = _panel.TabsCount - 1; index > tabIndex; index--)
                _panel.GetTab(index).Close(ClosingReason.User);
        }
    }
}

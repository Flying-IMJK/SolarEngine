using System.Linq;
using SE.GUI;

namespace SE.Editor.GUI
{
    public sealed class DockPanelProxy : ContainerControl
    {
        private readonly DockPanel m_Panel;
        private readonly Label m_Title;

        public DockPanelProxy(DockPanel panel)
            : base(new Rectangle(0, 0, 300, DockPanel.DefaultHeaderHeight))
        {
            m_Panel = panel;
            BackgroundColor = SE.GUI.Style.Current.BackgroundNormal;
            m_Title = new Label(new Rectangle(4, 0, 292, DockPanel.DefaultHeaderHeight), string.Empty)
            {
                AutoFocus = false,
                Enabled = false,
            };
            AddChild(m_Title);
        }

        public bool IsMouseLeftButtonDown { get; set; }
        public bool IsMouseRightButtonDown { get; set; }
        public bool IsMouseMiddleButtonDown { get; set; }
        public bool IsMouseDownOverCross { get; set; }
        public DockWindow? MouseDownWindow { get; set; }
        public Float2 MousePosition { get; set; }
        public DockWindow? StartDragAsyncWindow { get; set; }

        public DockWindow? GetTabAt(Float2 position)
        {
            return GetTabAt(position, out _);
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            MousePosition = location;
            MouseDownWindow = GetTabAt(MousePosition, out bool isOverCross);
            IsMouseDownOverCross = isOverCross;

            switch (button)
            {
            case MouseButton.Left:
                IsMouseLeftButtonDown = true;
                Root?.StartTrackingMouse(this);
                if (!IsMouseDownOverCross && MouseDownWindow != null)
                    m_Panel.SelectTab(MouseDownWindow);
                return MouseDownWindow != null;
            case MouseButton.Right:
                IsMouseRightButtonDown = true;
                if (MouseDownWindow != null)
                    m_Panel.SelectTab(MouseDownWindow, autoFocus: false);
                return MouseDownWindow != null;
            case MouseButton.Middle:
                IsMouseMiddleButtonDown = true;
                return MouseDownWindow != null;
            default:
                return base.OnMouseDown(location, button);
            }
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            MousePosition = location;
            DockWindow? tab = GetTabAt(MousePosition, out bool isOverCross);
            switch (button)
            {
            case MouseButton.Left when IsMouseLeftButtonDown:
                IsMouseLeftButtonDown = false;
                Root?.EndTrackingMouse();
                if (tab != null && ReferenceEquals(tab, MouseDownWindow) && IsMouseDownOverCross && isOverCross)
                    tab.Close(ClosingReason.User);
                MouseDownWindow = null;
                return true;
            case MouseButton.Right when IsMouseRightButtonDown:
                IsMouseRightButtonDown = false;
                if (tab != null)
                    ShowContextMenu(tab);
                MouseDownWindow = null;
                return tab != null;
            case MouseButton.Middle when IsMouseMiddleButtonDown:
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
            MousePosition = location;
            if (!IsMouseLeftButtonDown || MouseDownWindow == null)
            {
                base.OnMouseMove(location);
                return;
            }

            Rectangle header = new Rectangle(0, 0, Width, Height);
            if (!header.Contains(MousePosition))
            {
                DockWindow draggingWindow = MouseDownWindow;
                IsMouseLeftButtonDown = false;
                MouseDownWindow = null;
                Root?.EndTrackingMouse();
                Float2 rootPosition = PointToRoot(location);
                draggingWindow.ShowFloating(rootPosition, draggingWindow.DefaultSize);
                return;
            }

            if (!IsMouseDownOverCross && m_Panel.TabsCount > 1)
            {
                Rectangle currentTab = GetTabBounds(MouseDownWindow);
                if (!currentTab.Contains(MousePosition))
                {
                    int index = m_Panel.GetTabIndex(MouseDownWindow);
                    if (MousePosition.X < currentTab.X)
                        m_Panel.MoveTabLeft(index);
                    else
                        m_Panel.MoveTabRight(index);
                }
            }
        }

        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            DragDropEffect result = base.OnDragEnter(ref location, data);
            if (result != DragDropEffect.None)
                return result;
            return SelectTabUnderPointer(ref location);
        }

        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            DragDropEffect result = base.OnDragMove(ref location, data);
            if (result != DragDropEffect.None)
                return result;
            return SelectTabUnderPointer(ref location);
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

        private DockWindow? GetTabAt(Float2 position, out bool isOverCross)
        {
            isOverCross = false;
            if (m_Panel.TabsCount == 1 && new Rectangle(0, 0, Width, Height).Contains(position))
            {
                DockWindow tab = m_Panel.FirstTab!;
                Rectangle closeButton = GetCloseButtonBounds(0, Width);
                isOverCross = closeButton.Contains(position);
                return tab;
            }

            float x = 0.0f;
            foreach (DockWindow tab in m_Panel.Tabs)
            {
                float width = GetTabWidth(tab);
                Rectangle rect = new Rectangle(x, 0, width, Height);
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
            menu.AddButton("Close All", () => m_Panel.CloseAll(ClosingReason.User));
            menu.AddButton("Close All But This", () => CloseAllBut(tab));
            int tabIndex = m_Panel.GetTabIndex(tab);
            if (tabIndex >= 0 && tabIndex < m_Panel.TabsCount - 1)
                menu.AddButton("Close All To The Right", () => CloseAllToTheRight(tabIndex));
            if (!m_Panel.IsFloating)
            {
                menu.AddSeparator();
                menu.AddButton("Undock", tab.ShowFloating);
            }
            menu.Show(this, MousePosition.X, MousePosition.Y);
        }

        protected override void OnLayoutChildren()
        {
            m_Title.Text = m_Panel.SelectedTab?.Title ?? string.Empty;
            m_Title.SetBounds(4, 0, Width - 8, Height);
        }

        private Rectangle GetTabBounds(DockWindow tab)
        {
            if (m_Panel.TabsCount == 1 && ReferenceEquals(m_Panel.FirstTab, tab))
                return new Rectangle(0, 0, Width, Height);

            float x = 0.0f;
            foreach (DockWindow candidate in m_Panel.Tabs)
            {
                float width = GetTabWidth(candidate);
                if (ReferenceEquals(candidate, tab))
                    return new Rectangle(x, 0, width, Height);
                x += width;
            }

            return new Rectangle(0, 0, 0, 0);
        }

        private static float GetTabWidth(DockWindow tab)
        {
            return tab.TitleSize.X + DockPanel.DefaultButtonsSize + DockPanel.DefaultButtonsMargin * 2.0f + DockPanel.DefaultTextMargin * 2.0f;
        }

        private static Rectangle GetCloseButtonBounds(float tabX, float tabWidth)
        {
            return new Rectangle(
                tabX + tabWidth - DockPanel.DefaultButtonsSize - DockPanel.DefaultButtonsMargin,
                (DockPanel.DefaultHeaderHeight - DockPanel.DefaultButtonsSize) * 0.5f,
                DockPanel.DefaultButtonsSize,
                DockPanel.DefaultButtonsSize);
        }

        private DragDropEffect SelectTabUnderPointer(ref Float2 location)
        {
            DockWindow? tab = GetTabAt(location);
            if (tab == null)
                return DragDropEffect.None;

            m_Panel.SelectTab(tab);
            return DragDropEffect.Move;
        }

        private void CloseAllBut(DockWindow tab)
        {
            foreach (DockWindow candidate in m_Panel.Tabs.ToArray())
            {
                if (!ReferenceEquals(candidate, tab))
                    candidate.Close(ClosingReason.User);
            }
        }

        private void CloseAllToTheRight(int tabIndex)
        {
            for (int index = m_Panel.TabsCount - 1; index > tabIndex; index--)
                m_Panel.GetTab(index).Close(ClosingReason.User);
        }
    }
}

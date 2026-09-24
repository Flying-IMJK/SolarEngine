using System;
using SE.GUI;

namespace SE.Editor.GUI
{
    public sealed class DockPanelProxy : ContainerControl
    {
        private readonly DockPanel m_Panel;
        private double m_DragEnterTime = -1.0;

        public DockPanelProxy(DockPanel panel)
            : base(new Rectangle(0, 0, 64, 64))
        {
            AutoFocus = false;
            AnchorMin = Float2.Zero;
            AnchorMax = Float2.One;
            Offsets = Margin.Zero;
            m_Panel = panel;
        }

        public bool IsMouseLeftButtonDown { get; set; }
        public bool IsMouseRightButtonDown { get; set; }
        public bool IsMouseMiddleButtonDown { get; set; }
        public bool IsMouseDownOverCross { get; set; }
        public DockWindow? MouseDownWindow { get; set; }
        public Float2 MousePosition { get; set; } = Float2.Minimum;
        public DockWindow? StartDragAsyncWindow { get; set; }

        public DockWindow? GetTabAt(Float2 position) => GetTabAt(position, out _);

        public override void Draw()
        {
            base.Draw();
            if (!VisibleInHierarchy || IsDisposed)
                return;

            Style style = Style.Current;
            Font? font = style.FontMedium;
            Rectangle header = ToScreen(GetHeaderRectangle());
            int tabsCount = m_Panel.TabsCount;
            bool containsFocus = ContainsFocus &&
                                 (Root is not WindowRootControl root || root.Window.IsFocused);
            if (tabsCount > 0)
                Render2D.FillRectangle(header, style.BackgroundHighlighted);

            if (tabsCount == 1)
            {
                DockWindow tab = m_Panel.GetTab(0);
                Float2 titleSize = font?.MeasureText(tab.Title, new TextLayoutOptions()) ?? tab.TitleSize;
                float iconWidth = tab.Icon.IsValid
                    ? Math.Max(DockPanel.DefaultButtonsSize, header.Height - DockPanel.DefaultButtonsMargin)
                    : 0.0f;
                float tabSize = iconWidth + DockPanel.DefaultTextMargin + titleSize.X +
                                DockPanel.DefaultButtonsSize * 2.0f + DockPanel.DefaultButtonsMargin;
                Rectangle tabRect = ToScreen(new Rectangle(0, 0, tabSize, DockPanel.DefaultHeaderHeight));
                bool isMouseOver = GetHeaderRectangle().Contains(MousePosition);
                if (containsFocus)
                    Render2D.FillRectangle(tabRect, style.BackgroundSelected);

                if (tab.Icon.IsValid)
                {
                    Render2D.DrawSprite(tab.Icon,
                        ToScreen(new Rectangle(0, 0, iconWidth, iconWidth)),
                        style.Foreground);
                }
                if (font != null)
                {
                    Rectangle textRect = ToScreen(new Rectangle(
                        iconWidth + DockPanel.DefaultTextMargin,
                        DockPanel.DefaultTextMargin,
                        titleSize.X,
                        DockPanel.DefaultHeaderHeight - DockPanel.DefaultTextMargin));
                    Render2D.DrawText(font, tab.Title, textRect, style.TextColor,
                        TextAlignment.Center, TextAlignment.Center);
                }

                Rectangle cross = GetSingleTabDrawCloseButtonBounds(titleSize.X, iconWidth);
                bool overCross = isMouseOver && cross.Contains(MousePosition);
                Rectangle screenCross = ToScreen(cross);
                if (overCross)
                    Render2D.FillRectangle(screenCross, (containsFocus ? style.BackgroundSelected : style.LightBackground) * 1.3f);
                if (style.Cross.IsValid)
                    Render2D.DrawSprite(style.Cross, screenCross, style.ForegroundGrey);
                return;
            }

            Render2D.FillRectangle(header, style.LightBackground);
            float x = 0.0f;
            foreach (DockWindow tab in m_Panel.Tabs)
            {
                float width = GetTabWidth(tab);
                Rectangle tabRect = new Rectangle(x, 0, width, DockPanel.DefaultHeaderHeight);
                bool isMouseOver = tabRect.Contains(MousePosition);
                bool isSelected = ReferenceEquals(m_Panel.SelectedTab, tab);
                Color tabColor = style.BackgroundHighlighted;
                if (isSelected)
                {
                    tabColor = containsFocus ? style.BackgroundSelected : style.BackgroundNormal;
                    Render2D.FillRectangle(ToScreen(tabRect), tabColor);
                }
                else if (isMouseOver)
                {
                    Render2D.FillRectangle(ToScreen(tabRect), tabColor);
                }
                else
                {
                    Render2D.DrawLine(ScreenPos + tabRect.BottomLeft - new Float2(0, 1), ScreenPos + tabRect.UpperLeft, tabColor);
                    Render2D.DrawLine(ScreenPos + tabRect.BottomRight - new Float2(0, 1), ScreenPos + tabRect.UpperRight, tabColor);
                }

                float iconWidth = tab.Icon.IsValid ? DockPanel.DefaultButtonsSize + DockPanel.DefaultTextMargin : 0.0f;
                if (tab.Icon.IsValid)
                {
                    Render2D.DrawSprite(tab.Icon,
                        ToScreen(new Rectangle(x + DockPanel.DefaultTextMargin,
                            (DockPanel.DefaultHeaderHeight - DockPanel.DefaultButtonsSize) * 0.5f,
                            DockPanel.DefaultButtonsSize, DockPanel.DefaultButtonsSize)),
                        style.Foreground);
                }
                if (font != null)
                {
                    Render2D.DrawText(font, tab.Title,
                        ToScreen(new Rectangle(x + DockPanel.DefaultTextMargin + iconWidth, 0, 10000, DockPanel.DefaultHeaderHeight)),
                        style.TextColor, TextAlignment.Near, TextAlignment.Center);
                }

                if (isSelected || isMouseOver)
                {
                    Rectangle cross = GetCloseButtonBounds(x, width);
                    bool overCross = isMouseOver && cross.Contains(MousePosition);
                    if (overCross)
                        Render2D.FillRectangle(ToScreen(cross), tabColor * 1.3f);
                    if (style.Cross.IsValid)
                        Render2D.DrawSprite(style.Cross, ToScreen(cross), overCross ? style.Foreground : style.ForegroundGrey);
                }
                x += width;
            }

            Render2D.FillRectangle(
                ToScreen(new Rectangle(0, DockPanel.DefaultHeaderHeight - 2, Width, 2)),
                containsFocus ? style.BackgroundSelected : style.BackgroundNormal);
        }

        public override void OnLostFocus()
        {
            IsMouseLeftButtonDown = false;
            IsMouseRightButtonDown = false;
            IsMouseMiddleButtonDown = false;
            MouseDownWindow = null;
            MousePosition = Float2.Minimum;
            base.OnLostFocus();
        }

        public override void OnMouseEnter()
        {
            if (Root != null)
                MousePosition = PointFromRoot(Root.MousePosition);
            base.OnMouseEnter();
        }

        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            DockWindow? tab = GetTabAt(location);
            if (tab?.Root is WindowRootControl root && button == MouseButton.Left)
            {
                if (root.Window.IsMaximized)
                    root.Window.Restore();
                else
                    root.Window.Maximize();
                return true;
            }
            return base.OnMouseDoubleClick(location, button);
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            MousePosition = location;
            MouseDownWindow = GetTabAt(location, out bool overCross);
            // Native uses a local with the same name as the member, so the member stays unchanged.
            if (button == MouseButton.Left)
            {
                IsMouseLeftButtonDown = true;
                if (!overCross && MouseDownWindow != null)
                    m_Panel.SelectTab(MouseDownWindow);
            }
            else if (button == MouseButton.Right)
            {
                IsMouseRightButtonDown = true;
                if (MouseDownWindow != null)
                    m_Panel.SelectTab(MouseDownWindow, false);
            }
            else if (button == MouseButton.Middle)
            {
                IsMouseMiddleButtonDown = true;
            }
            return base.OnMouseDown(location, button);
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            MousePosition = location;
            DockWindow? tab = GetTabAt(location, out bool overCross);
            if (button == MouseButton.Left && IsMouseLeftButtonDown)
            {
                IsMouseLeftButtonDown = false;
                if (tab != null && ReferenceEquals(tab, MouseDownWindow) && IsMouseDownOverCross && overCross)
                    tab.Close(ClosingReason.User);
                MouseDownWindow = null;
            }
            else if (button == MouseButton.Right && IsMouseRightButtonDown)
            {
                IsMouseRightButtonDown = false;
                if (tab != null)
                    ShowContextMenu(tab, location);
            }
            else if (button == MouseButton.Middle && IsMouseMiddleButtonDown)
            {
                IsMouseMiddleButtonDown = false;
                tab?.Close(ClosingReason.User);
            }
            return base.OnMouseUp(location, button);
        }

        public override void OnMouseMove(Float2 location)
        {
            MousePosition = location;
            if (IsMouseLeftButtonDown)
            {
                if (!GetHeaderRectangle().Contains(location))
                {
                    IsMouseLeftButtonDown = false;
                    if (!IsMouseDownOverCross && MouseDownWindow != null)
                        StartDrag(MouseDownWindow);
                    MouseDownWindow = null;
                }
                else if (MouseDownWindow != null && m_Panel.TabsCount > 1)
                {
                    Rectangle current = GetTabBounds(MouseDownWindow);
                    if (!current.Contains(location))
                    {
                        int index = m_Panel.GetTabIndex(MouseDownWindow);
                        if (location.X < current.X)
                            m_Panel.MoveTabLeft(index);
                        else if (!ReferenceEquals(m_Panel.LastTab, MouseDownWindow))
                            m_Panel.MoveTabRight(index);
                        m_Panel.PerformLayout();
                    }
                }
            }
            base.OnMouseMove(location);
        }

        public override void OnMouseLeave()
        {
            if (IsMouseLeftButtonDown)
            {
                IsMouseLeftButtonDown = false;
                if (!IsMouseDownOverCross && MouseDownWindow != null)
                    StartDrag(MouseDownWindow);
                MouseDownWindow = null;
            }
            IsMouseRightButtonDown = false;
            IsMouseMiddleButtonDown = false;
            MousePosition = Float2.Minimum;
            base.OnMouseLeave();
        }

        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            DragDropEffect result = base.OnDragEnter(ref location, data);
            return result != DragDropEffect.None ? result : SelectTabUnderPointer(location);
        }

        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            DragDropEffect result = base.OnDragMove(ref location, data);
            return result != DragDropEffect.None ? result : SelectTabUnderPointer(location);
        }

        public override void OnDragLeave()
        {
            m_DragEnterTime = -1.0;
            base.OnDragLeave();
        }

        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);
            if (StartDragAsyncWindow != null)
                StartDragAsync();
        }

        protected override void OnLayoutChildren()
        {
            DockWindow? selected = m_Panel.SelectedTab;
            selected?.SetBounds(
                0.0f,
                DockPanel.DefaultHeaderHeight,
                Width,
                Math.Max(0.0f, Height - DockPanel.DefaultHeaderHeight));
        }

        public void ShowContextMenu(DockWindow tab, Float2 location)
        {
            ContextMenu menu = new ContextMenu();
            tab.OnShowContextMenu(menu);
            menu.AddButton("Close", () =>
            {
                tab.Close(ClosingReason.User);
                if (ReferenceEquals(tab, MouseDownWindow))
                    MouseDownWindow = null;
            });
            menu.AddButton("Close All", () => m_Panel.CloseAll());
            menu.AddButton("Close All But This", () => CloseAllBut(tab));
            if (m_Panel.GetTabIndex(tab) + 1 < m_Panel.TabsCount)
                menu.AddButton("Close All To The Right", () => CloseAllToTheRight(tab));

            if (!m_Panel.IsFloating)
            {
                menu.AddSeparator();
                menu.AddButton("Undock", tab.ShowFloating);
            }
            else if (tab.Root is WindowRootControl root)
            {
                menu.AddSeparator();
                if (root.Window.IsMaximized)
                    menu.AddButton("Restore", root.Window.Restore);
                else
                    menu.AddButton("Maximize", root.Window.Maximize);
            }
            menu.Show(this, location.X, location.Y);
        }

        private Rectangle GetHeaderRectangle() => new Rectangle(0, 0, Width, DockPanel.DefaultHeaderHeight);

        private DockWindow? GetTabAt(Float2 position, out bool closeButton)
        {
            closeButton = false;
            if (m_Panel.TabsCount == 1)
            {
                if (GetHeaderRectangle().Contains(position))
                {
                    closeButton = GetSingleTabHitCloseButtonBounds().Contains(position);
                    return m_Panel.GetTab(0);
                }
                return null;
            }

            float x = 0.0f;
            foreach (DockWindow tab in m_Panel.Tabs)
            {
                float width = GetTabWidth(tab);
                if (new Rectangle(x, 0, width, DockPanel.DefaultHeaderHeight).Contains(position))
                {
                    closeButton = GetCloseButtonBounds(x, width).Contains(position);
                    return tab;
                }
                x += width;
            }
            return null;
        }

        private Rectangle GetTabBounds(DockWindow tab)
        {
            if (m_Panel.TabsCount == 1)
                return GetHeaderRectangle();
            float x = 0.0f;
            foreach (DockWindow candidate in m_Panel.Tabs)
            {
                // Keep the native drag-reorder rectangle, including its omission of icon width.
                float width = candidate.TitleSize.X + DockPanel.DefaultButtonsSize +
                              2.0f * DockPanel.DefaultButtonsMargin + 2.0f * DockPanel.DefaultTextMargin;
                if (ReferenceEquals(candidate, tab))
                    return new Rectangle(x, 0, width, DockPanel.DefaultHeaderHeight);
                x += width;
            }
            return Rectangle.Empty;
        }

        private static float GetTabWidth(DockWindow tab)
        {
            float iconWidth = tab.Icon.IsValid ? DockPanel.DefaultButtonsSize + DockPanel.DefaultTextMargin : 0.0f;
            return tab.TitleSize.X + DockPanel.DefaultButtonsSize +
                   2.0f * DockPanel.DefaultButtonsMargin + 2.0f * DockPanel.DefaultTextMargin + iconWidth;
        }

        private static Rectangle GetCloseButtonBounds(float tabX, float tabWidth)
        {
            return new Rectangle(
                tabX + tabWidth - DockPanel.DefaultButtonsSize - DockPanel.DefaultButtonsMargin,
                (DockPanel.DefaultHeaderHeight - DockPanel.DefaultButtonsSize) * 0.5f,
                DockPanel.DefaultButtonsSize,
                DockPanel.DefaultButtonsSize);
        }

        private Rectangle GetSingleTabHitCloseButtonBounds()
        {
            return new Rectangle(
                Width - DockPanel.DefaultButtonsSize - DockPanel.DefaultButtonsMargin,
                (DockPanel.DefaultHeaderHeight - DockPanel.DefaultButtonsSize) * 0.5f,
                DockPanel.DefaultButtonsSize,
                DockPanel.DefaultButtonsSize);
        }

        private static Rectangle GetSingleTabDrawCloseButtonBounds(float titleWidth, float iconWidth)
        {
            return new Rectangle(
                iconWidth + DockPanel.DefaultTextMargin + titleWidth + DockPanel.DefaultButtonsSize,
                (DockPanel.DefaultHeaderHeight - DockPanel.DefaultButtonsSize) * 0.5f,
                DockPanel.DefaultButtonsSize,
                DockPanel.DefaultButtonsSize);
        }

        private Rectangle ToScreen(Rectangle local)
            => new Rectangle(ScreenPos + local.Location, local.Size);

        private void StartDrag(DockWindow window)
        {
            MouseDownWindow = null;
            StartDragAsyncWindow = window;
            StartDragAsync();
        }

        private void StartDragAsync()
        {
            DockWindow? window = StartDragAsyncWindow;
            StartDragAsyncWindow = null;
            if (window == null)
                return;

            if (m_Panel.ChildPanelsCount == 0 && m_Panel.TabsCount == 1 && m_Panel is FloatWindowDockPanel floating)
            {
                DockHintWindow.Create(floating);
            }
            else
            {
                int index = m_Panel.GetTabIndex(window);
                if (index == 0)
                    index = m_Panel.TabsCount;
                m_Panel.SelectTab(index - 1);
                DockHintWindow.Create(window);
            }
        }

        private DragDropEffect SelectTabUnderPointer(Float2 location)
        {
            DockWindow? tab = GetTabAt(location);
            if (tab == null)
            {
                m_DragEnterTime = -1.0;
                return DragDropEffect.None;
            }

            double time = Environment.TickCount64 * 0.001;
            if (m_DragEnterTime < 0.0)
                m_DragEnterTime = time;
            if (time - m_DragEnterTime < 0.3)
                return DragDropEffect.Link;

            m_DragEnterTime = -1.0;
            m_Panel.SelectTab(tab);
            Update(0.0f);
            return DragDropEffect.Move;
        }

        private void CloseAllBut(DockWindow tab)
        {
            // Forward iteration matches native mutation behavior exactly.
            for (int index = 0; index < m_Panel.TabsCount; index++)
            {
                DockWindow candidate = m_Panel.GetTab(index);
                if (!ReferenceEquals(candidate, tab))
                    candidate.Close();
            }
        }

        private void CloseAllToTheRight(DockWindow tab)
        {
            for (int index = m_Panel.GetTabIndex(tab) + 1; index < m_Panel.TabsCount; index++)
                m_Panel.GetTab(index).Close();
        }

    }
}

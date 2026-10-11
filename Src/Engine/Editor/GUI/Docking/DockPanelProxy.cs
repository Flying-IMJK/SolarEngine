#nullable disable annotations

using System;
using System.Collections.Generic;
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Proxy control used for docking <see cref="DockWindow"/> inside <see cref="DockPanel"/>.
    /// </summary>
    /// <seealso cref="ContainerControl" />
    public class DockPanelProxy : ContainerControl
    {
        /// <summary>
        /// The is mouse down flag (left button).
        /// </summary>
        public bool IsMouseLeftButtonDown = false;

        /// <summary>
        /// The is mouse down flag (right button).
        /// </summary>
        public bool IsMouseRightButtonDown = false;

        /// <summary>
        /// The is mouse down flag (middle button).
        /// </summary>
        public bool IsMouseMiddleButtonDown = false;

        /// <summary>
        /// The is mouse down over cross button flag.
        /// </summary>
        public bool IsMouseDownOverCross = false;

        /// <summary>
        /// The mouse down window.
        /// </summary>
        public DockWindow MouseDownWindow = null;

        /// <summary>
        /// The mouse position.
        /// </summary>
        public Float2 MousePosition = Float2.Minimum;

        /// <summary>
        /// The start drag asynchronous window.
        /// </summary>
        public DockWindow StartDragAsyncWindow = null;

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // Cache data
            Style style = Style.Current;
            RootControl window = Root;
            bool containsFocus = ContainsFocus && ((WindowRootControl)window).Window.IsFocused;
            Rectangle headerRect = GetHeaderRectangle();
            int tabsCount = _panel.TabsCount;

            if (tabsCount > 0)
            {
                Render2D.FillRectangle(headerRect, style.BackgroundHighlighted);
            }

            // Check if has only one window docked
            if (tabsCount == 1)
            {
                DockWindow tab = _panel.GetTab(0);

                string tabTitle = tab.Title;
                Float2 tabTitleSize = style.FontMedium.MeasureText(tabTitle, TextLayoutOptions.Default);
                float iconWidth = tab.Icon.IsValid
                    ? Math.Max(DockPanel.DefaultButtonsSize, headerRect.Height - DockPanel.DefaultButtonsMargin)
                    : 0;
                float tabSize = iconWidth + DockPanel.DefaultTextMargin + tabTitleSize.X +
                    DockPanel.DefaultButtonsSize * 2 + DockPanel.DefaultButtonsMargin;

                // Draw header
                Rectangle tabRect = new Rectangle(headerRect.Left, headerRect.Y, tabSize, headerRect.Height);
                bool isMouseOver = headerRect.Contains(MousePosition);
                if (containsFocus)
                {
                    Render2D.FillRectangle(tabRect, style.BackgroundSelected);
                }

                if (tab.Icon.IsValid)
                {
                    Render2D.DrawSprite(
                        tab.Icon,
                        new Rectangle(tabRect.Left, tabRect.Y, iconWidth, iconWidth),
                        style.Foreground);
                }

                // Draw text
                Rectangle textRect = new Rectangle(
                    tabRect.Left + iconWidth + DockPanel.DefaultTextMargin,
                    tabRect.Y + DockPanel.DefaultTextMargin,
                    tabTitleSize.X,
                    tabRect.Height - DockPanel.DefaultTextMargin);

                Render2D.DrawText(
                    style.FontMedium,
                    tabTitle,
                    textRect,
                    style.TextColor,
                    TextAlignment.Center,
                    TextAlignment.Center);

                // Draw cross
                Rectangle crossRect = new Rectangle(
                    textRect.Right + DockPanel.DefaultButtonsSize,
                    (tabRect.Height - DockPanel.DefaultButtonsSize) / 2,
                    DockPanel.DefaultButtonsSize,
                    DockPanel.DefaultButtonsSize);
                bool isMouseOverCross = isMouseOver && crossRect.Contains(MousePosition);
                if (isMouseOverCross)
                {
                    Render2D.FillRectangle(crossRect,
                        (containsFocus ? style.BackgroundSelected : style.LightBackground) * 1.3f);
                }
                Render2D.DrawSprite(style.Cross, crossRect, style.ForegroundGrey);
            }
            else
            {
                // Draw background
                Render2D.FillRectangle(headerRect, style.LightBackground);

                // Render all tabs
                float x = 0;
                for (int i = 0; i < tabsCount; i++)
                {
                    // Cache data
                    DockWindow tab = _panel.GetTab(i);
                    Color tabColor = Color.Black;
                    Float2 titleSize = tab.TitleSize;
                    float iconWidth = tab.Icon.IsValid
                        ? DockPanel.DefaultButtonsSize + DockPanel.DefaultTextMargin
                        : 0;
                    float width = titleSize.X + DockPanel.DefaultButtonsSize +
                        2 * DockPanel.DefaultButtonsMargin +
                        DockPanel.DefaultTextMargin + DockPanel.DefaultTextMargin + iconWidth;
                    Rectangle tabRect = new Rectangle(x, 0, width, DockPanel.DefaultHeaderHeight);
                    bool isMouseOver = tabRect.Contains(MousePosition);
                    bool isSelected = _panel.SelectedTab == tab;

                    // Check if tab is selected
                    if (isSelected)
                    {
                        tabColor = containsFocus ? style.BackgroundSelected : style.BackgroundNormal;
                        Render2D.FillRectangle(tabRect, tabColor);
                    }
                    // Check if mouse is over
                    else if (isMouseOver)
                    {
                        tabColor = style.BackgroundHighlighted;
                        Render2D.FillRectangle(tabRect, tabColor);
                    }
                    else
                    {
                        tabColor = style.BackgroundHighlighted;
                        Render2D.DrawLine(tabRect.BottomLeft - new Float2(0, 1), tabRect.UpperLeft, tabColor);
                        Render2D.DrawLine(tabRect.BottomRight - new Float2(0, 1), tabRect.UpperRight, tabColor);
                    }

                    if (tab.Icon.IsValid)
                    {
                        Render2D.DrawSprite(
                            tab.Icon,
                            new Rectangle(
                                x + DockPanel.DefaultTextMargin,
                                (DockPanel.DefaultHeaderHeight - DockPanel.DefaultButtonsSize) / 2,
                                DockPanel.DefaultButtonsSize,
                                DockPanel.DefaultButtonsSize),
                            style.Foreground);
                    }

                    // Draw text
                    Render2D.DrawText(
                        style.FontMedium,
                        tab.Title,
                        new Rectangle(
                            x + DockPanel.DefaultTextMargin + iconWidth,
                            0,
                            10000,
                            DockPanel.DefaultHeaderHeight),
                        style.TextColor,
                        TextAlignment.Near,
                        TextAlignment.Center);

                    // Draw cross
                    if (isSelected || isMouseOver)
                    {
                        Rectangle crossRect = new Rectangle(
                            x + width - DockPanel.DefaultButtonsSize - DockPanel.DefaultButtonsMargin,
                            (DockPanel.DefaultHeaderHeight - DockPanel.DefaultButtonsSize) / 2,
                            DockPanel.DefaultButtonsSize,
                            DockPanel.DefaultButtonsSize);
                        bool isMouseOverCross = isMouseOver && crossRect.Contains(MousePosition);
                        if (isMouseOverCross)
                        {
                            Render2D.FillRectangle(crossRect, tabColor * 1.3f);
                        }
                        Render2D.DrawSprite(style.Cross, crossRect,
                            isMouseOverCross ? style.Foreground : style.ForegroundGrey);
                    }

                    // Move
                    x += width;
                }

                // Draw selected tab strip
                Render2D.FillRectangle(
                    new Rectangle(0, DockPanel.DefaultHeaderHeight - 2, Width, 2),
                    containsFocus ? style.BackgroundSelected : style.BackgroundNormal);
            }
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            IsMouseLeftButtonDown = false;
            IsMouseRightButtonDown = false;
            IsMouseMiddleButtonDown = false;
            MouseDownWindow = null;
            MousePosition = Float2.Minimum;

            base.OnLostFocus();
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            MousePosition = location;

            base.OnMouseEnter(location);
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            // Maximize/restore on double click
            bool closeButton;
            DockWindow tab = GetTabAtPos(location, out closeButton);

            WindowRootControl rootWindow = null;
            if (tab != null)
            {
                rootWindow = tab.RootWindow();
            }
            if (rootWindow != null && button == MouseButton.Left)
            {
                if (rootWindow.IsMaximized())
                {
                    rootWindow.Restore();
                }
                else
                {
                    rootWindow.Maximize();
                }
                return true;
            }

            return base.OnMouseDoubleClick(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            bool IsMouseDownOverCross;
            MouseDownWindow = GetTabAtPos(location, out IsMouseDownOverCross);

            // Check buttons
            if (button == MouseButton.Left)
            {
                // Cache data
                IsMouseLeftButtonDown = true;
                if (!IsMouseDownOverCross && MouseDownWindow != null)
                {
                    _panel.SelectTab(MouseDownWindow);
                }
            }
            else if (button == MouseButton.Right)
            {
                // Cache data
                IsMouseRightButtonDown = true;
                if (MouseDownWindow != null)
                {
                    _panel.SelectTab(MouseDownWindow, false);
                }
            }
            else if (button == MouseButton.Middle)
            {
                // Cache data
                IsMouseMiddleButtonDown = true;
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            // Check tabs under mouse position at the beginning and at the end
            bool overCross;
            DockWindow tab = GetTabAtPos(location, out overCross);

            // Check buttons
            if (button == MouseButton.Left && IsMouseLeftButtonDown)
            {
                // Clear flag
                IsMouseLeftButtonDown = false;

                // Check if tabs are the same and cross was pressed
                if (tab != null && tab == MouseDownWindow && IsMouseDownOverCross && overCross)
                {
                    tab.Close(ClosingReason.User);
                }
                MouseDownWindow = null;
            }
            else if (button == MouseButton.Right && IsMouseRightButtonDown)
            {
                // Clear flag
                IsMouseRightButtonDown = false;

                if (tab != null)
                {
                    ShowContextMenu(tab, location);
                }
            }
            else if (button == MouseButton.Middle && IsMouseMiddleButtonDown)
            {
                // Clear flag
                IsMouseMiddleButtonDown = false;

                if (tab != null)
                {
                    tab.Close(ClosingReason.User);
                }
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            MousePosition = location;
            if (IsMouseLeftButtonDown)
            {
                // Check if mouse is outside the header
                if (!GetHeaderRectangle().Contains(location))
                {
                    // Clear flag
                    IsMouseLeftButtonDown = false;

                    // Check tab under the mouse
                    if (!IsMouseDownOverCross && MouseDownWindow != null)
                    {
                        StartDrag(MouseDownWindow);
                    }
                    MouseDownWindow = null;
                }
                // Check if has more than one tab to change order
                else if (MouseDownWindow != null && _panel.TabsCount > 1)
                {
                    // Check if mouse left current tab rect
                    Rectangle currWinRect = GetTabRect(MouseDownWindow);
                    if (!currWinRect.Contains(location))
                    {
                        int index = _panel.GetTabIndex(MouseDownWindow);

                        // Check if move right or left
                        if (location.X < currWinRect.X)
                        {
                            // Move left
                            _panel.MoveTabLeft(index);
                        }
                        else if (_panel.LastTab != MouseDownWindow)
                        {
                            // Move right
                            _panel.MoveTabRight(index);
                        }

                        // Update
                        _panel.PerformLayout();
                    }
                }
            }

            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            if (IsMouseLeftButtonDown)
            {
                IsMouseLeftButtonDown = false;

                // Check tabs under mouse position
                if (!IsMouseDownOverCross && MouseDownWindow != null)
                {
                    StartDrag(MouseDownWindow);
                }
                MouseDownWindow = null;
            }
            IsMouseRightButtonDown = false;
            IsMouseMiddleButtonDown = false;
            MousePosition = Float2.Minimum;

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            DragDropEffect result = base.OnDragEnter(ref location, data);
            if (result != DragDropEffect.None)
            {
                return result;
            }
            return TrySelectTabUnderLocation(location);
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            DragDropEffect result = base.OnDragMove(ref location, data);
            if (result != DragDropEffect.None)
            {
                return result;
            }
            return TrySelectTabUnderLocation(location);
        }

        /// <inheritdoc />
        public override void OnDragLeave()
        {
            _dragEnterTime = -1;

            base.OnDragLeave();
        }

        /// <inheritdoc />
        public override Rectangle GetDesireClientArea()
        {
            return new Rectangle(0, DockPanel.DefaultHeaderHeight,
                Width, Height - DockPanel.DefaultHeaderHeight);
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="DockPanelProxy"/> class.
        /// </summary>
        /// <param name="panel">The panel.</param>
        public DockPanelProxy(DockPanel panel) : base(0, 0, 64, 64)
        {
            AutoFocus = false;

            _panel = panel;
            AnchorPreset = AnchorPresets.StretchAll;
            Offsets = Margin.Zero;
        }

        private DockPanel _panel = null;
        private double _dragEnterTime = -1;

        private Rectangle GetHeaderRectangle()
        {
            return new Rectangle(0, 0, Width, DockPanel.DefaultHeaderHeight);
        }

        private DockWindow GetTabAtPos(Float2 position, out bool closeButton)
        {
            DockWindow result = null;
            closeButton = false;

            int tabsCount = _panel.TabsCount;
            if (tabsCount == 1)
            {
                Rectangle crossRect = new Rectangle(
                    Width - DockPanel.DefaultButtonsSize - DockPanel.DefaultButtonsMargin,
                    (DockPanel.DefaultHeaderHeight - DockPanel.DefaultButtonsSize) / 2,
                    DockPanel.DefaultButtonsSize,
                    DockPanel.DefaultButtonsSize);
                if (GetHeaderRectangle().Contains(position))
                {
                    closeButton = crossRect.Contains(position);
                    result = _panel.GetTab(0);
                }
            }
            else
            {
                float x = 0;
                for (int i = 0; i < tabsCount; i++)
                {
                    DockWindow tab = _panel.GetTab(i);
                    Float2 titleSize = tab.TitleSize;
                    float iconWidth = tab.Icon.IsValid
                        ? DockPanel.DefaultButtonsSize + DockPanel.DefaultTextMargin
                        : 0;
                    float width = titleSize.X + DockPanel.DefaultButtonsSize +
                        2 * DockPanel.DefaultButtonsMargin +
                        DockPanel.DefaultTextMargin + DockPanel.DefaultTextMargin + iconWidth;
                    Rectangle tabRect = new Rectangle(x, 0, width, DockPanel.DefaultHeaderHeight);
                    bool isMouseOver = tabRect.Contains(position);
                    if (isMouseOver)
                    {
                        Rectangle crossRect = new Rectangle(
                            x + width - DockPanel.DefaultButtonsSize - DockPanel.DefaultButtonsMargin,
                            (DockPanel.DefaultHeaderHeight - DockPanel.DefaultButtonsSize) / 2,
                            DockPanel.DefaultButtonsSize,
                            DockPanel.DefaultButtonsSize);
                        closeButton = crossRect.Contains(position);
                        result = tab;
                        break;
                    }
                    x += width;
                }
            }

            return result;
        }

        private Rectangle GetTabRect(DockWindow win)
        {
            SE.Debug.Assert(_panel.ContainsTab(win), "Window is not a tab of this panel.");

            int tabsCount = _panel.TabsCount;
            if (tabsCount == 1)
            {
                return GetHeaderRectangle();
            }
            else
            {
                float x = 0;
                for (int i = 0; i < tabsCount; i++)
                {
                    DockWindow tab = _panel.GetTab(i);
                    Float2 titleSize = tab.TitleSize;
                    float width = titleSize.X + DockPanel.DefaultButtonsSize +
                        2 * DockPanel.DefaultButtonsMargin +
                        DockPanel.DefaultTextMargin + DockPanel.DefaultTextMargin;
                    if (tab == win)
                    {
                        return new Rectangle(x, 0, width, DockPanel.DefaultHeaderHeight);
                    }
                    x += width;
                }
            }

            return Rectangle.Empty;
        }

        private void StartDrag(DockWindow win)
        {
            // Clear cache
            MouseDownWindow = null;
            StartDragAsyncWindow = win;

            // Register for late update in an async manner (to prevent from crash due to changing UI structure on window undock)
            StartDragAsync();
        }

        private void StartDragAsync()
        {
            if (StartDragAsyncWindow != null)
            {
                DockWindow win = StartDragAsyncWindow;
                StartDragAsyncWindow = null;

                // Check if has only one window docked and is floating
                if (_panel.ChildPanelsCount == 0 && _panel.TabsCount == 1 && _panel.IsFloating)
                {
                    // Create docking hint window but in an async manner
                    DockHintWindow.Create(_panel as FloatWindowDockPanel);
                }
                else
                {
                    // Select another tab
                    int index = _panel.GetTabIndex(win);
                    if (index == 0)
                    {
                        index = _panel.TabsCount;
                    }
                    _panel.SelectTab(index - 1);

                    // Create docking hint window
                    DockHintWindow.Create(win);
                }
            }
        }

        private DragDropEffect TrySelectTabUnderLocation(Float2 location)
        {
            bool closeButton;
            DockWindow tab = GetTabAtPos(location, out closeButton);
            if (tab != null)
            {
                // Auto-select tab only if drag takes some time
                double time = Environment.TickCount64 * 0.001;
                if (_dragEnterTime < 0)
                {
                    _dragEnterTime = time;
                }
                if (time - _dragEnterTime < 0.3f)
                {
                    return DragDropEffect.Link;
                }
                _dragEnterTime = -1;

                _panel.SelectTab(tab);
                Update(0); // Fake update
                return DragDropEffect.Move;
            }
            _dragEnterTime = -1;
            return DragDropEffect.None;
        }

        private void ShowContextMenu(DockWindow tab, Float2 location)
        {
            ContextMenu menu = new ContextMenu();
            menu.Tag = tab;

            tab.OnShowContextMenu(menu);
            menu.AddButton("Close", OnTabMenuCloseClicked);
            menu.AddButton("Close All", OnTabMenuCloseAllClicked);
            menu.AddButton("Close All But This", OnTabMenuCloseAllButThisClicked);
            if (_panel.Tabs.IndexOf(tab) + 1 < _panel.TabsCount)
            {
                menu.AddButton("Close All To The Right", OnTabMenuCloseAllToTheRightClicked);
            }
            if (!_panel.IsFloating)
            {
                menu.AddSeparator();
                menu.AddButton("Undock", OnTabMenuUndockClicked);
            }
            else if (!(tab.RootWindow() != null && tab.RootWindow().IsMaximized()))
            {
                menu.AddSeparator();
                menu.AddButton("Maximize", OnTabMenuMaximizeClicked);
            }
            else if (tab.RootWindow() != null && tab.RootWindow().IsMaximized())
            {
                menu.AddSeparator();
                menu.AddButton("Restore", OnTabMenuRestoreClicked);
            }
            menu.Show(this, location);
        }

        private void OnTabMenuCloseClicked(ContextMenuButton button)
        {
            DockWindow tab = (DockWindow)button.ParentContextMenu.Tag;
            tab.Close(ClosingReason.User);
            if (tab == MouseDownWindow)
            {
                MouseDownWindow = null;
            }
        }

        private void OnTabMenuCloseAllClicked(ContextMenuButton button)
        {
            _panel.CloseAll();
        }

        private void OnTabMenuCloseAllButThisClicked(ContextMenuButton button)
        {
            DockWindow tab = (DockWindow)button.ParentContextMenu.Tag;
            List<DockWindow> windows = _panel.Tabs;
            for (int i = 0; i < windows.Count; i++)
            {
                if (windows[i] != tab)
                {
                    windows[i].Close();
                }
            }
        }

        private void OnTabMenuCloseAllToTheRightClicked(ContextMenuButton button)
        {
            DockWindow tab = (DockWindow)button.ParentContextMenu.Tag;
            List<DockWindow> windows = _panel.Tabs;
            for (int i = _panel.Tabs.IndexOf(tab) + 1; i < windows.Count; i++)
            {
                windows[i].Close();
            }
        }

        private void OnTabMenuUndockClicked(ContextMenuButton button)
        {
            DockWindow tab = (DockWindow)button.ParentContextMenu.Tag;
            tab.ShowFloating();
        }

        private void OnTabMenuMaximizeClicked(ContextMenuButton button)
        {
            DockWindow tab = (DockWindow)button.ParentContextMenu.Tag;
            tab.RootWindow().Maximize();
        }

        private void OnTabMenuRestoreClicked(ContextMenuButton button)
        {
            DockWindow tab = (DockWindow)button.ParentContextMenu.Tag;
            tab.RootWindow().Restore();
        }
    }
}



#include "DockPanelProxy.h"

#include "DockHintWindow.h"
#include "DockPanel.h"
#include "DockWindow.h"
#include "FloatWindowDockPanel.h"
#include "Editor/GUI/ContextMenu/ContextMenu.h"
#include "Editor/GUI/ContextMenu/ContextMenuButton.h"
#include "Runtime/Engine.h"
#include "Runtime/Graphics/GraphicWindow.h"
#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/Style.h"
#include "Runtime/UI/GUI/WindowRootControl.h"

namespace SE::Editor
{
    void DockPanelProxy::Draw()
    {
        ContainerControl::Draw();

        // Cache data
        Style* style = Style::Current;
        RootControl* window = Root;
        bool containsFocus = ContainsFocus() && static_cast<WindowRootControl*>(window)->Window()->IsFocused();
        Rectangle headerRect = GetHeaderRectangle();
        int tabsCount = _panel->TabsCount;

        if (tabsCount > 0)
        {
            Render2D::FillRectangle(headerRect, style->BackgroundHighlighted);
        }

        // Check if has only one window docked
        if (tabsCount == 1)
        {
            DockWindow* tab = _panel->GetTab(0);

            String tabTitle = tab->Title.operator->();
            Float2 tabTitleSize = style->FontMedium->MeasureText(tabTitle);
            float iconWidth = tab->Icon.IsValid() ? Math::Max(DockPanel::DefaultButtonsSize, headerRect.GetHeight() - DockPanel::DefaultButtonsMargin) : 0;
            float tabSize = iconWidth + DockPanel::DefaultTextMargin + tabTitleSize.x + DockPanel::DefaultButtonsSize * 2 + DockPanel::DefaultButtonsMargin;

            // Draw header
            Rectangle tabRect = Rectangle(headerRect.GetLeft(), headerRect.GetY(), tabSize, headerRect.GetHeight());
            bool isMouseOver = headerRect.Contains(MousePosition);
            if (containsFocus)
            {
                Render2D::FillRectangle(tabRect, style->BackgroundSelected);
            }

            if (tab->Icon.IsValid())
            {
                Render2D::DrawSprite(
                    tab->Icon,
                    Rectangle(tabRect.GetLeft(), tabRect.GetY(),iconWidth, iconWidth),
                    style->Foreground);
            }

            // Draw text
            Rectangle textRect = Rectangle(tabRect.GetLeft() + iconWidth + DockPanel::DefaultTextMargin, tabRect.GetY() + DockPanel::DefaultTextMargin,
                    tabTitleSize.x, tabRect.GetHeight() - DockPanel::DefaultTextMargin);

            Render2D::RenderText(style->FontMedium,
                tabTitle,
                textRect,
                style->TextColor,
                TextAlignment::Center,
                TextAlignment::Center);

            // Draw cross
            Rectangle crossRect = Rectangle(textRect.GetRight() + DockPanel::DefaultButtonsSize,
                (tabRect.GetHeight() - DockPanel::DefaultButtonsSize) / 2,
                DockPanel::DefaultButtonsSize,
                DockPanel::DefaultButtonsSize);
            bool isMouseOverCross = isMouseOver && crossRect.Contains(MousePosition);
            if (isMouseOverCross)
            {
                Render2D::FillRectangle(crossRect, (containsFocus ? style->BackgroundSelected : style->LightBackground) * 1.3f);
            }
            Render2D::DrawSprite(*style->Cross, crossRect, style->ForegroundGrey);
        }
        else
        {
            // Draw background
            Render2D::FillRectangle(headerRect, style->LightBackground);

            // Render all tabs
            float x = 0;
            for (int i = 0; i < tabsCount; i++)
            {
                // Cache data
                DockWindow* tab = _panel->GetTab(i);
                Color tabColor = Colors::Black;
                Float2 titleSize = tab->TitleSize;
                float iconWidth = tab->Icon.IsValid() ? DockPanel::DefaultButtonsSize + DockPanel::DefaultTextMargin : 0;
                float width = titleSize.x + DockPanel::DefaultButtonsSize + 2 * DockPanel::DefaultButtonsMargin + DockPanel::DefaultTextMargin + DockPanel::DefaultTextMargin + iconWidth;
                Rectangle tabRect = Rectangle(x, 0, width, DockPanel::DefaultHeaderHeight);
                bool isMouseOver = tabRect.Contains(MousePosition);
                bool isSelected = _panel->SelectedTab == tab;

                // Check if tab is selected
                if (isSelected)
                {
                    tabColor = containsFocus ? style->BackgroundSelected : style->BackgroundNormal;
                    Render2D::FillRectangle(tabRect, tabColor);
                }
                // Check if mouse is over
                else if (isMouseOver)
                {
                    tabColor = style->BackgroundHighlighted;
                    Render2D::FillRectangle(tabRect, tabColor);
                }
                else
                {
                    tabColor = style->BackgroundHighlighted;
                    Render2D::DrawLine(tabRect.GetBottomLeft() - Float2(0, 1), tabRect.GetUpperLeft(), tabColor);
                    Render2D::DrawLine(tabRect.GetBottomRight() - Float2(0, 1), tabRect.GetUpperRight(), tabColor);
                }

                if (tab->Icon.IsValid())
                {
                    Render2D::DrawSprite(
                        tab->Icon,
                        Rectangle(x + DockPanel::DefaultTextMargin, (DockPanel::DefaultHeaderHeight - DockPanel::DefaultButtonsSize) / 2,
                            DockPanel::DefaultButtonsSize, DockPanel::DefaultButtonsSize),
                        style->Foreground);

                }

                // Draw text
                Render2D::RenderText(
                    style->FontMedium,
                    tab->Title.operator->(),
                    Rectangle(x + DockPanel::DefaultTextMargin + iconWidth, 0, 10000, DockPanel::DefaultHeaderHeight),
                    style->TextColor,
                    TextAlignment::Near,
                    TextAlignment::Center);

                // Draw cross
                if (isSelected || isMouseOver)
                {
                    Rectangle crossRect = Rectangle(x + width - DockPanel::DefaultButtonsSize - DockPanel::DefaultButtonsMargin,
                        (DockPanel::DefaultHeaderHeight - DockPanel::DefaultButtonsSize) / 2,
                        DockPanel::DefaultButtonsSize, DockPanel::DefaultButtonsSize);
                    bool isMouseOverCross = isMouseOver && crossRect.Contains(MousePosition);
                    if (isMouseOverCross)
                        Render2D::FillRectangle(crossRect, tabColor * 1.3f);
                    Render2D::DrawSprite(*style->Cross, crossRect, isMouseOverCross ? style->Foreground : style->ForegroundGrey);
                }

                // Move
                x += width;
            }

            // Draw selected tab strip
            Render2D::FillRectangle(Rectangle(0, DockPanel::DefaultHeaderHeight - 2, Width, 2), containsFocus ? style->BackgroundSelected : style->BackgroundNormal);
        }
    }
    
    void DockPanelProxy::OnLostFocus()
    {
        IsMouseLeftButtonDown = false;
        IsMouseRightButtonDown = false;
        IsMouseMiddleButtonDown = false;
        MouseDownWindow = nullptr;
        MousePosition = Float2::Minimum;

        ContainerControl::OnLostFocus();
    }
    
    void DockPanelProxy::OnMouseEnter(Float2 location)
    {
        MousePosition = location;

        ContainerControl::OnMouseEnter(location);
    }
    
    bool DockPanelProxy::OnMouseDoubleClick(Float2 location, MouseButton button)
    {
        // Maximize/restore on double click
        bool t;
        auto tab = GetTabAtPos(location, t);

        WindowRootControl* rootWindow = nullptr;
        if (tab != nullptr)
        {
            rootWindow = tab->RootWindow();
        }
        if (rootWindow != nullptr && button == MouseButton::Left)
        {
            if (rootWindow->IsMaximized())
                rootWindow->Restore();
            else
                rootWindow->Maximize();
            return true;
        }

        return ContainerControl::OnMouseDoubleClick(location, button);
    }
    
    bool DockPanelProxy::OnMouseDown(Float2 location, MouseButton button)
    {
        bool IsMouseDownOverCross;
        MouseDownWindow = GetTabAtPos(location, IsMouseDownOverCross);

        // Check buttons
        if (button == MouseButton::Left)
        {
            // Cache data
            IsMouseLeftButtonDown = true;
            if (!IsMouseDownOverCross && MouseDownWindow != nullptr)
                _panel->SelectTab(MouseDownWindow);
        }
        else if (button == MouseButton::Right)
        {
            // Cache data
            IsMouseRightButtonDown = true;
            if (MouseDownWindow != nullptr)
                _panel->SelectTab(MouseDownWindow, false);
        }
        else if (button == MouseButton::Middle)
        {
            // Cache data
            IsMouseMiddleButtonDown = true;
        }

        return ContainerControl::OnMouseDown(location, button);
    }
    
    bool DockPanelProxy::OnMouseUp(Float2 location, MouseButton button)
    {
        // Check tabs under mouse position at the beginning and at the end
        bool overCross;
        auto tab = GetTabAtPos(location, overCross);

        // Check buttons
        if (button == MouseButton::Left && IsMouseLeftButtonDown)
        {
            // Clear flag
            IsMouseLeftButtonDown = false;

            // Check if tabs are the same and cross was pressed
            if (tab != nullptr && tab == MouseDownWindow && IsMouseDownOverCross && overCross)
                tab->Close(ClosingReason::User);
            MouseDownWindow = nullptr;
        }
        else if (button == MouseButton::Right && IsMouseRightButtonDown)
        {
            // Clear flag
            IsMouseRightButtonDown = false;

            if (tab != nullptr)
            {
                ShowContextMenu(tab, location);
            }
        }
        else if (button == MouseButton::Middle && IsMouseMiddleButtonDown)
        {
            // Clear flag
            IsMouseMiddleButtonDown = false;

            if (tab != nullptr)
            {
                tab->Close(ClosingReason::User);
            }
        }

        return ContainerControl::OnMouseUp(location, button);
    }
    
    void DockPanelProxy::OnMouseMove(Float2 location)
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
                if (!IsMouseDownOverCross && MouseDownWindow != nullptr)
                    StartDrag(MouseDownWindow);
                MouseDownWindow = nullptr;
            }
            // Check if has more than one tab to change order
            else if (MouseDownWindow != nullptr && _panel->TabsCount > 1)
            {
                // Check if mouse left current tab rect
                Rectangle currWinRect = GetTabRect(MouseDownWindow);
                if (!currWinRect.Contains(location))
                {
                    int index = _panel->GetTabIndex(MouseDownWindow);

                    // Check if move right or left
                    if (location.x < currWinRect.GetX())
                    {
                        // Move left
                        _panel->MoveTabLeft(index);
                    }
                    else if (_panel->LastTab != MouseDownWindow)
                    {
                        // Move right
                        _panel->MoveTabRight(index);
                    }

                    // Update
                    _panel->PerformLayout();
                }
            }
        }

        ContainerControl::OnMouseMove(location);
    }
    
    void DockPanelProxy::OnMouseLeave()
    {
        if (IsMouseLeftButtonDown)
        {
            IsMouseLeftButtonDown = false;

            // Check tabs under mouse position
            if (!IsMouseDownOverCross && MouseDownWindow != nullptr)
                StartDrag(MouseDownWindow);
            MouseDownWindow = nullptr;
        }
        IsMouseRightButtonDown = false;
        IsMouseMiddleButtonDown = false;
        MousePosition = Float2::Minimum;

        ContainerControl::OnMouseLeave();
    }

    DragDropEffect DockPanelProxy::OnDragEnter(const Float2& location, DragData* data)
    {
        DragDropEffect result = ContainerControl::OnDragEnter(location, data);
        if (result != DragDropEffect::None)
            return result;
        return TrySelectTabUnderLocation(location);
    }

    DragDropEffect DockPanelProxy::OnDragMove(const Float2& location, DragData* data)
    {
        DragDropEffect result = ContainerControl::OnDragMove(location, data);
        if (result != DragDropEffect::None)
            return result;
        return TrySelectTabUnderLocation(location);
    }

    void DockPanelProxy::OnDragLeave()
    {
        _dragEnterTime = -1;

        ContainerControl::OnDragLeave();
    }
    
    Rectangle DockPanelProxy::GetDesireClientArea()
    {
        return Rectangle(0, DockPanel::DefaultHeaderHeight, Width, Height - DockPanel::DefaultHeaderHeight);
    }

    DockPanelProxy::DockPanelProxy(DockPanel* panel) : ContainerControl(0, 0, 64, 64)
    {
        AutoFocus = false;

        _panel = panel;
        AnchorPreset = AnchorPresets::StretchAll;
        Offsets = Margin::Zero;
    }

    Rectangle DockPanelProxy::GetHeaderRectangle()
    {
        return Rectangle(0, 0, Width, DockPanel::DefaultHeaderHeight);
    }

    DockWindow* DockPanelProxy::GetTabAtPos(Float2 position, bool& closeButton)
    {
        DockWindow* result = nullptr;
        closeButton = false;

        auto tabsCount = _panel->TabsCount;
        if (tabsCount == 1)
        {
            auto crossRect = Rectangle(Width - DockPanel::DefaultButtonsSize - DockPanel::DefaultButtonsMargin, (DockPanel::DefaultHeaderHeight - DockPanel::DefaultButtonsSize) / 2, DockPanel::DefaultButtonsSize, DockPanel::DefaultButtonsSize);
            if (GetHeaderRectangle().Contains(position))
            {
                closeButton = crossRect.Contains(position);
                result = _panel->GetTab(0);
            }
        }
        else
        {
            float x = 0;
            for (int i = 0; i < tabsCount; i++)
            {
                auto tab = _panel->GetTab(i);
                Float2 titleSize = tab->TitleSize;
                float iconWidth = tab->Icon.IsValid() ? DockPanel::DefaultButtonsSize + DockPanel::DefaultTextMargin : 0;
                float width = titleSize.x + DockPanel::DefaultButtonsSize + 2 * DockPanel::DefaultButtonsMargin + DockPanel::DefaultTextMargin + DockPanel::DefaultTextMargin + iconWidth;
                Rectangle tabRect = Rectangle(x, 0, width, DockPanel::DefaultHeaderHeight);
                auto isMouseOver = tabRect.Contains(position);
                if (isMouseOver)
                {
                    Rectangle crossRect = Rectangle(x + width - DockPanel::DefaultButtonsSize - DockPanel::DefaultButtonsMargin, (DockPanel::DefaultHeaderHeight - DockPanel::DefaultButtonsSize) / 2, DockPanel::DefaultButtonsSize, DockPanel::DefaultButtonsSize);
                    closeButton = crossRect.Contains(position);
                    result = tab;
                    break;
                }
                x += width;
            }
        }

        return result;
    }

    Rectangle DockPanelProxy::GetTabRect(DockWindow* win)
    {
        ENGINE_ASSERT(_panel->ContainsTab(win));

        int tabsCount = _panel->TabsCount;
        if (tabsCount == 1)
        {
            return GetHeaderRectangle();
        }
        else
        {
            float x = 0;
            for (int i = 0; i < tabsCount; i++)
            {
                DockWindow* tab = _panel->GetTab(i);
                Float2 titleSize = tab->TitleSize;
                float width = titleSize.x + DockPanel::DefaultButtonsSize + 2 * DockPanel::DefaultButtonsMargin + DockPanel::DefaultTextMargin + DockPanel::DefaultTextMargin;
                if (tab == win)
                {
                    return Rectangle(x, 0, width, DockPanel::DefaultHeaderHeight);
                }
                x += width;
            }
        }

        return Rectangle::Empty;
    }

    void DockPanelProxy::StartDrag(DockWindow* win)
    {
        // Clear cache
        MouseDownWindow = nullptr;
        StartDragAsyncWindow = win;

        // Register for late update in an async manner (to prevent from crash due to changing UI structure on window undock)
        Engine::LateUpdateCall.BindUnique<DockPanelProxy, &DockPanelProxy::StartDragAsync>(this);
        StartDragAsync();
    }

    void DockPanelProxy::StartDragAsync()
    {
        Engine::LateUpdateCall.Unbind<DockPanelProxy, &DockPanelProxy::StartDragAsync>(this);

        if (StartDragAsyncWindow != nullptr) 
        {
            auto win = StartDragAsyncWindow;
            StartDragAsyncWindow = nullptr;

            // Check if has only one window docked and is floating
            if (_panel->ChildPanelsCount == 0 && _panel->TabsCount == 1 && _panel->IsFloating)
            {
                // Create docking hint window but in an async manner
                DockHintWindow::Create(TypeTryCast<FloatWindowDockPanel>(_panel));
            }
            else
            {
                // Select another tab
                int index = _panel->GetTabIndex(win);
                if (index == 0)
                    index = _panel->TabsCount;
                _panel->SelectTab(index - 1);

                // Create docking hint window
                DockHintWindow::Create(win);
            }
        }
    }

    DragDropEffect DockPanelProxy::TrySelectTabUnderLocation(const Float2& location)
    {
        bool closeButton;
        DockWindow* tab = GetTabAtPos(location, closeButton);
        if (tab != nullptr)
        {
            // Auto-select tab only if drag takes some time
            double time = Platform::GetTimeSeconds();
            if (_dragEnterTime < 0)
                _dragEnterTime = time;
            if (time - _dragEnterTime < 0.3f)
                return DragDropEffect::Link;
            _dragEnterTime = -1;

            _panel->SelectTab(tab);
            Update(0); // Fake update
            return DragDropEffect::Move;
        }
        _dragEnterTime = -1;
        return DragDropEffect::None;
    }

    void DockPanelProxy::ShowContextMenu(DockWindow* tab, Float2& location)
    {
        ContextMenu* menu = New<ContextMenu>();
        menu->Tag = tab;

        tab->OnShowContextMenu(menu);
        menu->AddButton(SE_TEXT("Close"), CreateFunc<DockPanelProxy, &DockPanelProxy::OnTabMenuCloseClicked>(this));
        menu->AddButton(SE_TEXT("Close All"), CreateFunc<DockPanelProxy, &DockPanelProxy::OnTabMenuCloseAllClicked>(this));
        menu->AddButton(SE_TEXT("Close All But This"), CreateFunc<DockPanelProxy, &DockPanelProxy::OnTabMenuCloseAllButThisClicked>(this));
        if (_panel->Tabs.operator->().Find(tab) + 1 < _panel->TabsCount)
        {
            menu->AddButton(SE_TEXT("Close All To The Right"), CreateFunc<DockPanelProxy, &DockPanelProxy::OnTabMenuCloseAllToTheRightClicked>(this));
        }
        if (!_panel->IsFloating)
        {
            menu->AddSeparator();
            menu->AddButton(SE_TEXT("Undock"), CreateFunc<DockPanelProxy, &DockPanelProxy::OnTabMenuUndockClicked>(this));
        }
        else if (!(tab->RootWindow() != nullptr && tab->RootWindow()->IsMaximized()))
        {
            menu->AddSeparator();
            menu->AddButton(SE_TEXT("Maximize"), CreateFunc<DockPanelProxy, &DockPanelProxy::OnTabMenuMaximizeClicked>(this));
        }
        else if (tab->RootWindow() != nullptr && tab->RootWindow()->IsMaximized())
        {
            menu->AddSeparator();
            menu->AddButton(SE_TEXT("Restore"), CreateFunc<DockPanelProxy, &DockPanelProxy::OnTabMenuRestoreClicked>(this));
        }
        menu->Show(this, location);
    }

    void DockPanelProxy::OnTabMenuCloseClicked(ContextMenuButton* button)
    {
        DockWindow* tab = std::any_cast<DockWindow*>(button->ParentContextMenu->Tag);
        tab->Close(ClosingReason::User);
        if (tab == MouseDownWindow)
            MouseDownWindow = nullptr;
    }

    void DockPanelProxy::OnTabMenuCloseAllClicked(ContextMenuButton* button)
    {
        _panel->CloseAll();
    }

    void DockPanelProxy::OnTabMenuCloseAllButThisClicked(ContextMenuButton* button)
    {
        DockWindow* tab = std::any_cast<DockWindow*>(button->ParentContextMenu->Tag);
        List<DockWindow*>& windows = _panel->Tabs;
        for (int i = 0; i < windows.Count(); i++)
        {
            if (windows[i] != tab)
                windows[i]->Close();
        }
    }

    void DockPanelProxy::OnTabMenuCloseAllToTheRightClicked(ContextMenuButton* button)
    {
        DockWindow* tab = std::any_cast<DockWindow*>(button->ParentContextMenu->Tag);
        List<DockWindow*>& windows = _panel->Tabs;
        for (int i = _panel->Tabs.operator->().Find(tab) + 1; i < windows.Count(); i++)
        {
            windows[i]->Close();
        }
    }

    void DockPanelProxy::OnTabMenuUndockClicked(ContextMenuButton* button)
    {
        DockWindow* tab = std::any_cast<DockWindow*>(button->ParentContextMenu->Tag);
        tab->ShowFloating();
    }

    void DockPanelProxy::OnTabMenuMaximizeClicked(ContextMenuButton* button)
    {
        DockWindow* tab = std::any_cast<DockWindow*>(button->ParentContextMenu->Tag);
        tab->RootWindow()->Maximize();
    }

    void DockPanelProxy::OnTabMenuRestoreClicked(ContextMenuButton* button)
    {
        DockWindow* tab = std::any_cast<DockWindow*>(button->ParentContextMenu->Tag);
        tab->RootWindow()->Restore();
    }
} // SE
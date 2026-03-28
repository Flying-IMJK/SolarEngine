

#include "DockHintWindow.h"

#include "DockWindow.h"
#include "FloatWindowDockPanel.h"
#include "MasterDockPanel.h"
#include "Runtime/Graphics/GraphicWindow.h"
#include "Runtime/UI/GUI/Style.h"
#include "Runtime/UI/GUI/WindowRootControl.h"

namespace SE::Editor
{
	DockHintWindow::DockHintWindow(FloatWindowDockPanel* toMove)
	{
		_OnMouseUpCall = CreateFunc<DockHintWindow, &DockHintWindow::OnMouseUp>(this);
		_OnMouseMoveCall = CreateFunc<DockHintWindow, &DockHintWindow::OnMouseMove>(this);
		_OnLostFocusCall = CreateFunc<DockHintWindow, &DockHintWindow::OnLostFocus>(this);

		_toMove = toMove;
		_toSet = DockState::Float;
		GraphicWindow* window = toMove->Window->Window();

		// Remove focus from drag target
		_toMove->Focus();
		_toMove->Defocus();

		// Focus window
		window->Focus();

		// Check if window is maximized and restore window.
		if (window->IsMaximized())
		{
			// Restore window and set position to mouse.
			Float2 mousePos = window->GetMousePosition();
			Float2 previousSize = window->GetSize();
			window->Restore();
			window->SetPosition(Platform::GetMousePosition() - mousePos * window->GetSize() / previousSize);
		}

		// Calculate dragging offset and move window to the destination position
		Float2 mouseScreenPosition = Platform::GetMousePosition();

		// If the _toMove window was not focused when initializing this window, the result vector only contains zeros
		// and to prevent a failure, we need to perform an update for the drag offset at later time which will be done in the OnMouseMove event handler.
		if (mouseScreenPosition != Float2::Zero)
			CalculateDragOffset(mouseScreenPosition);
		else
			_lateDragOffsetUpdate = true;

		// Get initial size
		_defaultWindowSize = window->GetSize();

		// Init proxy window
		Proxy::Init(_defaultWindowSize);

		// Bind events
		Proxy::Window->MouseUpEvent.BindUnique(_OnMouseUpCall);;
		Proxy::Window->MouseMoveEvent.BindUnique(_OnMouseMoveCall);
		Proxy::Window->LostFocusEvent.BindUnique(_OnLostFocusCall);

		// Start tracking mouse
		Proxy::Window->StartTrackingMouse(false);

		// Update window GUI
		Proxy::Window->GetGUI()->PerformLayout();

		// Update rectangles
		UpdateRects();

		// Hide base window
		window->Hide();

		// Enable hit window presentation
		Proxy::Window->SetRenderingEnabled(true);
		Proxy::Window->Show();
		Proxy::Window->Focus();
	}
	
	void DockHintWindow::CalculateDragOffset(Float2 mouseScreenPosition)
	{
		Float2 baseWinPos = _toMove->Window->Window()->GetPosition();
		_dragOffset = mouseScreenPosition - baseWinPos;
	}
	
	void DockHintWindow::UpdateRects()
    {
        // Cache mouse position
        _mouse = Platform::GetMousePosition();

        // Check intersection with any dock panel
        Float2 uiMouse = _mouse;
        _toDock = _toMove->MasterPanel->HitTest(uiMouse, _toMove);

        // Check dock state to use
        bool showProxyHints = _toDock != nullptr;
        bool showBorderHints = showProxyHints;
        bool showCenterHint = showProxyHints;
        if (showProxyHints)
        {
            // If moved window has not only tabs but also child panels disable docking as tab
            if (_toMove->ChildPanelsCount > 0)
                showCenterHint = false;

            // Disable docking windows with one or more dock panels inside
            if (_toMove->ChildPanelsCount > 0)
                showBorderHints = false;

            // Get dock area
            _rectDock = _toDock->DockAreaBounds;

            // Cache dock rectangles
            Float2 size = _rectDock.Size;
            Float2 offset = _rectDock.Location;
            float borderMargin = 4.0f;
            float hintWindowsSize = Proxy::HintWindowsSize * Platform::GetDpiScale();
            float hintWindowsSize2 = hintWindowsSize * 0.5f;
            float centerX = size.x * 0.5f;
            float centerY = size.y * 0.5f;
            _rUpper = Rectangle(centerX - hintWindowsSize2, borderMargin, hintWindowsSize, hintWindowsSize) + offset;
            _rBottom = Rectangle(centerX - hintWindowsSize2, size.y - hintWindowsSize - borderMargin, hintWindowsSize, hintWindowsSize) + offset;
            _rLeft = Rectangle(borderMargin, centerY - hintWindowsSize2, hintWindowsSize, hintWindowsSize) + offset;
            _rRight = Rectangle(size.x - hintWindowsSize - borderMargin, centerY - hintWindowsSize2, hintWindowsSize, hintWindowsSize) + offset;
            _rCenter = Rectangle(centerX - hintWindowsSize2, centerY - hintWindowsSize2, hintWindowsSize, hintWindowsSize) + offset;

            // Hit test
            DockState toSet = DockState::Float;
            if (showBorderHints)
            {
                if (_rUpper.Contains(_mouse))
                    toSet = DockState::DockTop;
                else if (_rBottom.Contains(_mouse))
                    toSet = DockState::DockBottom;
                else if (_rLeft.Contains(_mouse))
                    toSet = DockState::DockLeft;
                else if (_rRight.Contains(_mouse))
                    toSet = DockState::DockRight;
            }
            if (showCenterHint && _rCenter.Contains(_mouse))
                toSet = DockState::DockFill;
            _toSet = toSet;

            // Show proxy hint windows
            Proxy::Down->SetPosition(_rBottom.Location);
            Proxy::Left->SetPosition(_rLeft.Location);
            Proxy::Right->SetPosition(_rRight.Location);
            Proxy::Up->SetPosition(_rUpper.Location);
            Proxy::Center->SetPosition(_rCenter.Location);
        }
        else
        {
            _toSet = DockState::Float;
        }

        // Update proxy hint windows visibility
        Proxy::Down->SetIsVisible(showProxyHints & showBorderHints);
        Proxy::Left->SetIsVisible(showProxyHints & showBorderHints);
        Proxy::Right->SetIsVisible(showProxyHints & showBorderHints);
        Proxy::Up->SetIsVisible(showProxyHints & showBorderHints);
        Proxy::Center->SetIsVisible(showProxyHints & showCenterHint);

        // Calculate proxy/dock/window rectangles
        if (_toDock == nullptr)
        {
            // Floating window over nothing
            _rectWindow = Rectangle(_mouse - _dragOffset, _defaultWindowSize);
        }
        else
        {
            if (_toSet == DockState::Float)
            {
                // Floating window over dock panel
                _rectWindow = Rectangle(_mouse - _dragOffset, _defaultWindowSize);
            }
            else
            {
                // Use only part of the dock panel to show hint
                _rectWindow = CalculateDockRect(_toSet, _rectDock);
            }
        }

        // Update proxy window
        Proxy::Window->SetClientBounds(_rectWindow);
    }
	
	void DockHintWindow::OnMouseUp(const Float2& location, MouseButton button)
	{
		if (button == MouseButton::Left)
		{
			Dispose();
		}
	}
	
	void DockHintWindow::OnMouseMove(const Float2& mousePos)
	{
		// Recalculate the drag offset because the current mouse screen position was invalid when we initialized the window
		if (_lateDragOffsetUpdate)
		{
			// Calculate dragging offset and move window to the destination position
			CalculateDragOffset(mousePos);

			// Reset state
			_lateDragOffsetUpdate = false;
		}

		UpdateRects();
	}
	
	void DockHintWindow::OnLostFocus()
	{
		Dispose();
		Delete(this);
	}
	
	void DockHintWindow::Dispose()
    {
        // End tracking mouse
        Proxy::Window->EndTrackingMouse();

        // Disable rendering
        Proxy::Window->SetRenderingEnabled(false);

        // Unbind events
        Proxy::Window->MouseUpEvent.Unbind(_OnMouseUpCall);
        Proxy::Window->MouseMoveEvent.Unbind(_OnMouseMoveCall);
        Proxy::Window->LostFocusEvent.Unbind(_OnLostFocusCall);

		_OnMouseMoveCall.Unbind();
		_OnMouseMoveCall.Unbind();
		_OnLostFocusCall.Unbind();

        // Hide the proxy
        Proxy::Hide();

        if (_toMove == nullptr)
            return;

        // Check if window won't be docked
        if (_toSet == DockState::Float)
        {
        	if (_toMove->Window == nullptr)
        	{
        		return;
        	}
            GraphicWindow* window = _toMove->Window->Window();
            if (window == nullptr)
                return;
            Float2 mouse = Platform::GetMousePosition();

            // Move base window
            window->SetPosition(mouse - _dragOffset);

            // Show base window
            window->Show();
        }
        else
        {
            bool hasNoChildPanels = _toMove->ChildPanelsCount == 0;

            // Check if window has only single tab
            if (hasNoChildPanels && _toMove->TabsCount == 1)
            {
                // Dock window
                _toMove->GetTab(0)->Show(_toSet, _toDock);
            }
            // Check if dock as tab and has no child panels
            else if (hasNoChildPanels && _toSet == DockState::DockFill)
            {
                // Dock all tabs
                while (_toMove->TabsCount > 0)
                {
                    _toMove->GetTab(0)->Show(DockState::DockFill, _toDock);
                }
            }
            else
            {
                DockWindow* selectedTab = _toMove->SelectedTab;

                // Dock the first tab into the target location
                DockWindow* firstTab = _toMove->GetTab(0);
                firstTab->Show(_toSet, _toDock);

                // Dock rest of the tabs
                while (_toMove->TabsCount > 0)
                {
                    _toMove->GetTab(0)->Show(DockState::DockFill, firstTab);
                }

                // Keep selected tab being selected
            	if (selectedTab != nullptr)
            	{
            		selectedTab->SelectTab();
            	}
            }

            // Focus target window
            _toDock->Root->Focus();
        }

        _toMove = nullptr;
    }
	
	DockHintWindow* DockHintWindow::Create(FloatWindowDockPanel* toMove)
	{
		ENGINE_ASSERT(toMove != nullptr);
		
		return New<DockHintWindow>(toMove);
	}
	
	DockHintWindow* DockHintWindow::Create(DockWindow* toMove)
	{
		ENGINE_ASSERT(toMove != nullptr);

		// Show floating
		toMove->ShowFloating();

		// Move window to the mouse position (with some offset for caption bar)
		WindowRootControl* window = static_cast<WindowRootControl*>(toMove->Root.operator->());
		Float2 mouse = Platform::GetMousePosition();
		window->Window()->SetPosition(mouse - Float2(8, 8));

		// Get floating panel
		FloatWindowDockPanel* floatingPanelToMove = TypeTryCast<FloatWindowDockPanel>(window->GetChild(0));

		return New<DockHintWindow>(floatingPanelToMove);
	}
	
	Rectangle DockHintWindow::CalculateDockRect(DockState state, Rectangle& rect)
	{
		Rectangle result = rect;
		switch (state)
		{
		case DockState::DockFill:
			result.Location.y += DockPanel::DefaultHeaderHeight;
			result.Size.y -= DockPanel::DefaultHeaderHeight;
			break;
		case DockState::DockTop:
			result.Size.y *= DockPanel::DefaultSplitterValue;
			break;
		case DockState::DockLeft:
			result.Size.x *= DockPanel::DefaultSplitterValue;
			break;
		case DockState::DockBottom:
			result.Location.y += result.Size.y * (1 - DockPanel::DefaultSplitterValue);
			result.Size.y *= DockPanel::DefaultSplitterValue;
			break;
		case DockState::DockRight:
			result.Location.x += result.Size.x * (1 - DockPanel::DefaultSplitterValue);
			result.Size.x *= DockPanel::DefaultSplitterValue;
			break;
		}
		return result;
	}


	GraphicWindow* DockHintWindow::Proxy::Window = nullptr;
	GraphicWindow* DockHintWindow::Proxy::Left = nullptr;
	GraphicWindow* DockHintWindow::Proxy::Right = nullptr;
	GraphicWindow* DockHintWindow::Proxy::Up = nullptr;
	GraphicWindow* DockHintWindow::Proxy::Down = nullptr;
	GraphicWindow* DockHintWindow::Proxy::Center = nullptr;


	void DockHintWindow::Proxy::InitHitProxy()
	{
		CreateProxy(Left, SE_TEXT("DockHint.Left"));
		CreateProxy(Right, SE_TEXT("DockHint.Right"));
		CreateProxy(Up, SE_TEXT("DockHint.Up"));
		CreateProxy(Down, SE_TEXT("DockHint.Down"));
		CreateProxy(Center, SE_TEXT("DockHint.Center"));
	}
	
	void DockHintWindow::Proxy::Init(Float2& initSize)
	{
		if (Window == nullptr)
		{
			CreateWindowSettings settings = DefaultWindowSettings();
			settings.Title = "DockHint.Window";
			settings.Size = initSize;
			settings.AllowInput = true;
			settings.AllowMaximize = false;
			settings.AllowMinimize = false;
			settings.HasBorder = false;
			settings.HasSizingFrame = false;
			settings.IsRegularWindow = false;
			settings.SupportsTransparency = true;
			settings.ShowInTaskbar = false;
			settings.ShowAfterFirstPaint = false;
			settings.IsTopmost = true;

			Window = New<GraphicWindow>(settings);
			Window->SetOpacity(0.6f);
			Window->GetGUI()->BackgroundColor = Style::Current->DragWindow;
		}
		else
		{
			// Resize proxy
			Window->SetClientSize(initSize);
		}

		InitHitProxy();
	}
	
	void DockHintWindow::Proxy::Hide()
	{
		HideProxy(Window);
		HideProxy(Left);
		HideProxy(Right);
		HideProxy(Up);
		HideProxy(Down);
		HideProxy(Center);
	}
	
	void DockHintWindow::Proxy::Dispose()
	{
		DisposeProxy(Window);
		DisposeProxy(Left);
		DisposeProxy(Right);
		DisposeProxy(Up);
		DisposeProxy(Down);
		DisposeProxy(Center);
	}
	
	void DockHintWindow::Proxy::CreateProxy(GraphicWindow* &win, StringView name)
	{
		if (win != nullptr)
			return;

		CreateWindowSettings settings = DefaultWindowSettings();
		settings.Title = name;
		settings.Size = Float2(HintWindowsSize * Platform::GetDpiScale());
		settings.AllowInput = false;
		settings.AllowMaximize = false;
		settings.AllowMinimize = false;
		settings.HasBorder = false;
		settings.HasSizingFrame = false;
		settings.IsRegularWindow = false;
		settings.SupportsTransparency = true;
		settings.ShowInTaskbar = false;
		settings.ActivateWhenFirstShown = false;
		settings.IsTopmost = true;
		settings.ShowAfterFirstPaint = false;

		win = New<GraphicWindow>(settings);
		win->SetOpacity(0.6f);
		win->GetGUI()->BackgroundColor = Style::Current->DragWindow;
	}
	
	void DockHintWindow::Proxy::HideProxy(GraphicWindow* win)
	{
		if (win)
		{
			win->Hide();
		}
	}

	void DockHintWindow::Proxy::DisposeProxy(GraphicWindow* win)
	{
		if (win)
		{
			win->Close(ClosingReason::User);
			win = nullptr;
		}
	}
} // SE

#include "WindowRootControl.h"

#include "DragData.h"
#include "Style.h"
#include "Runtime/Graphics/GraphicWindow.h"

namespace SE
{
	String WindowRootControl::GetTitle() const
	{
		return _window->GetTitle();
	}

	void WindowRootControl::SetTitle(StringView title) const
	{
		return _window->SetTitle(title);
	}

	bool WindowRootControl::IsFullscreen() const
	{
		return _window->IsFullscreen();
	}

	bool WindowRootControl::IsWindowed() const
	{
		return _window->IsWindowed();
	}

	bool WindowRootControl::IsShown() const
	{
		return _window->IsVisible();
	}

	bool WindowRootControl::IsMinimized() const
	{
		return _window->IsMinimized();
	}

	bool WindowRootControl::IsMaximized() const
	{
		return _window->IsMaximized();
	}

	WindowRootControl::WindowRootControl(): _window(), _focusedControl(), _trackingControl()
	{
	}

	WindowRootControl::WindowRootControl(GraphicWindow* window)
	{
		_window = window;
		ClipChildren = false;

		if (Style::Current != nullptr)
			BackgroundColor = Style::Current->Background;
	}



	void WindowRootControl::Show() const
	{
		_window->Show();
	}

	void WindowRootControl::Hide() const
	{
		_window->Show();
	}

	void WindowRootControl::Minimize() const
	{
		_window->Minimize();
	}

	void WindowRootControl::Maximize() const
	{
		_window->Maximize();
	}

	void WindowRootControl::Restore() const
	{
		_window->Restore();
	}

	void WindowRootControl::Close(ClosingReason reason) const
	{
		_window->Close(reason);
	}

	void WindowRootControl::BringToFront(bool force) const
	{
		_window->BringToFront(force);
	}

	void WindowRootControl::FlashWindow() const
	{
		_window->FlashWindow();
	}

	Control* WindowRootControl::GetFocusedControl()
	{
		return _focusedControl;
	}
	void WindowRootControl::SetFocusedControl(Control* value)
	{
		ENGINE_ASSERT_M(_focusedControl != nullptr || _focusedControl->Root == this, SE_TEXT("Invalid control to focus"));
		Focus(value);
	}

	CursorType WindowRootControl::__GetCursor()
	{
		return _window->GetCursor();
	}

	void WindowRootControl::__SetCursor(CursorType value)
	{
		_window->SetCursor(value);
	}

	Float2 WindowRootControl::GetTrackingMouseOffset()
	{
		return _window->GetTrackingMouseOffset() / _window->GetDpiScale();
	}

	WindowRootControl* WindowRootControl::RootWindow()
	{
		return this;
	}
	Float2 WindowRootControl::GetMousePosition()
	{
		return _window->GetMousePosition() / _window->GetDpiScale();
	}

	void WindowRootControl::SetMousePosition(Float2 value)
	{
		_window->SetMousePosition(value * _window->GetDpiScale());
	}

	void WindowRootControl::StartTrackingMouse(Control* control, bool useMouseScreenOffset)
	{
		ENGINE_ASSERT(control != nullptr);
		if (_trackingControl == control)
			return;

		if (_trackingControl != nullptr)
			EndTrackingMouse();

		if (control->AutoFocus)
			Focus(control);

		_trackingControl = control;

		_window->StartTrackingMouse(useMouseScreenOffset);
	}

	void WindowRootControl::EndTrackingMouse()
	{
		if (_trackingControl != nullptr)
		{
			Control* control = _trackingControl;
			_trackingControl = nullptr;
			control->OnEndMouseCapture();

			_window->EndTrackingMouse();
		}
	}
	
	bool WindowRootControl::GetKey(KeyboardKeys key)
	{
		return _window->GetKey(key);
	}

	bool WindowRootControl::GetKeyDown(KeyboardKeys key)
	{
		return _window->GetKeyDown(key);
	}

	/// <inheritdoc />
	bool WindowRootControl::GetKeyUp(KeyboardKeys key)
	{
		return _window->GetKeyUp(key);
	}
	
	bool WindowRootControl::GetMouseButton(MouseButton button)
	{
		return _window->GetMouseButton(button);
	}
	
	bool WindowRootControl::GetMouseButtonDown(MouseButton button)
	{
		return _window->GetMouseButtonDown(button);
	}
	
	bool WindowRootControl::GetMouseButtonUp(MouseButton button)
	{
		return _window->GetMouseButtonUp(button);
	}
	
	Float2 WindowRootControl::PointFromScreen(Float2 location)
	{
		return _window->ScreenToClient(location) / _window->GetDpiScale();
	}
	
	Float2 WindowRootControl::PointToScreen(Float2 location)
	{
		return _window->ClientToScreen(location * _window->GetDpiScale());
	}
	
	void WindowRootControl::Focus()
	{
		_window->Focus();
	}

	/// <inheritdoc />
	void WindowRootControl::DoDragDrop(DragData* data)
	{
		if (data -> GetDataType() == DragDataType::Text)
		{
			// _window->DoDragDrop(data);
		}
	}

	bool WindowRootControl::OnMouseDown(Float2 location, MouseButton button)
	{
		if (_trackingControl != nullptr)
		{
			return _trackingControl->OnMouseDown(_trackingControl->PointFromWindow(location), button);
		}

		return RootControl::OnMouseDown(location, button);
	}

	bool WindowRootControl::OnMouseUp(Float2 location, MouseButton button)
	{
		if (_trackingControl != nullptr)
		{
			return _trackingControl->OnMouseUp(_trackingControl->PointFromWindow(location), button);
		}

		return RootControl::OnMouseUp(location, button);
	}

	bool WindowRootControl::OnMouseDoubleClick(Float2 location, MouseButton button)
	{
		if (_trackingControl != nullptr)
		{
			return _trackingControl->OnMouseDoubleClick(_trackingControl->PointFromWindow(location), button);
		}

		return RootControl::OnMouseDoubleClick(location, button);
	}

	bool WindowRootControl::OnMouseWheel(Float2 location, float delta)
	{
		if (_trackingControl != nullptr)
		{
			return _trackingControl->OnMouseWheel(_trackingControl->PointFromWindow(location), delta);
		}

		return RootControl::OnMouseWheel(location, delta);
	}

	void WindowRootControl::OnMouseMove(Float2 location)
	{
		if (_trackingControl != nullptr)
		{
			_trackingControl->OnMouseMove(_trackingControl->PointFromWindow(location));
			return;
		}

		RootControl::OnMouseMove(location);
	}

	bool WindowRootControl::Focus(Control* c)
	{
		if (__GetIsDisposing() || _focusedControl == c)
			return false;

		// Change focused control
		Control* previous = _focusedControl;
		_focusedControl = c;

		// Fire events
		if (previous != nullptr)
		{
			previous->OnLostFocus();
			ENGINE_ASSERT(!previous->IsFocused);
		}
		if (_focusedControl != nullptr)
		{
			_focusedControl->OnGetFocus();
			ENGINE_ASSERT(_focusedControl->IsFocused);
		}

		// Update flags
		UpdateContainsFocus();

		return true;
	}
} // SE
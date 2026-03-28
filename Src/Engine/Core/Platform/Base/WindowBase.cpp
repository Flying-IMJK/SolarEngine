
#include "Core/Platform/Window.h"
#include "Core/Profiler/ProfilerCPU.h"
#include "Core/Platform/CreateWindowSettings.h"
#include "Core/Platform/WindowsManager.h"
#include "WindowBase.h"
#include "Core/Input/Input.h"

#if USE_CSHARP
// Helper macros for calling C# events
#define BEGIN_INVOKE_EVENT(name, paramsCount) \
	auto managedInstance = GetManagedInstance(); \
    if (managedInstance) \
	{ \
	    static MMethod* _method_##name = nullptr; \
	    if (_method_##name == nullptr) \
	    { \
		    _method_##name = GetClass()->GetMethod("Internal_" #name, paramsCount); \
		    if (_method_##name == nullptr) \
		    { \
			    LOG(Fatal, "Missing Window method " #name); \
		    } \
	    }
#define END_INVOKE_EVENT(name) \
        if (exception) \
	    { \
		    MException ex(exception); \
		    ex.Log(LogType::Error, TEXT("Window." #name)); \
	    } \
	}
#define INVOKE_EVENT(name, paramsCount, param0, param1, param2) BEGIN_INVOKE_EVENT(name, paramsCount) \
	    void* params[3]; \
	    params[0] = param0; \
	    params[1] = param1; \
	    params[2] = param2; \
	    MObject* exception = nullptr; \
	    _method_##name->Invoke(managedInstance, params, &exception); \
	    END_INVOKE_EVENT(name)
#define INVOKE_EVENT_PARAMS_3(name, param0, param1, param2) INVOKE_EVENT(name, 3, param0, param1, param2)
#define INVOKE_EVENT_PARAMS_2(name, param0, param1) INVOKE_EVENT(name, 2, param0, param1, nullptr)
#define INVOKE_EVENT_PARAMS_1(name, param0) INVOKE_EVENT(name, 1, param0, nullptr, nullptr)
#define INVOKE_EVENT_PARAMS_0(name) INVOKE_EVENT(name, 0, nullptr, nullptr, nullptr)
#define INVOKE_DRAG_EVENT(name) \
	if (result != DragDropEffect::None) return; \
    BEGIN_INVOKE_EVENT(name, 3) \
	void* params[3]; \
	params[0] = (void*)&mousePosition; \
	Array<String> outputData; \
	bool isText; \
	if(data->GetType() == IGuiData::Type::Text)\
	{ \
		isText = true; outputData.Add(data->GetAsText()); \
	} \
	else \
	{ \
		isText = false; data->GetAsFiles(&outputData); \
	} \
	params[1] = (void*)&isText; \
	MArray* outputDataManaged = MCore::Array::New(MCore::TypeCache::String, outputData.Count()); \
    MString** outputDataManagedPtr = MCore::Array::GetAddress<MString*>(outputDataManaged); \
	for (int32 i = 0; i < outputData.Count(); i++) \
        outputDataManagedPtr[i] = MUtils::ToString(outputData[i]); \
	params[2] = outputDataManaged; \
	MObject* exception = nullptr; \
	auto resultObj = _method_##name->Invoke(GetManagedInstance(), params, &exception); \
    if (resultObj) \
	    result = (DragDropEffect)MUtils::Unbox<int32>(resultObj); \
	END_INVOKE_EVENT(name)
#else
#define INVOKE_EVENT(name, paramsCount, param0, param1, param2)
#define INVOKE_EVENT_PARAMS_3(name, param0, param1, param2) INVOKE_EVENT(name, 3, param0, param1, param2)
#define INVOKE_EVENT_PARAMS_2(name, param0, param1) INVOKE_EVENT(name, 2, param0, param1, nullptr)
#define INVOKE_EVENT_PARAMS_1(name, param0) INVOKE_EVENT(name, 1, param0, nullptr, nullptr)
#define INVOKE_EVENT_PARAMS_0(name) INVOKE_EVENT(name, 0, nullptr, nullptr, nullptr)
#define INVOKE_DRAG_EVENT(name)
#endif

namespace SE
{
	Window* WindowBase::mainWindow = nullptr;

	void WindowBase::SetMainWindow(Window* window)
	{
		mainWindow = window;
	}

	Window* WindowBase::GetMainWindow()
	{
		return mainWindow;
	}

	bool WindowBase::IsMain() const
	{
		return mainWindow != nullptr && mainWindow == this;
	}


	WindowBase::WindowBase(const CreateWindowSettings& settings)
		: m_Visible(false), _minimized(false), _maximized(false), m_IsClosing(false), m_ShowAfterFirstPaint(settings.ShowAfterFirstPaint), m_Focused(false), _fullscreen(settings.Fullscreen),
		  m_Settings(settings), m_Title(settings.Title), m_Cursor(CursorType::Default), m_ClientSize(settings.Size),
		  m_Dpi(96), m_DpiScale(1.0f), m_TrackingMouseOffset(Float2::Zero)
	{
		// Update window location based on start location
		if (settings.StartPosition == WindowStartPosition::CenterParent
			|| settings.StartPosition == WindowStartPosition::CenterScreen)
		{
			Rectangle parentBounds = Rectangle(Float2::Zero, Platform::GetDesktopSize());
			if (settings.Parent != nullptr && settings.StartPosition == WindowStartPosition::CenterParent)
				parentBounds = settings.Parent->GetClientBounds();

			// Move to the center of target bounds area
			// A little hack but platform implementation constructor will place a window
			((CreateWindowSettings&)settings).Position = parentBounds.Location + (parentBounds.Size - settings.Size) * 0.5f;
		}

		WindowsManager::Register((Window*)this);
	}

	WindowBase::~WindowBase()
	{
		WindowsManager::Unregister((Window*)this);
	}

	bool WindowBase::IsVisible() const
	{
		return m_Visible;
	}

	bool WindowBase::IsFullscreen() const
	{
		return _fullscreen;
	}

	void WindowBase::SetIsFullscreen(bool isFullscreen)
	{
		LOG_INFO("Window", "Changing window fullscreen mode to {0}", isFullscreen);

		FullscreenSwapChain(isFullscreen);
	}

	void WindowBase::SetIsVisible(bool isVisible)
	{
		// Check if state will change
		if (IsVisible() != isVisible)
		{
			// Check if show or hide a window
			if (isVisible)
				Show();
			else
				Hide();
		}
	}

/*	String WindowBase::ToString() const
	{
		return GetTitle();
	}*/


	void WindowBase::OnCharInput(Char c)
	{
		PROFILE_CPU_NAMED("GUI.OnCharInput");
		CharInputEvent(c);
	}

	void WindowBase::OnKeyDown(KeyboardKeys key)
	{
		PROFILE_CPU_NAMED("GUI.OnKeyDown");
		KeyDownEvent(key);
	}

	void WindowBase::OnKeyUp(KeyboardKeys key)
	{
		PROFILE_CPU_NAMED("GUI.OnKeyUp");
		KeyUpEvent(key);
	}

	void WindowBase::OnMouseDown(const Float2& mousePosition, MouseButton button)
	{
		PROFILE_CPU_NAMED("GUI.OnMouseDown");
		MouseDownEvent(mousePosition, button);
	}

	void WindowBase::OnMouseUp(const Float2& mousePosition, MouseButton button)
	{
		PROFILE_CPU_NAMED("GUI.OnMouseUp");
		MouseUpEvent(mousePosition, button);
	}

	void WindowBase::OnMouseDoubleClick(const Float2& mousePosition, MouseButton button)
	{
		PROFILE_CPU_NAMED("GUI.OnMouseDoubleClick");
		MouseDoubleClickEvent(mousePosition, button);
	}

	void WindowBase::OnMouseWheel(const Float2& mousePosition, float delta)
	{
		PROFILE_CPU_NAMED("GUI.OnMouseWheel");
		MouseWheelEvent(mousePosition, delta);
	}

	void WindowBase::OnMouseMove(const Float2& mousePosition)
	{
		PROFILE_CPU_NAMED("GUI.OnMouseMove");
		MouseMoveEvent(mousePosition);
	}

	void WindowBase::OnMouseLeave()
	{
		PROFILE_CPU_NAMED("GUI.OnMouseLeave");
		MouseLeaveEvent();
	}

	void WindowBase::OnTouchDown(const Float2& pointerPosition, int32 pointerId)
	{
		PROFILE_CPU_NAMED("GUI.OnTouchDown");
		TouchDownEvent(pointerPosition, pointerId);
	}

	void WindowBase::OnTouchMove(const Float2& pointerPosition, int32 pointerId)
	{
		PROFILE_CPU_NAMED("GUI.OnTouchMove");
		TouchMoveEvent(pointerPosition, pointerId);
	}

	void WindowBase::OnTouchUp(const Float2& pointerPosition, int32 pointerId)
	{
		PROFILE_CPU_NAMED("GUI.OnTouchUp");
		TouchUpEvent(pointerPosition, pointerId);
	}

	void WindowBase::OnDragEnter(IGuiData* data, const Float2& mousePosition, DragDropEffect& result)
	{
		DragEnterEvent(data, mousePosition, result);
	}

	void WindowBase::OnDragOver(IGuiData* data, const Float2& mousePosition, DragDropEffect& result)
	{
		DragOverEvent(data, mousePosition, result);
	}

	void WindowBase::OnDragDrop(IGuiData* data, const Float2& mousePosition, DragDropEffect& result)
	{
		DragDropEvent(data, mousePosition, result);
	}

	void WindowBase::OnDragLeave()
	{
		DragLeaveEvent();
	}

	void WindowBase::OnHitTest(const Float2& mousePosition, WindowHitCodes& result, bool& handled)
	{
		HitTestEvent(mousePosition, result, handled);
		if (handled)
			return;
	}

	void WindowBase::OnLeftButtonHit(WindowHitCodes hit, bool& result)
	{
		LeftButtonHitEvent(hit, result);
		if (result)
			return;
	}

	void WindowBase::OnClosing(ClosingReason reason, bool& cancel)
	{
		ClosingEvent(reason, cancel);
	}

	StringView WindowBase::GetInputText() const
	{
		return m_Settings.AllowInput && m_Focused ? Input::GetInputText() : StringView::Empty;
	}

	bool WindowBase::GetKey(KeyboardKeys key) const
	{
		return m_Settings.AllowInput && m_Focused ? Input::GetKey(key) : false;
	}

	bool WindowBase::GetKeyDown(KeyboardKeys key) const
	{
		return m_Settings.AllowInput && m_Focused ? Input::GetKeyDown(key) : false;
	}

	bool WindowBase::GetKeyUp(KeyboardKeys key) const
	{
		return m_Settings.AllowInput && m_Focused ? Input::GetKeyUp(key) : false;
	}

	Float2 WindowBase::GetMousePosition() const
	{
		return m_Settings.AllowInput && m_Focused ? ScreenToClient(Input::GetMouseScreenPosition()) : Float2::Minimum;
	}

	void WindowBase::SetMousePosition(const Float2& position) const
	{
		if (m_Settings.AllowInput && m_Focused)
		{
			Input::SetMouseScreenPosition(ClientToScreen(position));
		}
	}

	Float2 WindowBase::GetMousePositionDelta() const
	{
		return m_Settings.AllowInput && m_Focused ? Input::GetMousePositionDelta() : Float2::Zero;
	}

	float WindowBase::GetMouseScrollDelta() const
	{
		return m_Settings.AllowInput && m_Focused ? Input::GetMouseScrollDelta() : 0.0f;
	}

	bool WindowBase::GetMouseButton(MouseButton button) const
	{
		return m_Settings.AllowInput && m_Focused ? Input::GetMouseButton(button) : false;
	}

	bool WindowBase::GetMouseButtonDown(MouseButton button) const
	{
		return m_Settings.AllowInput && m_Focused ? Input::GetMouseButtonDown(button) : false;
	}

	bool WindowBase::GetMouseButtonUp(MouseButton button) const
	{
		return m_Settings.AllowInput && m_Focused ? Input::GetMouseButtonUp(button) : false;
	}

	void WindowBase::OnShow()
	{
		PROFILE_CPU_NAMED("GUI.OnShow");
		INVOKE_EVENT_PARAMS_0(OnShow);
		ShownEvent();
	}

	void WindowBase::OnResize(int32 width, int32 height)
	{
		PROFILE_CPU_NAMED("GUI.OnResize");
		ResizeSwapChain(width, height);
		ResizedEvent({ static_cast<float>(width), static_cast<float>(height) });
	}

	void WindowBase::OnClosed()
	{
		// We have to call this from close event to finish WindowBase destroy process (you cannot use WindowBase after Close)
		ENGINE_ASSERT(m_IsClosing);

		// Dispose swap chain (it will wait for GPU work to be done)
		ReleaseSwapChain();

		// Send event
		ClosedEvent();
		INVOKE_EVENT_PARAMS_0(OnClosed);

		// Unregister
		WindowsManager::Unregister(static_cast<Window*>(this));

//		// Delete object
//		DeleteObject(1);
	}

	void WindowBase::OnGotFocus()
	{
		if (m_Focused)
			return;
		m_Focused = true;

		GetFocusEvent();
		INVOKE_EVENT_PARAMS_0(OnGotFocus);
	}

	void WindowBase::OnLostFocus()
	{
		if (!m_Focused)
			return;
		m_Focused = false;

		LostFocusEvent();
		INVOKE_EVENT_PARAMS_0(OnLostFocus);
	}

	void WindowBase::OnUpdate()
	{
		PROFILE_CPU_NAMED("GUI.OnUpdate");
		UpdateEvent();
	}

	void WindowBase::OnDraw()
	{
		PROFILE_CPU_NAMED("GUI.OnDraw");
		DrawEvent();
	}

	void WindowBase::Show()
	{
		const auto clientSize = GetClientSize();
		const auto width = static_cast<int32>(clientSize.x);
		const auto height = static_cast<int32>(clientSize.y);
		m_Visible = true;

		InitSwapChain();

		// Call GUI event
		OnResize(width, height);
		OnShow();
	}

	void WindowBase::Hide()
	{
		if (!m_Visible)
			return;
		EndClippingCursor();
		EndTrackingMouse();
		m_Visible = false;
		m_ShowAfterFirstPaint = m_Settings.ShowAfterFirstPaint;
		HiddenEvent();
	}

	void WindowBase::Close(ClosingReason reason)
	{
		// Prevent from calling close during or after close action
		if (m_IsClosing)
		{
			return;
		}
		m_IsClosing = true;

		// Check if can close window
		bool cancel = false;
		OnClosing(reason, cancel);
		if (cancel && reason != ClosingReason::EngineExit) // Disallow to skip closing on Engine Exit (danger situation)
		{
			// Back
			m_IsClosing = false;
			return;
		}

		// Close
		Hide();
		OnClosed();
	}

	bool WindowBase::IsForegroundWindow() const
	{
		return m_Focused;
	}

}

#include "Runtime/Core/Platform/Window.h"
#include "Runtime/Core/Profiler/ProfilerCPU.h"
#include "Runtime/Core/Platform/CreateWindowSettings.h"
#include "Runtime/Core/Platform/WindowsManager.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRClass.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRCore.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRException.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRMethod.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRUtils.h"
#include "Runtime/Core/Scripting/Scripting.h"
#include "Runtime/Core/Platform/IGuiData.h"
#include "Runtime/Graphics/GraphicWindow.h"
#include "WindowBase.h"
#include "Runtime/Input/Input.h"
#include "Runtime/Utilities/Time.h"

// Helper macros for calling the managed Window event bridge. Do not cache CLRMethod
// pointers here because managed assemblies may be reloaded while a native window lives.
#define INVOKE_EVENT(name, paramsCount, param0, param1, param2) \
	do \
	{ \
		if (Scripting::IsEveryAssemblyLoaded()) \
		{ \
			CLRObject* managedInstance = GetManagedInstance(); \
			CLRClass* managedClass = GetClass(); \
			CLRMethod* method = managedClass ? managedClass->GetMethod("Internal_" #name, paramsCount) : nullptr; \
			if (managedInstance && method) \
			{ \
				void* params[3] = { param0, param1, param2 }; \
				CLRObject* exception = nullptr; \
				method->Invoke(managedInstance, params, &exception); \
				if (exception) \
					CLRException(exception).Log(Log::Severity::Error, SE_TEXT("Window." #name)); \
			} \
			else if (managedInstance && !method) \
			{ \
				LOG_ERROR("Scripting", "Missing managed Window callback Internal_{0}.", String(#name)); \
			} \
		} \
	} while (false)
#define INVOKE_EVENT_PARAMS_3(name, param0, param1, param2) INVOKE_EVENT(name, 3, param0, param1, param2)
#define INVOKE_EVENT_PARAMS_2(name, param0, param1) INVOKE_EVENT(name, 2, param0, param1, nullptr)
#define INVOKE_EVENT_PARAMS_1(name, param0) INVOKE_EVENT(name, 1, param0, nullptr, nullptr)
#define INVOKE_EVENT_PARAMS_0(name) INVOKE_EVENT(name, 0, nullptr, nullptr, nullptr)
#define INVOKE_DRAG_EVENT(name) \
	do \
	{ \
		if (result != DragDropEffect::None || !data || !Scripting::IsEveryAssemblyLoaded()) \
			break; \
		CLRObject* managedInstance = GetManagedInstance(); \
		CLRClass* managedClass = GetClass(); \
		CLRMethod* method = managedClass ? managedClass->GetMethod("Internal_" #name, 3) : nullptr; \
		if (!managedInstance || !method) \
		{ \
			if (managedInstance) \
				LOG_ERROR("Scripting", "Missing managed Window callback Internal_{0}.", String(#name)); \
			break; \
		} \
		List<String> values; \
		const bool isText = data->GetType() == IGuiData::Type::Text; \
		if (isText) \
			values.Add(data->GetAsText()); \
		else \
			data->GetAsFiles(&values); \
		CLRArray* managedValues = CLRCore::Array::New(CLRCore::TypeCache::String, values.Count()); \
		CLRString** managedValuesPtr = CLRCore::Array::GetAddress<CLRString*>(managedValues); \
		for (int32 index = 0; index < values.Count(); index++) \
			managedValuesPtr[index] = CLRUtils::ToString(values[index]); \
		void* params[] = { (void*)&mousePosition, (void*)&isText, managedValues }; \
		CLRObject* exception = nullptr; \
		CLRObject* managedResult = method->Invoke(managedInstance, params, &exception); \
		if (exception) \
			CLRException(exception).Log(Log::Severity::Error, SE_TEXT("Window." #name)); \
		else if (managedResult) \
			result = static_cast<DragDropEffect>(CLRUtils::Unbox<int32>(managedResult)); \
	} while (false)

namespace SE
{
	Window* WindowBase::mainWindow = nullptr;

	WindowBase* WindowBase::Create(CreateWindowSettings settings)
	{
		settings.Backend = CreateWindowSettings::GuiBackend::Managed;
		return New<GraphicWindow>(settings);
	}

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
		: ScriptingObject(SpawnParams(UID::New(), WindowBase::TypeInitializer)),
		  m_Visible(false), _minimized(false), _maximized(false), m_IsClosing(false), m_ShowAfterFirstPaint(settings.ShowAfterFirstPaint), m_Focused(false), _fullscreen(settings.Fullscreen),
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
		INVOKE_EVENT_PARAMS_1(OnCharInput, &c);
	}

	void WindowBase::OnKeyDown(KeyboardKeys key)
	{
		PROFILE_CPU_NAMED("GUI.OnKeyDown");
		KeyDownEvent(key);
		INVOKE_EVENT_PARAMS_1(OnKeyDown, &key);
	}

	void WindowBase::OnKeyUp(KeyboardKeys key)
	{
		PROFILE_CPU_NAMED("GUI.OnKeyUp");
		KeyUpEvent(key);
		INVOKE_EVENT_PARAMS_1(OnKeyUp, &key);
	}

	void WindowBase::OnMouseDown(const Float2& mousePosition, MouseButton button)
	{
		PROFILE_CPU_NAMED("GUI.OnMouseDown");
		MouseDownEvent(mousePosition, button);
		INVOKE_EVENT_PARAMS_2(OnMouseDown, (void*)&mousePosition, &button);
	}

	void WindowBase::OnMouseUp(const Float2& mousePosition, MouseButton button)
	{
		PROFILE_CPU_NAMED("GUI.OnMouseUp");
		MouseUpEvent(mousePosition, button);
		INVOKE_EVENT_PARAMS_2(OnMouseUp, (void*)&mousePosition, &button);
	}

	void WindowBase::OnMouseDoubleClick(const Float2& mousePosition, MouseButton button)
	{
		PROFILE_CPU_NAMED("GUI.OnMouseDoubleClick");
		MouseDoubleClickEvent(mousePosition, button);
		INVOKE_EVENT_PARAMS_2(OnMouseDoubleClick, (void*)&mousePosition, &button);
	}

	void WindowBase::OnMouseWheel(const Float2& mousePosition, float delta)
	{
		PROFILE_CPU_NAMED("GUI.OnMouseWheel");
		MouseWheelEvent(mousePosition, delta);
		INVOKE_EVENT_PARAMS_2(OnMouseWheel, (void*)&mousePosition, &delta);
	}

	void WindowBase::OnMouseMove(const Float2& mousePosition)
	{
		PROFILE_CPU_NAMED("GUI.OnMouseMove");
		MouseMoveEvent(mousePosition);
		INVOKE_EVENT_PARAMS_1(OnMouseMove, (void*)&mousePosition);
	}

	void WindowBase::OnMouseLeave()
	{
		PROFILE_CPU_NAMED("GUI.OnMouseLeave");
		MouseLeaveEvent();
		INVOKE_EVENT_PARAMS_0(OnMouseLeave);
	}

	void WindowBase::OnTouchDown(const Float2& pointerPosition, int32 pointerId)
	{
		PROFILE_CPU_NAMED("GUI.OnTouchDown");
		TouchDownEvent(pointerPosition, pointerId);
		INVOKE_EVENT_PARAMS_2(OnTouchDown, (void*)&pointerPosition, &pointerId);
	}

	void WindowBase::OnTouchMove(const Float2& pointerPosition, int32 pointerId)
	{
		PROFILE_CPU_NAMED("GUI.OnTouchMove");
		TouchMoveEvent(pointerPosition, pointerId);
		INVOKE_EVENT_PARAMS_2(OnTouchMove, (void*)&pointerPosition, &pointerId);
	}

	void WindowBase::OnTouchUp(const Float2& pointerPosition, int32 pointerId)
	{
		PROFILE_CPU_NAMED("GUI.OnTouchUp");
		TouchUpEvent(pointerPosition, pointerId);
		INVOKE_EVENT_PARAMS_2(OnTouchUp, (void*)&pointerPosition, &pointerId);
	}

	void WindowBase::OnDragEnter(IGuiData* data, const Float2& mousePosition, DragDropEffect& result)
	{
		DragEnterEvent(data, mousePosition, result);
		INVOKE_DRAG_EVENT(OnDragEnter);
	}

	void WindowBase::OnDragOver(IGuiData* data, const Float2& mousePosition, DragDropEffect& result)
	{
		DragOverEvent(data, mousePosition, result);
		INVOKE_DRAG_EVENT(OnDragOver);
	}

	void WindowBase::OnDragDrop(IGuiData* data, const Float2& mousePosition, DragDropEffect& result)
	{
		DragDropEvent(data, mousePosition, result);
		INVOKE_DRAG_EVENT(OnDragDrop);
	}

	void WindowBase::OnDragLeave()
	{
		DragLeaveEvent();
		INVOKE_EVENT_PARAMS_0(OnDragLeave);
	}

	void WindowBase::OnHitTest(const Float2& mousePosition, WindowHitCodes& result, bool& handled)
	{
		HitTestEvent(mousePosition, result, handled);
		if (handled)
			return;
		INVOKE_EVENT_PARAMS_3(OnHitTest, (void*)&mousePosition, &result, &handled);
	}

	void WindowBase::OnLeftButtonHit(WindowHitCodes hit, bool& result)
	{
		LeftButtonHitEvent(hit, result);
		if (result)
			return;
		INVOKE_EVENT_PARAMS_2(OnLeftButtonHit, &hit, &result);
	}

	void WindowBase::OnClosing(ClosingReason reason, bool& cancel)
	{
		ClosingEvent(reason, cancel);
		INVOKE_EVENT_PARAMS_2(OnClosing, &reason, &cancel);
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
		INVOKE_EVENT_PARAMS_2(OnResize, &width, &height);
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
		float deltaTime = Time::Update.UnscaledDeltaTime.GetTotalSeconds();
		INVOKE_EVENT_PARAMS_1(OnUpdate, &deltaTime);
	}

	void WindowBase::OnDraw()
	{
		PROFILE_CPU_NAMED("GUI.OnDraw");
		INVOKE_EVENT_PARAMS_0(OnDraw);
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

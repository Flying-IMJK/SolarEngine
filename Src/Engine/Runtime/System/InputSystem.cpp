
#include "Runtime/Utilities/Time.h"
#include "Runtime/Engine.h"
#include "Runtime/System/ProfilingSystem.h"

#include "Runtime/Input/Input.h"
#include "Runtime/Input/InputDevice.h"
#include "Runtime/Input/Mouse.h"
#include "Runtime/Input/Keyboard.h"
#include "Runtime/Input/Gamepad.h"
#include "Runtime/Core/Platform/WindowsManager.h"
#include "Runtime/Core/Platform/CriticalSection.h"

namespace SE
{
	class InputSystem : public ISystem
	{
	public:
		ENGINE_SYSTEM(InputSystem)

		InputSystem() : ISystem(SE_TEXT("Input"), -60)
		{
		}

		bool OnInit() override;
		void OnUpdate() override;
		void OnDispose() override;
	};

	ENGINE_SYSTEM_REGISTER(InputSystem);

	void InputSystem::OnUpdate()
	{
		PROFILE_CPU();
		const auto frame = Time::Update.TicksCount;
		const auto dt = Time::Update.UnscaledDeltaTime.GetTotalSeconds();
		Input::InputEvents.Clear();

		// If application has no user focus then simply clear the state
		if (!Engine::HasFocus)
		{
			if (Input::Mouse)
				Input::Mouse->ResetState();
			if (Input::Keyboard)
				Input::Keyboard->ResetState();
			for (int32 i = 0; i < Input::Gamepads.Count(); i++)
				Input::Gamepads[i]->ResetState();
			Input::Axes.Clear();
			Input::Actions.Clear();
			return;
		}

		// Update input devices state
		if (Input::Mouse)
		{
			if (Input::Mouse->Update(Input::InputEvents))
			{
//				Input::Mouse->DeleteObject();
				Input::Mouse = nullptr;
			}
		}
		if (Input::Keyboard)
		{
			if (Input::Keyboard->Update(Input::InputEvents))
			{
//				Input::Keyboard->DeleteObject();
				Input::Keyboard = nullptr;
			}
		}
		for (int32 i = 0; i < Input::Gamepads.Count(); i++)
		{
			if (Input::Gamepads[i]->Update(Input::InputEvents))
			{
//				Input::Gamepads[i]->DeleteObject();
				Input::Gamepads.RemoveAtKeepOrder(i);
				Input::OnGamepadsChanged();
				i--;
				if (Input::Gamepads.IsEmpty())
					break;
			}
		}
		for (int32 i = 0; i < Input::CustomDevices.Count(); i++)
		{
			if (Input::CustomDevices[i]->Update(Input::InputEvents))
			{
//				Input::CustomDevices[i]->DeleteObject();
				Input::CustomDevices.RemoveAtKeepOrder(i);
				i--;
				if (Input::CustomDevices.IsEmpty())
					break;
			}
		}

		// Send gamepads change events
		if (Input::GamepadsChangedState)
		{
			Input::GamepadsChangedState = false;
			Input::GamepadsChanged();
		}

		// Pick the first focused window for input events
		WindowsManager::WindowsLocker.Lock();
		Window* defaultWindow = nullptr;
		for (auto window : WindowsManager::Windows)
		{
			if (window->IsFocused() && window->GetSettings().AllowInput)
			{
				defaultWindow = window;
				break;
			}
		}
		WindowsManager::WindowsLocker.Unlock();

		// Send input events for the focused window
		WindowsManager::WindowsLocker.Lock();
		for (const auto& e : Input::InputEvents)
		{
			auto window = e.Target ? e.Target : defaultWindow;
			if (!window || !WindowsManager::Windows.Contains(window))
				continue;
			switch (e.Type)
			{
				// Keyboard events
			case InputEventType::Char:
				window->OnCharInput(e.charData);
				break;
			case InputEventType::KeyDown:
				window->OnKeyDown(e.keyData);
				break;
			case InputEventType::KeyUp:
				window->OnKeyUp(e.keyData);
				break;
				// Mouse events
			case InputEventType::MouseDown:
				window->OnMouseDown(window->ScreenToClient(e.mouseData.Position), e.mouseData.Button);
				break;
			case InputEventType::MouseUp:
				window->OnMouseUp(window->ScreenToClient(e.mouseData.Position), e.mouseData.Button);
				break;
			case InputEventType::MouseDoubleClick:
				window->OnMouseDoubleClick(window->ScreenToClient(e.mouseData.Position), e.mouseData.Button);
				break;
			case InputEventType::MouseWheel:
				window->OnMouseWheel(window->ScreenToClient(e.mouseWheelData.Position), e.mouseWheelData.WheelDelta);
				break;
			case InputEventType::MouseMove:
				window->OnMouseMove(window->ScreenToClient(e.mouseData.Position));
				break;
			case InputEventType::MouseLeave:
				window->OnMouseLeave();
				break;
				// Touch events
			case InputEventType::TouchDown:
				window->OnTouchDown(window->ScreenToClient(e.touchData.Position), e.touchData.PointerId);
				break;
			case InputEventType::TouchMove:
				window->OnTouchMove(window->ScreenToClient(e.touchData.Position), e.touchData.PointerId);
				break;
			case InputEventType::TouchUp:
				window->OnTouchUp(window->ScreenToClient(e.touchData.Position), e.touchData.PointerId);
				break;
			}
		}
		WindowsManager::WindowsLocker.Unlock();

		// Skip if game has no focus to handle the input
/*		if (!Engine::HasGameViewportFocus())
		{
			Input::Axes.Clear();
			Input::Actions.Clear();
			return;
		}*/

		// Send input events
		for (const auto& e : Input::InputEvents)
		{
			switch (e.Type)
			{
				// Keyboard events
			case InputEventType::Char:
				Input::CharInput(e.charData);
				break;
			case InputEventType::KeyDown:
				Input::KeyDown(e.keyData);
				break;
			case InputEventType::KeyUp:
				Input::KeyUp(e.keyData);
				break;
				// Mouse events
			case InputEventType::MouseDown:
				Input::MouseDown(e.mouseData.Position, e.mouseData.Button);
				break;
			case InputEventType::MouseUp:
				Input::MouseUp(e.mouseData.Position, e.mouseData.Button);
				break;
			case InputEventType::MouseDoubleClick:
				Input::MouseDoubleClick(e.mouseData.Position, e.mouseData.Button);
				break;
			case InputEventType::MouseWheel:
				Input::MouseWheel(e.mouseWheelData.Position, e.mouseWheelData.WheelDelta);
				break;
			case InputEventType::MouseMove:
				Input::MouseMove(e.mouseData.Position);
				break;
			case InputEventType::MouseLeave:
				Input::MouseLeave();
				break;
				// Touch events
			case InputEventType::TouchDown:
				Input::TouchDown(e.touchData.Position, e.touchData.PointerId);
				break;
			case InputEventType::TouchMove:
				Input::TouchMove(e.touchData.Position, e.touchData.PointerId);
				break;
			case InputEventType::TouchUp:
				Input::TouchUp(e.touchData.Position, e.touchData.PointerId);
				break;
			}
		}

		// Update all actions
		for (int32 i = 0; i < Input::ActionMappings.Count(); i++)
		{
			const auto& config = Input::ActionMappings[i];
			const StringView name = config.Name;
			Input::ActionData& data = Input::Actions[name];

			data.Active = false;
			data.State = InputActionState::Waiting;

			// Mark as updated in this frame
			data.FrameIndex = frame;
		}
		for (int32 i = 0; i < Input::ActionMappings.Count(); i++)
		{
			const auto& config = Input::ActionMappings[i];
			const StringView name = config.Name;
			Input::ActionData& data = Input::Actions[name];

			bool isActive;
			if (config.Mode == InputActionMode::Pressing)
			{
				isActive = Input::GetKey(config.Key) || Input::GetMouseButton(config.MouseButton) || Input::GetGamepadButton(config.Gamepad, config.GamepadButton);
			}
			else if (config.Mode == InputActionMode::Press)
			{
				isActive = Input::GetKeyDown(config.Key) || Input::GetMouseButtonDown(config.MouseButton) || Input::GetGamepadButtonDown(config.Gamepad, config.GamepadButton);
			}
			else
			{
				isActive = Input::GetKeyUp(config.Key) || Input::GetMouseButtonUp(config.MouseButton) || Input::GetGamepadButtonUp(config.Gamepad, config.GamepadButton);
			}

			if (Input::GetKeyDown(config.Key) || Input::GetMouseButtonDown(config.MouseButton) || Input::GetGamepadButtonDown(config.Gamepad, config.GamepadButton))
			{
				data.State = InputActionState::Press;
			}
			else if (Input::GetKey(config.Key) || Input::GetMouseButton(config.MouseButton) || Input::GetGamepadButton(config.Gamepad, config.GamepadButton))
			{
				data.State = InputActionState::Pressing;
			}
			else if (Input::GetKeyUp(config.Key) || Input::GetMouseButtonUp(config.MouseButton) || Input::GetGamepadButtonUp(config.Gamepad, config.GamepadButton))
			{
				data.State = InputActionState::Release;
			}

			data.Active |= isActive;
		}

		// Update all axes
		Input::AxesValues.Resize(Input::AxisMappings.Count(), false);
		for (int32 i = 0; i < Input::AxisMappings.Count(); i++)
		{
			const auto& config = Input::AxisMappings[i];
			const StringView name = config.Name;
			const Input::AxisData& data = Input::Axes[name];

			// Get key raw value
			const bool isPositiveKey = Input::GetKey(config.PositiveButton);
			const bool isNegativeKey = Input::GetKey(config.NegativeButton);
			float keyRawValue = 0;
			if (isPositiveKey && !isNegativeKey)
			{
				keyRawValue = 1;
			}
			else if (!isPositiveKey && isNegativeKey)
			{
				keyRawValue = -1;
			}

			// Apply keyboard curve smoothing and snapping
			float prevKeyValue = data.PrevKeyValue;
			if (config.Snap && (data.PrevKeyValue * keyRawValue < 0))
			{
				prevKeyValue = 0;
			}

			float keyValue;
			if (Math::Abs(prevKeyValue) <= Math::Abs(keyRawValue))
			{
				keyValue = Math::Lerp(prevKeyValue, keyRawValue, Math::Saturate(dt * config.Sensitivity));
			}
			else
			{
				keyValue = Math::Lerp(prevKeyValue, keyRawValue, Math::Saturate(dt * config.Gravity));
			}

			// Get axis raw value
			float axisRawValue = 0.0f;
			switch (config.Axis)
			{
			case InputAxisType::MouseX:
				axisRawValue = Input::GetMousePositionDelta().x * config.Sensitivity;
				break;
			case InputAxisType::MouseY:
				axisRawValue = Input::GetMousePositionDelta().y * config.Sensitivity;
				break;
			case InputAxisType::MouseWheel:
				axisRawValue = Input::GetMouseScrollDelta() * config.Sensitivity;
				break;
			case InputAxisType::GamepadLeftStickX:
				axisRawValue = Input::GetGamepadAxis(config.Gamepad, GamepadAxis::LeftStickX);
				break;
			case InputAxisType::GamepadLeftStickY:
				axisRawValue = Input::GetGamepadAxis(config.Gamepad, GamepadAxis::LeftStickY);
				break;
			case InputAxisType::GamepadRightStickX:
				axisRawValue = Input::GetGamepadAxis(config.Gamepad, GamepadAxis::RightStickX);
				break;
			case InputAxisType::GamepadRightStickY:
				axisRawValue = Input::GetGamepadAxis(config.Gamepad, GamepadAxis::RightStickY);
				break;
			case InputAxisType::GamepadLeftTrigger:
				axisRawValue = Input::GetGamepadAxis(config.Gamepad, GamepadAxis::LeftTrigger);
				break;
			case InputAxisType::GamepadRightTrigger:
				axisRawValue = Input::GetGamepadAxis(config.Gamepad, GamepadAxis::RightTrigger);
				break;
			case InputAxisType::GamepadDPadX:
				if (Input::GetGamepadButton(config.Gamepad, GamepadButton::DPadRight))
					axisRawValue = 1;
				else if (Input::GetGamepadButton(config.Gamepad, GamepadButton::DPadLeft))
					axisRawValue = -1;
				break;
			case InputAxisType::GamepadDPadY:
				if (Input::GetGamepadButton(config.Gamepad, GamepadButton::DPadUp))
					axisRawValue = 1;
				else if (Input::GetGamepadButton(config.Gamepad, GamepadButton::DPadDown))
					axisRawValue = -1;
				break;
			}

			// Apply dead zone
			const float deadZone = config.DeadZone;
			float axisValue = axisRawValue >= deadZone || axisRawValue <= -deadZone ? axisRawValue : 0.0f;
			keyValue = keyValue >= deadZone || keyValue <= -deadZone ? keyValue : 0.0f;

			auto& e = Input::AxesValues[i];
			e.Used = false;
			e.PrevKeyValue = keyRawValue;

			// Select keyboard input or axis input (choose the higher absolute values)
			e.Value = Math::Abs(keyValue) > Math::Abs(axisValue) ? keyValue : axisValue;
			e.RawValue = Math::Abs(keyRawValue) > Math::Abs(axisRawValue) ? keyRawValue : axisRawValue;

			// Scale
			e.Value *= config.Scale;
		}
		for (int32 i = 0; i < Input::AxisMappings.Count(); i++)
		{
			auto& e = Input::AxesValues[i];
			if (e.Used)
				continue;
			const auto& config = Input::AxisMappings[i];
			const StringView name = config.Name;
			Input::AxisData& data = Input::Axes[name];

			// Blend final axis raw value between all entries
			// Virtual axis with the same name may be used more than once, select the highest absolute value
			for (int32 j = i + 1; j < Input::AxisMappings.Count(); j++)
			{
				auto& other = Input::AxesValues[j];
				if (!other.Used && Input::AxisMappings[j].Name == config.Name)
				{
					if (Math::Abs(other.Value) > Math::Abs(e.Value))
					{
						e = other;
					}
					other.Used = true;
				}
			}

			// Setup axis data
			data.PrevKeyValue = e.PrevKeyValue;
			data.ValueRaw = e.RawValue;
			data.Value = e.Value;

			// Mark as updated in this frame
			data.FrameIndex = frame;
		}

		// Remove not used entries
		for (auto i = Input::Actions.begin(); i.IsNotEnd(); ++i)
		{
			if (i->Value.FrameIndex != frame)
			{
				Input::Actions.Remove(i);
			}
		}
		for (auto i = Input::Axes.begin(); i.IsNotEnd(); ++i)
		{
			if (i->Value.FrameIndex != frame)
			{
				Input::Axes.Remove(i);
			}
		}

		// Lock mouse if need to
/*		const auto lockMode = Screen::GetCursorLock();
		if (lockMode == CursorLockMode::Locked)
		{
			Input::SetMousePosition(Screen::GetSize() * 0.5f);
		}*/

		// Send events for the active actions and axes (send events only in play mode)
/*		if (!Time::GetGamePaused())
		{
			for (auto i = Input::Axes.begin(); i.IsNotEnd(); ++i)
			{
				if (!Math::IsNearEqual(i->Value.Value, i->Value.PrevKeyValue))
				{
					Input::AxisValueChanged(i->Key);
				}
			}

			for (auto i = Input::Actions.begin(); i.IsNotEnd(); ++i)
			{
				if (i->Value.State != InputActionState::Waiting)
				{
					Input::ActionTriggered(i->Key, i->Value.State);
				}
			}
		}*/
	}

	void InputSystem::OnDispose()
	{
		// Dispose input devices
		if (Input::Mouse)
		{
//			Input::Mouse->DeleteObject();
			Input::Mouse = nullptr;
		}
		if (Input::Keyboard)
		{
//			Input::Keyboard->DeleteObject();
			Input::Keyboard = nullptr;
		}
		for (int32 i = 0; i < Input::Gamepads.Count(); i++)
		{
//			Input::Gamepads[i]->DeleteObject();
		}
		Input::Gamepads.Clear();
		for (int32 i = 0; i < Input::CustomDevices.Count(); i++)
		{
//			Input::CustomDevices[i]->DeleteObject();
		}
		Input::CustomDevices.Clear();
	}

	bool InputSystem::OnInit()
	{
		return ISystem::OnInit();
	}
}
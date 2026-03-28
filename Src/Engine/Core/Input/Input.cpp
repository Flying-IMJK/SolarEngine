
#include "Input.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Gamepad.h"
#include "Enums.h"

#include "Core/Platform/Window.h"
#include "Core/Platform/WindowsManager.h"

namespace SE
{
	Mouse* Input::Mouse = nullptr;
	Keyboard* Input::Keyboard = nullptr;
	List<Gamepad*, FixedAllocation<MAX_GAMEPADS>> Input::Gamepads;
	Action Input::GamepadsChanged;
	List<InputDevice*, InlinedAllocation<16>> Input::CustomDevices;
	Delegate<Char> Input::CharInput;
	Delegate<KeyboardKeys> Input::KeyDown;
	Delegate<KeyboardKeys> Input::KeyUp;
	Delegate<const Float2&, MouseButton> Input::MouseDown;
	Delegate<const Float2&, MouseButton> Input::MouseUp;
	Delegate<const Float2&, MouseButton> Input::MouseDoubleClick;
	Delegate<const Float2&, float> Input::MouseWheel;
	Delegate<const Float2&> Input::MouseMove;
	Action Input::MouseLeave;
	Delegate<const Float2&, int32> Input::TouchDown;
	Delegate<const Float2&, int32> Input::TouchMove;
	Delegate<const Float2&, int32> Input::TouchUp;
	Delegate<StringView, InputActionState> Input::ActionTriggered;
	Delegate<StringView> Input::AxisValueChanged;
	List <ActionConfig> Input::ActionMappings;
	List <AxisConfig> Input::AxisMappings;

	Dictionary<String, Input::ActionData> Input::Actions;
	Dictionary<String, Input::AxisData> Input::Axes;
	bool Input::GamepadsChangedState = true;
	List<Input::AxisEvaluation> Input::AxesValues;
	InputEventQueue Input::InputEvents;
	

/*	void InputSettings::Apply()
	{
		Input::ActionMappings = ActionMappings;
		Input::AxisMappings = AxisMappings;
	}

	void InputSettings::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
	{
		const auto actionMappings = stream.FindMember("ActionMappings");
		if (actionMappings != stream.MemberEnd())
		{
			auto& actionMappingsArray = actionMappings->value;
			if (actionMappingsArray.IsArray())
			{
				ActionMappings.Resize(actionMappingsArray.Size(), false);
				for (uint32 i = 0; i < actionMappingsArray.Size(); i++)
				{
					auto& v = actionMappingsArray[i];
					if (!v.IsObject())
						continue;

					ActionConfig& config = ActionMappings[i];
					config.Name = JsonTools::GetString(v, "Name");
					config.Mode = JsonTools::GetEnum(v, "Mode", InputActionMode::Pressing);
					config.Key = JsonTools::GetEnum(v, "Key", KeyboardKeys::None);
					config.MouseButton = JsonTools::GetEnum(v, "MouseButton", MouseButton::None);
					config.GamepadButton = JsonTools::GetEnum(v, "GamepadButton", GamepadButton::None);
					config.Gamepad = JsonTools::GetEnum(v, "Gamepad", InputGamepadIndex::All);
				}
			}
			else
			{
				ActionMappings.Resize(0, false);
			}
		}

		const auto axisMappings = stream.FindMember("AxisMappings");
		if (axisMappings != stream.MemberEnd())
		{
			auto& axisMappingsArray = axisMappings->value;
			if (axisMappingsArray.IsArray())
			{
				AxisMappings.Resize(axisMappingsArray.Size(), false);
				for (uint32 i = 0; i < axisMappingsArray.Size(); i++)
				{
					auto& v = axisMappingsArray[i];
					if (!v.IsObject())
						continue;

					AxisConfig& config = AxisMappings[i];
					config.Name = JsonTools::GetString(v, "Name");
					config.Axis = JsonTools::GetEnum(v, "Axis", InputAxisType::MouseX);
					config.Gamepad = JsonTools::GetEnum(v, "Gamepad", InputGamepadIndex::All);
					config.PositiveButton = JsonTools::GetEnum(v, "PositiveButton", KeyboardKeys::None);
					config.NegativeButton = JsonTools::GetEnum(v, "NegativeButton", KeyboardKeys::None);
					config.DeadZone = JsonTools::GetFloat(v, "DeadZone", 0.1f);
					config.Sensitivity = JsonTools::GetFloat(v, "Sensitivity", 0.4f);
					config.Gravity = JsonTools::GetFloat(v, "Gravity", 1.0f);
					config.Scale = JsonTools::GetFloat(v, "Scale", 1.0f);
					config.Snap = JsonTools::GetBool(v, "Snap", false);
				}
			}
			else
			{
				AxisMappings.Resize(0, false);
			}
		}
	}*/

	void Mouse::OnMouseMoved(const Float2& newPosition)
	{
		_prevState.MousePosition = newPosition;
		_state.MousePosition = newPosition;
	}

	void Mouse::OnMouseDown(const Float2& position, const MouseButton button, Window* target)
	{
		InputEvent& e = _queue.AddOne();
		e.Type = InputEventType::MouseDown;
		e.Target = target;
		e.mouseData.Button = button;
		e.mouseData.Position = position;
	}

	bool Mouse::IsAnyButtonDown() const
	{
		// TODO: optimize with SIMD
		bool result = false;
		for (auto e : Mouse::_state.MouseButtons)
			result |= e;
		return result;
	}

	void Mouse::OnMouseUp(const Float2& position, const MouseButton button, Window* target)
	{
		InputEvent& e = _queue.AddOne();
		e.Type = InputEventType::MouseUp;
		e.Target = target;
		e.mouseData.Button = button;
		e.mouseData.Position = position;
	}

	void Mouse::OnMouseDoubleClick(const Float2& position, const MouseButton button, Window* target)
	{
		InputEvent& e = _queue.AddOne();
		e.Type = InputEventType::MouseDoubleClick;
		e.Target = target;
		e.mouseData.Button = button;
		e.mouseData.Position = position;
	}

	void Mouse::OnMouseMove(const Float2& position, Window* target)
	{
		InputEvent& e = _queue.AddOne();
		e.Type = InputEventType::MouseMove;
		e.Target = target;
		e.mouseData.Position = position;
	}

	void Mouse::OnMouseLeave(Window* target)
	{
		InputEvent& e = _queue.AddOne();
		e.Type = InputEventType::MouseLeave;
		e.Target = target;
	}

	void Mouse::OnMouseWheel(const Float2& position, float delta, Window* target)
	{
		InputEvent& e = _queue.AddOne();
		e.Type = InputEventType::MouseWheel;
		e.Target = target;
		e.mouseWheelData.WheelDelta = delta;
		e.mouseWheelData.Position = position;
	}

	void Mouse::ResetState()
	{
		InputDevice::ResetState();

		_prevState.Clear();
		_state.Clear();
	}

	bool Mouse::Update(InputEventQueue& queue)
	{
		// Move the current state to the previous
		Platform::MemoryCopy(&_prevState, &_state, sizeof(State));

		// Gather new events
		if (UpdateState())
			return true;

		// Handle events
		_state.MouseWheelDelta = 0;
		for (int32 i = 0; i < _queue.Count(); i++)
		{
			const InputEvent& e = _queue[i];
			switch (e.Type)
			{
			case InputEventType::MouseDown:
			{
				_state.MouseButtons[static_cast<int32>(e.mouseData.Button)] = true;
				break;
			}
			case InputEventType::MouseUp:
			{
				_state.MouseButtons[static_cast<int32>(e.mouseData.Button)] = false;
				break;
			}
			case InputEventType::MouseDoubleClick:
			{
				_state.MouseButtons[static_cast<int32>(e.mouseData.Button)] = true;
				break;
			}
			case InputEventType::MouseWheel:
			{
				_state.MouseWheelDelta += e.mouseWheelData.WheelDelta;
				break;
			}
			case InputEventType::MouseMove:
			{
				_state.MousePosition = e.mouseData.Position;
				break;
			}
			case InputEventType::MouseLeave:
			{
				break;
			}
			}
		}

		// Send events further
		queue.Add(_queue);
		_queue.Clear();
		return false;
	}

	void Keyboard::State::Clear()
	{
		Platform::MemoryClear(this, sizeof(State));
	}

	Keyboard::Keyboard()
		: InputDevice(SE_TEXT("Keyboard"))
	{
		_state.Clear();
		_prevState.Clear();
	}

	bool Keyboard::IsAnyKeyDown() const
	{
		// TODO: optimize with SIMD
		bool result = false;
		for (auto e : _state.Keys)
			result |= e;
		return result;
	}

	void Keyboard::OnCharInput(Char c, Window* target)
	{
		// Skip control characters
		if (c < 32)
			return;

		InputEvent& e = _queue.AddOne();
		e.Type = InputEventType::Char;
		e.Target = target;
		e.charData = c;
	}

	void Keyboard::OnKeyUp(KeyboardKeys key, Window* target)
	{
		if (key >= KeyboardKeys::MAX)
			return;
		InputEvent& e = _queue.AddOne();
		e.Type = InputEventType::KeyUp;
		e.Target = target;
		e.keyData = key;
	}

	void Keyboard::OnKeyDown(KeyboardKeys key, Window* target)
	{
		if (key >= KeyboardKeys::MAX)
			return;
		InputEvent& e = _queue.AddOne();
		e.Type = InputEventType::KeyDown;
		e.Target = target;
		e.keyData = key;
	}

	void Keyboard::ResetState()
	{
		InputDevice::ResetState();

		_prevState.Clear();
		_state.Clear();
	}

	bool Keyboard::Update(InputEventQueue& queue)
	{
		// Move the current state to the previous
		Platform::MemoryCopy(&_prevState, &_state, sizeof(State));

		// Gather new events
		if (UpdateState())
			return true;

		// Handle events
		_state.InputTextLength = 0;
		for (int32 i = 0; i < _queue.Count(); i++)
		{
			const InputEvent& e = _queue[i];
			switch (e.Type)
			{
			case InputEventType::Char:
			{
				if (_state.InputTextLength < ARRAY_SIZE(_state.InputText) - 1)
					_state.InputText[_state.InputTextLength++] = e.charData;
				break;
			}
			case InputEventType::KeyDown:
			{
				_state.Keys[static_cast<int32>(e.keyData)] = true;
				break;
			}
			case InputEventType::KeyUp:
			{
				_state.Keys[static_cast<int32>(e.keyData)] = false;
				break;
			}
			}
		}

		// Send events further
		queue.Add(_queue);
		_queue.Clear();
		return false;
	}

	int32 Input::GetGamepadsCount()
	{
		return Gamepads.Count();
	}

	Gamepad* Input::GetGamepad(int32 index)
	{
		if (index >= 0 && index < Gamepads.Count())
			return Gamepads[index];
		return nullptr;
	}

	void Input::OnGamepadsChanged()
	{
		GamepadsChangedState = true;
	}

	StringView Input::GetInputText()
	{
		return Keyboard ? Keyboard->GetInputText() : StringView::Empty;
	}

	bool Input::GetKey(const KeyboardKeys key)
	{
		return Keyboard ? Keyboard->GetKey(key) : false;
	}

	bool Input::GetKeyDown(const KeyboardKeys key)
	{
		return Keyboard ? Keyboard->GetKeyDown(key) : false;
	}

	bool Input::GetKeyUp(const KeyboardKeys key)
	{
		return Keyboard ? Keyboard->GetKeyUp(key) : false;
	}

	Float2 Input::GetMousePosition()
	{
		return Mouse ? Mouse->GetPosition()/*Screen::ScreenToGameViewport(Mouse->GetPosition())*/ : Float2::Minimum;
	}

	void Input::SetMousePosition(const Float2& position)
	{
		if (Mouse/* && Engine::HasGameViewportFocus()*/)
		{
			const auto pos = position;//Screen::GameViewportToScreen(position);
			if (pos > Float2::Minimum)
				Mouse->SetMousePosition(pos);
		}
	}

	Float2 Input::GetMouseScreenPosition()
	{
		return Mouse ? Mouse->GetPosition() : Float2::Minimum;
	}

	void Input::SetMouseScreenPosition(const Float2& position)
	{
		if (Mouse/* && Engine::HasFocus*/)
		{
			Mouse->SetMousePosition(position);
		}
	}

	Float2 Input::GetMousePositionDelta()
	{
		return Mouse ? Mouse->GetPositionDelta() : Float2::Zero;
	}

	float Input::GetMouseScrollDelta()
	{
		return Mouse ? Mouse->GetScrollDelta() : 0.0f;
	}

	bool Input::GetMouseButton(const MouseButton button)
	{
		return Mouse ? Mouse->GetButton(button) : false;
	}

	bool Input::GetMouseButtonDown(const MouseButton button)
	{
		return Mouse ? Mouse->GetButtonDown(button) : false;
	}

	bool Input::GetMouseButtonUp(const MouseButton button)
	{
		return Mouse ? Mouse->GetButtonUp(button) : false;
	}

	float Input::GetGamepadAxis(int32 gamepadIndex, GamepadAxis axis)
	{
		if (gamepadIndex >= 0 && gamepadIndex < Gamepads.Count())
			return Gamepads[gamepadIndex]->GetAxis(axis);
		return 0.0f;
	}

	bool Input::GetGamepadButton(int32 gamepadIndex, GamepadButton button)
	{
		if (gamepadIndex >= 0 && gamepadIndex < Gamepads.Count())
			return Gamepads[gamepadIndex]->GetButton(button);
		return false;
	}

	bool Input::GetGamepadButtonDown(int32 gamepadIndex, GamepadButton button)
	{
		if (gamepadIndex >= 0 && gamepadIndex < Gamepads.Count())
			return Gamepads[gamepadIndex]->GetButtonDown(button);
		return false;
	}

	bool Input::GetGamepadButtonUp(int32 gamepadIndex, GamepadButton button)
	{
		if (gamepadIndex >= 0 && gamepadIndex < Gamepads.Count())
			return Gamepads[gamepadIndex]->GetButtonUp(button);
		return false;
	}

	float Input::GetGamepadAxis(InputGamepadIndex gamepad, GamepadAxis axis)
	{
		if (gamepad == InputGamepadIndex::All)
		{
			float result = 0.0f;
			for (auto g : Gamepads)
			{
				float v = g->GetAxis(axis);
				if (Math::Abs(v) > Math::Abs(result))
					result = v;
			}
			return result;
		}
		else
		{
			const auto index = static_cast<int32>(gamepad);
			if (index < Gamepads.Count())
				return Gamepads[index]->GetAxis(axis);
		}
		return false;
	}

	bool Input::GetGamepadButton(InputGamepadIndex gamepad, GamepadButton button)
	{
		if (gamepad == InputGamepadIndex::All)
		{
			for (auto g : Gamepads)
			{
				if (g->GetButton(button))
					return true;
			}
		}
		else
		{
			const auto index = static_cast<int32>(gamepad);
			if (index < Gamepads.Count())
				return Gamepads[index]->GetButton(button);
		}
		return false;
	}

	bool Input::GetGamepadButtonDown(InputGamepadIndex gamepad, GamepadButton button)
	{
		if (gamepad == InputGamepadIndex::All)
		{
			for (auto g : Gamepads)
			{
				if (g->GetButtonDown(button))
					return true;
			}
		}
		else
		{
			const auto index = static_cast<int32>(gamepad);
			if (index < Gamepads.Count())
				return Gamepads[index]->GetButtonDown(button);
		}
		return false;
	}

	bool Input::GetGamepadButtonUp(InputGamepadIndex gamepad, GamepadButton button)
	{
		if (gamepad == InputGamepadIndex::All)
		{
			for (auto g : Gamepads)
			{
				if (g->GetButtonUp(button))
					return true;
			}
		}
		else
		{
			const auto index = static_cast<int32>(gamepad);
			if (index < Gamepads.Count())
				return Gamepads[index]->GetButtonUp(button);
		}
		return false;
	}

	bool Input::GetAction(const StringView& name)
	{
		const auto e = Actions.TryGet(name);
		return e ? e->Active : false;
	}

	InputActionState Input::GetActionState(const StringView& name)
	{
		const auto e = Actions.TryGet(name);
		if (e != nullptr)
		{
			return e->State;
		}
		return InputActionState::None;
	}

	float Input::GetAxis(const StringView& name)
	{
		const auto e = Axes.TryGet(name);
		return e ? e->Value : false;
	}

	float Input::GetAxisRaw(const StringView& name)
	{
		const auto e = Axes.TryGet(name);
		return e ? e->ValueRaw : false;
	}
}
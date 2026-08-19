
#include "Input.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Gamepad.h"
#include "Enums.h"

#include "Runtime/Core/Platform/Window.h"
#include "Runtime/Core/Platform/WindowsManager.h"
#include "Runtime/Core/Systems.h"
#include "Runtime/Core/Profiler/Profiler.h"
#include "Runtime/Engine.h"
#include "Runtime/Utilities/Time.h"

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



	class InputSystem : public ISystem
    {
    public:
        ENGINE_SYSTEM(InputSystem)

        InputSystem() : ISystem(SE_TEXT("Input"), -60) {}

        bool OnInit() override;
        void OnUpdate() override;
        void OnDispose() override;
    };

    ENGINE_SYSTEM_REGISTER(InputSystem);

    void InputSystem::OnUpdate()
    {
        PROFILE_CPU();
        const auto frame = Time::Update.TicksCount;
        const auto dt    = Time::Update.UnscaledDeltaTime.GetTotalSeconds();
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
                    window->OnMouseWheel(window->ScreenToClient(e.mouseWheelData.Position),
                                         e.mouseWheelData.WheelDelta);
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
            const auto&        config = Input::ActionMappings[i];
            const StringView   name   = config.Name;
            Input::ActionData& data   = Input::Actions[name];

            data.Active = false;
            data.State  = InputActionState::Waiting;

            // Mark as updated in this frame
            data.FrameIndex = frame;
        }
        for (int32 i = 0; i < Input::ActionMappings.Count(); i++)
        {
            const auto&        config = Input::ActionMappings[i];
            const StringView   name   = config.Name;
            Input::ActionData& data   = Input::Actions[name];

            bool isActive;
            if (config.Mode == InputActionMode::Pressing)
            {
                isActive = Input::GetKey(config.Key) || Input::GetMouseButton(config.MouseButton) ||
                           Input::GetGamepadButton(config.Gamepad, config.GamepadButton);
            }
            else if (config.Mode == InputActionMode::Press)
            {
                isActive = Input::GetKeyDown(config.Key) || Input::GetMouseButtonDown(config.MouseButton) ||
                           Input::GetGamepadButtonDown(config.Gamepad, config.GamepadButton);
            }
            else
            {
                isActive = Input::GetKeyUp(config.Key) || Input::GetMouseButtonUp(config.MouseButton) ||
                           Input::GetGamepadButtonUp(config.Gamepad, config.GamepadButton);
            }

            if (Input::GetKeyDown(config.Key) || Input::GetMouseButtonDown(config.MouseButton) ||
                Input::GetGamepadButtonDown(config.Gamepad, config.GamepadButton))
            {
                data.State = InputActionState::Press;
            }
            else if (Input::GetKey(config.Key) || Input::GetMouseButton(config.MouseButton) ||
                     Input::GetGamepadButton(config.Gamepad, config.GamepadButton))
            {
                data.State = InputActionState::Pressing;
            }
            else if (Input::GetKeyUp(config.Key) || Input::GetMouseButtonUp(config.MouseButton) ||
                     Input::GetGamepadButtonUp(config.Gamepad, config.GamepadButton))
            {
                data.State = InputActionState::Release;
            }

            data.Active |= isActive;
        }

        // Update all axes
        Input::AxesValues.Resize(Input::AxisMappings.Count(), false);
        for (int32 i = 0; i < Input::AxisMappings.Count(); i++)
        {
            const auto&            config = Input::AxisMappings[i];
            const StringView       name   = config.Name;
            const Input::AxisData& data   = Input::Axes[name];

            // Get key raw value
            const bool isPositiveKey = Input::GetKey(config.PositiveButton);
            const bool isNegativeKey = Input::GetKey(config.NegativeButton);
            float      keyRawValue   = 0;
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
            const float deadZone  = config.DeadZone;
            float       axisValue = axisRawValue >= deadZone || axisRawValue <= -deadZone ? axisRawValue : 0.0f;
            keyValue              = keyValue >= deadZone || keyValue <= -deadZone ? keyValue : 0.0f;

            auto& e        = Input::AxesValues[i];
            e.Used         = false;
            e.PrevKeyValue = keyRawValue;

            // Select keyboard input or axis input (choose the higher absolute values)
            e.Value    = Math::Abs(keyValue) > Math::Abs(axisValue) ? keyValue : axisValue;
            e.RawValue = Math::Abs(keyRawValue) > Math::Abs(axisRawValue) ? keyRawValue : axisRawValue;

            // Scale
            e.Value *= config.Scale;
        }
        for (int32 i = 0; i < Input::AxisMappings.Count(); i++)
        {
            auto& e = Input::AxesValues[i];
            if (e.Used)
                continue;
            const auto&      config = Input::AxisMappings[i];
            const StringView name   = config.Name;
            Input::AxisData& data   = Input::Axes[name];

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
            data.ValueRaw     = e.RawValue;
            data.Value        = e.Value;

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

    bool InputSystem::OnInit() { return ISystem::OnInit(); }
}
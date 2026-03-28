#pragma once

#include "Core/API.h"
#include "Core/Types/Variable.h"
#include "Core/Types/Delegate.h"
#include "Core/Math/Vector2.h"

#include "KeyboardKeys.h"
#include "VirtualInput.h"

namespace SE
{
	class Mouse;
	class Keyboard;
	class Gamepad;
	class InputDevice;

	enum class InputEventType
	{
		Char,
		KeyDown,
		KeyUp,
		MouseDown,
		MouseUp,
		MouseDoubleClick,
		MouseWheel,
		MouseMove,
		MouseLeave,
		TouchDown,
		TouchMove,
		TouchUp,
	};

	struct InputEvent
	{
		InputEventType Type;
		Window* Target;

		union
		{
			Char charData;
			KeyboardKeys keyData;
			struct MouseData
			{
				MouseButton Button;
				Float2 Position;
			} mouseData;

			struct MouseWheelData
			{
				float WheelDelta;
				Float2 Position;
			} mouseWheelData;

			struct TouchData
			{
				Float2 Position;
				int32 PointerId;
			} touchData;
		};

		InputEvent()
		{
		}

		InputEvent(const InputEvent& e)
		{
			Platform::MemoryCopy(this, &e, sizeof(InputEvent));
		}
	};

	typedef List<InputEvent, InlinedAllocation<32>> InputEventQueue;

	/// <summary>
	/// The user input handling service.
	/// </summary>
	class SE_API_CORE Input
	{
	public:
		friend class InputSystem;

		/// <summary>
		/// Gets the mouse (null if platform does not support mouse or it is not connected).
		/// </summary>
		static Mouse* Mouse;

		/// <summary>
		/// Gets the keyboard (null if platform does not support keyboard or it is not connected).
		/// </summary>
		static Keyboard* Keyboard;

		/// <summary>
		/// Gets the gamepads.
		/// </summary>
		static List<Gamepad*, FixedAllocation<MAX_GAMEPADS>> Gamepads;

		/// <summary>
		/// Gets the gamepads count.
		/// </summary>
		/// <returns>The amount of active gamepads devices.</returns>
		static int32 GetGamepadsCount();

		/// <summary>
		/// Gets the gamepads count.
		/// </summary>
		/// <param name="index">The gamepad index.</param>
		/// <returns>The gamepad device or null if index is invalid.</returns>
		static Gamepad* GetGamepad(int32 index);

		/// <summary>
		/// Action called when gamepads collection gets changed (during input update).
		/// </summary>
		static Action GamepadsChanged;

		/// <summary>
		/// Called when gamepads collection gets changed.
		/// </summary>
		static void OnGamepadsChanged();

		/// <summary>
		/// Gets or sets the custom input devices.
		/// </summary>
		static List<InputDevice*, InlinedAllocation<16>> CustomDevices;

	public:

		/// <summary>
		/// Event fired on character input.
		/// </summary>
		static Delegate<Char> CharInput;

		/// <summary>
		/// Event fired on key pressed.
		/// </summary>
		static Delegate<KeyboardKeys> KeyDown;

		/// <summary>
		/// Event fired on key released.
		/// </summary>
		static Delegate<KeyboardKeys> KeyUp;

		/// <summary>
		/// Event fired when mouse button goes down.
		/// </summary>
		static Delegate<const Float2&, MouseButton> MouseDown;

		/// <summary>
		/// Event fired when mouse button goes up.
		/// </summary>
		static Delegate<const Float2&, MouseButton> MouseUp;

		/// <summary>
		/// Event fired when mouse button double clicks.
		/// </summary>
		static Delegate<const Float2&, MouseButton> MouseDoubleClick;

		/// <summary>
		/// Event fired when mouse wheel is scrolling (wheel delta is normalized).
		/// </summary>
		static Delegate<const Float2&, float> MouseWheel;

		/// <summary>
		/// Event fired when mouse moves.
		/// </summary>
		static Delegate<const Float2&> MouseMove;

		/// <summary>
		/// Event fired when mouse leaves window.
		/// </summary>
		static Action MouseLeave;

		/// <summary>
		/// Event fired when touch action begins.
		/// </summary>
		static Delegate<const Float2&, int32> TouchDown;

		/// <summary>
		/// Event fired when touch action moves.
		/// </summary>
		static Delegate<const Float2&, int32> TouchMove;

		/// <summary>
		/// Event fired when touch action ends.
		/// </summary>
		static Delegate<const Float2&, int32> TouchUp;

	public:
		/// <summary>
		/// Gets the text entered during the current frame (Unicode).
		/// </summary>
		/// <returns>The input text (Unicode).</returns>
		static StringView GetInputText();

		/// <summary>
		/// Gets the key state (true if key is being pressed during this frame).
		/// </summary>
		/// <param name="key">Key ID to check</param>
		/// <returns>True while the user holds down the key identified by id</returns>
		static bool GetKey(KeyboardKeys key);

		/// <summary>
		/// Gets the key 'down' state (true if key was pressed in this frame).
		/// </summary>
		/// <param name="key">Key ID to check</param>
		/// <returns>True during the frame the user starts pressing down the key</returns>
		static bool GetKeyDown(KeyboardKeys key);

		/// <summary>
		/// Gets the key 'up' state (true if key was released in this frame).
		/// </summary>
		/// <param name="key">Key ID to check</param>
		/// <returns>True during the frame the user releases the key</returns>
		static bool GetKeyUp(KeyboardKeys key);

	public:
		/// <summary>
		/// Gets the mouse position in game window coordinates.
		/// </summary>
		/// <returns>Mouse cursor coordinates</returns>
		static Float2 GetMousePosition();

		/// <summary>
		/// Sets the mouse position in game window coordinates.
		/// </summary>
		/// <param name="position">Mouse position to set on</param>
		static void SetMousePosition(const Float2& position);

		/// <summary>
		/// Gets the mouse position in screen-space coordinates.
		/// </summary>
		/// <returns>Mouse cursor coordinates</returns>
		static Float2 GetMouseScreenPosition();

		/// <summary>
		/// Sets the mouse position in screen-space coordinates.
		/// </summary>
		/// <param name="position">Mouse position to set on</param>
		static void SetMouseScreenPosition(const Float2& position);

		/// <summary>
		/// Gets the mouse position change during the last frame.
		/// </summary>
		/// <returns>Mouse cursor position delta</returns>
		static Float2 GetMousePositionDelta();

		/// <summary>
		/// Gets the mouse wheel change during the last frame.
		/// </summary>
		/// <returns>Mouse wheel value delta</returns>
		static float GetMouseScrollDelta();

		/// <summary>
		/// Gets the mouse button state.
		/// </summary>
		/// <param name="button">Mouse button to check</param>
		/// <returns>True while the user holds down the button</returns>
		static bool GetMouseButton(MouseButton button);

		/// <summary>
		/// Gets the mouse button down state.
		/// </summary>
		/// <param name="button">Mouse button to check</param>
		/// <returns>True during the frame the user starts pressing down the button</returns>
		static bool GetMouseButtonDown(MouseButton button);

		/// <summary>
		/// Gets the mouse button up state.
		/// </summary>
		/// <param name="button">Mouse button to check</param>
		/// <returns>True during the frame the user releases the button</returns>
		static bool GetMouseButtonUp(MouseButton button);

	public:
		/// <summary>
		/// Gets the gamepad axis value.
		/// </summary>
		/// <param name="gamepadIndex">The gamepad index</param>
		/// <param name="axis">Gamepad axis to check</param>
		/// <returns>Axis value.</returns>
		static float GetGamepadAxis(int32 gamepadIndex, GamepadAxis axis);

		/// <summary>
		/// Gets the gamepad button state (true if being pressed during the current frame).
		/// </summary>
		/// <param name="gamepadIndex">The gamepad index</param>
		/// <param name="button">Gamepad button to check</param>
		/// <returns>True if user holds down the button, otherwise false.</returns>
		static bool GetGamepadButton(int32 gamepadIndex, GamepadButton button);

		/// <summary>
		/// Gets the gamepad button down state (true if was pressed during the current frame).
		/// </summary>
		/// <param name="gamepadIndex">The gamepad index</param>
		/// <param name="button">Gamepad button to check</param>
		/// <returns>True if user starts pressing down the button, otherwise false.</returns>
		static bool GetGamepadButtonDown(int32 gamepadIndex, GamepadButton button);

		/// <summary>
		/// Gets the gamepad button up state (true if was released during the current frame).
		/// </summary>
		/// <param name="gamepadIndex">The gamepad index</param>
		/// <param name="button">Gamepad button to check</param>
		/// <returns>True if user releases the button, otherwise false.</returns>
		static bool GetGamepadButtonUp(int32 gamepadIndex, GamepadButton button);

		/// <summary>
		/// Gets the gamepad axis value.
		/// </summary>
		/// <param name="gamepad">The gamepad</param>
		/// <param name="axis">Gamepad axis to check</param>
		/// <returns>Axis value.</returns>
		static float GetGamepadAxis(InputGamepadIndex gamepad, GamepadAxis axis);

		/// <summary>
		/// Gets the gamepad button state (true if being pressed during the current frame).
		/// </summary>
		/// <param name="gamepad">The gamepad</param>
		/// <param name="button">Gamepad button to check</param>
		/// <returns>True if user holds down the button, otherwise false.</returns>
		static bool GetGamepadButton(InputGamepadIndex gamepad, GamepadButton button);

		/// <summary>
		/// Gets the gamepad button down state (true if was pressed during the current frame).
		/// </summary>
		/// <param name="gamepad">The gamepad</param>
		/// <param name="button">Gamepad button to check</param>
		/// <returns>True if user starts pressing down the button, otherwise false.</returns>
		static bool GetGamepadButtonDown(InputGamepadIndex gamepad, GamepadButton button);

		/// <summary>
		/// Gets the gamepad button up state (true if was released during the current frame).
		/// </summary>
		/// <param name="gamepad">The gamepad</param>
		/// <param name="button">Gamepad button to check</param>
		/// <returns>True if user releases the button, otherwise false.</returns>
		static bool GetGamepadButtonUp(InputGamepadIndex gamepad, GamepadButton button);

	public:
		/// <summary>
		/// Maps a discrete button or key press events to a "friendly name" that will later be bound to event-driven behavior. The end effect is that pressing (and/or releasing) a key, mouse button, or keypad button.
		/// </summary>
		static List<ActionConfig> ActionMappings;

		/// <summary>
		/// Maps keyboard, controller, or mouse inputs to a "friendly name" that will later be bound to continuous game behavior, such as movement. The inputs mapped in AxisMappings are continuously polled, even if they are just reporting that their input value.
		/// </summary>
		static List<AxisConfig> AxisMappings;

		/// <summary>
		/// Event fired when virtual input action is triggered. Called before scripts update. See <see cref="ActionMappings"/> to edit configuration.
		/// </summary>
		/// <seealso cref="InputEvent"/>
		static Delegate<StringView, InputActionState> ActionTriggered;

		/// <summary>
		/// Event fired when virtual input axis is changed. Called before scripts update. See <see cref="AxisMappings"/> to edit configuration.
		/// </summary>
		/// <seealso cref="InputAxis"/>
		static Delegate<StringView> AxisValueChanged;

		/// <summary>
		/// Gets the value of the virtual action identified by name. Use <see cref="ActionMappings"/> to get the current config.
		/// </summary>
		/// <param name="name">The action name.</param>
		/// <returns>True if action has been triggered in the current frame (e.g. button pressed), otherwise false.</returns>
		/// <seealso cref="ActionMappings"/>
		static bool GetAction(const StringView& name);

		/// <summary>
		/// Gets the value of the virtual action identified by name. Use <see cref="ActionMappings"/> to get the current config.
		/// </summary>
		/// <param name="name">The action name.</param>
		/// <returns>A InputActionPhase determining the current phase of the Action (e.g If it was just pressed, is being held or just released).</returns>
		/// <seealso cref="ActionMappings"/>
		static InputActionState GetActionState(const StringView& name);

		/// <summary>
		/// Gets the value of the virtual axis identified by name. Use <see cref="AxisMappings"/> to get the current config.
		/// </summary>
		/// <param name="name">The action name.</param>
		/// <returns>The current axis value (e.g for gamepads it's in the range -1..1). Value is smoothed to reduce artifacts.</returns>
		/// <seealso cref="AxisMappings"/>
		static float GetAxis(const StringView& name);

		/// <summary>
		/// Gets the raw value of the virtual axis identified by name with no smoothing filtering applied. Use <see cref="AxisMappings"/> to get the current config.
		/// </summary>
		/// <param name="name">The action name.</param>
		/// <returns>The current axis value (e.g for gamepads it's in the range -1..1). No smoothing applied.</returns>
		/// <seealso cref="AxisMappings"/>
		static float GetAxisRaw(const StringView& name);

	public:
		struct AxisEvaluation
		{
			float RawValue;
			float Value;
			float PrevKeyValue;
			bool Used;
		};

		struct ActionData
		{
			bool Active;
			uint64 FrameIndex;
			InputActionState State;

			ActionData()
			{
				Active = false;
				FrameIndex = 0;
				State = InputActionState::Waiting;
			}
		};

		struct AxisData
		{
			float Value;
			float ValueRaw;
			float PrevKeyValue;
			uint64 FrameIndex;

			AxisData()
			{
				Value = 0.0f;
				ValueRaw = 0.0f;
				PrevKeyValue = 0.0f;
				FrameIndex = 0;
			}
		};

	private:
		static Dictionary<String, ActionData> Actions;
		static Dictionary<String, AxisData> Axes;
		static bool GamepadsChangedState;
		static List<AxisEvaluation> AxesValues;
		static InputEventQueue InputEvents;
	};

} // SE

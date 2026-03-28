
#pragma once

#include "InputDevice.h"
#include "Core/Types/UID.h"
#include "Core/Math/Vector2.h"

namespace SE
{
	/// <summary>
	/// General identifiers for potential force feedback channels. These will be mapped according to the platform specific implementation.
	/// </summary>
	struct SE_API_CORE GamepadVibrationState
	{
		/// <summary>
		/// The left large motor vibration.
		/// </summary>
		float LeftLarge;
	
		/// <summary>
		/// The left small motor vibration.
		/// </summary>
		float LeftSmall;
	
		/// <summary>
		/// The right large motor vibration.
		/// </summary>
		float RightLarge;
	
		/// <summary>
		/// The right small motor vibration.
		/// </summary>
		float RightSmall;
	
		GamepadVibrationState()
			: LeftLarge(0.0f)
			, LeftSmall(0.0f)
			, RightLarge(0.0f)
			, RightSmall(0.0f)
		{
		}
	};

	/// <summary>
	/// Gamepad buttons and axis mapping description.
	/// Allows converting input from the different gamepads into a universal format (see <see cref="Gamepad::State::ButtonTypes"/> and <see cref="Gamepad::State::AxisTypes"/>).
	/// </summary>
	struct SE_API_CORE GamepadLayout
	{
		/// <summary>
		/// The buttons mapping. Index by gamepad button id from 0 to 31 (see <see cref="Gamepad::State::ButtonTypes"/>).
		/// </summary>
		GamepadButton Buttons[(int32)GamepadButton::MAX];

		/// <summary>
		/// The axis mapping. Index by gamepad axis id from 0 to 5 (see <see cref="Gamepad::State::AxisTypes"/>).
		/// </summary>
		GamepadAxis Axis[(int32)GamepadAxis::MAX];

		/// <summary>
		/// The axis ranges mapping (X is scale, Y is offset. Eg. mappedVal = X * value + Y). It allows to invert any axis or map axis range.
		/// </summary>
		Float2 AxisMap[(int32)GamepadAxis::MAX];

		/// <summary>
		/// Initializes layout with default values.
		/// </summary>
		void Init();
	};

	/// <summary>
	/// Represents a single hardware gamepad device. Used by the Input to report raw gamepad input events.
	/// </summary>
	class SE_API_CORE Gamepad : public InputDevice
	{
	public:
		/// <summary>
		/// The universal gamepad state description. All hardware gamepad device handlers should map input to match this structure.
		/// Later on, each gamepad may use individual layout for a game.
		/// </summary>
		struct State
		{
			/// <summary>
			/// The buttons state (pressed if true).
			/// </summary>
			bool Buttons[(int32)GamepadButton::MAX];

			/// <summary>
			/// The axis state (normalized value).
			/// </summary>
			float Axis[(int32)GamepadAxis::MAX];

			/// <summary>
			/// Clears the state.
			/// </summary>
			void Clear()
			{
				Platform::MemoryClear(this, sizeof(State));
			}
		};

	protected:
		UID _productId;
		State _state;
		State _mappedState;
		State _mappedPrevState;

		explicit Gamepad(const UID& productId, const String& name);

	public:
		/// <summary>
		/// The gamepad layout.
		/// </summary>
		GamepadLayout Layout;

	public:
		/// <summary>
		/// Gets the gamepad device type identifier.
		/// </summary>
		/// <returns>The id.</returns>
		FORCE_INLINE const UID& GetProductID() const
		{
			return _productId;
		}

		/// <summary>
		/// Gets the current gamepad state.
		/// </summary>
		/// <param name="result">The result.</param>
		FORCE_INLINE void GetState(State& result) const
		{
			result = _state;
		}

		/// <summary>
		/// Gets the gamepad axis value.
		/// </summary>
		/// <param name="axis">Gamepad axis to check</param>
		/// <returns>Axis value.</returns>
		FORCE_INLINE float GetAxis(const GamepadAxis axis) const
		{
			return _mappedState.Axis[static_cast<int32>(axis)];
		}

		/// <summary>
		/// Gets the gamepad button state (true if being pressed during the current frame).
		/// </summary>
		/// <param name="button">Gamepad button to check</param>
		/// <returns>True if user holds down the button, otherwise false.</returns>
		FORCE_INLINE bool GetButton(const GamepadButton button) const
		{
			return _mappedState.Buttons[static_cast<int32>(button)];
		}

		/// <summary>
		/// Gets the gamepad button down state (true if was pressed during the current frame).
		/// </summary>
		/// <param name="button">Gamepad button to check</param>
		/// <returns>True if user starts pressing down the button, otherwise false.</returns>
		FORCE_INLINE bool GetButtonDown(const GamepadButton button) const
		{
			return _mappedState.Buttons[static_cast<int32>(button)] && !_mappedPrevState.Buttons[static_cast<int32>(button)];
		}

		/// <summary>
		/// Checks if any gamepad button is currently pressed.
		/// </summary>
		bool IsAnyButtonDown() const;

		/// <summary>
		/// Gets the gamepad button up state (true if was released during the current frame).
		/// </summary>
		/// <param name="button">Gamepad button to check</param>
		/// <returns>True if user releases the button, otherwise false.</returns>
		FORCE_INLINE bool GetButtonUp(const GamepadButton button) const
		{
			return !_mappedState.Buttons[static_cast<int32>(button)] && _mappedPrevState.Buttons[static_cast<int32>(button)];
		}

	public:
		/// <summary>
		/// Sets the state of the gamepad vibration. Ignored if controller does not support this.
		/// </summary>
		/// <param name="state">The state.</param>
		virtual void SetVibration(const GamepadVibrationState& state)
		{
		}

		/// <summary>
		/// Sets the color of the gamepad light. Ignored if controller does not support this.
		/// </summary>
		/// <param name="color">The color.</param>
		virtual void SetColor(const Color& color)
		{
		}

		/// <summary>
		/// Resets the color of the gamepad light to the default. Ignored if controller does not support this.
		/// </summary>
		virtual void ResetColor()
		{
		}

	public:
		// [InputDevice]
		void ResetState() override;
		bool Update(InputEventQueue& queue) final override;
	};
}
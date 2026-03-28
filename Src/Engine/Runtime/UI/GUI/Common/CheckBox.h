#pragma once
#include "Runtime/UI/GUI/Control.h"

namespace SE
{
    class IBrush;
    /// <summary>
    /// The checkbox control states.
    /// </summary>
    enum CheckBoxState
    {
        /// <summary>
        /// The default state.
        /// </summary>
        Default,

        /// <summary>
        /// The checked state.
        /// </summary>
        Checked,

        /// <summary>
        /// The intermediate state.
        /// </summary>
        Intermediate,
    };

    /// <summary>
    /// Check box control.
    /// </summary>
    class SE_API_RUNTIME CheckBox : public Control
    {
    protected:
        /// <summary>
        /// True if checked is being pressed (by mouse or touch).
        /// </summary>
        bool _isPressed;

        /// <summary>
        /// The current state.
        /// </summary>
        CheckBoxState _state;

        /// <summary>
        /// The mouse over box state.
        /// </summary>
        bool _mouseOverBox;

        /// <summary>
        /// The box size.
        /// </summary>
        float _boxSize;

        /// <summary>
        /// The box rectangle.
        /// </summary>
        Rectangle _box;

    public:
        /// <summary>
        /// Gets or sets the state of the checkbox.
        /// </summary>
        PRO(State, CheckBox, CheckBoxState, __GetState, __SetState);

        /// <summary>
        /// Gets or sets a value indicating whether this <see cref="CheckBox"/> is checked.
        /// </summary>
        PRO(Checked, CheckBox, bool, __GetChecked, __SetChecked);

        /// <summary>
        /// Gets or sets a value indicating whether this <see cref="CheckBox"/> is in the intermediate state.
        /// </summary>
        PRO(Intermediate, CheckBox, bool, __GetIntermediate, __SetIntermediate);

        /// <summary>
        /// Gets or sets the size of the box.
        /// </summary>
        PRO(BoxSize, CheckBox, float, __GetBoxSizee, __SetBoxSize);

        /// <summary>
        /// Gets or sets whether to have a border.
        /// </summary>
        bool HasBorder = true;
        
        /// <summary>
        /// Gets or sets the border thickness.
        /// </summary>
        float BorderThickness  = 1.0f;

        /// <summary>
        /// Gets or sets the color of the border.
        /// </summary>
        Color BorderColor;

        /// <summary>
        /// Gets or sets the border color when checkbox is hovered.
        /// </summary>
        Color BorderColorHighlighted;

        /// <summary>
        /// Gets or sets the color of the checkbox icon.
        /// </summary>
        Color ImageColor;

        /// <summary>
        /// Gets or sets the image used to render checkbox checked state.
        /// </summary>
        IBrush* CheckedImage;

        /// <summary>
        /// Gets or sets the image used to render checkbox intermediate state.
        /// </summary>
        IBrush* IntermediateImage;

        /// <summary>
        /// Event fired when 'checked' state gets changed.
        /// </summary>
        Delegate<CheckBox*> StateChanged;

        /// <summary>
        /// Initializes a new instance of the <see cref="CheckBox"/> class.
        /// </summary>
        CheckBox(): CheckBox(0, 0)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="CheckBox"/> class.
        /// </summary>
        /// <param name="x">The x.</param>
        /// <param name="y">The y.</param>
        /// <param name="isChecked">if set to <c>true</c> set checked on start.</param>
        /// <param name="size">The checkbox size.</param>
        CheckBox(float x, float y, bool isChecked = false, float size = 18);

        ~CheckBox() override;
        
        /// <summary>
        /// Toggles the checked state.
        /// </summary>
        void Toggle()
        {
            Checked = !Checked;
        }

        /// <inheritdoc />
        void Draw() override;

        /// <inheritdoc />
        bool ContainsPoint(Float2& location, bool precise = false) override;

        /// <inheritdoc />
        void OnMouseMove(Float2 location) override;

        /// <inheritdoc />
        bool OnMouseDown(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        bool OnMouseDoubleClick(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        bool OnMouseUp(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        void OnMouseLeave() override;

        /// <inheritdoc />
        bool OnTouchDown(Float2 location, int pointerId) override;

        /// <inheritdoc />
        bool OnTouchUp(Float2 location, int pointerId) override;

        /// <inheritdoc />
        void OnTouchLeave() override;

        /// <inheritdoc />
        void OnLostFocus() override;
        
        /// <inheritdoc />
        void OnSubmit() override;

    private:
        void CacheBox()
        {
            _box = Rectangle(0, (Height - _boxSize) * 0.5f, _boxSize, _boxSize);
        }

    protected:
        void OnSizeChanged() override
        {
            Control::OnSizeChanged();

            CacheBox();
        }

        /// <summary>
        /// Called when mouse or touch clicks the checkbox.
        /// </summary>
        virtual void OnClick()
        {
            Toggle();
        }

        /// <summary>
        /// Called when checkbox starts to be pressed by the used (via mouse or touch).
        /// </summary>
        virtual void OnPressBegin()
        {
            _isPressed = true;
            if (AutoFocus)
                Focus();
        }

        /// <summary>
        /// Called when checkbox ends to be pressed by the used (via mouse or touch).
        /// </summary>
        virtual void OnPressEnd()
        {
            _isPressed = false;
        }

    private:
        CheckBoxState __GetState() { return _state; }
        void __SetState(CheckBoxState value);
        bool __GetChecked() { return _state == CheckBoxState::Checked; }
        void __SetChecked(bool value);
        bool __GetIntermediate() { return _state == CheckBoxState::Intermediate; }
        void __SetIntermediate(bool value);
        float __GetBoxSizee() { return _boxSize; }
        void __SetBoxSize(float value);
    };
} // SE


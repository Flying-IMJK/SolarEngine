#pragma once

#include "Runtime/Graphics/GraphicWindow.h"
#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/Style.h"
#include "Runtime/UI/GUI/WindowRootControl.h"
#include "Runtime/UI/GUI/Common/TextBox.h"

namespace SE::Editor
{
    /// <summary>
    /// Base class for text boxes for float/int value editing. Supports slider and range clamping.
    /// </summary>
    /// <typeparam name="T">The value type.</typeparam>
    template <typename T>
    class ValueBox : public TextBox
    {
    protected:
        /// <summary>
        /// The sliding box size.
        /// </summary>
        constexpr static float SlidingBoxSize = 12.0f;

        /// <summary>
        /// The current value.
        /// </summary>
        T _value;

        /// <summary>
        /// The minimum value.
        /// </summary>
        T _min;

        /// <summary>
        /// The maximum value.
        /// </summary>
        T _max;

        /// <summary>
        /// The slider speed.
        /// </summary>
        float _slideSpeed;

        /// <summary>
        /// True if slider is in use.
        /// </summary>
        bool _isSliding;

        /// <summary>
        /// The value cached on sliding start.
        /// </summary>
        T _startSlideValue;

        /// <summary>
        /// The text cached on editing start. Used to compare with the end result to detect changes.
        /// </summary>
        String _startEditText;

    private:
        Float2 _startSlideLocation;
        double _clickStartTime = -1;
        bool _cursorChanged;
        Float2 _mouseClickedPosition;

    public:
        /// <summary>
        /// Occurs when value gets changed.
        /// </summary>
        Action ValueChanged;

        /// <summary>
        /// Occurs when value gets changed.
        /// </summary>
        Delegate<ValueBox*> BoxValueChanged;

        /// <summary>
        /// Gets or sets the value.
        /// </summary>
        virtual T GetValue() = 0;
        virtual void SetValue(T value) = 0;

        /// <summary>
        /// Gets or sets the minimum value.
        /// </summary>
        virtual T GetMinValue() = 0;
        virtual void SetMinValue(T value) = 0;

        /// <summary>
        /// Gets or sets the maximum value.
        /// </summary>
        virtual T GetMaxValue() = 0;
        virtual void SetMaxValue(T value) = 0;

        /// <summary>
        /// Gets a value indicating whether user is using a slider.
        /// </summary>
        PRO_GET(IsSliding, ValueBox, bool, __GetIsSliding);

        /// <summary>
        /// Occurs when sliding starts.
        /// </summary>
        Action SlidingStart;

        /// <summary>
        /// Occurs when sliding ends.
        /// </summary>
        Action SlidingEnd;

        /// <summary>
        /// Gets or sets the slider speed. Use value 0 to disable and hide slider UI.
        /// </summary>
        PRO(SlideSpeed, ValueBox, float, __GetSlideSpeed, __SetSlideSpeed);

        /// <inheritdoc />
        void Draw() override
        {
            TextBox::Draw();

            if (CanUseSliding)
            {
                Style* style = Style::Current;

                // Draw sliding UI
                Render2D::DrawSprite(style->Scalar, SlideRect, style->Foreground);

                // Check if is sliding
                if (_isSliding)
                {
                    // Draw overlay
                    Rectangle bounds = Rectangle(Float2::Zero, Size);
                    Render2D::FillRectangle(bounds, style->Selection);
                    Render2D::DrawRectangle(bounds, style->SelectionBorder);
                }
            }
        }

        /// <inheritdoc />
        void OnGetFocus() override
        {
            TextBox::OnGetFocus();

            SelectAll();
        }

        /// <inheritdoc />
        void OnLostFocus() override
        {
            // Check if was sliding
            if (_isSliding)
            {
                EndSliding();

                TextBox::OnLostFocus();
            }
            else
            {
                TextBox::OnLostFocus();

                // Update
                UpdateText();
            }

            Cursor = CursorType::Default;

            ResetViewOffset();
        }

        /// <inheritdoc />
        bool OnMouseDown(Float2 location, MouseButton button) override
        {
            if (button == MouseButton::Left && CanUseSliding && SlideRect.operator->().Contains(location))
            {
                // Start sliding
                _isSliding = true;
                _startSlideLocation = location;
                _startSlideValue = _value;
                StartMouseCapture(true);
                EndEditOnClick = false;

                // Hide cursor and cache location
                Cursor = CursorType::Hidden;
                _mouseClickedPosition = PointToWindow(location);
                _cursorChanged = true;

                SlidingStart();
                return true;
            }

            if (button == MouseButton::Left && !IsFocused)
                _clickStartTime = Platform::GetTimeSeconds();

            return TextBox::OnMouseDown(location, button);
        }

        /// <inheritdoc />
        void OnMouseMove(Float2 location) override
        {
            if (_isSliding && !RootWindow()->Window()->IsMouseFlippingHorizontally())
            {
                // Update sliding
                Float2 slideLocation = location + Root->GetTrackingMouseOffset();
                ApplySliding(Math::RoundToInt(slideLocation.x - _startSlideLocation.x) * _slideSpeed);
                return;
            }

            // Update cursor type so user knows they can slide value
            if (CanUseSliding && SlideRect.operator->().Contains(location) && !_isSliding)
            {
                Cursor = CursorType::SizeWE;
                _cursorChanged = true;
            }
            else if (_cursorChanged && !_isSliding)
            {
                Cursor = CursorType::Default;
                _cursorChanged = false;
            }

            TextBox::OnMouseMove(location);
        }

        /// <inheritdoc />
        bool OnMouseUp(Float2 location, MouseButton button) override
        {
            if (button == MouseButton::Left && _isSliding)
            {
                // End sliding and return mouse to original location
                RootWindow()->SetMousePosition(_mouseClickedPosition);
                EndSliding();
                return true;
            }

            if (button == MouseButton::Left && _clickStartTime > 0 && (Platform::GetTimeSeconds() - _clickStartTime) < 0.2f)
            {
                _clickStartTime = -1;
                OnSelectingEnd();
                SelectAll();
                return true;
            }

            return TextBox::OnMouseUp(location, button);
        }

        /// <inheritdoc />
        void OnMouseLeave() override
        {
            if (_cursorChanged)
            {
                Cursor = CursorType::Default;
                _cursorChanged = false;
            }

            TextBox::OnMouseLeave();
        }

        /// <inheritdoc />
        void OnEndMouseCapture() override
        {
            // Check if was sliding
            if (_isSliding)
            {
                EndSliding();
            }
            else
            {
                TextBox::OnEndMouseCapture();
            }
        }

    protected:
        /// <summary>
        /// Initializes a new instance of the <see cref="ValueBox{T}"/> class.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="x">The x.</param>
        /// <param name="y">The y.</param>
        /// <param name="width">The width.</param>
        /// <param name="min">The minimum.</param>
        /// <param name="max">The maximum.</param>
        /// <param name="sliderSpeed">The slider speed.</param>
        ValueBox(T value, float x, float y, float width, T min, T max, float sliderSpeed): TextBox(false, x, y, width)
        {
            _value = value;
            _min = min;
            _max = max;
            _slideSpeed = sliderSpeed;
        }

        /// <summary>
        /// Updates the text of the textbox.
        /// </summary>
        virtual void UpdateText() = 0;

        /// <summary>
        /// Tries the get value from the textbox text.
        /// </summary>
        virtual void TryGetValue() = 0;

        /// <summary>
        /// Applies the sliding delta to the value.
        /// </summary>
        /// <param name="delta">The delta (scaled).</param>
        virtual void ApplySliding(float delta) = 0;

        /// <summary>
        /// Called when value gets changed.
        /// </summary>
        virtual void OnValueChanged()
        {
            ValueChanged();
            BoxValueChanged(this);
        }

        /// <summary>
        /// Gets a value indicating whether this value box can use sliding.
        /// </summary>
        PRO_GET(CanUseSliding, ValueBox, bool, __GetCanUseSliding);

        /// <summary>
        /// Gets the slide rectangle.
        /// </summary>
        PRO_GET(SlideRect, ValueBox, Rectangle, __GetSlideRect);

        /// <inheritdoc />
        void OnEditBegin() override
        {
            TextBox::OnEditBegin();

            _startEditText = _text;
        }

        /// <inheritdoc />
        void OnEditEnd() override
        {
            if (_startEditText != _text)
            {
                // Update value
                TryGetValue();
            }
            _startEditText.Clear();

            TextBox::OnEditEnd();
        }

        /// <inheritdoc />
        Rectangle GetTextClipRectangle() override
        {
            Rectangle result = TextBox::TextRectangle;
            if (CanUseSliding)
            {
                result.Size.x -= SlidingBoxSize;
            }
            return result;
        }

        virtual Rectangle __GetSlideRect()
        {
            float x = Width - SlidingBoxSize - 1.0f;
            float y = (Height - SlidingBoxSize) * 0.5f;
            return Rectangle(x, y, SlidingBoxSize, SlidingBoxSize);
        }

        virtual bool __GetCanUseSliding() { return _slideSpeed > Math::EPSILON; }

        Rectangle __GetTextRectangle() override
        {
            Rectangle result = TextBox::__GetTextRectangle();
            if (CanUseSliding)
            {
                result.Size.x -= SlidingBoxSize;
            }
            return result;
        }

    private:

        void EndSliding()
        {
            _isSliding = false;
            EndEditOnClick = true;
            EndMouseCapture();
            if (_cursorChanged)
            {
                Cursor = CursorType::Default;
                _cursorChanged = false;
            }
            SlidingEnd();
            Defocus();
            if (Parent!= nullptr)
            {
                Parent->Focus();
            }
        }

        float __GetSlideSpeed()
        {
            return _slideSpeed;
        }
        void __SetSlideSpeed(float value)
        {
            _slideSpeed = value;
        }

        bool __GetIsSliding() { return _isSliding; }
    };

} // SE

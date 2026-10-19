#nullable disable annotations

using System;
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Base class for text boxes for float/int value editing. Supports slider and range clamping.
    /// </summary>
    /// <typeparam name="T">The value type.</typeparam>
    public abstract class ValueBox<T> : TextBox
    {
        /// <summary>
        /// The sliding box size.
        /// </summary>
        protected const float SlidingBoxSize = 12.0f;

        /// <summary>
        /// The current value.
        /// </summary>
        protected T _value;

        /// <summary>
        /// The minimum value.
        /// </summary>
        protected T _min;

        /// <summary>
        /// The maximum value.
        /// </summary>
        protected T _max;

        /// <summary>
        /// The slider speed.
        /// </summary>
        protected float _slideSpeed;

        /// <summary>
        /// True if slider is in use.
        /// </summary>
        protected bool _isSliding;

        /// <summary>
        /// The value cached on sliding start.
        /// </summary>
        protected T _startSlideValue;

        /// <summary>
        /// The text cached on editing start. Used to compare with the end result to detect changes.
        /// </summary>
        protected string _startEditText = string.Empty;

        private Float2 m_StartSlideLocation;
        private double m_ClickStartTime = -1;
        private bool m_CursorChanged;
        private Float2 m_MouseClickedPosition;

        /// <summary>
        /// Occurs when value gets changed.
        /// </summary>
        public Action ValueChanged;

        /// <summary>
        /// Occurs when value gets changed.
        /// </summary>
        public Action<ValueBox<T>> BoxValueChanged;

        /// <summary>
        /// Gets or sets the value.
        /// </summary>
        public abstract T GetValue();

        public abstract void SetValue(T value);

        /// <summary>
        /// Gets or sets the minimum value.
        /// </summary>
        public abstract T GetMinValue();

        public abstract void SetMinValue(T value);

        /// <summary>
        /// Gets or sets the maximum value.
        /// </summary>
        public abstract T GetMaxValue();

        public abstract void SetMaxValue(T value);

        /// <summary>
        /// Gets a value indicating whether user is using a slider.
        /// </summary>
        public bool IsSliding => _isSliding;

        /// <summary>
        /// Occurs when sliding starts.
        /// </summary>
        public Action SlidingStart;

        /// <summary>
        /// Occurs when sliding ends.
        /// </summary>
        public Action SlidingEnd;

        /// <summary>
        /// Gets or sets the slider speed. Use value 0 to disable and hide slider UI.
        /// </summary>
        public float SlideSpeed
        {
            get => _slideSpeed;
            set => _slideSpeed = value;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            if (CanUseSliding)
            {
                Style style = Style.Current;

                // Draw sliding UI
                Render2D.DrawSprite(style.Scalar, SlideRect, style.Foreground);

                // Check if is sliding
                if (_isSliding)
                {
                    // Draw overlay
                    Rectangle bounds = new Rectangle(Float2.Zero, Size);
                    Render2D.FillRectangle(bounds, style.Selection);
                    Render2D.DrawRectangle(bounds, style.SelectionBorder);
                }
            }
        }

        /// <inheritdoc />
        public override void OnGetFocus()
        {
            base.OnGetFocus();
            SelectAll();
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            // Check if was sliding
            if (_isSliding)
            {
                EndSliding();
                base.OnLostFocus();
            }
            else
            {
                base.OnLostFocus();

                // Update
                UpdateText();
            }

            Cursor = CursorType.Default;
            ResetViewOffset();
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && CanUseSliding && SlideRect.Contains(location))
            {
                // Start sliding
                _isSliding = true;
                m_StartSlideLocation = location;
                _startSlideValue = _value;
                StartMouseCapture(true);
                EndEditOnClick = false;

                // Hide cursor and cache location
                Cursor = CursorType.Hidden;
                m_MouseClickedPosition = PointToWindow(location);
                m_CursorChanged = true;

                SlidingStart?.Invoke();
                return true;
            }

            if (button == MouseButton.Left && !IsFocused)
            {
                m_ClickStartTime = SE.Platform.TimeSeconds;
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            if (_isSliding && !RootWindow().Window.IsMouseFlippingHorizontally)
            {
                // Update sliding
                Float2 slideLocation = location + Root.GetTrackingMouseOffset();
                ApplySliding(Mathf.RoundToInt(slideLocation.X - m_StartSlideLocation.X) * _slideSpeed);
                return;
            }

            // Update cursor type so user knows they can slide value
            if (CanUseSliding && SlideRect.Contains(location) && !_isSliding)
            {
                Cursor = CursorType.SizeWE;
                m_CursorChanged = true;
            }
            else if (m_CursorChanged && !_isSliding)
            {
                Cursor = CursorType.Default;
                m_CursorChanged = false;
            }

            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && _isSliding)
            {
                // End sliding and return mouse to original location
                RootWindow().SetMousePosition(m_MouseClickedPosition);
                EndSliding();
                return true;
            }

            if (button == MouseButton.Left && m_ClickStartTime > 0 &&
                (SE.Platform.TimeSeconds - m_ClickStartTime) < 0.2f)
            {
                m_ClickStartTime = -1;
                OnSelectingEnd();
                SelectAll();
                return true;
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            if (m_CursorChanged)
            {
                Cursor = CursorType.Default;
                m_CursorChanged = false;
            }

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override void OnEndMouseCapture()
        {
            // Check if was sliding
            if (_isSliding)
            {
                EndSliding();
            }
            else
            {
                base.OnEndMouseCapture();
            }
        }

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
        protected ValueBox(T value, float x, float y, float width, T min, T max, float sliderSpeed)
            : base(false, x, y, width)
        {
            _value = value;
            _min = min;
            _max = max;
            _slideSpeed = sliderSpeed;
        }

        /// <summary>
        /// Updates the text of the textbox.
        /// </summary>
        protected abstract void UpdateText();

        /// <summary>
        /// Tries the get value from the textbox text.
        /// </summary>
        protected abstract void TryGetValue();

        /// <summary>
        /// Applies the sliding delta to the value.
        /// </summary>
        /// <param name="delta">The delta (scaled).</param>
        protected abstract void ApplySliding(float delta);

        /// <summary>
        /// Called when value gets changed.
        /// </summary>
        protected virtual void OnValueChanged()
        {
            ValueChanged?.Invoke();
            BoxValueChanged?.Invoke(this);
        }

        /// <summary>
        /// Gets a value indicating whether this value box can use sliding.
        /// </summary>
        protected virtual bool CanUseSliding => _slideSpeed > Mathf.Epsilon;

        /// <summary>
        /// Gets the slide rectangle.
        /// </summary>
        protected virtual Rectangle SlideRect
        {
            get
            {
                float x = Width - SlidingBoxSize - 1.0f;
                float y = (Height - SlidingBoxSize) * 0.5f;
                return new Rectangle(x, y, SlidingBoxSize, SlidingBoxSize);
            }
        }

        /// <inheritdoc />
        protected override void OnEditBegin()
        {
            base.OnEditBegin();
            _startEditText = _text;
        }

        /// <inheritdoc />
        protected override void OnEditEnd()
        {
            if (_startEditText != _text)
            {
                // Update value
                TryGetValue();
            }
            _startEditText = string.Empty;

            base.OnEditEnd();
        }

        /// <inheritdoc />
        protected override Rectangle GetTextClipRectangle()
        {
            Rectangle result = base.TextRectangle;
            if (CanUseSliding)
            {
                result.Width -= SlidingBoxSize;
            }
            return result;
        }

        protected override Rectangle TextRectangle
        {
            get
            {
                Rectangle result = base.TextRectangle;
                if (CanUseSliding)
                {
                    result.Width -= SlidingBoxSize;
                }
                return result;
            }
        }

        private void EndSliding()
        {
            _isSliding = false;
            EndEditOnClick = true;
            EndMouseCapture();
            if (m_CursorChanged)
            {
                Cursor = CursorType.Default;
                m_CursorChanged = false;
            }
            SlidingEnd?.Invoke();
            Defocus();
            if (Parent != null)
            {
                Parent.Focus();
            }
        }
    }
}

// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Globalization;
using SE;
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Float value editor with fixed size text box and slider.
    /// </summary>
    [HideInEditor]
    public class SliderControl : ContainerControl
    {
        /// <summary>
        /// The horizontal slider control.
        /// </summary>
        /// <seealso cref="FlaxEngine.GUI.Control" />
        [HideInEditor]
        protected class Slider : Control
        {
            /// <summary>
            /// The default size.
            /// </summary>
            public const int DefaultSize = 16;

            /// <summary>
            /// The default thickness.
            /// </summary>
            public const int DefaultThickness = 6;

            /// <summary>
            /// The minimum value (constant)
            /// </summary>
            public const float Minimum = 0.0f;

            /// <summary>
            /// The maximum value (constant).
            /// </summary>
            public const float Maximum = 100.0f;

            private float m_Value;
            private Rectangle m_ThumbRect;
            private float m_ThumbCenter, m_ThumbSize;
            private bool m_IsSliding;

            /// <summary>
            /// Gets or sets the value (normalized to range 0-100).
            /// </summary>
            public float Value
            {
                get => m_Value;
                set
                {
                    value = Mathf.Clamp(value, Minimum, Maximum);
                    if (value != m_Value)
                    {
                        m_Value = value;

                        // Update
                        UpdateThumb();
                        ValueChanged?.Invoke();
                    }
                }
            }

            /// <summary>
            /// Occurs when value gets changed.
            /// </summary>
            public Action ValueChanged;

            /// <summary>
            /// The color of the slider track line.
            /// </summary>
            public Color TrackLineColor { get; set; }

            /// <summary>
            /// The color of the slider thumb when it's not selected.
            /// </summary>
            public Color ThumbColor { get; set; }

            /// <summary>
            /// The color of the slider thumb when it's selected.
            /// </summary>
            public Color ThumbColorSelected { get; set; }

            /// <summary>
            /// The color of the slider thumb when it's hovered.
            /// </summary>
            public Color ThumbColorHovered { get; set; }

            /// <summary>
            /// Gets a value indicating whether user is using a slider.
            /// </summary>
            public bool IsSliding => m_IsSliding;

            /// <summary>
            /// Occurs when sliding starts.
            /// </summary>
            public Action SlidingStart;

            /// <summary>
            /// Occurs when sliding ends.
            /// </summary>
            public Action SlidingEnd;

            /// <summary>
            /// Initializes a new instance of the <see cref="Slider"/> class.
            /// </summary>
            /// <param name="width">The width.</param>
            /// <param name="height">The height.</param>
            public Slider(float width, float height)
            : base(0, 0, width, height)
            {
                Style style = Style.Current;
                TrackLineColor = style.BackgroundHighlighted;
                ThumbColor = style.BackgroundNormal;
                ThumbColorSelected = style.BackgroundSelected;
                ThumbColorHovered = style.BackgroundHighlighted;
            }

            private void UpdateThumb()
            {
                // Cache data
                float trackSize = TrackSize;
                float range = Maximum - Minimum;
                m_ThumbSize = Mathf.Min(trackSize, Mathf.Max(trackSize / range * 10.0f, 30.0f));
                float pixelRange = trackSize - m_ThumbSize;
                float perc = (m_Value - Minimum) / range;
                float thumbPosition = (int)(perc * pixelRange);
                m_ThumbCenter = thumbPosition + m_ThumbSize / 2;
                m_ThumbRect = new Rectangle(thumbPosition + 4, (Height - DefaultThickness) / 2, m_ThumbSize - 8,
                    DefaultThickness);
            }

            private void EndSliding()
            {
                m_IsSliding = false;
                EndMouseCapture();
                SlidingEnd?.Invoke();
                Defocus();
                Parent?.Focus();
            }

            /// <summary>
            /// Gets the size of the track.
            /// </summary>
            private float TrackSize => Width;

            /// <inheritdoc />
            public override void Draw()
            {
                base.Draw();

                // Draw track line
                Rectangle lineRect = new Rectangle(4, Height / 2, Width - 8, 1);
                Render2D.FillRectangle(lineRect, TrackLineColor);

                // Draw thumb
                bool mouseOverThumb = m_ThumbRect.Contains(PointFromWindow(Root.GetMousePosition()));
                Render2D.FillRectangle(m_ThumbRect, m_IsSliding ? ThumbColorSelected : mouseOverThumb
                    ? ThumbColorHovered : ThumbColor);
            }

            /// <inheritdoc />
            public override void OnLostFocus()
            {
                if (m_IsSliding)
                {
                    EndSliding();
                }

                base.OnLostFocus();
            }

            /// <inheritdoc />
            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left)
                {
                    Focus();
                    float mousePosition = location.X;

                    if (m_ThumbRect.Contains(ref location))
                    {
                        // Start sliding
                        m_IsSliding = true;
                        StartMouseCapture();
                        SlidingStart?.Invoke();
                        return true;
                    }
                    else
                    {
                        // Click change
                        Value += (mousePosition < m_ThumbCenter ? -1 : 1) * 10;
                        Defocus();
                        Parent?.Focus();
                    }
                }

                return base.OnMouseDown(location, button);
            }

            /// <inheritdoc />
            public override void OnMouseMove(Float2 location)
            {
                if (m_IsSliding)
                {
                    // Update sliding
                    Float2 slidePosition = location + Root.GetTrackingMouseOffset();
                    Value = Mathf.Remap(slidePosition.X, 4, TrackSize - 4, Minimum, Maximum);
                    if (Mathf.NearEqual(Value, Maximum))
                    {
                        Value = Maximum;
                    }
                    else if (Mathf.NearEqual(Value, Minimum))
                    {
                        Value = Minimum;
                    }
                }
                else
                {
                    base.OnMouseMove(location);
                }
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left && m_IsSliding)
                {
                    EndSliding();
                    return true;
                }

                return base.OnMouseUp(location, button);
            }

            /// <inheritdoc />
            public override void OnEndMouseCapture()
            {
                // Check if was sliding
                if (m_IsSliding)
                {
                    EndSliding();
                }
                else
                {
                    base.OnEndMouseCapture();
                }
            }

            /// <inheritdoc />
            protected override void OnSizeChanged()
            {
                base.OnSizeChanged();

                UpdateThumb();
            }
        }

        /// <summary>
        /// The slider.
        /// </summary>
        protected Slider _slider;

        /// <summary>
        /// The text box.
        /// </summary>
        protected TextBox _textBox;

        /// <summary>
        /// The text box size (rest will be the slider area).
        /// </summary>
        protected const float TextBoxSize = 30.0f;

        private float m_Value;
        private float m_Min, m_Max;

        private bool m_ValueIsChanging;

        /// <summary>
        /// Occurs when value gets changed.
        /// </summary>
        public event Action ValueChanged;

        /// <summary>
        /// Gets or sets the value.
        /// </summary>
        public float Value
        {
            get => m_Value;
            set
            {
                value = Mathf.Clamp(value, m_Min, m_Max);
                if (Math.Abs(m_Value - value) > Mathf.Epsilon)
                {
                    // Set value
                    m_Value = value;

                    // Update
                    m_ValueIsChanging = true;
                    UpdateText();
                    UpdateSlider();
                    m_ValueIsChanging = false;
                    OnValueChanged();
                }
            }
        }

        /// <summary>
        /// Gets or sets the minimum value.
        /// </summary>
        public float MinValue
        {
            get => m_Min;
            set
            {
                if (m_Min != value)
                {
                    if (value > m_Max)
                    {
                        throw new ArgumentException();
                    }

                    m_Min = value;
                    Value = Value;
                }
            }
        }

        /// <summary>
        /// Gets or sets the maximum value.
        /// </summary>
        public float MaxValue
        {
            get => m_Max;
            set
            {
                if (m_Max != value)
                {
                    if (value < m_Min)
                    {
                        throw new ArgumentException();
                    }

                    m_Max = value;
                    Value = Value;
                }
            }
        }

        /// <summary>
        /// Gets a value indicating whether user is using a slider.
        /// </summary>
        public bool IsSliding => _slider.IsSliding;

        /// <summary>
        /// Occurs when sliding starts.
        /// </summary>
        public event Action SlidingStart;

        /// <summary>
        /// Occurs when sliding ends.
        /// </summary>
        public event Action SlidingEnd;

        /// <summary>
        /// Initializes a new instance of the <see cref="SliderControl"/> class.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="x">The position x.</param>
        /// <param name="y">The position y.</param>
        /// <param name="width">The width.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        public SliderControl(float value, float x = 0, float y = 0, float width = 120, float min = float.MinValue,
            float max = float.MaxValue)
        : base(x, y, width, TextBox.DefaultHeight)
        {
            AutoFocus = true;

            m_Min = min;
            m_Max = max;
            m_Value = Mathf.Clamp(value, min, max);

            float split = Width - TextBoxSize;
            _slider = new Slider(split, Height)
            {
                Parent = this,
            };
            _slider.ValueChanged += SliderOnValueChanged;
            _slider.SlidingStart += SlidingStart;
            _slider.SlidingEnd += SliderOnSliderEnd;
            _textBox = new TextBox(false, split, 0)
            {
                Text = m_Value.ToString(CultureInfo.InvariantCulture),
                Parent = this,
                Location = new Float2(split, 0),
                Size = new Float2(Height, TextBoxSize),
            };
            _textBox.EditEnd += OnTextBoxEditEnd;
        }

        private void SliderOnSliderEnd()
        {
            SlidingEnd?.Invoke();
            Defocus();
            Parent?.Focus();
        }

        private void SliderOnValueChanged()
        {
            if (m_ValueIsChanging)
            {
                return;
            }

            Value = Mathf.Remap(_slider.Value, Slider.Minimum, Slider.Maximum, MinValue, MaxValue);
        }

        private void OnTextBoxEditEnd()
        {
            if (m_ValueIsChanging)
            {
                return;
            }

            string text = _textBox.Text.Replace(',', '.');
            if (double.TryParse(text, NumberStyles.Float | NumberStyles.AllowThousands, CultureInfo
                .InvariantCulture, out double value))
            {
                Value = (float)Math.Round(value, 5);
            }
            else
            {
                UpdateText();
            }
            Defocus();
            Parent?.Focus();
        }

        /// <summary>
        /// Sets the limits from the attribute.
        /// </summary>
        /// <param name="limits">The limits.</param>
        public void SetLimits(RangeAttribute limits)
        {
            m_Min = limits.Min;
            m_Max = Mathf.Max(m_Min, limits.Max);
            Value = Value;
        }

        /// <summary>
        /// Sets the limits from the attribute.
        /// </summary>
        /// <param name="limits">The limits.</param>
        public void SetLimits(LimitAttribute limits)
        {
            m_Min = limits.Min;
            m_Max = Mathf.Max(m_Min, limits.Max);
            Value = Value;
        }

        /// <summary>
        /// Updates the text of the textbox.
        /// </summary>
        protected virtual void UpdateText()
        {
            _textBox.Text = m_Value.ToString(CultureInfo.InvariantCulture);
        }

        /// <summary>
        /// Updates the slider value.
        /// </summary>
        protected virtual void UpdateSlider()
        {
            _slider.Value = Mathf.Remap(m_Value, MinValue, MaxValue, Slider.Minimum, Slider.Maximum);
        }

        /// <summary>
        /// Called when value gets changed.
        /// </summary>
        protected virtual void OnValueChanged()
        {
            ValueChanged?.Invoke();
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();

            float split = Width - TextBoxSize;
            _slider.Bounds = new Rectangle(0, 0, split, Height);
            _textBox.Bounds = new Rectangle(split, 0, TextBoxSize, Height);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _slider = null;
            _textBox = null;

            base.OnDestroy();
        }
    }
}

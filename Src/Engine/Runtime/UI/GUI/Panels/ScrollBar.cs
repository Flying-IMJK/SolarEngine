#nullable disable annotations

using System;

namespace SE.GUI
{
    public abstract class ScrollBar : Control
    {
        // Scrolling
        private float m_ClickChange = 20.0f, m_ScrollChange = 50.0f;
        private float m_Minimum, m_Maximum = 100.0f;
        private float m_StartValue, m_Value, m_TargetValue;
        private Orientation m_Orientation;
        private Action<float> m_Update;
        private Action<float> m_OnUpdate;

        // Input
        private float m_MouseOffset;

        // Thumb data
        private Rectangle m_ThumbRect, m_TrackRect;
        private bool m_ThumbClicked;
        private float m_ThumbCenter, m_ThumbSize;

        // Smoothing
        private float m_ThumbOpacity = DefaultMinimumOpacity;
        private float m_ScrollAnimationProgress = 0.0f;

        /// <summary>
        /// The default minimum opacity.
        /// </summary>
        public const float DefaultMinimumOpacity = 0.75f;

        /// <summary>
        /// Gets the orientation.
        /// </summary>
        public Orientation GetOrientation()
        {
            return m_Orientation;
        }

        /// <summary>
        /// Gets or sets the thumb box thickness.
        /// </summary>
        public float ThumbThickness = 8.0f;

        /// <summary>
        /// Gets or sets the track line thickness.
        /// </summary>
        public float TrackThickness = 2.0f;

        /// <summary>
        /// The maximum time it takes to animate from current to target scroll position
        /// </summary>
        public float ScrollAnimationDuration = 0.18f;

        /// <summary>
        /// Gets a value indicating whether use scroll value smoothing.
        /// </summary>
        public bool UseSmoothing()
        {
            return EnableSmoothing && !Mathf.IsZero(ScrollAnimationDuration);
        }

        /// <summary>
        /// Enables scroll smoothing
        /// </summary>
        public bool EnableSmoothing = true;

        /// <summary>
        /// The track color.
        /// </summary>
        public Color TrackColor;

        /// <summary>
        /// The thumb color.
        /// </summary>
        public Color ThumbColor;

        /// <summary>
        /// The selected thumb color.
        /// </summary>
        public Color ThumbSelectedColor;

        /// <summary>
        /// Gets or sets the minimum value.
        /// </summary>
        public float GetMinimum()
        {
            return m_Minimum;
        }

        public void SetMinimum(float value)
        {
            if (value > m_Maximum)
            {
                value = m_Maximum;
            }

            m_Minimum = value;
            if (value < m_Minimum)
            {
                value = m_Minimum;
            }
        }

        /// <summary>
        /// Gets or sets the maximum value.
        /// </summary>
        public float GetMaximum()
        {
            return m_Maximum;
        }

        public void SetMaximum(float value)
        {
            if (value < m_Minimum)
            {
                value = m_Minimum;
            }

            m_Maximum = value;
            if (value > m_Maximum)
            {
                value = m_Maximum;
            }
        }

        // Compatibility for LogWindow, which is excluded from the migration.
        public float Maximum
        {
            get => GetMaximum();
            set => SetMaximum(value);
        }

        /// <summary>
        /// Gets or sets the scroll value (current, smooth).
        /// </summary>
        public float GetValue()
        {
            return m_Value;
        }

        public void SetValue(float value)
        {
            value = ClampLikeNative(value, m_Minimum, m_Maximum);
            if (!Mathf.WithinEpsilon((double)value, m_TargetValue, Mathf.Epsilon))
            {
                m_TargetValue = value;
                m_StartValue = m_Value;
                m_ScrollAnimationProgress = 0.0f;

                // Check if skip smoothing
                if (!UseSmoothing())
                {
                    m_Value = value;
                    m_StartValue = value;
                    m_ScrollAnimationProgress = 1.0f;
                    OnValueChanged();
                }
                else
                {
                    SetUpdate(m_Update, m_OnUpdate);
                }
            }
        }

        /// <summary>
        /// Gets or sets the target value (target, not smooth).
        /// </summary>
        public float GetTargetValue()
        {
            return m_TargetValue;
        }

        public void SetTargetValue(float value)
        {
            value = ClampLikeNative(value, m_Minimum, m_Maximum);
            if (!Mathf.WithinEpsilon((double)value, m_TargetValue, Mathf.Epsilon))
            {
                m_TargetValue = value;
                m_Value = value;
                SetUpdate(m_Update, null);
                OnValueChanged();
            }
        }

        /// <summary>
        /// Gets or sets the speed for the scroll on mouse wheel.
        /// </summary>
        public float ScrollSpeedWheel;

        /// <summary>
        /// Gets or sets the speed for the scroll on mouse click.
        /// </summary>
        public float ScrollSpeedClick;

        /// <summary>
        /// Gets the value slow down.
        /// </summary>
        public float ValueSlowDown()
        {
            return m_TargetValue - m_Value;
        }

        /// <summary>
        /// Gets a value indicating whether thumb is being clicked (scroll bar is in use).
        /// </summary>
        public bool IsThumbClicked()
        {
            return m_ThumbClicked;
        }

        /// <summary>
        /// Occurs when value gets changed.
        /// </summary>
        public Action ValueChanged;

        /// <summary>
        /// Enables/disabled scrolling by user.
        /// </summary>
        public bool ThumbEnabled = true;

        /// <summary>
        /// Cuts the scroll bar value smoothing and imminently goes to the target scroll value.
        /// </summary>
        public void FastScroll()
        {
            if (!Mathf.WithinEpsilon((double)m_Value, m_TargetValue, Mathf.Epsilon))
            {
                m_Value = m_TargetValue = m_StartValue;
                m_ScrollAnimationProgress = 0.0f;
                SetUpdate(m_Update, null);
                OnValueChanged();
            }
        }

        /// <summary>
        /// Scrolls the view to the desire range (favors minimum value if cannot cover whole range in a bounds).
        /// </summary>
        /// <param name="min">The view minimum.</param>
        /// <param name="max">The view maximum.</param>
        /// <param name="fastScroll">True of scroll to the item quickly without smoothing.</param>
        public void ScrollViewTo(float min, float max, bool fastScroll = false)
        {
            // Check if we need to change view
            float viewMin = m_Value;
            float viewSize = TrackSize();
            float viewMax = viewMin + viewSize;
            if (viewMin > viewMax || min < viewMin || min > viewMax)
            {
                if (fastScroll)
                {
                    SetTargetValue(min);
                }
                else
                {
                    SetValue(min);
                }
            }
            /*else if (Mathf.IsNotInRange(max, viewMin, viewMax))
            {
                Value = max - viewSize;
            }*/
        }

        /// <summary>
        /// Sets the scroll range (min and max at once).
        /// </summary>
        /// <param name="minimum">The minimum scroll range value (see <see cref="Minimum"/>).</param>
        /// <param name="maximum">The maximum scroll range value (see <see cref="Minimum"/>).</param>
        public void SetScrollRange(float minimum, float maximum)
        {
            SE.Debug.Assert(minimum <= maximum, "ArgumentOutOfRange");

            m_Minimum = minimum;
            m_Maximum = maximum;

            if (m_Value < minimum)
            {
                SetValue(minimum);
            }
            else if (m_Value > maximum)
            {
                m_Value = maximum;
            }

            UpdateThumb();
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // var style = Style.Current;
            Render2D.FillRectangle(m_TrackRect, TrackColor * m_ThumbOpacity);
            Render2D.FillRectangle(m_ThumbRect, (m_ThumbClicked ? ThumbSelectedColor : ThumbColor) * m_ThumbOpacity);
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            EndTracking();

            base.OnLostFocus();
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            if (m_ThumbClicked)
            {
                Float2 slidePosition = location + Root.GetTrackingMouseOffset();
                if (Parent is ScrollableControl panel)
                {
                    slidePosition += panel.GetViewOffset(); // Hardcoded fix
                }

                float mousePosition = m_Orientation == Orientation.Vertical ? slidePosition.Y : slidePosition.X;
                float percentage = (mousePosition - m_MouseOffset - m_ThumbSize / 2) / (TrackSize() - m_ThumbSize);
                SetTargetValue(m_Minimum + percentage * (m_Maximum - m_Minimum));
            }
        }

        /// <inheritdoc />
        public override bool OnMouseWheel(Float2 location, float delta)
        {
            if (ThumbEnabled)
            {
                // Scroll
                SetValue(m_TargetValue - delta * m_ScrollChange);
            }

            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && ThumbEnabled)
            {
                // Remove focus
                RootControl root = Root;
                if (root.GetFocusedControl() != null)
                {
                    root.GetFocusedControl().Defocus();
                }

                float mousePosition = m_Orientation == Orientation.Vertical ? location.Y : location.X;

                if (m_ThumbRect.Contains(location))
                {
                    // Start moving thumb
                    m_ThumbClicked = true;
                    m_MouseOffset = mousePosition - m_ThumbCenter;

                    // Start capturing mouse
                    StartMouseCapture();
                }
                else
                {
                    // Click change
                    SetValue(m_Value + (mousePosition < m_ThumbCenter ? -1 : 1) * m_ClickChange);
                }
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            EndTracking();

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override void OnEndMouseCapture()
        {
            EndTracking();
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            base.OnMouseEnter(location);

            SetUpdate(m_Update, m_OnUpdate);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            base.OnMouseLeave();

            SetUpdate(m_Update, m_OnUpdate);
        }

        public void Reset()
        {
            m_Value = m_TargetValue = m_StartValue = 0.0f;
            m_ScrollAnimationProgress = 0.0f;
        }

        private void UpdateThumb()
        {
            // Cache data
            float width = Width;
            float height = Height;
            float trackSize = TrackSize();
            float range = m_Maximum - m_Minimum;
            float minimumThumbSize = trackSize / range * 100.0f;
            float maximumThumbSize = minimumThumbSize < 50.0f ? 50.0f : minimumThumbSize;
            float remainingTrackSize = trackSize - 10.0f;
            m_ThumbSize = maximumThumbSize < remainingTrackSize ? maximumThumbSize : remainingTrackSize;
            float pixelRange = trackSize - m_ThumbSize;
            float percentage = (m_Value - m_Minimum) / range;
            float thumbPosition = (int)(percentage * pixelRange);
            m_ThumbCenter = thumbPosition + m_ThumbSize / 2;
            m_ThumbRect = m_Orientation == Orientation.Vertical
                ? new Rectangle((width - ThumbThickness) / 2, thumbPosition + 4, ThumbThickness, m_ThumbSize - 8)
                : new Rectangle(thumbPosition + 4, (height - ThumbThickness) / 2, m_ThumbSize - 8, ThumbThickness);
            m_TrackRect = m_Orientation == Orientation.Vertical
                ? new Rectangle((width - TrackThickness) / 2, 4, TrackThickness, height - 8)
                : new Rectangle(4, (height - TrackThickness) / 2, width - 8, TrackThickness);
        }

        private void EndTracking()
        {
            // Check flag
            if (m_ThumbClicked)
            {
                // Clear flag
                m_ThumbClicked = false;

                // End capturing mouse
                EndMouseCapture();
            }
        }

        private void OnUpdate(float deltaTime)
        {
            bool isDeltaSlow = deltaTime > (1 / 20.0f);

            // Opacity smoothing
            float targetOpacity = IsMouseOver ? 1.0f : DefaultMinimumOpacity;
            m_ThumbOpacity = isDeltaSlow ? targetOpacity : Mathf.Lerp(m_ThumbOpacity, targetOpacity, deltaTime * 10.0f);
            bool needUpdate = MathF.Abs(m_ThumbOpacity - targetOpacity) > 0.001f;

            // Ensure scroll bar is visible and smoothing is required
            if (Visible && MathF.Abs(m_TargetValue - m_Value) > 0.01f)
            {
                // Interpolate or not if running slow
                float value;
                if (!isDeltaSlow && UseSmoothing())
                {
                    // percentage of scroll from 0 to _scrollChange, ex. 0.5 at _scrollChange / 2
                    float minScrollChangeRatio = ClampLikeNative(MathF.Abs(m_TargetValue - m_StartValue)
                        / m_ScrollChange, 0.0f, 1.0f);

                    // shorten the duration if we scrolled less than _scrollChange
                    float actualDuration = ScrollAnimationDuration * minScrollChangeRatio;
                    float step = deltaTime / actualDuration;

                    float progress = m_ScrollAnimationProgress;
                    progress = ClampLikeNative(progress + step, 0.0f, 1.0f);

                    // https://easings.net/#easeOutSine
                    float easedProgress = MathF.Sin((progress * MathF.PI) / 2);
                    value = Mathf.Lerp(m_StartValue, m_TargetValue, easedProgress);

                    m_ScrollAnimationProgress = progress;
                }
                else
                {
                    value = m_TargetValue;
                    m_StartValue = m_TargetValue;
                    m_ScrollAnimationProgress = 0.0f;
                }

                m_Value = value;
                OnValueChanged();
                needUpdate = true;
            }

            // End updating if all animations are done
            if (!needUpdate)
            {
                SetUpdate(m_Update, null);
            }
        }

        private static float ClampLikeNative(float value, float lowerBound, float upperBound)
        {
            if (lowerBound > upperBound)
            {
                return value;
            }

            float maximum = value < lowerBound ? lowerBound : value;
            return upperBound < maximum ? upperBound : maximum;
        }

        /// <summary>
        /// Called when value gets changed.
        /// </summary>
        protected virtual void OnValueChanged()
        {
            UpdateThumb();

            ValueChanged?.Invoke();
        }

        /// <inheritdoc />
        protected override void OnSizeChanged()
        {
            base.OnSizeChanged();

            UpdateThumb();
        }

        /// <summary>
        /// Gets the size of the track.
        /// </summary>
        protected abstract float TrackSize();

        protected ScrollBar()
        {
            // Standalone scrollbars use the same palette; Panel may override it.
            Style style = Style.Current;
            TrackColor = style.BackgroundHighlighted;
            ThumbColor = style.BackgroundNormal;
            ThumbSelectedColor = style.BackgroundSelected;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ScrollBar"/> class.
        /// </summary>
        /// <param name="orientation">The orientation.</param>
        protected ScrollBar(Orientation orientation) : this()
        {
            m_OnUpdate = OnUpdate;

            AutoFocus = false;

            m_Orientation = orientation;
        }

        /// <inheritdoc />
        protected override void AddUpdateCallbacks(RootControl root)
        {
            base.AddUpdateCallbacks(root);

            if (m_Update != null)
            {
                root.UpdateCallbacksToAdd.Add(m_Update);
            }
        }

        /// <inheritdoc />
        protected override void RemoveUpdateCallbacks(RootControl root)
        {
            base.RemoveUpdateCallbacks(root);

            if (m_Update != null)
            {
                root.UpdateCallbacksToRemove.Add(m_Update);
            }
        }
    }
}

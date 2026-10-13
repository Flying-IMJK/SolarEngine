#nullable disable annotations

using System;

namespace SE.GUI
{
    public abstract class ScrollBar : Control
    {
        // Scrolling
        private float _clickChange = 20.0f, _scrollChange = 50.0f;
        private float _minimum, _maximum = 100.0f;
        private float _startValue, _value, _targetValue;
        private Orientation _orientation;
        private Action<float> _update;
        private Action<float> m_OnUpdate;

        // Input
        private float _mouseOffset;

        // Thumb data
        private Rectangle _thumbRect, _trackRect;
        private bool _thumbClicked;
        private float _thumbCenter, _thumbSize;

        // Smoothing
        private float _thumbOpacity = DefaultMinimumOpacity;
        private float _scrollAnimationProgress = 0.0f;

        /// <summary>
        /// The default minimum opacity.
        /// </summary>
        public const float DefaultMinimumOpacity = 0.75f;

        /// <summary>
        /// Gets the orientation.
        /// </summary>
        public Orientation GetOrientation()
        {
            return _orientation;
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
            return _minimum;
        }

        public void SetMinimum(float value)
        {
            if (value > _maximum)
            {
                value = _maximum;
            }

            _minimum = value;
            if (value < _minimum)
            {
                value = _minimum;
            }
        }

        /// <summary>
        /// Gets or sets the maximum value.
        /// </summary>
        public float GetMaximum()
        {
            return _maximum;
        }

        public void SetMaximum(float value)
        {
            if (value < _minimum)
            {
                value = _minimum;
            }

            _maximum = value;
            if (value > _maximum)
            {
                value = _maximum;
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
            return _value;
        }

        public void SetValue(float value)
        {
            value = ClampLikeNative(value, _minimum, _maximum);
            if (!Mathf.WithinEpsilon((double)value, _targetValue, Mathf.Epsilon))
            {
                _targetValue = value;
                _startValue = _value;
                _scrollAnimationProgress = 0.0f;

                // Check if skip smoothing
                if (!UseSmoothing())
                {
                    _value = value;
                    _startValue = value;
                    _scrollAnimationProgress = 1.0f;
                    OnValueChanged();
                }
                else
                {
                    SetUpdate(_update, m_OnUpdate);
                }
            }
        }

        /// <summary>
        /// Gets or sets the target value (target, not smooth).
        /// </summary>
        public float GetTargetValue()
        {
            return _targetValue;
        }

        public void SetTargetValue(float value)
        {
            value = ClampLikeNative(value, _minimum, _maximum);
            if (!Mathf.WithinEpsilon((double)value, _targetValue, Mathf.Epsilon))
            {
                _targetValue = value;
                _value = value;
                SetUpdate(_update, null);
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
            return _targetValue - _value;
        }

        /// <summary>
        /// Gets a value indicating whether thumb is being clicked (scroll bar is in use).
        /// </summary>
        public bool IsThumbClicked()
        {
            return _thumbClicked;
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
            if (!Mathf.WithinEpsilon((double)_value, _targetValue, Mathf.Epsilon))
            {
                _value = _targetValue = _startValue;
                _scrollAnimationProgress = 0.0f;
                SetUpdate(_update, null);
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
            float viewMin = _value;
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

            _minimum = minimum;
            _maximum = maximum;

            if (_value < minimum)
            {
                SetValue(minimum);
            }
            else if (_value > maximum)
            {
                _value = maximum;
            }

            UpdateThumb();
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // var style = Style.Current;
            Render2D.FillRectangle(_trackRect, TrackColor * _thumbOpacity);
            Render2D.FillRectangle(_thumbRect, (_thumbClicked ? ThumbSelectedColor : ThumbColor) * _thumbOpacity);
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
            if (_thumbClicked)
            {
                Float2 slidePosition = location + Root.GetTrackingMouseOffset();
                if (Parent is ScrollableControl panel)
                {
                    slidePosition += panel.GetViewOffset(); // Hardcoded fix
                }

                float mousePosition = _orientation == Orientation.Vertical ? slidePosition.Y : slidePosition.X;
                float percentage = (mousePosition - _mouseOffset - _thumbSize / 2) / (TrackSize() - _thumbSize);
                SetTargetValue(_minimum + percentage * (_maximum - _minimum));
            }
        }

        /// <inheritdoc />
        public override bool OnMouseWheel(Float2 location, float delta)
        {
            if (ThumbEnabled)
            {
                // Scroll
                SetValue(_targetValue - delta * _scrollChange);
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

                float mousePosition = _orientation == Orientation.Vertical ? location.Y : location.X;

                if (_thumbRect.Contains(location))
                {
                    // Start moving thumb
                    _thumbClicked = true;
                    _mouseOffset = mousePosition - _thumbCenter;

                    // Start capturing mouse
                    StartMouseCapture();
                }
                else
                {
                    // Click change
                    SetValue(_value + (mousePosition < _thumbCenter ? -1 : 1) * _clickChange);
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

            SetUpdate(_update, m_OnUpdate);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            base.OnMouseLeave();

            SetUpdate(_update, m_OnUpdate);
        }

        public void Reset()
        {
            _value = _targetValue = _startValue = 0.0f;
            _scrollAnimationProgress = 0.0f;
        }

        private void UpdateThumb()
        {
            // Cache data
            float width = Width;
            float height = Height;
            float trackSize = TrackSize();
            float range = _maximum - _minimum;
            float minimumThumbSize = trackSize / range * 100.0f;
            float maximumThumbSize = minimumThumbSize < 50.0f ? 50.0f : minimumThumbSize;
            float remainingTrackSize = trackSize - 10.0f;
            _thumbSize = maximumThumbSize < remainingTrackSize ? maximumThumbSize : remainingTrackSize;
            float pixelRange = trackSize - _thumbSize;
            float percentage = (_value - _minimum) / range;
            float thumbPosition = (int)(percentage * pixelRange);
            _thumbCenter = thumbPosition + _thumbSize / 2;
            _thumbRect = _orientation == Orientation.Vertical
                ? new Rectangle((width - ThumbThickness) / 2, thumbPosition + 4, ThumbThickness, _thumbSize - 8)
                : new Rectangle(thumbPosition + 4, (height - ThumbThickness) / 2, _thumbSize - 8, ThumbThickness);
            _trackRect = _orientation == Orientation.Vertical
                ? new Rectangle((width - TrackThickness) / 2, 4, TrackThickness, height - 8)
                : new Rectangle(4, (height - TrackThickness) / 2, width - 8, TrackThickness);
        }

        private void EndTracking()
        {
            // Check flag
            if (_thumbClicked)
            {
                // Clear flag
                _thumbClicked = false;

                // End capturing mouse
                EndMouseCapture();
            }
        }

        private void OnUpdate(float deltaTime)
        {
            bool isDeltaSlow = deltaTime > (1 / 20.0f);

            // Opacity smoothing
            float targetOpacity = IsMouseOver ? 1.0f : DefaultMinimumOpacity;
            _thumbOpacity = isDeltaSlow ? targetOpacity : Mathf.Lerp(_thumbOpacity, targetOpacity, deltaTime * 10.0f);
            bool needUpdate = MathF.Abs(_thumbOpacity - targetOpacity) > 0.001f;

            // Ensure scroll bar is visible and smoothing is required
            if (Visible && MathF.Abs(_targetValue - _value) > 0.01f)
            {
                // Interpolate or not if running slow
                float value;
                if (!isDeltaSlow && UseSmoothing())
                {
                    // percentage of scroll from 0 to _scrollChange, ex. 0.5 at _scrollChange / 2
                    float minScrollChangeRatio = ClampLikeNative(MathF.Abs(_targetValue - _startValue) / _scrollChange, 0.0f, 1.0f);

                    // shorten the duration if we scrolled less than _scrollChange
                    float actualDuration = ScrollAnimationDuration * minScrollChangeRatio;
                    float step = deltaTime / actualDuration;

                    float progress = _scrollAnimationProgress;
                    progress = ClampLikeNative(progress + step, 0.0f, 1.0f);

                    // https://easings.net/#easeOutSine
                    float easedProgress = MathF.Sin((progress * MathF.PI) / 2);
                    value = Mathf.Lerp(_startValue, _targetValue, easedProgress);

                    _scrollAnimationProgress = progress;
                }
                else
                {
                    value = _targetValue;
                    _startValue = _targetValue;
                    _scrollAnimationProgress = 0.0f;
                }

                _value = value;
                OnValueChanged();
                needUpdate = true;
            }

            // End updating if all animations are done
            if (!needUpdate)
            {
                SetUpdate(_update, null);
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
            var style = Style.Current;
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

            _orientation = orientation;
        }

        /// <inheritdoc />
        protected override void AddUpdateCallbacks(RootControl root)
        {
            base.AddUpdateCallbacks(root);

            if (_update != null)
            {
                root.UpdateCallbacksToAdd.Add(_update);
            }
        }

        /// <inheritdoc />
        protected override void RemoveUpdateCallbacks(RootControl root)
        {
            base.RemoveUpdateCallbacks(root);

            if (_update != null)
            {
                root.UpdateCallbacksToRemove.Add(_update);
            }
        }
    }
}

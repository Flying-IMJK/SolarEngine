using System;
using SE.GUI;

namespace SE.Editor.GUI
{
    public abstract class ValueBox<T> : TextBox where T : IComparable<T>
    {
        protected T CurrentValue;
        protected T Minimum;
        protected T Maximum;
        protected float SliderSpeed;
        protected bool Sliding;
        protected T StartSlideValue = default!;
        protected string StartEditText = string.Empty;
        private float m_LastSlideX;

        protected ValueBox(T value, float x, float y, float width, T min, T max, float sliderSpeed)
            : base(new Rectangle(x, y, width, 18.0f))
        {
            CurrentValue = Clamp(value, min, max);
            Minimum = min;
            Maximum = max;
            SliderSpeed = sliderSpeed;
        }

        public event Action? ValueChanged;
        public event Action<ValueBox<T>>? BoxValueChanged;
        public event Action? SlidingStart;
        public event Action? SlidingEnd;

        public T Value
        {
            get => GetValue();
            set => SetValue(value);
        }

        public T MinValue
        {
            get => Minimum;
            set
            {
                Minimum = value;
                SetValue(CurrentValue);
            }
        }

        public T MaxValue
        {
            get => Maximum;
            set
            {
                Maximum = value;
                SetValue(CurrentValue);
            }
        }

        public bool IsSliding => Sliding;
        public float SlideSpeed
        {
            get => SliderSpeed;
            set => SliderSpeed = value;
        }

        public bool CanUseSliding => SliderSpeed > float.Epsilon;
        public Rectangle SlideRect => new Rectangle(Width - 13.0f, (Height - 12.0f) * 0.5f, 12.0f, 12.0f);

        public virtual T GetValue()
        {
            return CurrentValue;
        }

        public virtual void SetValue(T value)
        {
            T clamped = Clamp(value, Minimum, Maximum);
            if (CurrentValue.CompareTo(clamped) == 0)
                return;

            CurrentValue = clamped;
            UpdateText();
            OnValueChanged();
        }

        public void BeginSliding()
        {
            if (!CanUseSliding || Sliding)
                return;

            Sliding = true;
            StartSlideValue = CurrentValue;
            SlidingStart?.Invoke();
        }

        public void ApplySlidingDelta(float delta)
        {
            if (Sliding)
                ApplySliding(delta * SliderSpeed);
        }

        public void EndSliding()
        {
            if (!Sliding)
                return;

            Sliding = false;
            SlidingEnd?.Invoke();
        }

        public override void BeginEdit()
        {
            if (IsEditing)
                return;

            base.BeginEdit();
            StartEditText = Text;
        }

        public override void EndEdit()
        {
            if (!IsEditing)
                return;

            base.EndEdit();
            if (StartEditText != Text)
                TryGetValue();
            StartEditText = string.Empty;
        }

        public void CancelEdit()
        {
            if (!IsEditing)
                return;

            Text = StartEditText;
            StartEditText = string.Empty;
            base.EndEdit();
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && SlideRect.Contains(location) && CanUseSliding)
            {
                m_LastSlideX = location.X;
                BeginSliding();
                Root?.StartTrackingMouse(this);
                return true;
            }

            if (button == MouseButton.Left)
            {
                BeginEdit();
                return true;
            }

            return false;
        }

        public override void OnMouseMove(Float2 location)
        {
            if (!Sliding)
            {
                base.OnMouseMove(location);
                return;
            }

            ApplySlidingDelta(location.X - m_LastSlideX);
            m_LastSlideX = location.X;
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && Sliding)
            {
                EndSliding();
                Root?.EndTrackingMouse();
                return true;
            }

            return button == MouseButton.Left;
        }

        public override bool OnCharInput(char character)
        {
            if (!IsEditing || char.IsControl(character))
                return false;

            Text += character;
            return true;
        }

        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (!IsEditing)
                return base.OnKeyDown(key);

            switch (key)
            {
            case KeyboardKeys.Backspace:
                if (Text.Length > 0)
                    Text = Text[..^1];
                return true;
            case KeyboardKeys.Return:
                EndEdit();
                return true;
            case KeyboardKeys.Escape:
                CancelEdit();
                return true;
            default:
                return false;
            }
        }

        public override void ClearState()
        {
            EndSliding();
            CancelEdit();
            base.ClearState();
        }

        protected abstract void UpdateText();
        protected abstract void TryGetValue();
        protected abstract void ApplySliding(float delta);

        protected virtual void OnValueChanged()
        {
            ValueChanged?.Invoke();
            BoxValueChanged?.Invoke(this);
        }

        private static T Clamp(T value, T min, T max)
        {
            if (value.CompareTo(min) < 0)
                return min;
            if (value.CompareTo(max) > 0)
                return max;
            return value;
        }
    }
}

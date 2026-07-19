using System;
using SE.GUI;

namespace SE.Editor.GUI
{
    public abstract class ValueBox<T> : TextBox where T : IComparable<T>
    {
        protected T _value;
        protected T _min;
        protected T _max;
        protected float _slideSpeed;
        protected bool _isSliding;
        protected T _startSlideValue = default!;
        protected string _startEditText = string.Empty;
        private float _lastSlideX;

        protected ValueBox(T value, float x, float y, float width, T min, T max, float sliderSpeed)
            : base(new Rectangle(x, y, width, 18.0f))
        {
            _value = Clamp(value, min, max);
            _min = min;
            _max = max;
            _slideSpeed = sliderSpeed;
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
            get => _min;
            set
            {
                _min = value;
                SetValue(_value);
            }
        }

        public T MaxValue
        {
            get => _max;
            set
            {
                _max = value;
                SetValue(_value);
            }
        }

        public bool IsSliding => _isSliding;
        public float SlideSpeed
        {
            get => _slideSpeed;
            set => _slideSpeed = value;
        }

        public bool CanUseSliding => _slideSpeed > float.Epsilon;
        public GuiRect SlideRect => new GuiRect(Width - 13.0f, (Height - 12.0f) * 0.5f, 12.0f, 12.0f);

        public virtual T GetValue()
        {
            return _value;
        }

        public virtual void SetValue(T value)
        {
            T clamped = Clamp(value, _min, _max);
            if (_value.CompareTo(clamped) == 0)
                return;

            _value = clamped;
            UpdateText();
            OnValueChanged();
        }

        public void BeginSliding()
        {
            if (!CanUseSliding || _isSliding)
                return;

            _isSliding = true;
            _startSlideValue = _value;
            SlidingStart?.Invoke();
        }

        public void ApplySlidingDelta(float delta)
        {
            if (_isSliding)
                ApplySliding(delta * _slideSpeed);
        }

        public void EndSliding()
        {
            if (!_isSliding)
                return;

            _isSliding = false;
            SlidingEnd?.Invoke();
        }

        public override void BeginEdit()
        {
            if (IsEditing)
                return;

            base.BeginEdit();
            _startEditText = Text;
        }

        public override void EndEdit()
        {
            if (!IsEditing)
                return;

            base.EndEdit();
            if (_startEditText != Text)
                TryGetValue();
            _startEditText = string.Empty;
        }

        public void CancelEdit()
        {
            if (!IsEditing)
                return;

            Text = _startEditText;
            _startEditText = string.Empty;
            base.EndEdit();
        }

        public override bool OnMouseDown(Float2 location, int button)
        {
            if (button == 1 && SlideRect.Contains(new GuiPoint(location.X, location.Y)) && CanUseSliding)
            {
                _lastSlideX = location.X;
                BeginSliding();
                Root?.StartTrackingMouse(this);
                return true;
            }

            if (button == 1)
            {
                BeginEdit();
                return true;
            }

            return false;
        }

        public override void OnMouseMove(Float2 location)
        {
            if (!_isSliding)
            {
                base.OnMouseMove(location);
                return;
            }

            ApplySlidingDelta(location.X - _lastSlideX);
            _lastSlideX = location.X;
        }

        public override bool OnMouseUp(Float2 location, int button)
        {
            if (button == 1 && _isSliding)
            {
                EndSliding();
                Root?.EndTrackingMouse();
                return true;
            }

            return button == 1;
        }

        public override bool OnCharInput(char character)
        {
            if (!IsEditing || char.IsControl(character))
                return false;

            Text += character;
            return true;
        }

        public override bool OnKeyDown(int key)
        {
            if (!IsEditing)
                return base.OnKeyDown(key);

            switch (key)
            {
            case 8: // Backspace
                if (Text.Length > 0)
                    Text = Text[..^1];
                return true;
            case 13: // Enter
                EndEdit();
                return true;
            case 27: // Escape
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

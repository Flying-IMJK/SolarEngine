// Managed editor GUI feature implementation.
using System;
using System.Collections.Generic;
using SE.GUI;

namespace SE.Editor.GUI
{
    public sealed class ToolStrip : ContainerControl
    {
        public const float DefaultMarginV = 1.0f;
        public const float DefaultMarginH = 2.0f;

        private readonly List<ToolStripItem> _items = new List<ToolStripItem>();

        public ToolStrip(float height, float y, float width)
            : base(new Rectangle(0, y, width, height))
        {
            SetBounds(0, y, width, height);
        }

        public event Action<ToolStripButton>? ButtonClicked;
        public event Action<ToolStripButton>? SecondaryButtonClicked;

        public IReadOnlyList<ToolStripItem> Items => _items;
        public float ItemsHeight => Math.Max(0.0f, Height - 2.0f * DefaultMarginV);

        public ToolStripButton? LastButton
        {
            get
            {
                for (int i = _items.Count - 1; i >= 0; i--)
                {
                    if (_items[i] is ToolStripButton button)
                        return button;
                }

                return null;
            }
        }

        public int ButtonsCount
        {
            get
            {
                int result = 0;
                foreach (ToolStripItem item in _items)
                {
                    if (item is ToolStripButton)
                        result++;
                }

                return result;
            }
        }

        public ToolStripButton AddButton(string text, Action? onClick = null)
        {
            ToolStripButton button = new ToolStripButton(ItemsHeight, text);
            if (onClick != null)
                button.Clicked += _ => onClick();

            button.Clicked += OnChildButtonClicked;
            button.SecondaryClicked += OnChildButtonSecondaryClicked;
            _items.Add(button);
            AddChild(button);
            PerformLayout();
            return button;
        }

        public ToolStripSeparator AddSeparator()
        {
            ToolStripSeparator separator = new ToolStripSeparator(ItemsHeight);
            _items.Add(separator);
            AddChild(separator);
            PerformLayout();
            return separator;
        }

        protected override void OnLayoutChildren()
        {
            float x = DefaultMarginH;
            float h = ItemsHeight;
            foreach (ToolStripItem item in _items)
            {
                if (!item.Visible)
                    continue;

                item.SetBounds(x, DefaultMarginV, item.Width, h);
                x += item.Width + DefaultMarginH;
            }
        }

        private void OnChildButtonClicked(ToolStripButton button)
        {
            ButtonClicked?.Invoke(button);
        }

        private void OnChildButtonSecondaryClicked(ToolStripButton button)
        {
            SecondaryButtonClicked?.Invoke(button);
        }
    }

    public abstract class ToolStripItem : Control
    {
        protected ToolStripItem(Rectangle bounds)
            : base(bounds)
        {
        }
    }

    public sealed class ToolStripButton : ToolStripItem
    {
        private string _text;
        private bool _isPressed;

        public ToolStripButton(float height, string text)
            : base(new Rectangle(0, 0, MeasureWidth(height, text), height))
        {
            _text = text;
            SetBounds(0, 0, MeasureWidth(height, text), height);
        }

        public event Action<ToolStripButton>? Clicked;
        public event Action<ToolStripButton>? SecondaryClicked;

        public bool Checked { get; private set; }
        public bool AutoCheck { get; private set; }

        public string Text
        {
            get => _text;
            set
            {
                _text = value;
                SetBounds(X, Y, MeasureWidth(Height, value), Height);
            }
        }

        public ToolStripButton SetAutoCheck(bool value)
        {
            AutoCheck = value;
            return this;
        }

        public ToolStripButton SetChecked(bool value)
        {
            Checked = value;
            return this;
        }

        public override bool OnMouseDown(Float2 location, int button)
        {
            if (button != 1)
                return false;

            _isPressed = true;
            Root?.StartTrackingMouse(this);
            return true;
        }

        public override bool OnMouseUp(Float2 location, int button)
        {
            if (button != 1 || !_isPressed)
                return false;

            _isPressed = false;
            Root?.EndTrackingMouse();
            if (location.X >= 0.0f && location.Y >= 0.0f && location.X <= Width && location.Y <= Height)
                InvokeClick();
            return true;
        }

        public override void ClearState()
        {
            _isPressed = false;
            base.ClearState();
        }

        public void InvokeClick()
        {
            if (AutoCheck)
                Checked = !Checked;

            Clicked?.Invoke(this);
        }

        public void InvokeSecondaryClick()
        {
            SecondaryClicked?.Invoke(this);
        }

        private static float MeasureWidth(float height, string text)
        {
            float width = 4.0f;
            if (string.IsNullOrEmpty(text))
                return Math.Max(width, height);

            return width + text.Length * 7.0f + 6.0f;
        }
    }

    public sealed class ToolStripSeparator : ToolStripItem
    {
        public ToolStripSeparator(float height)
            : base(new Rectangle(0, 0, 4, height))
        {
            SetBounds(0, 0, 4, height);
        }
    }
}

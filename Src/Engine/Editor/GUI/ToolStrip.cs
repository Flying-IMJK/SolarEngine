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

        private readonly List<ToolStripItem> m_Items = new List<ToolStripItem>();

        public ToolStrip(float height, float y, float width)
            : base(new Rectangle(0, y, width, height))
        {
            AutoFocus = false;
            BackgroundColor = Style.Current.BackgroundNormal;
            SetBounds(0, y, width, height);

            // Flax scales this offset with Editor.Options.Interface.IconsScale.
            // Keep the Editor option dependency disabled until it is exposed to managed code.
        }

        public event Action<ToolStripButton>? ButtonClicked;
        public event Action<ToolStripButton>? SecondaryButtonClicked;

        public IReadOnlyList<ToolStripItem> Items => m_Items;
        public float ItemsHeight => Math.Max(0.0f, Height - 2.0f * DefaultMarginV);

        public ToolStripButton? LastButton
        {
            get
            {
                for (int i = m_Items.Count - 1; i >= 0; i--)
                {
                    if (m_Items[i] is ToolStripButton button)
                    {
                        return button;
                    }
                }

                return null;
            }
        }

        public int ButtonsCount
        {
            get
            {
                int result = 0;
                foreach (ToolStripItem item in m_Items)
                {
                    if (item is ToolStripButton)
                    {
                        result++;
                    }
                }

                return result;
            }
        }

        public ToolStripButton AddButton(string text, Action? onClick = null)
        {
            ToolStripButton button = new ToolStripButton(ItemsHeight, text);
            if (onClick != null)
            {
                button.Clicked += _ => onClick();
            }

            button.Clicked += OnChildButtonClicked;
            button.SecondaryClicked += OnChildButtonSecondaryClicked;
            m_Items.Add(button);
            AddChild(button);
            PerformLayout();
            return button;
        }

        public ToolStripSeparator AddSeparator()
        {
            ToolStripSeparator separator = new ToolStripSeparator(ItemsHeight);
            m_Items.Add(separator);
            AddChild(separator);
            PerformLayout();
            return separator;
        }

        protected override void OnLayoutChildren()
        {
            float x = DefaultMarginH;
            float h = ItemsHeight;
            foreach (ToolStripItem item in m_Items)
            {
                if (!item.Visible)
                {
                    continue;
                }

                item.SetBounds(x, DefaultMarginV, item.Width, h);
                x += item.Width + DefaultMarginH;
            }
        }

        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (base.OnKeyDown(key))
                return true;

            // Flax forwards unhandled keys to the owning EditorWindow InputActions collection.
            // Keep that Editor shortcut route disabled; no replacement shortcut behavior is introduced.
            return false;
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
        private string m_Text;
        private bool m_IsPrimaryMouseDown;
        private bool m_IsSecondaryMouseDown;

        public ToolStripButton(float height, string text)
            : base(new Rectangle(0, 0, MeasureWidth(height, text), height))
        {
            m_Text = text;
            SetBounds(0, 0, MeasureWidth(height, text), height);
        }

        public event Action<ToolStripButton>? Clicked;
        public event Action<ToolStripButton>? SecondaryClicked;

        public bool Checked { get; private set; }
        public bool AutoCheck { get; private set; }
        public ContextMenu? ContextMenu { get; set; }

        public string Text
        {
            get => m_Text;
            set
            {
                m_Text = value;
                SetBounds(X, Y, MeasureWidth(Height, value), Height);
                Parent?.PerformLayout();
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

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                m_IsPrimaryMouseDown = true;
                Root?.StartTrackingMouse(this);
                return true;
            }

            if (button == MouseButton.Right)
            {
                m_IsSecondaryMouseDown = true;
                Root?.StartTrackingMouse(this);
                return true;
            }

            return base.OnMouseDown(location, button);
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && m_IsPrimaryMouseDown)
            {
                m_IsPrimaryMouseDown = false;
                Root?.EndTrackingMouse();
                InvokeClick();
                return true;
            }

            if (button == MouseButton.Right && m_IsSecondaryMouseDown)
            {
                m_IsSecondaryMouseDown = false;
                Root?.EndTrackingMouse();
                InvokeSecondaryClick();
                ContextMenu?.Show(this, 0.0f, Height);
                return true;
            }

            return base.OnMouseUp(location, button);
        }

        public override void ClearState()
        {
            m_IsPrimaryMouseDown = false;
            m_IsSecondaryMouseDown = false;
            base.ClearState();
        }

        protected override void OnDraw()
        {
            bool enabled = EnabledInHierarchy;
            if (enabled && (IsMouseOver || Checked))
            {
                Color background = Checked
                    ? Style.Current.BackgroundSelected
                    : (m_IsPrimaryMouseDown || m_IsSecondaryMouseDown)
                        ? Style.Current.BackgroundHighlighted
                        : Style.Current.BackgroundNormal;
                Rectangle bounds = ScreenBounds;
                Render2D.FillRectangle(ref bounds, ref background);
            }

            Font? font = Style.Current.FontMedium;
            if (ReferenceEquals(font, null) || string.IsNullOrEmpty(Text))
                return;

            Rectangle textBounds = ScreenBounds;
            Color textColor = enabled ? Style.Current.Foreground : Style.Current.ForegroundDisabled;
            Render2D.RenderText(font, Text, ref textBounds, ref textColor, TextAlignment.Center, TextAlignment.Center, TextWrapping.NoWrap);
        }

        public void InvokeClick()
        {
            if (AutoCheck)
            {
                Checked = !Checked;
            }

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
            {
                return Math.Max(width, height);
            }

            return width + text.Length * 7.0f + 6.0f;
        }
    }

    public sealed class ToolStripSeparator : ToolStripItem
    {
        public ToolStripSeparator(float height)
            : base(new Rectangle(0, 0, 4, height))
        {
            SetBounds(0, 0, 4, height);
            AutoFocus = false;
        }

        protected override void OnDraw()
        {
            Rectangle line = new Rectangle(ScreenPos.X + (Width - 1.0f) * 0.5f, ScreenPos.Y + 2.0f, 1.0f, MathF.Max(0.0f, Height - 4.0f));
            Color color = Style.Current.BackgroundHighlighted;
            Render2D.FillRectangle(ref line, ref color);
        }
    }
}

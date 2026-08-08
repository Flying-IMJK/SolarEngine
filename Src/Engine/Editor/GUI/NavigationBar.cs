// Managed editor GUI feature implementation.
using System;
using System.Collections.Generic;
using SE.GUI;

namespace SE.Editor.GUI
{
    public sealed class NavigationBar : Panel
    {
        public const float DefaultButtonsMargin = 2.0f;

        private readonly List<NavigationButton> m_Buttons = new List<NavigationButton>();

        public NavigationBar()
            : this(0.0f, 0.0f, 0.0f, 0.0f)
        {
        }

        public NavigationBar(float x, float y, float width, float height)
            : base(new Rectangle(x, y, width, height))
        {
            SetBounds(x, y, width, height);
        }

        public IReadOnlyList<NavigationButton> Buttons => m_Buttons;

        public NavigationButton AddButton(string text)
        {
            NavigationButton button = new NavigationButton(text, Height);
            m_Buttons.Add(button);
            AddChild(button);
            PerformLayout();
            return button;
        }

        public void UpdateBounds(ToolStrip toolStrip, float parentWidth)
        {
            ToolStripButton? lastButton = toolStrip.LastButton;
            float x = lastButton != null ? lastButton.X + lastButton.Width + 8.0f : 8.0f;
            SetBounds(x, toolStrip.Y, parentWidth - x - 8.0f, toolStrip.Height);
        }

        public void UpdateBounds(ToolStrip toolStrip)
        {
            ArgumentNullException.ThrowIfNull(toolStrip);
            UpdateBounds(toolStrip, Parent?.Width ?? Width);
        }

        protected override void OnLayoutChildren()
        {
            float x = DefaultButtonsMargin;
            foreach (NavigationButton button in m_Buttons)
            {
                if (!button.Visible)
                    continue;

                button.SetBounds(x, 0, button.MeasureWidth(), Height);
                x += button.Width + DefaultButtonsMargin;
            }
        }
    }

    public sealed class NavigationButton : Button
    {
        public const float DefaultMargin = 6.0f;

        public NavigationButton(string text, float height)
            : base(new Rectangle(0, 0, 2.0f * DefaultMargin, height), text)
        {
            SetBounds(0, 0, MeasureWidth(), height);
        }

        public float MeasureWidth()
        {
            return Text.Length * 7.0f + 2.0f * DefaultMargin;
        }
    }
}

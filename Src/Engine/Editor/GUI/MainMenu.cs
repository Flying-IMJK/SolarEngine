// Managed editor GUI feature implementation.
using System;
using System.Collections.Generic;
using SE.GUI;

namespace SE.Editor.GUI
{
    public sealed class MainMenu : ContainerControl
    {
        public const float DefaultHeight = 28.0f;

        private readonly List<MainMenuButton> m_Buttons = new List<MainMenuButton>();
        private MainMenuButton? m_Selected;

        public MainMenu(float width)
            : base(new Rectangle(0, 0, width, DefaultHeight))
        {
            SetBounds(0, 0, width, DefaultHeight);
        }

        public IReadOnlyList<MainMenuButton> Buttons => m_Buttons;

        public MainMenuButton? Selected
        {
            get => m_Selected;
            set
            {
                if (ReferenceEquals(m_Selected, value))
                    return;

                if (m_Selected != null)
                {
                    m_Selected.ContextMenu.VisibleChanged -= OnSelectedContextMenuVisibleChanged;
                    m_Selected.ContextMenu.Hide();
                }
                m_Selected = value;
                if (m_Selected != null && m_Selected.ContextMenu.HasItems)
                {
                    m_Selected.ContextMenu.Show(m_Selected, 0, m_Selected.Height);
                    m_Selected.ContextMenu.VisibleChanged += OnSelectedContextMenuVisibleChanged;
                }
            }
        }

        private void OnSelectedContextMenuVisibleChanged(ContextMenu contextMenu)
        {
            if (!contextMenu.IsOpened && ReferenceEquals(m_Selected?.ContextMenu, contextMenu))
                Selected = null;
        }

        public MainMenuButton AddButton(string text)
        {
            MainMenuButton button = new MainMenuButton(text);
            m_Buttons.Add(button);
            AddChild(button);
            PerformLayout();
            return button;
        }

        public MainMenuButton? GetButton(string text)
        {
            foreach (MainMenuButton button in m_Buttons)
            {
                if (string.Equals(button.Text, text, StringComparison.OrdinalIgnoreCase))
                    return button;
            }

            return null;
        }

        protected override void OnLayoutChildren()
        {
            float x = 0;
            foreach (MainMenuButton button in m_Buttons)
            {
                if (!button.Visible)
                    continue;

                float width = button.MeasureWidth();
                button.SetBounds(x, 0, width, Height);
                x += width;
            }
        }
    }

    public sealed class MainMenuButton : Control
    {
        public MainMenuButton(string text)
            : base(new Rectangle(0, 0, 32, 16))
        {
            Text = text;
            ContextMenu = new ContextMenu();
        }

        public string Text { get; }
        public ContextMenu ContextMenu { get; }

        public float MeasureWidth()
        {
            return 18.0f + Text.Length * 7.0f;
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (Parent is MainMenu mainMenu)
                mainMenu.Selected = this;
            return true;
        }

        public override void OnMouseEnter()
        {
            base.OnMouseEnter();
            if (Parent is MainMenu mainMenu && mainMenu.Selected != null)
                mainMenu.Selected = this;
        }

        protected override void OnDraw()
        {
            bool opened = ContextMenu.IsOpened;
            bool active = EnabledInHierarchy && ContextMenu.HasItems && (opened || IsMouseOver);
            if (active)
            {
                Color background = opened ? Style.Current.Background : Style.Current.BackgroundHighlighted;
                Rectangle bounds = ScreenBounds;
                Render2D.FillRectangle(ref bounds, ref background);
            }

            Font? font = Style.Current.FontMedium;
            if (ReferenceEquals(font, null))
                return;

            Rectangle textBounds = ScreenBounds;
            Color textColor = active ? Style.Current.Foreground : Style.Current.ForegroundDisabled;
            Render2D.RenderText(font, Text, ref textBounds, ref textColor, TextAlignment.Center, TextAlignment.Center, TextWrapping.NoWrap);
        }
    }
}

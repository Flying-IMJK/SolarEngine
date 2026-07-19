// Managed editor GUI feature implementation.
using System.Collections.Generic;
using SE.GUI;

namespace SE.Editor.GUI
{
    public sealed class MainMenu : ContainerControl
    {
        public const float DefaultHeight = 28.0f;

        private readonly List<MainMenuButton> _buttons = new List<MainMenuButton>();
        private MainMenuButton? _selected;

        public MainMenu(float width)
            : base(new Rectangle(0, 0, width, DefaultHeight))
        {
            SetBounds(0, 0, width, DefaultHeight);
        }

        public IReadOnlyList<MainMenuButton> Buttons => _buttons;

        public MainMenuButton? Selected
        {
            get => _selected;
            set
            {
                if (ReferenceEquals(_selected, value))
                    return;

                _selected?.ContextMenu.Hide();
                _selected = value;
                if (_selected != null && _selected.ContextMenu.HasItems)
                {
                    _selected.ContextMenu.Show(_selected, 0, _selected.Height);
                }
            }
        }

        public MainMenuButton AddButton(string text)
        {
            MainMenuButton button = new MainMenuButton(text);
            _buttons.Add(button);
            AddChild(button);
            PerformLayout();
            return button;
        }

        public MainMenuButton? GetButton(string text)
        {
            foreach (MainMenuButton button in _buttons)
            {
                if (button.Text == text)
                    return button;
            }

            return null;
        }

        protected override void OnLayoutChildren()
        {
            float x = 0;
            foreach (MainMenuButton button in _buttons)
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

        public override bool OnMouseDown(Float2 location, int button)
        {
            if (button != 1)
                return false;

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

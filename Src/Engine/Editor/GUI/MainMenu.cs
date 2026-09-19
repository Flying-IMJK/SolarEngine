// Managed editor GUI feature implementation.
using System;
using System.Collections.Generic;
using SE.GUI;

namespace SE.Editor.GUI
{
    public sealed class MainMenu : ContainerControl
    {
        public const float DefaultHeight = 28.0f;

        private const float WindowButtonWidth = 46.0f;
        private const float ResizeBorderThickness = 5.0f;
        private const string WindowIconPath = "Assets/Icon/Engine_Icon";
        private const string WindowIconsFontPath = "Assets/Fonts/SegMDL2";
        private const string ChromeMinimize = "\uE921";
        private const string ChromeMaximize = "\uE922";
        private const string ChromeRestore = "\uE923";
        private const string ChromeClose = "\uE8BB";

        private readonly List<MainMenuButton> m_Buttons = new List<MainMenuButton>();
        private readonly Window m_Window;
        private readonly Window.HitTestDelegate m_HitTestDelegate;
        private readonly Image m_Icon;
        private readonly Label m_Title;
        private readonly Button m_MinimizeButton;
        private readonly Button m_MaximizeButton;
        private readonly Button m_CloseButton;
        private MainMenuButton? m_Selected;
        private bool m_WindowClosed;

        public MainMenu(Window window, float width)
            : base(new Rectangle(0, 0, width, DefaultHeight))
        {
            ArgumentNullException.ThrowIfNull(window);

            m_Window = window;
            AutoFocus = false;
            IsScrollable = false;
            BackgroundColor = Style.Current.LightBackground;

            Texture? windowIcon = AssetContent.LoadInternal<Texture>(WindowIconPath);
            FontAsset? windowIconsFont = AssetContent.LoadInternal<FontAsset>(WindowIconsFontPath);
            Font? iconFont = windowIconsFont?.CreateFont(9.0f);

            m_Icon = new Image(new Rectangle(0, 0, DefaultHeight, DefaultHeight))
            {
                Margin = new Margin(6.0f),
                Brush = windowIcon != null ? new TextureBrush(windowIcon) : null,
                Color = Style.Current.Foreground,
                KeepAspectRatio = false,
            };

            m_Title = new Label(new Rectangle(0, 0, width, DefaultHeight), m_Window.Title)
            {
                HorizontalAlignment = TextAlignment.Center,
                VerticalAlignment = TextAlignment.Center,
                TextColor = Style.Current.ForegroundGrey,
            };

            m_MinimizeButton = CreateWindowButton(ChromeMinimize, iconFont);
            m_MaximizeButton = CreateWindowButton(m_Window.IsMaximized ? ChromeRestore : ChromeMaximize, iconFont);
            m_CloseButton = CreateWindowButton(ChromeClose, iconFont);
            m_CloseButton.BackgroundColorHighlighted = Color.Red;
            m_CloseButton.BackgroundColorPressed = Color.Red.RGBMultiplied(1.3f);

            m_MinimizeButton.Clicked += _ => m_Window.Minimize();
            m_MaximizeButton.Clicked += _ => ToggleMaximized();
            m_CloseButton.Clicked += _ => m_Window.Close(SE.ClosingReason.User);

            AddChild(m_Icon);
            AddChild(m_Title);
            AddChild(m_MinimizeButton);
            AddChild(m_MaximizeButton);
            AddChild(m_CloseButton);

            m_HitTestDelegate = OnWindowHitTest;
            m_Window.HitTest = m_HitTestDelegate;
            m_Window.Closed += OnWindowClosed;
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

        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);
            if (m_WindowClosed)
                return;

            m_Title.Text = m_Window.Title;
            m_MaximizeButton.Text = m_Window.IsMaximized ? ChromeRestore : ChromeMaximize;
        }

        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            if (base.OnMouseDoubleClick(location, button))
                return true;

            Control? child = HitTest(PointToRoot(location));
            if (button == MouseButton.Left && child is not Button && child is not MainMenuButton && !ReferenceEquals(child, m_Icon))
            {
                ToggleMaximized();
            }

            return true;
        }

        protected override void OnLayoutChildren()
        {
            float x = DefaultHeight;
            foreach (MainMenuButton button in m_Buttons)
            {
                if (!button.Visible)
                    continue;

                float width = button.MeasureWidth();
                button.SetBounds(x, 0, width, Height);
                x += width;
            }

            float right = Width;
            m_CloseButton.SetBounds(right - WindowButtonWidth, 0, WindowButtonWidth, Height);
            right = m_CloseButton.X;
            m_MaximizeButton.SetBounds(right - WindowButtonWidth, 0, WindowButtonWidth, Height);
            right = m_MaximizeButton.X;
            m_MinimizeButton.SetBounds(right - WindowButtonWidth, 0, WindowButtonWidth, Height);
            right = m_MinimizeButton.X;

            m_Icon.SetBounds(0, 0, DefaultHeight, Height);
            m_Title.SetBounds(x + 2.0f, 0, Math.Max(0.0f, right - x - 4.0f), Height);
        }

        protected override void OnDispose()
        {
            DetachWindow();
            base.OnDispose();
        }

        private static Button CreateWindowButton(string text, Font? font)
        {
            return new Button(new Rectangle(0, 0, WindowButtonWidth, DefaultHeight), text)
            {
                AutoFocus = false,
                Font = font,
                TextColor = Style.Current.Foreground,
                BackgroundColor = Color.Transparent,
                BackgroundColorHighlighted = Style.Current.LightBackground.RGBMultiplied(0.8f),
                BackgroundColorPressed = Style.Current.LightBackground.RGBMultiplied(0.65f),
            };
        }

        private WindowHitCodes OnWindowHitTest(ref Float2 screenPosition)
        {
            if (m_WindowClosed || m_Window.IsMinimized)
                return WindowHitCodes.NoWhere;

            float dpiScale = Math.Max(m_Window.DpiScale, 0.001f);
            Float2 clientPosition = m_Window.ScreenToClient(screenPosition * dpiScale) / dpiScale;
            Float2 windowSize = m_Window.Size / dpiScale;

            if (!m_Window.IsMaximized)
            {
                bool left = clientPosition.X <= ResizeBorderThickness;
                bool right = clientPosition.X >= windowSize.X - ResizeBorderThickness;
                bool top = clientPosition.Y <= ResizeBorderThickness;
                bool bottom = clientPosition.Y >= windowSize.Y - ResizeBorderThickness;

                if (bottom && left)
                    return WindowHitCodes.BottomLeft;
                if (bottom && right)
                    return WindowHitCodes.BottomRight;
                if (top && left)
                    return WindowHitCodes.TopLeft;
                if (top && right)
                    return WindowHitCodes.TopRight;
                if (right)
                    return WindowHitCodes.Right;
                if (left)
                    return WindowHitCodes.Left;
                if (top)
                    return WindowHitCodes.Top;
                if (bottom)
                    return WindowHitCodes.Bottom;
            }

            if (ScreenBounds.Contains(clientPosition))
            {
                Control? control = HitTest(clientPosition);
                if (control == null || ReferenceEquals(control, this) || ReferenceEquals(control, m_Title))
                    return WindowHitCodes.Caption;
            }

            return WindowHitCodes.Client;
        }

        private void ToggleMaximized()
        {
            if (m_WindowClosed)
                return;

            if (m_Window.IsMaximized)
                m_Window.Restore();
            else
                m_Window.Maximize();
        }

        private void OnWindowClosed()
        {
            m_WindowClosed = true;
            DetachWindow();
        }

        private void DetachWindow()
        {
            if (m_Window.HitTest == m_HitTestDelegate)
                m_Window.HitTest = null;
            m_Window.Closed -= OnWindowClosed;
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
                Render2D.FillRectangle(bounds, background);
            }

            Font? font = Style.Current.FontMedium;
            if (ReferenceEquals(font, null))
                return;

            Rectangle textBounds = ScreenBounds;
            Color textColor = active ? Style.Current.Foreground : Style.Current.ForegroundDisabled;
            Render2D.RenderText(font, Text, textBounds, textColor, TextAlignment.Center, TextAlignment.Center, TextWrapping.NoWrap);
        }
    }
}

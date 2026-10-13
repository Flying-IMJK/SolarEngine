#nullable disable annotations

using SE.GUI;

namespace SE.Editor.GUI
{
    public class MainMenu : ContainerControl
    {
        private Window.HitTestDelegate _hitTestCall;
        private bool _useCustomWindowSystem = false;
        private Image _icon = null;
        private Label _title = null;
        private Button _closeButton = null;
        private Button _minimizeButton = null;
        private Button _maximizeButton = null;
        private Window _window = null;
        private MainMenuButton _selected = null;

        private void OnSelectedContextMenuVisibleChanged(Control control)
        {
            if (_selected != null)
            {
                Selected = null;
            }
        }

        /// <summary>
        /// Return the rightmost button.
        /// </summary>
        /// <returns>Rightmost button, null if there is no <see cref="MainMenuButton"/>.</returns>
        private MainMenuButton GetRightButton()
        {
            MainMenuButton b = null;
            foreach (Control control in Children)
            {
                bool isMenuButton = control is MainMenuButton;
                if (b == null && isMenuButton)
                {
                    b = (MainMenuButton)control;
                }

                if (isMenuButton && b != null && control.Right > b.Right)
                {
                    b = (MainMenuButton)control;
                }
            }
            return b;
        }

        private void OnWindowClosed()
        {
            if (_window != null)
            {
                _window.HitTest = null;
                _window = null;
            }
        }

        private WindowHitCodes OnHitTest(Float2 mouse)
        {
            float dpiScale = _window.DpiScale;

            if (_window.IsMinimized)
            {
                return WindowHitCodes.NoWhere;
            }

            if (!_window.IsMaximized)
            {
                Float2 pos = _window.ScreenToClient(mouse * dpiScale); // pos is not DPI adjusted
                Float2 winSize = _window.Size;

                // Distance from which the mouse is considered to be on the border/corner
                float distance = 5.0f * dpiScale;

                if (pos.Y > winSize.Y - distance && pos.X < distance)
                {
                    return WindowHitCodes.BottomLeft;
                }
                if (pos.X > winSize.X - distance && pos.Y > winSize.Y - distance)
                {
                    return WindowHitCodes.BottomRight;
                }
                if (pos.Y < distance && pos.X < distance)
                {
                    return WindowHitCodes.TopLeft;
                }
                if (pos.Y < distance && pos.X > winSize.X - distance)
                {
                    return WindowHitCodes.TopRight;
                }
                if (pos.X > winSize.X - distance)
                {
                    return WindowHitCodes.Right;
                }
                if (pos.X < distance)
                {
                    return WindowHitCodes.Left;
                }
                if (pos.Y < distance)
                {
                    return WindowHitCodes.Top;
                }
                if (pos.Y > winSize.Y - distance)
                {
                    return WindowHitCodes.Bottom;
                }
            }

            Float2 mousePos = PointFromScreen(mouse * dpiScale);
            Control controlUnderMouse = GetChildAt(mousePos);
            bool isMouseOverSth = controlUnderMouse != null && controlUnderMouse != _title;

            MainMenuButton rb = GetRightButton();
            if (rb == null)
            {
                Float2 upperLeft = _title.UpperLeft;
                Rectangle rectangle = new Rectangle(upperLeft,
                    new Float2(_minimizeButton.BottomLeft.X - upperLeft.X, _title.Height));
                if (rectangle.Contains(mousePos) && !isMouseOverSth)
                {
                    return WindowHitCodes.Caption;
                }
            }
            else if (_minimizeButton != null &&
                new Rectangle(rb.UpperRight, _minimizeButton.BottomLeft - rb.UpperRight).Contains(mousePos) &&
                !isMouseOverSth)
            {
                return WindowHitCodes.Caption;
            }

            return WindowHitCodes.Client;
        }

        /// <summary>
        /// Gets or sets the selected button (with opened context menu).
        /// </summary>
        public MainMenuButton Selected
        {
            get => _selected;
            set
            {
                if (_selected == value)
                {
                    return;
                }

                if (_selected != null)
                {
                    _selected.ContextMenu.VisibleChanged -= OnSelectedContextMenuVisibleChanged;
                    _selected.ContextMenu.Hide();
                }

                _selected = value;

                if (_selected != null && _selected.ContextMenu.HasChildren)
                {
                    _selected.ContextMenu.Show(_selected, new Float2(0, _selected.Height));
                    _selected.ContextMenu.VisibleChanged += OnSelectedContextMenuVisibleChanged;
                }
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="MainMenu"/> class.
        /// </summary>
        /// <param name="mainWindow">The main window.</param>
        public MainMenu(RootControl mainWindow) : base(0, 0, 0, 20)
        {
            AutoFocus = false;
            AnchorPreset = AnchorPresets.HorizontalStretchTop;

            _useCustomWindowSystem = true;
            if (_useCustomWindowSystem)
            {
                BackgroundColor = Style.Current.LightBackground;
                Height = 28;

                Texture windowIcon = AssetContent.LoadAsyncInternal<Texture>(EditorAssets.WindowIcon);
                FontAsset windowIconsFont = AssetContent.LoadAsyncInternal<FontAsset>(EditorAssets.WindowIconsFont);

                windowIcon.WaitForLoaded();
                windowIconsFont.WaitForLoaded();

                Font iconFont = windowIconsFont.CreateFont(9);
                _hitTestCall = OnWindowHitTest;

                _window = mainWindow.RootWindow().Window;
                _window.HitTest = _hitTestCall;
                _window.Closed += OnWindowClosed;

                // ScriptsBuilder.GetBinariesConfiguration(...)
                _icon = new Image();
                _icon.Margin = new Margin(6, 6, 6, 6);
                _icon.Brush = new TextureBrush(windowIcon);
                // Preserve the engine texture's colors in both normal and hover states.
                _icon.Color = Color.White;
                _icon.MouseOverColor = Color.White;
                _icon.KeepAspectRatio = false;
                _icon.Parent = this;

                _title = new Label(0, 0, Width, Height);
                _title.Text = _window.Title;
                _title.HorizontalAlignment = TextAlignment.Center;
                _title.VerticalAlignment = TextAlignment.Center;
                _title.ClipText = true;
                _title.TextColor = Style.Current.ForegroundGrey;
                _title.TextColorHighlighted = Style.Current.ForegroundGrey;
                _title.Parent = this;

                _closeButton = new Button(0, 0, 46, Height);
                _closeButton.Text = EditorAssets.SegMDL2Icons.ChromeClose;
                _closeButton.Font = iconFont;
                _closeButton.TextColor = Style.Current.Foreground;
                _closeButton.BackgroundColor = Color.Transparent;
                _closeButton.BackgroundColorHighlighted = Color.Red;
                _closeButton.BackgroundColorSelected = Color.Red.RGBMultiplied(1.3f);
                _closeButton.BorderColor = Color.Transparent;
                _closeButton.BorderColorHighlighted = Color.Transparent;
                _closeButton.BorderColorSelected = Color.Transparent;
                _closeButton.Parent = this;
                _closeButton.Clicked += () =>
                {
                    _window.Close(SE.ClosingReason.User);
                };

                _minimizeButton = new Button(0, 0, 46, Height);
                _minimizeButton.Text = EditorAssets.SegMDL2Icons.ChromeMinimize;
                _minimizeButton.Font = iconFont;
                _minimizeButton.TextColor = Style.Current.Foreground;
                _minimizeButton.BackgroundColor = Color.Transparent;
                _minimizeButton.BackgroundColorHighlighted = Style.Current.LightBackground.RGBMultiplied(0.8f);
                _minimizeButton.BorderColor = Color.Transparent;
                _minimizeButton.BorderColorHighlighted = Color.Transparent;
                _minimizeButton.BorderColorSelected = Color.Transparent;
                _minimizeButton.Parent = this;
                _minimizeButton.Clicked += () =>
                {
                    _window.Minimize();
                };

                _maximizeButton = new Button(0, 0, 46, Height);
                _maximizeButton.Text = _window.IsMaximized
                    ? EditorAssets.SegMDL2Icons.ChromeRestore
                    : EditorAssets.SegMDL2Icons.ChromeMaximize;
                _maximizeButton.TextColor = Style.Current.Foreground;
                _maximizeButton.Font = iconFont;
                _maximizeButton.BackgroundColor = Color.Transparent;
                _maximizeButton.BackgroundColorHighlighted = Style.Current.LightBackground.RGBMultiplied(0.8f);
                _maximizeButton.BorderColor = Color.Transparent;
                _maximizeButton.BorderColorHighlighted = Color.Transparent;
                _maximizeButton.BorderColorSelected = Color.Transparent;
                _maximizeButton.Parent = this;
                _maximizeButton.Clicked += () =>
                {
                    if (_window.IsMaximized)
                    {
                        _window.Restore();
                    }
                    else
                    {
                        _window.Maximize();
                    }
                };
            }
            else
            {
                BackgroundColor = Style.Current.LightBackground;
            }
        }

        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            if (_maximizeButton != null)
            {
                _maximizeButton.Text = _window.IsMaximized
                    ? EditorAssets.SegMDL2Icons.ChromeRestore
                    : EditorAssets.SegMDL2Icons.ChromeMaximize;
            }
        }

        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="text">The button text.</param>
        /// <returns>Created button control.</returns>
        public MainMenuButton AddButton(string text)
        {
            return AddChild(new MainMenuButton(text));
        }

        /// <summary>
        /// Gets the button.
        /// </summary>
        /// <param name="text">The button text.</param>
        /// <returns>The button or null if missing.</returns>
        public MainMenuButton GetButton(string text)
        {
            MainMenuButton result = null;
            for (int i = 0; i < ChildrenCount; i++)
            {
                Control control = Children[i];
                MainMenuButton button = control as MainMenuButton;
                if (button != null && button.Text == text)
                {
                    result = button;
                    break;
                }
            }
            return result;
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            if (base.OnMouseDoubleClick(location, button))
            {
                return true;
            }

            Control child = GetChildAtRecursive(location);
            if (_useCustomWindowSystem && child is not Button && child is not MainMenuButton)
            {
                if (_window.IsMaximized)
                {
                    _window.Restore();
                }
                else
                {
                    _window.Maximize();
                }
            }

            return true;
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (base.OnKeyDown(key))
            {
                return true;
            }

            // Fallback to the edit window for shortcuts
            return false;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            base.OnDestroy();

            if (_window != null)
            {
                OnWindowClosed();
            }
        }

        /// <inheritdoc />
        protected override void PerformLayoutAfterChildren()
        {
            float x = 0;

            if (_useCustomWindowSystem)
            {
                // Icon
                _icon.X = x;
                _icon.Size = new Float2(Height);
                x += _icon.Width;
            }

            // Arrange controls
            MainMenuButton rightMostButton = null;
            for (int i = 0; i < m_Children.Count; i++)
            {
                Control c = m_Children[i];
                MainMenuButton b = c as MainMenuButton;
                if (b != null && c.Visible)
                {
                    b.Bounds = new Rectangle(x, 0, b.Width, Height);

                    if (rightMostButton == null)
                    {
                        rightMostButton = b;
                    }
                    else if (rightMostButton.X < b.X)
                    {
                        rightMostButton = b;
                    }

                    x += b.Width;
                }
            }

            if (_useCustomWindowSystem)
            {
                // Buttons
                float width = _closeButton.Width;
                _closeButton.Height = Height;
                _closeButton.X = Width - width;
                _maximizeButton.Height = Height;
                _maximizeButton.X = _closeButton.X - _maximizeButton.Width;
                _minimizeButton.Height = Height;
                _minimizeButton.X = _maximizeButton.X - _minimizeButton.Width;

                // Title
                _title.Bounds = new Rectangle(x + 2, 0, _minimizeButton.Left - x - 4, Height);
                _title.Text = _window.Title;
            }
        }

        private WindowHitCodes OnWindowHitTest(ref Float2 mousePosition)
        {
            Float2 pos = mousePosition / _window.DpiScale;
            return OnHitTest(pos);
        }
    }

    public class MainMenuButton : Control
    {
        /// <summary>
        /// The button text.
        /// </summary>
        public string Text;

        /// <summary>
        /// The context menu.
        /// </summary>
        public ContextMenu ContextMenu = null;

        /// <summary>
        /// The background color when mouse is over.
        /// </summary>
        public Color BackgroundColorMouseOver = Color.Transparent;

        /// <summary>
        /// The background color when mouse is over and context menu is opened.
        /// </summary>
        public Color BackgroundColorMouseOverOpened = Color.Transparent;

        /// <summary>
        /// Initializes a new instance of the <see cref="MainMenuButton"/> class.
        /// </summary>
        /// <param name="text">The text.</param>
        public MainMenuButton(string text) : base(0, 0, 32, 16)
        {
            ContextMenu = new ContextMenu();

            Text = text;

            Style style = Style.Current;
            if (true)
            {
                BackgroundColorMouseOver = style.BackgroundHighlighted;
                BackgroundColorMouseOverOpened = style.Background;
            }
            else
            {
                BackgroundColorMouseOver = BackgroundColorMouseOverOpened = style.LightBackground * 1.3f;
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            // Cache data
            Style style = Style.Current;
            Rectangle clientRect = new Rectangle(0, 0, Width, Height);
            bool hasChildItems = ContextMenu.HasChildren;
            bool isOpened = ContextMenu.IsOpened;
            bool enabled = EnabledInHierarchy;

            // Draw background
            if (enabled && hasChildItems && (isOpened || IsMouseOver))
            {
                Render2D.FillRectangle(clientRect,
                    isOpened ? BackgroundColorMouseOverOpened : BackgroundColorMouseOver);
            }

            // Draw text
            Render2D.DrawText(style.FontMedium, Text, clientRect,
                enabled && hasChildItems ? style.Foreground : style.ForegroundDisabled,
                TextAlignment.Center, TextAlignment.Center);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            Focus();

            MainMenu mainMenu = Parent as MainMenu;
            if (mainMenu != null)
            {
                mainMenu.Selected = this;
            }

            return true;
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            base.OnMouseEnter(location);

            MainMenu mainMenu = Parent as MainMenu;
            if (mainMenu != null && mainMenu.Selected != null)
            {
                mainMenu.Selected = this;
            }
        }

        /// <inheritdoc />
        public override void PerformLayout(bool force = false)
        {
            Style style = Style.Current;
            float width = 18;

            if (style.FontMedium != null)
            {
                width += style.FontMedium.MeasureText(Text, TextLayoutOptions.Default).X;
            }

            Width = width;
        }
    }
}

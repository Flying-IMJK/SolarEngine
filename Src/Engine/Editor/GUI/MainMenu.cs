
using SE.GUI;

namespace SE.Editor.GUI
{
    public class MainMenu : ContainerControl
    {
        private Window.HitTestDelegate m_HitTestCall;
        private bool m_UseCustomWindowSystem = false;
        private Image m_Icon = null;
        private Label m_Title = null;
        private Button m_CloseButton = null;
        private Button m_MinimizeButton = null;
        private Button m_MaximizeButton = null;
        private Window m_Window = null;
        private MainMenuButton m_Selected = null;

        private void OnSelectedContextMenuVisibleChanged(Control control)
        {
            if (m_Selected != null)
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
            if (m_Window != null)
            {
                m_Window.HitTest = null;
                m_Window = null;
            }
        }

        private WindowHitCodes OnHitTest(Float2 mouse)
        {
            float dpiScale = m_Window.DpiScale;

            if (m_Window.IsMinimized)
            {
                return WindowHitCodes.NoWhere;
            }

            if (!m_Window.IsMaximized)
            {
                Float2 pos = m_Window.ScreenToClient(mouse * dpiScale); // pos is not DPI adjusted
                Float2 winSize = m_Window.Size;

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
            bool isMouseOverSth = controlUnderMouse != null && controlUnderMouse != m_Title;

            MainMenuButton rb = GetRightButton();
            if (rb == null)
            {
                Float2 upperLeft = m_Title.UpperLeft;
                Rectangle rectangle = new Rectangle(upperLeft,
                    new Float2(m_MinimizeButton.BottomLeft.X - upperLeft.X, m_Title.Height));
                if (rectangle.Contains(mousePos) && !isMouseOverSth)
                {
                    return WindowHitCodes.Caption;
                }
            }
            else if (m_MinimizeButton != null &&
                new Rectangle(rb.UpperRight, m_MinimizeButton.BottomLeft - rb.UpperRight).Contains(mousePos) &&
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
            get => m_Selected;
            set
            {
                if (m_Selected == value)
                {
                    return;
                }

                if (m_Selected != null)
                {
                    m_Selected.ContextMenu.VisibleChanged -= OnSelectedContextMenuVisibleChanged;
                    m_Selected.ContextMenu.Hide();
                }

                m_Selected = value;

                if (m_Selected != null && m_Selected.ContextMenu.HasChildren)
                {
                    m_Selected.ContextMenu.Show(m_Selected, new Float2(0, m_Selected.Height));
                    m_Selected.ContextMenu.VisibleChanged += OnSelectedContextMenuVisibleChanged;
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

            m_UseCustomWindowSystem = true;
            if (m_UseCustomWindowSystem)
            {
                BackgroundColor = Style.Current.LightBackground;
                Height = 28;

                Texture windowIcon = AssetContent.LoadAsyncInternal<Texture>(EditorAssets.WindowIcon);
                FontAsset windowIconsFont = AssetContent.LoadAsyncInternal<FontAsset>(EditorAssets.WindowIconsFont);

                windowIcon.WaitForLoaded();
                windowIconsFont.WaitForLoaded();

                Font iconFont = windowIconsFont.CreateFont(9);
                m_HitTestCall = OnWindowHitTest;

                m_Window = mainWindow.RootWindow().Window;
                m_Window.HitTest = m_HitTestCall;
                m_Window.Closed += OnWindowClosed;

                // ScriptsBuilder.GetBinariesConfiguration(...)
                m_Icon = new Image();
                m_Icon.Margin = new Margin(6, 6, 6, 6);
                m_Icon.Brush = new TextureBrush(windowIcon);
                // Preserve the engine texture's colors in both normal and hover states.
                m_Icon.Color = Color.White;
                m_Icon.MouseOverColor = Color.White;
                m_Icon.KeepAspectRatio = false;
                m_Icon.Parent = this;

                m_Title = new Label(0, 0, Width, Height);
                m_Title.Text = m_Window.Title;
                m_Title.HorizontalAlignment = TextAlignment.Center;
                m_Title.VerticalAlignment = TextAlignment.Center;
                m_Title.ClipText = true;
                m_Title.TextColor = Style.Current.ForegroundGrey;
                m_Title.TextColorHighlighted = Style.Current.ForegroundGrey;
                m_Title.Parent = this;

                m_CloseButton = new Button(0, 0, 46, Height);
                m_CloseButton.Text = EditorAssets.SegMDL2Icons.ChromeClose;
                m_CloseButton.Font = iconFont;
                m_CloseButton.TextColor = Style.Current.Foreground;
                m_CloseButton.BackgroundColor = Color.Transparent;
                m_CloseButton.BackgroundColorHighlighted = Color.Red;
                m_CloseButton.BackgroundColorSelected = Color.Red.RGBMultiplied(1.3f);
                m_CloseButton.BorderColor = Color.Transparent;
                m_CloseButton.BorderColorHighlighted = Color.Transparent;
                m_CloseButton.BorderColorSelected = Color.Transparent;
                m_CloseButton.Parent = this;
                m_CloseButton.Clicked += () =>
                {
                    m_Window.Close(SE.ClosingReason.User);
                };

                m_MinimizeButton = new Button(0, 0, 46, Height);
                m_MinimizeButton.Text = EditorAssets.SegMDL2Icons.ChromeMinimize;
                m_MinimizeButton.Font = iconFont;
                m_MinimizeButton.TextColor = Style.Current.Foreground;
                m_MinimizeButton.BackgroundColor = Color.Transparent;
                m_MinimizeButton.BackgroundColorHighlighted = Style.Current.LightBackground.RGBMultiplied(0.8f);
                m_MinimizeButton.BorderColor = Color.Transparent;
                m_MinimizeButton.BorderColorHighlighted = Color.Transparent;
                m_MinimizeButton.BorderColorSelected = Color.Transparent;
                m_MinimizeButton.Parent = this;
                m_MinimizeButton.Clicked += () =>
                {
                    m_Window.Minimize();
                };

                m_MaximizeButton = new Button(0, 0, 46, Height);
                m_MaximizeButton.Text = m_Window.IsMaximized
                    ? EditorAssets.SegMDL2Icons.ChromeRestore
                    : EditorAssets.SegMDL2Icons.ChromeMaximize;
                m_MaximizeButton.TextColor = Style.Current.Foreground;
                m_MaximizeButton.Font = iconFont;
                m_MaximizeButton.BackgroundColor = Color.Transparent;
                m_MaximizeButton.BackgroundColorHighlighted = Style.Current.LightBackground.RGBMultiplied(0.8f);
                m_MaximizeButton.BorderColor = Color.Transparent;
                m_MaximizeButton.BorderColorHighlighted = Color.Transparent;
                m_MaximizeButton.BorderColorSelected = Color.Transparent;
                m_MaximizeButton.Parent = this;
                m_MaximizeButton.Clicked += () =>
                {
                    if (m_Window.IsMaximized)
                    {
                        m_Window.Restore();
                    }
                    else
                    {
                        m_Window.Maximize();
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

            if (m_MaximizeButton != null)
            {
                m_MaximizeButton.Text = m_Window.IsMaximized
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
            if (m_UseCustomWindowSystem && child is not Button && child is not MainMenuButton)
            {
                if (m_Window.IsMaximized)
                {
                    m_Window.Restore();
                }
                else
                {
                    m_Window.Maximize();
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

            if (m_Window != null)
            {
                OnWindowClosed();
            }
        }

        /// <inheritdoc />
        protected override void PerformLayoutAfterChildren()
        {
            float x = 0;

            if (m_UseCustomWindowSystem)
            {
                // Icon
                m_Icon.X = x;
                m_Icon.Size = new Float2(Height);
                x += m_Icon.Width;
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

            if (m_UseCustomWindowSystem)
            {
                // Buttons
                float width = m_CloseButton.Width;
                m_CloseButton.Height = Height;
                m_CloseButton.X = Width - width;
                m_MaximizeButton.Height = Height;
                m_MaximizeButton.X = m_CloseButton.X - m_MaximizeButton.Width;
                m_MinimizeButton.Height = Height;
                m_MinimizeButton.X = m_MaximizeButton.X - m_MinimizeButton.Width;

                // Title
                m_Title.Bounds = new Rectangle(x + 2, 0, m_MinimizeButton.Left - x - 4, Height);
                m_Title.Text = m_Window.Title;
            }
        }

        private WindowHitCodes OnWindowHitTest(ref Float2 mousePosition)
        {
            Float2 pos = mousePosition / m_Window.DpiScale;
            return OnHitTest(pos);
        }
    }
}

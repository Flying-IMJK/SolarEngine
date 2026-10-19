namespace SE.GUI
{
    public class Tooltip : ContainerControl
    {
        private float m_TimeToPopupLeft;
        private Control m_LastTarget;
        private Control m_ShowTarget;
        private string m_CurrentText = string.Empty;
        private SE.Window m_Window;

        private void UpdateWindowSize()
        {
            if (m_Window != null)
            {
                m_Window.ClientSize = Size * m_Window.DpiScale;
            }
        }

        private void WrapPosition(Float2 locationSS, float flipOffset = 0.0f)
        {
            if (m_ShowTarget == null || m_ShowTarget.RootWindow() == null)
            {
                return;
            }

            // Calculate popup direction
            float dpiScale = m_ShowTarget.RootWindow().Window.DpiScale;
            Float2 dpiSize = Size * dpiScale;
            Rectangle monitorBounds = Platform.GetMonitorBounds(locationSS);
            Float2 rightBottomMonitorBounds = monitorBounds.BottomRight;
            Float2 rightBottomLocationSS = locationSS + dpiSize;

            // Prioritize tooltip placement within parent window, fall back to virtual desktop
            if (rightBottomMonitorBounds.Y < rightBottomLocationSS.Y)
            {
                // Direction: up
                locationSS.Y -= dpiSize.Y + flipOffset;
            }

            if (rightBottomMonitorBounds.X < rightBottomLocationSS.X)
            {
                // Direction: left
                locationSS.X -= dpiSize.X + flipOffset * 2;
            }
        }

        /// <summary>
        /// Gets or sets the time in seconds that mouse have to be over the target to show the tooltip.
        /// </summary>
        public float TimeToShow = 0.3f; // 300ms by default

        /// <summary>
        /// Gets or sets the maximum width of the tooltip. Used to wrap text that overflows and ensure that tooltip
        /// stays readable.
        /// </summary>
        public float MaxWidth = 500.0f;

        /// <summary>
        /// Gets the tooltip window.
        /// </summary>
        public SE.Window Window()
        {
            return m_Window;
        }

        /// <summary>
        /// Shows tooltip over given control.
        /// </summary>
        /// <param name="target">The parent control to attach to it.</param>
        /// <param name="location">Popup menu origin location in parent control coordinates.</param>
        /// <param name="targetArea">Tooltip target area of interest.</param>
        public void Show(Control target, Float2 location, Rectangle targetArea)
        {
            SE.Debug.Assert(target != null, "Invalid tooltip target.");

            // Ensure to be closed
            Hide();

            // Block showing tooltips when application is not focused
            if (!Platform.HasFocus)
            {
                return;
            }

            // Unlock and perform controls update
            UnlockChildrenRecursive();
            PerformLayout();

            // Calculate popup direction and initial location
            RootControl parentWin = target.Root;
            if (parentWin == null)
            {
                return;
            }

            float dpiScale = target.RootWindow().Window.DpiScale;
            Float2 dpiSize = Size * dpiScale;
            Float2 locationWS = target.PointToWindow(location);
            Float2 locationSS = parentWin.PointToScreen(locationWS);
            m_ShowTarget = target;
            WrapPosition(locationSS);

            // Create window
            CreateWindowSettings desc = SE.Window.CreateDefaultSettings();
            desc.StartPosition = WindowStartPosition.Manual;
            desc.Position = locationSS;
            desc.Size = dpiSize;
            desc.Fullscreen = false;
            desc.HasBorder = false;
            desc.SupportsTransparency = false;
            desc.ShowInTaskbar = false;
            desc.ActivateWhenFirstShown = false;
            desc.AllowInput = false;
            desc.AllowMinimize = false;
            desc.AllowMaximize = false;
            desc.AllowDragAndDrop = false;
            desc.IsTopmost = true;
            desc.IsRegularWindow = false;
            desc.HasSizingFrame = false;
            desc.ShowAfterFirstPaint = true;
            m_Window = SE.Window.Create(desc);
            SE.Debug.Assert(m_Window != null, "Failed to create tooltip window.");

            // Attach to the window and focus
            Parent = m_Window.GUI;
            Visible = true;
            m_Window.Show();
            m_ShowTarget.OnTooltipShown(this);
        }

        /// <summary>
        /// Hides the popup.
        /// </summary>
        public void Hide()
        {
            if (!Visible)
            {
                return;
            }

            // Unlink
            IsLayoutLocked = true;
            Parent = null;

            // Close window
            if (m_Window != null)
            {
                SE.Window window = m_Window;
                m_Window = null;
                window.Close();
            }

            // Hide
            Visible = false;
        }

        /// <summary>
        /// Called when mouse enters a control.
        /// </summary>
        /// <param name="target">The target.</param>
        public void OnMouseEnterControl(Control target)
        {
            m_LastTarget = target;
            m_TimeToPopupLeft = TimeToShow;
        }

        /// <summary>
        /// Called when mouse is over a control.
        /// </summary>
        /// <param name="target">The target.</param>
        /// <param name="dt">The delta time.</param>
        public void OnMouseOverControl(Control target, float dt)
        {
            if (!Visible && m_TimeToPopupLeft > 0.0f)
            {
                m_LastTarget = target;
                m_TimeToPopupLeft -= dt;

                if (m_TimeToPopupLeft <= 0.0f)
                {
                    Float2 location;
                    Rectangle area;
                    if (m_LastTarget.OnShowTooltip(m_CurrentText, out location, out area))
                    {
                        Show(m_LastTarget, location, area);
                    }
                }
            }
        }

        /// <summary>
        /// Called when mouse leaves a control.
        /// </summary>
        /// <param name="target">The target.</param>
        public void OnMouseLeaveControl(Control target)
        {
            if (Visible)
            {
                Hide();
            }

            m_LastTarget = null;
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            Float2 mousePos = Input.MouseScreenPosition;
            Float2 location = m_ShowTarget.PointFromScreen(mousePos);
            if (!m_ShowTarget.OnTestTooltipOverControl(location))
            {
                // Auto hide if mouse leaves control area
                Hide();
            }
            else
            {
                // Position tooltip when mouse moves
                WrapPosition(mousePos, 10);
                if (m_Window != null)
                {
                    m_Window.Position = mousePos + new Float2(15, 10);
                }
            }

            base.Update(deltaTime);
        }

        /// <inheritdoc />
        public override void Draw()
        {
            Style style = Style.Current;

            // Background
            Render2D.FillRectangle(new Rectangle(Float2.Zero, Size), Color.Lerp(style.BackgroundSelected, style
                .Background, 0.6f));
            Render2D.FillRectangle(new Rectangle(1.1f, 1.1f, Width - 2, Height - 2), style.Background);

            // Padding for text
            Rectangle textRect = GetClientArea();
            textRect.X += 5;
            textRect.Width -= 10;

            TextLayoutOptions layout = TextLayoutOptions.Default;
            layout.Bounds = textRect;
            layout.HorizontalAlignment = TextAlignment.Center;
            layout.VerticalAlignment = TextAlignment.Center;
            layout.TextWrapping = TextWrapping.WrapWords;
            layout.Scale = 1.0f;
            layout.BaseLinesGapScale = 1.0f;

            // Tooltip text
            Render2D.RenderText(style.FontMedium, m_CurrentText, style.Foreground, layout);
        }

        /// <inheritdoc />
        public override bool OnShowTooltip(string text, out Float2 location, out Rectangle area)
        {
            base.OnShowTooltip(text, out location, out area);

            // It's better not to show tooltip for a tooltip.
            // It would be kind of tooltipness.
            return false;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            Hide();

            base.OnDestroy();
        }

        public Tooltip() : base(0, 0, 300, 24)
        {
            Visible = false;
            AutoFocus = false;
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();

            Float2 prevSize = Size;
            Style style = Style.Current;

            // Calculate size of the tooltip
            Float2 size = Float2.Zero;
            if (style != null && style.FontMedium != null && m_CurrentText.Length > 0)
            {
                TextLayoutOptions layout = TextLayoutOptions.Default;
                layout.Bounds = new Rectangle(0, 0, MaxWidth, 10000000);
                layout.HorizontalAlignment = TextAlignment.Center;
                layout.VerticalAlignment = TextAlignment.Center;
                layout.TextWrapping = TextWrapping.WrapWords;
                FontLineCache[] items = style.FontMedium.ProcessText(m_CurrentText, layout);
                for (int i = 0; i < items.Length; i++)
                {
                    FontLineCache item = items[i];
                    float itemWidth = item.Size.X + 8.0f;
                    size.X = size.X < itemWidth ? itemWidth : size.X;
                    size.Y += item.Size.Y;
                }
                //size.X += style.FontMedium.MeasureText(_currentText).X;
            }

            Size = size + new Float2(24.0f);

            // Check if is visible size get changed
            if (Visible && prevSize != Size)
            {
                UpdateWindowSize();
            }
        }
    }
}

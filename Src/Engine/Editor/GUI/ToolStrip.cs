#nullable disable annotations

using System;
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Tool strip with child items.
    /// </summary>
    public class ToolStrip : ContainerControl
    {
        /// <summary>
        /// The default margin vertically.
        /// </summary>
        public const int DefaultMarginV = 1;

        /// <summary>
        /// The default margin horizontally.
        /// </summary>
        public const int DefaultMarginH = 2;

        /// <summary>
        /// Event fired when button gets clicked with the primary mouse button.
        /// </summary>
        public Action<ToolStripButton> ButtonClicked;

        /// <summary>
        /// Event fired when button gets clicked with the secondary mouse button.
        /// </summary>
        public Action<ToolStripButton> SecondaryButtonClicked;

        /// <summary>
        /// Tries to get the last button.
        /// </summary>
        public ToolStripButton LastButton
        {
            get
            {
                for (int i = m_Children.Count - 1; i >= 0; i--)
                {
                    ToolStripButton button = m_Children[i] as ToolStripButton;
                    if (button != null)
                    {
                        return button;
                    }
                }

                return null;
            }
        }

        /// <summary>
        /// Gets amount of buttons that has been added
        /// </summary>
        public int ButtonsCount
        {
            get
            {
                int result = 0;
                for (int i = 0; i < m_Children.Count; i++)
                {
                    if (m_Children[i] is ToolStripButton)
                    {
                        result++;
                    }
                }

                return result;
            }
        }

        /// <summary>
        /// Gets the height for the items.
        /// </summary>
        public float ItemsHeight => Height - 2 * DefaultMarginV;

        /// <summary>
        /// Initializes a new instance of the <see cref="ToolStrip"/> class.
        /// </summary>
        /// <param name="height">The toolstrip height.</param>
        /// <param name="y">The toolstrip Y position.</param>
        public ToolStrip(float height, float y = 0)
        {
            AutoFocus = false;
            AnchorPreset = AnchorPresets.HorizontalStretchTop;
            BackgroundColor = Style.Current.LightBackground;
            Offsets = new Margin(0, 0, y, height);
        }

        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="sprite">The icon sprite.</param>
        /// <param name="onClick">The custom action to call on button clicked.</param>
        /// <returns>The button.</returns>
        public ToolStripButton AddButton(SpriteHandle sprite, Action onClick = null)
        {
            ToolStripButton button = new ToolStripButton(ItemsHeight, sprite);
            button.Parent = this;

            if (onClick != null)
            {
                button.Clicked += onClick;
            }

            return button;
        }

        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="sprite">The icon sprite.</param>
        /// <param name="text">The text.</param>
        /// <param name="onClick">The custom action to call on button clicked.</param>
        /// <returns>The button.</returns>
        public ToolStripButton AddButton(SpriteHandle sprite, string text, Action onClick = null)
        {
            ToolStripButton button = new ToolStripButton(ItemsHeight, sprite);
            button.Text = text;
            button.Parent = this;

            if (onClick != null)
            {
                button.Clicked += onClick;
            }

            return button;
        }

        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="onClick">The custom action to call on button clicked.</param>
        /// <returns>The button.</returns>
        public ToolStripButton AddButton(string text, Action onClick = null)
        {
            ToolStripButton button = new ToolStripButton(ItemsHeight, SpriteHandle.Invalid);
            button.Text = text;
            button.Parent = this;

            if (onClick != null)
            {
                button.Clicked += onClick;
            }

            return button;
        }

        /// <summary>
        /// Adds the separator.
        /// </summary>
        /// <returns>The separator.</returns>
        public ToolStripSeparator AddSeparator()
        {
            return AddChild(new ToolStripSeparator(ItemsHeight));
        }

        public void OnButtonClicked(ToolStripButton button)
        {
            if (ButtonClicked != null)
            {
                ButtonClicked(button);
            }
        }

        public void OnSecondaryButtonClicked(ToolStripButton button)
        {
            if (SecondaryButtonClicked != null)
            {
                SecondaryButtonClicked(button);
            }
        }

        /// <inheritdoc />
        public override void OnChildResized(Control control)
        {
            base.OnChildResized(control);

            PerformLayout();
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (base.OnKeyDown(key))
            {
                return true;
            }

            // Fallback to the owning window for shortcuts
            EditorWindow editorWindow = null;
            ContainerControl c = Parent;
            while (c != null && editorWindow == null)
            {
                editorWindow = c as EditorWindow;
                c = c.Parent;
            }

            return false;
            /*var editor = Editor.Instance;
            if (editorWindow == null)
            {
                editorWindow = editor.Windows.EditWin; // Fallback to main editor window
            }
            return editorWindow.InputActions.Process(editor, this, key);*/
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            // Arrange controls
            float x = DefaultMarginH;
            float h = ItemsHeight;
            for (int i = 0; i < m_Children.Count; i++)
            {
                Control c = m_Children[i];
                if (c.Visible)
                {
                    float w = c.Width;
                    c.Bounds = new Rectangle(x, DefaultMarginV, w, h);
                    x += w + DefaultMarginH;
                }
            }
        }
    }

    /// <summary>
    /// Tool strip button control.
    /// </summary>
    public class ToolStripButton : Control
    {
        private SpriteHandle m_Icon;
        private string m_Text = string.Empty;
        private bool m_PrimaryMouseDown;
        private bool m_SecondaryMouseDown;

        /// <summary>
        /// The default margin for button parts (icon, text, etc.).
        /// </summary>
        public const int DefaultMargin = 2;

        /// <summary>
        /// Event fired when user clicks the button.
        /// </summary>
        public Action Clicked;

        /// <summary>
        /// Event fired when user clicks the button.
        /// </summary>
        public Action SecondaryClicked;

        /// <summary>
        /// The checked state.
        /// </summary>
        public bool Checked;

        /// <summary>
        /// The automatic check mode.
        /// </summary>
        public bool AutoCheck;

        /// <summary>
        /// Gets or sets the button text.
        /// </summary>
        public string Text
        {
            get
            {
                return m_Text;
            }
            set
            {
                m_Text = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// The icon.
        /// </summary>
        public SpriteHandle Icon
        {
            get
            {
                return m_Icon;
            }
            set
            {
                m_Icon = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// A reference to a context menu to raise when the secondary mouse button is pressed.
        /// </summary>
        public ContextMenu ContextMenu;

        /// <summary>
        /// Initializes a new instance of the <see cref="ToolStripButton"/> class.
        /// </summary>
        /// <param name="height">The height.</param>
        /// <param name="icon">The icon.</param>
        public ToolStripButton(float height, SpriteHandle icon) : base(0, 0, height, height)
        {
            m_Icon = icon;
        }

        /// <summary>
        /// Sets the automatic check mode.
        /// </summary>
        /// <param name="value">True if use auto check, otherwise false.</param>
        /// <returns>This button.</returns>
        public ToolStripButton SetAutoCheck(bool value)
        {
            AutoCheck = value;
            return this;
        }

        /// <summary>
        /// Sets the checked state.
        /// </summary>
        /// <param name="value">True if check it, otherwise false.</param>
        /// <returns>This button.</returns>
        public ToolStripButton SetChecked(bool value)
        {
            Checked = value;
            return this;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // Cache data
            Style style = Style.Current;
            float iconSize = Height - DefaultMargin;
            Rectangle clientRect = new Rectangle(Float2.Zero, Size);
            Rectangle iconRect = new Rectangle(DefaultMargin, DefaultMargin, iconSize, iconSize);
            Rectangle textRect = new Rectangle(DefaultMargin, 0, 0, Height);
            bool enabled = EnabledInHierarchy;
            bool mouseButtonDown = m_PrimaryMouseDown || m_SecondaryMouseDown;

            // Draw background
            if (enabled && (IsMouseOver || IsNavFocused || Checked))
            {
                Color color = style.Background * 1.3f;
                if (Checked)
                {
                    color = style.BackgroundSelected;
                }
                else if (mouseButtonDown)
                {
                    color = style.BackgroundHighlighted;
                }

                Render2D.FillRectangle(clientRect, color);
            }

            // Draw icon
            if (m_Icon.IsValid)
            {
                Render2D.DrawSprite(m_Icon, iconRect, enabled ? style.Foreground : style.ForegroundDisabled);
                textRect.Location.X += iconSize + DefaultMargin;
            }

            // Draw text
            if (m_Text.Length > 0)
            {
                textRect.Size.X = Width - DefaultMargin - textRect.Left;
                Render2D.RenderText(style.FontMedium, m_Text, textRect, enabled ? style.TextColor : style.ForegroundDisabled,
                    TextAlignment.Near, TextAlignment.Center);
            }
        }

        /// <inheritdoc />
        public override void PerformLayout(bool force = false)
        {
            Style style = Style.Current;
            float iconSize = Height - DefaultMargin;
            bool hasSprite = m_Icon.IsValid;
            float width = DefaultMargin * 2;

            if (hasSprite)
            {
                width += iconSize;
            }

            if (m_Text.Length > 0 && style != null && style.FontMedium != null)
            {
                width += style.FontMedium.MeasureText(m_Text, TextLayoutOptions.Default).X + (hasSprite ? DefaultMargin : 0);
            }

            Width = width;
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                m_PrimaryMouseDown = true;
                Focus();
                return true;
            }

            if (button == MouseButton.Right)
            {
                m_SecondaryMouseDown = true;
                Focus();
                return true;
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && m_PrimaryMouseDown)
            {
                m_PrimaryMouseDown = false;
                if (AutoCheck)
                {
                    Checked = !Checked;
                }

                if (Clicked != null)
                {
                    Clicked();
                }

                ToolStrip toolStrip = Parent as ToolStrip;
                if (toolStrip != null)
                {
                    toolStrip.OnButtonClicked(this);
                }

                return true;
            }

            if (button == MouseButton.Right && m_SecondaryMouseDown)
            {
                m_SecondaryMouseDown = false;
                if (SecondaryClicked != null)
                {
                    SecondaryClicked();
                }

                ToolStrip toolStrip = Parent as ToolStrip;
                if (toolStrip != null)
                {
                    toolStrip.OnButtonClicked(this);
                }

                if (ContextMenu != null)
                {
                    ContextMenu.Show(this, new Float2(0, Height));
                }

                return true;
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            m_PrimaryMouseDown = false;
            m_SecondaryMouseDown = false;

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            m_PrimaryMouseDown = false;
            m_SecondaryMouseDown = false;

            base.OnLostFocus();
        }
    }

    /// <summary>
    /// ToolStrip separator control.
    /// </summary>
    public class ToolStripSeparator : Control
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ToolStripSeparator"/> class.
        /// </summary>
        /// <param name="height">The height.</param>
        public ToolStripSeparator(float height) : base(0, 0, 4, height)
        {
            AutoFocus = false;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // Draw the separator line
            Render2D.FillRectangle(new Rectangle((Width - 4) / 2, 2, 1, Height - 4), Style.Current.LightBackground * 1.3f);
        }
    }
}

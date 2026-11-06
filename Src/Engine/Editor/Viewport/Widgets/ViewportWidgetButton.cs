#nullable disable annotations

using System;
using SE.Editor.GUI;
using SE.GUI;

namespace SE.Editor
{
    public class ViewportWidgetButton : Control
    {
        private string m_Text;
        private ContextMenu m_Cm;
        private bool m_AutoCheck;
        private bool m_IsMosueDown;
        private float m_ForcedTextWidth;

        /// <summary>
        /// Event fired when user toggles checked state.
        /// </summary>
        public event Action<ViewportWidgetButton> Toggled;

        /// <summary>
        /// Event fired when user click the button.
        /// </summary>
        public event Action<ViewportWidgetButton> Clicked;

        /// <summary>
        /// The icon.
        /// </summary>
        public SpriteHandle Icon;

        /// <summary>Gets the popup menu. The caller owns its lifetime.</summary>
        public ContextMenu ContextMenu => m_Cm;

        /// <summary>
        /// Gets or sets the text.
        /// </summary>
        public string Text
        {
            get => m_Text;
            set
            {
                m_Text = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether this <see cref="ViewportWidgetButton"/> is checked.
        /// </summary>
        public bool Checked;

        /// <summary>
        /// Initializes a new instance of the <see cref="ViewportWidgetButton"/> class.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="icon">The icon.</param>
        /// <param name="contextMenu">The context menu.</param>
        /// <param name="autoCheck">If set to <c>true</c> will be automatic checked on mouse click.</param>
        /// <param name="textWidth">Forces the text to be drawn with the specified width.</param>
        public ViewportWidgetButton(string text, SpriteHandle icon, ContextMenu contextMenu = null, bool autoCheck =
            false, float textWidth = 0.0f)
            : base(0, 0, CalculateButtonWidth(textWidth, icon.IsValid, contextMenu != null), ViewportWidgetsContainer.WidgetsHeight)
        {
            m_Text = text;
            Icon = icon;
            m_Cm = contextMenu;
            m_AutoCheck = autoCheck;
            m_ForcedTextWidth = textWidth;
            AutoFocus = false;

            if (m_Cm != null)
            {
                m_Cm.VisibleChanged += CmOnVisibleChanged;
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            // Cache data
            Style style = Style.Current;
            float iconSize = ViewportWidgetsContainer.WidgetsIconSize;
            Rectangle clientRect = new Rectangle(Float2.Zero, Size);
            Rectangle iconRect = new Rectangle(4, (Height - iconSize) / 2, iconSize, iconSize);
            Rectangle textRect = new Rectangle(4, 0, Width - 8 - (m_Cm != null ? 12 : 0), Height);
            Color foreground = EnabledInHierarchy ? style.ForegroundViewport : style.ForegroundDisabled;

            // Keep the active tool highlighted independently of the hover state.
            if (Checked)
            {
                Render2D.FillRectangle(clientRect, style.BackgroundSelected * (IsMouseOver ? 0.9f : 0.6f));
            }
            else if (EnabledInHierarchy && IsMouseOver)
            {
                Render2D.FillRectangle(clientRect, style.BackgroundHighlighted);
            }

            // Check if has icon
            if (Icon.IsValid)
            {
                // Draw icon
                Render2D.DrawSprite(Icon, iconRect, foreground);

                // Update text rectangle
                textRect.X += iconSize + 4;
                textRect.Width -= iconSize + 4;
            }

            if (!string.IsNullOrEmpty(m_Text))
            {
                Render2D.RenderText(style.FontMedium, m_Text, textRect, foreground,
                    TextAlignment.Center, TextAlignment.Center);
            }
            if (m_Cm != null)
            {
                Render2D.DrawSprite(EditorIcons.ArrowDown12, new Rectangle(Width - 12, (Height - 8) / 2, 8, 8), foreground);
            }
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button != MouseButton.Left)
            {
                return base.OnMouseDown(location, button);
            }
            m_IsMosueDown = true;
            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button != MouseButton.Left || !m_IsMosueDown)
            {
                return base.OnMouseUp(location, button);
            }
            m_IsMosueDown = false;
            if (m_AutoCheck)
            {
                // Toggle
                Checked = !Checked;
                Toggled?.Invoke(this);
            }

            if (m_Cm != null)
            {
                m_Cm.Show(this, new Float2(-1, Height + 2));
            }
            else
            {
                Clicked?.Invoke(this);
            }
            return true;
        }

        /// <inheritdoc />
        public override void PerformLayout(bool force = false)
        {
            Style style = Style.Current;

            if (style != null && style.FontMedium != null)
            {
                Width = CalculateButtonWidth(m_ForcedTextWidth > 0.0f ? m_ForcedTextWidth : style.FontMedium
                    .MeasureText(m_Text, TextLayoutOptions.Default).X,
                    Icon.IsValid, m_Cm != null);
            }
        }

        private void CmOnVisibleChanged(Control control)
        {
            if (m_Cm != null && !m_Cm.IsOpened)
            {
                if (Tag is EditorViewport viewport && !viewport.IsDisposed)
                {
                    viewport.Focus();
                }
                else if (HasParent && Parent.HasParent)
                {
                    // Focus viewport
                    Parent.Parent.Focus();
                }
            }
        }

        public override void OnMouseLeave()
        {
            m_IsMosueDown = false;
            base.OnMouseLeave();
        }

        public override void OnLostFocus()
        {
            m_IsMosueDown = false;
            base.OnLostFocus();
        }

        public override void OnDestroy()
        {
            if (m_Cm != null)
            {
                m_Cm.VisibleChanged -= CmOnVisibleChanged;
            }
            Clicked = null;
            Toggled = null;
            base.OnDestroy();
        }

        private static float CalculateButtonWidth(float textWidth, bool hasIcon, bool hasMenu)
        {
            return Mathf.Max(24, 8 + (hasIcon ? ViewportWidgetsContainer.WidgetsIconSize : 0) +
                (textWidth > 0 ? textWidth + (hasIcon ? 4 : 0) : 0) + (hasMenu ? 12 : 0));
        }
    }
}

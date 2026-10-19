#nullable disable annotations

using System;

namespace SE.GUI
{
    public class TextBox : TextBoxBase
    {
        /// <summary>
        /// Gets or sets the watermark text to show grayed when textbox is empty.
        /// </summary>
        public string WatermarkText
        {
            get => _watermarkText;
            set => _watermarkText = value;
        }

        /// <summary>
        /// The text case.
        /// </summary>
        public TextCaseOptions CaseOption = TextCaseOptions.None;

        /// <summary>
        /// Whether to bold the text.
        /// </summary>
        public bool Bold = false;

        /// <summary>
        /// Whether to italicize the text.
        /// </summary>
        public bool Italic = false;

        /// <summary>
        /// The vertical alignment of the text.
        /// </summary>
        public TextAlignment VerticalAlignment
        {
            get => m_Layout.VerticalAlignment;
            set => m_Layout.VerticalAlignment = value;
        }

        /// <summary>
        /// The vertical alignment of the text.
        /// </summary>
        public TextAlignment HorizontalAlignment
        {
            get => m_Layout.HorizontalAlignment;
            set => m_Layout.HorizontalAlignment = value;
        }

        /// <summary>
        /// Gets or sets the text wrapping within the control bounds.
        /// </summary>
        public TextWrapping Wrapping
        {
            get => m_Layout.TextWrapping;
            set => m_Layout.TextWrapping = value;
        }

        /// <summary>
        /// Gets or sets the font.
        /// </summary>
        public FontReference Font = new FontReference();

        /// <summary>
        /// Gets or sets the custom material used to render the text. It must has domain set to GUI and have a public
        /// texture parameter named Font used to sample font atlas texture with font characters data.
        /// </summary>
        public MaterialBase TextMaterial;

        /// <summary>
        /// Gets or sets the color of the text.
        /// </summary>
        public Color TextColor;

        /// <summary>
        /// Gets or sets the color of the text.
        /// </summary>
        public Color WatermarkTextColor;

        /// <summary>
        /// Gets or sets the color of the selection (Transparent if not used).
        /// </summary>
        public Color SelectionColor;

        public TextBox()
        {
        }

        /// <summary>
        /// Init
        /// </summary>
        /// <param name="isMultiline">Enable/disable multiline text input support</param>
        /// <param name="x">Position X coordinate</param>
        /// <param name="y">Position Y coordinate</param>
        /// <param name="width">Width</param>
        public TextBox(bool isMultiline, float x, float y, float width = 120) : base(isMultiline, x, y, width)
        {
            m_Layout = TextLayoutOptions.Default;
            m_Layout.VerticalAlignment = IsMultiline ? TextAlignment.Near : TextAlignment.Center;
            m_Layout.TextWrapping = TextWrapping.NoWrap;
            m_Layout.Bounds = new Rectangle(DefaultMargin, 1, Width - 2 * DefaultMargin, Height - 2);

            Style style = Style.Current;
            Font = new FontReference(style.FontMedium);
            TextColor = style.Foreground;
            WatermarkTextColor = style.ForegroundDisabled;
            SelectionColor = style.BackgroundSelected;
        }

        /// <inheritdoc />
        public override Float2 GetTextSize()
        {
            SE.Font font = GetFont();
            if (font == null)
            {
                return Float2.Zero;
            }

            return font.MeasureText(ConvertedText(), m_Layout);
        }

        /// <inheritdoc />
        public override Float2 GetCharPosition(int index, ref float height)
        {
            SE.Font font = GetFont();
            if (font == null)
            {
                height = Height;
                return Float2.Zero;
            }
            height = font.GetHeight() / GetDpiScale();
            return font.GetCharPosition(ConvertedText(), index, m_Layout);
        }

        /// <inheritdoc />
        public override int HitTestText(Float2 location)
        {
            SE.Font font = GetFont();
            if (font == null)
            {
                return 0;
            }

            return font.HitTestText(ConvertedText(), location, m_Layout);
        }

        /// <inheritdoc />
        public override void DrawSelf()
        {
            // Cache data
            Rectangle rect = new(Float2.Zero, Size);
            bool enabled = EnabledInHierarchy;
            SE.Font font = GetFont();
            if (font == null)
            {
                return;
            }

            // Background
            Color backColor = BackgroundColor;
            if (IsMouseOver || IsNavFocused)
            {
                backColor = BackgroundSelectedColor;
            }
            Render2D.FillRectangle(rect, backColor);
            if (HasBorder)
            {
                Render2D.DrawRectangle(rect, IsFocused ? BorderSelectedColor : BorderColor, BorderThickness);
            }

            // Apply view offset and clip mask
            if (ClipText)
            {
                Render2D.PushClip(GetTextClipRectangle());
            }

            bool useViewOffset = !_viewOffset.IsZero;
            if (useViewOffset)
            {
                Render2D.PushTransform(Matrix3x3.Translation2D(-_viewOffset));
            }

            string text = ConvertedText();

            // Check if sth is selected to draw selection
            if (HasSelection)
            {
                Float2 leftEdge = font.GetCharPosition(text, SelectionLeft, m_Layout);
                Float2 rightEdge = font.GetCharPosition(text, SelectionRight, m_Layout);
                float fontHeight = font.GetHeight();
                float textHeight = fontHeight / GetDpiScale();

                // Draw selection background
                float alpha = Mathf.Min(1.0f, MathF.Cos(_animateTime * BackgroundSelectedFlashSpeed) * 0.5f + 1.3f);
                alpha *= alpha;
                Color selectionColor = SelectionColor * alpha;
                int selectedLinesCount = 1 + Mathf.FloorToInt((rightEdge.Y - leftEdge.Y) / textHeight);
                if (selectedLinesCount == 1)
                {
                    // Selected is part of single line
                    Rectangle r1 = new Rectangle(leftEdge.X, leftEdge.Y, rightEdge.X - leftEdge.X, fontHeight);
                    Render2D.FillRectangle(r1, selectionColor);
                }
                else
                {
                    float leftMargin = m_Layout.Bounds.Location.X;

                    // Selected is more than one line
                    Rectangle r1 = new Rectangle(leftEdge.X, leftEdge.Y, 1000000000, fontHeight);
                    Render2D.FillRectangle(r1, selectionColor);
                    for (int i = 3; i <= selectedLinesCount; i++)
                    {
                        leftEdge.Y += textHeight;
                        Rectangle r = new Rectangle(leftMargin, leftEdge.Y, 1000000000, fontHeight);
                        Render2D.FillRectangle(r, selectionColor);
                    }
                    Rectangle r2 = new Rectangle(leftMargin, rightEdge.Y, rightEdge.X - leftMargin, fontHeight);
                    Render2D.FillRectangle(r2, selectionColor);
                }
            }

            // Text or watermark
            if (text.Length > 0)
            {
                Color color = TextColor;
                if (!enabled)
                {
                    color *= 0.6f;
                }

                Render2D.RenderText(font, text, color, m_Layout, TextMaterial);
            }
            else if (_watermarkText.Length > 0)
            {
                Render2D.RenderText(font, _watermarkText, WatermarkTextColor, m_Layout, TextMaterial);
            }

            // Caret
            if (IsFocused && CaretPosition > -1)
            {
                float alpha = MathF.Cos(_animateTime * CaretFlashSpeed) * 0.5f + 0.7f;
                alpha = alpha < 0 ? 0 : alpha < 1 ? alpha : 1;
                alpha = alpha * alpha * alpha * alpha * alpha * alpha;
                if (CaretPosition == 0)
                {
                    Rectangle bounds = CaretBounds;
                    if (m_Layout.VerticalAlignment == TextAlignment.Center)
                    {
                        bounds.Y = m_Layout.Bounds.Y + m_Layout.Bounds.Height * 0.5f - bounds.Height * 0.5f;
                    }
                    else if (m_Layout.VerticalAlignment == TextAlignment.Far)
                    {
                        bounds.Y = m_Layout.Bounds.Y + m_Layout.Bounds.Height - bounds.Height;
                    }

                    if (m_Layout.HorizontalAlignment == TextAlignment.Center)
                    {
                        bounds.X = m_Layout.Bounds.X + m_Layout.Bounds.Width * 0.5f - bounds.Width * 0.5f;
                    }
                    else if (m_Layout.HorizontalAlignment == TextAlignment.Far)
                    {
                        bounds.X = m_Layout.Bounds.X + m_Layout.Bounds.Width - bounds.Width;
                    }
                    Render2D.FillRectangle(bounds, CaretColor * alpha);
                }
                else
                {
                    Render2D.FillRectangle(CaretBounds, CaretColor * alpha);
                }
            }

            // Restore rendering state
            if (useViewOffset)
            {
                Render2D.PopTransform();
            }

            if (ClipText)
            {
                Render2D.PopClip();
            }
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            if (IsSelectable)
            {
                SelectAll();
            }

            return base.OnMouseDoubleClick(location, button);
        }

        /// <inheritdoc />
        protected override void OnIsMultilineChanged()
        {
            base.OnIsMultilineChanged();
            m_Layout.VerticalAlignment = IsMultiline ? TextAlignment.Near : TextAlignment.Center;
        }

        /// <inheritdoc />
        protected override void OnSizeChanged()
        {
            base.OnSizeChanged();
            m_Layout.Bounds = TextRectangle;
        }

        /// <summary>
        /// The watermark text.
        /// </summary>
        protected string _watermarkText = string.Empty;

        private TextLayoutOptions m_Layout;

        private SE.Font GetFont()
        {
            SE.Font font;
            if (Bold)
            {
                font = Italic ? Font.GetBold().GetItalic().GetFont() : Font.GetBold().GetFont();
            }
            else if (Italic)
            {
                font = Font.GetItalic().GetFont();
            }
            else
            {
                font = Font.GetFont();
            }

            return font;
        }

        private string ConvertedText()
        {
            string text = _text;
            switch (CaseOption)
            {
                case TextCaseOptions.Uppercase:
                    text = text.ToUpper();
                    break;
                case TextCaseOptions.Lowercase:
                    text = text.ToLower();
                    break;
            }
            return text;
        }
    }
}

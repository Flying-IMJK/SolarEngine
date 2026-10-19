#nullable disable annotations
using System;

namespace SE.GUI
{
    /// <summary>
    /// Options for text case
    /// </summary>
    public enum TextCaseOptions
    {
        /// <summary>
        /// No text case.
        /// </summary>
        None,

        /// <summary>
        /// Uppercase.
        /// </summary>
        Uppercase,

        /// <summary>
        /// Lowercase
        /// </summary>
        Lowercase,
    }

    public class Label : ContainerControl
    {
        /// <summary>
        /// The text.
        /// </summary>
        protected string _text = string.Empty;

        /// <summary>
        /// The font.
        /// </summary>
        protected FontReference _font = new FontReference();

        private bool m_AutoWidth;
        private bool m_AutoHeight;
        private bool m_AutoFitText;
        private Float2 m_TextSize;

        /// <summary>
        /// Gets or sets the text.
        /// </summary>
        public string Text
        {
            get
            {
                return _text;
            }
            set
            {
                if (_text != value)
                {
                    _text = value;
                    m_TextSize = Float2.Zero;
                    PerformLayout();
                }
            }
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
        /// Gets or sets the color of the text.
        /// </summary>
        public Color TextColor;

        /// <summary>
        /// Gets or sets the color of the text when it is highlighted (mouse is over).
        /// </summary>
        public Color TextColorHighlighted;

        /// <summary>
        /// Gets or sets the horizontal text alignment within the control bounds.
        /// </summary>
        public TextAlignment HorizontalAlignment = TextAlignment.Center;

        /// <summary>
        /// Gets or sets the vertical text alignment within the control bounds.
        /// </summary>
        public TextAlignment VerticalAlignment = TextAlignment.Center;

        /// <summary>
        /// Gets or sets the text wrapping within the control bounds.
        /// </summary>
        public TextWrapping Wrapping = TextWrapping.NoWrap;

        /// <summary>
        /// Gets or sets the text wrapping within the control bounds.
        /// </summary>
        public float BaseLinesGapScale = 1.0f;

        /// <summary>
        /// Gets or sets the font.
        /// </summary>
        public FontReference Font
        {
            get
            {
                return new FontReference(_font);
            }
            set
            {
                if (_font != value)
                {
                    _font = new FontReference(value);

                    if (m_AutoWidth || m_AutoHeight || m_AutoFitText)
                    {
                        m_TextSize = Float2.Zero;
                        PerformLayout();
                    }
                }
            }
        }

        /// <summary>
        /// Gets or sets the custom material used to render the text. It has to have domain set to GUI and have a public
        /// texture parameter named Font used to sample font atlas texture with font characters data.
        /// </summary>
        public MaterialBase Material;

        /// <summary>
        /// Gets or sets the margin for the text within the control bounds.
        /// </summary>
        public Margin Margin;

        /// <summary>
        /// Gets or sets a value indicating whether clip text during rendering.
        /// </summary>
        public bool ClipText;

        /// <summary>
        /// Gets or sets a value indicating whether set automatic width based on text contents.
        /// </summary>
        public bool GetAutoWidth()
        {
            return m_AutoWidth;
        }

        public void SetAutoWidth(bool value)
        {
            if (m_AutoWidth != value)
            {
                m_AutoWidth = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether set automatic height based on text contents.
        /// </summary>
        public bool GetAutoHeight()
        {
            return m_AutoHeight;
        }

        public void SetAutoHeight(bool value)
        {
            if (m_AutoHeight != value)
            {
                m_AutoHeight = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether scale text to fit the label control bounds. Disables using text
        /// alignment, automatic width and height.
        /// </summary>
        public bool GetAutoFitText()
        {
            return m_AutoFitText;
        }

        public void SetAutoFitText(bool value)
        {
            if (m_AutoFitText != value)
            {
                m_AutoFitText = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// Gets or sets the text scale range (min and max) for automatic fit text option. Can be used to constraint the
        /// text scale adjustment.
        /// </summary>
        // [VisibleIf(nameof(AutoFitText))]
        // [EditorOrder(110), DefaultValue(typeof(Float2), "0.1, 100"), Tooltip("The text scale range (min and max) for
        // automatic fit text option. Can be used to constraint the text scale adjustment.")]
        public Float2 AutoFitTextRange = new Float2(0.1f, 100.0f);

        public Label() : base(0, 0, 100, 20)
        {
            AutoFocus = false;
            Style style = Style.Current;
            if (style != null)
            {
                _font = new FontReference(style.FontMedium);
                TextColor = style.TextColor;
                TextColorHighlighted = style.TextColor;
            }
        }

        /// <inheritdoc />
        public Label(float x, float y, float width, float height) : base(x, y, width, height)
        {
            AutoFocus = false;
            Style style = Style.Current;
            if (style != null)
            {
                _font = new FontReference(style.FontMedium);
                TextColor = style.TextColor;
                TextColorHighlighted = style.TextColor;
            }
        }

        /// <inheritdoc />
        public override void DrawSelf()
        {
            base.DrawSelf();

            Rectangle rect = new Rectangle(new Float2(Margin.Left, Margin.Top), Size - Margin.Size);

            if (ClipText)
            {
                Render2D.PushClip(new Rectangle(Float2.Zero, Size));
            }

            Color color = IsMouseOver || IsNavFocused ? TextColorHighlighted : TextColor;
            if (!EnabledInHierarchy)
            {
                color *= 0.6f;
            }

            float scale = 1.0f;
            TextAlignment hAlignment = HorizontalAlignment;
            TextAlignment wAlignment = VerticalAlignment;
            if (m_AutoFitText)
            {
                if (!m_TextSize.IsZero)
                {
                    scale = (rect.Size / m_TextSize).MinValue;
                    scale = Mathf.Clamp(scale, AutoFitTextRange.X, AutoFitTextRange.Y);
                }
            }

            SE.Font font = GetFont();
            string text = ConvertedText();

            Render2D.RenderText(font, Material, text, rect, color, hAlignment, wAlignment, Wrapping,
                BaseLinesGapScale, scale);

            if (ClipText)
            {
                Render2D.PopClip();
            }
        }

        private SE.Font GetFont()
        {
            SE.Font font;
            if (Bold)
            {
                font = Italic ? _font.GetBold().GetItalic().GetFont() : _font.GetBold().GetFont();
            }
            else if (Italic)
            {
                font = _font.GetItalic().GetFont();
            }
            else
            {
                font = _font.GetFont();
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

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            if (m_AutoWidth || m_AutoHeight || m_AutoFitText)
            {
                SE.Font font = GetFont();
                string text = ConvertedText();
                if (font != null)
                {
                    // Calculate text size
                    TextLayoutOptions layout = TextLayoutOptions.Default;
                    layout.TextWrapping = Wrapping;
                    if (m_AutoHeight && !m_AutoWidth)
                    {
                        layout.Bounds.Size.X = Width - Margin.Width;
                    }
                    else if (m_AutoWidth && !m_AutoHeight)
                    {
                        layout.Bounds.Size.Y = Height - Margin.Height;
                    }

                    m_TextSize = font.MeasureText(text, layout);
                    m_TextSize.Y *= BaseLinesGapScale;

                    // Check if size is controlled via text
                    if (m_AutoWidth || m_AutoHeight)
                    {
                        Float2 size = Size;
                        if (m_AutoWidth)
                        {
                            size.X = m_TextSize.X + Margin.Width;
                        }

                        if (m_AutoHeight)
                        {
                            size.Y = m_TextSize.Y + Margin.Height;
                        }

                        Size = size;
                    }
                }
            }
            base.PerformLayoutBeforeChildren();
        }
    }
}

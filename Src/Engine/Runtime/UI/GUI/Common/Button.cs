#nullable disable annotations
using System;

namespace SE.GUI
{
    public class Button : ContainerControl
    {
        /// <summary>
        /// The default height for the buttons.
        /// </summary>
        public static float DefaultHeight = 24.0f;

        /// <summary>
        /// True if button is being pressed (by mouse or touch).
        /// </summary>
        public bool isPressed;

        /// <summary>
        /// The font.
        /// </summary>
        public Font Font;

        /// <summary>
        /// The text.
        /// </summary>
        public string Text = string.Empty;

        /// <summary>
        /// Gets or sets the custom material used to render the text. It must has domain set to GUI and have a public texture parameter named Font used to sample font atlas texture with font characters data.
        /// </summary>
        public MaterialBase TextMaterial;

        /// <summary>
        /// Gets or sets the color used to draw button text.
        /// </summary>
        public Color TextColor;

        /// <summary>
        /// Gets or sets the brush used for background drawing.
        /// </summary>
        public IBrush BackgroundBrush;

        /// <summary>
        /// Gets or sets the background color when button is highlighted.
        /// </summary>
        public Color BackgroundColorHighlighted;

        /// <summary>
        /// Gets or sets the background color when button is selected.
        /// </summary>
        public Color BackgroundColorSelected;

        /// <summary>
        /// Gets or sets whether the button has a border.
        /// </summary>
        public bool HasBorder = true;

        /// <summary>
        /// Gets or sets the border thickness.
        /// </summary>
        public float BorderThickness = 1.0f;

        /// <summary>
        /// Gets or sets the color of the border.
        /// </summary>
        public Color BorderColor;

        /// <summary>
        /// Gets or sets the border color when button is highlighted.
        /// </summary>
        public Color BorderColorHighlighted;

        /// <summary>
        /// Gets or sets the border color when button is selected.
        /// </summary>
        public Color BorderColorSelected;

        /// <summary>
        /// Event fired when user clicks on the button.
        /// </summary>
        public Action Clicked;

        /// <summary>
        /// Event fired when user clicks on the button.
        /// </summary>
        public Action<Button> ButtonClicked;

        /// <summary>
        /// Event fired when users mouse enters the control.
        /// </summary>
        public Action HoverBegin;

        /// <summary>
        /// Event fired when users mouse leaves the control.
        /// </summary>
        public Action HoverEnd;

        public Button() : this(0, 0, 0, 0)
        {
        }

        public Button(float x, float y, float width = 120.0f)
            : this(x, y, width, DefaultHeight)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Button"/> class.
        /// </summary>
        /// <param name="x">Position X coordinate</param>
        /// <param name="y">Position Y coordinate</param>
        /// <param name="width">Width</param>
        /// <param name="height">Height</param>
        public Button(float x, float y, float width, float height) : base(x, y, width, height)
        {
            Style style = Style.Current;
            if (style != null)
            {
                Font = style.FontMedium;
                TextColor = style.Foreground;
                BackgroundColor = style.BackgroundNormal;
                BorderColor = style.BorderNormal;
                BackgroundColorSelected = style.BackgroundSelected;
                BorderColorSelected = style.BorderSelected;
                BackgroundColorHighlighted = style.BackgroundHighlighted;
                BorderColorHighlighted = style.BorderHighlighted;
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Button"/> class.
        /// </summary>
        /// <param name="location">Position</param>
        /// <param name="size">Size</param>
        public Button(Float2 location, Float2 size)
            : base(location.X, location.Y, size.X, size.Y)
        {
        }

        /// <summary>
        /// Called when mouse or touch clicks the button.
        /// </summary>
        protected virtual void OnClick()
        {
            Clicked?.Invoke();
            ButtonClicked?.Invoke(this);
        }

        /// <summary>
        /// Called when button starts to be pressed by the used (via mouse or touch).
        /// </summary>
        protected virtual void OnPressBegin()
        {
            isPressed = true;
            if (AutoFocus)
            {
                Focus();
            }
        }

        /// <summary>
        /// Called when button ends to be pressed by the used (via mouse or touch).
        /// </summary>
        protected virtual void OnPressEnd()
        {
            isPressed = false;
        }

        /// <summary>
        /// Sets the button colors palette based on a given main color.
        /// </summary>
        /// <param name="color">The main color.</param>
        public virtual void SetColors(Color color)
        {
            BackgroundColor = color;
            BorderColor = color.RGBMultiplied(0.5f);
            BackgroundColorSelected = color.RGBMultiplied(0.8f);
            BorderColorSelected = BorderColor;
            BackgroundColorHighlighted = color.RGBMultiplied(1.2f);
            BorderColorHighlighted = BorderColor;
        }

        /// <inheritdoc />
        public override void ClearState()
        {
            base.ClearState();
            if (isPressed)
            {
                OnPressEnd();
            }
        }

        /// <inheritdoc />
        public override void DrawSelf()
        {
            // Cache data
            Rectangle clientRect = new Rectangle(Float2.Zero, Size);
            bool enabled = EnabledInHierarchy;
            Color backgroundColor = BackgroundColor;
            Color borderColor = BorderColor;
            Color textColor = TextColor;
            if (!enabled)
            {
                backgroundColor *= 0.5f;
                borderColor *= 0.5f;
                textColor *= 0.6f;
            }
            else if (isPressed)
            {
                backgroundColor = BackgroundColorSelected;
                borderColor = BorderColorSelected;
            }
            else if (IsMouseOver || IsNavFocused)
            {
                backgroundColor = BackgroundColorHighlighted;
                borderColor = BorderColorHighlighted;
            }

            // Draw background
            if (BackgroundBrush != null)
            {
                BackgroundBrush.Draw(clientRect, backgroundColor);
            }
            else
            {
                Render2D.FillRectangle(clientRect, backgroundColor);
            }

            if (HasBorder)
            {
                Render2D.DrawRectangle(clientRect, borderColor, BorderThickness);
            }

            // Draw text
            Render2D.RenderText(Font, TextMaterial, Text, clientRect, textColor, TextAlignment.Center, TextAlignment.Center);
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            base.OnMouseEnter(location);
            HoverBegin?.Invoke();
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            if (isPressed)
            {
                OnPressEnd();
            }

            HoverEnd?.Invoke();
            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
            {
                return true;
            }

            if (button == MouseButton.Left && !isPressed)
            {
                OnPressBegin();
                return true;
            }
            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (base.OnMouseUp(location, button))
            {
                return true;
            }

            if (button == MouseButton.Left && isPressed)
            {
                OnPressEnd();
                OnClick();
                return true;
            }
            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            if (base.OnMouseDoubleClick(location, button))
            {
                return true;
            }

            if (button == MouseButton.Left && isPressed)
            {
                OnPressEnd();
                OnClick();
                return true;
            }
            if (button == MouseButton.Left && !isPressed)
            {
                OnPressBegin();
                return true;
            }
            return false;
        }

        /// <inheritdoc />
        public override bool OnTouchDown(Float2 location, int pointerId)
        {
            if (base.OnTouchDown(location, pointerId))
            {
                return true;
            }

            if (!isPressed)
            {
                OnPressBegin();
                return true;
            }
            return false;
        }

        /// <inheritdoc />
        public override bool OnTouchUp(Float2 location, int pointerId)
        {
            if (base.OnTouchUp(location, pointerId))
            {
                return true;
            }

            if (isPressed)
            {
                OnPressEnd();
                OnClick();
                return true;
            }
            return false;
        }

        /// <inheritdoc />
        public override void OnTouchLeave()
        {
            if (isPressed)
            {
                OnPressEnd();
            }

            base.OnTouchLeave();
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            if (isPressed)
            {
                OnPressEnd();
            }

            base.OnLostFocus();
        }

        /// <inheritdoc />
        public override void OnSubmit()
        {
            OnClick();
            base.OnSubmit();
        }
    }
}

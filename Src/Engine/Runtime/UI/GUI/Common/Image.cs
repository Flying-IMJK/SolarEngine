#nullable disable annotations
using System;

namespace SE.GUI
{
    public class Image : ContainerControl
    {
        /// <summary>
        /// Gets or sets the image source.
        /// </summary>
        public IBrush Brush;

        /// <summary>
        /// Gets or sets the margin for the image.
        /// </summary>
        public Margin Margin;

        /// <summary>
        /// Gets or sets the color used to multiply the image pixels when mouse is over the image.
        /// </summary>
        public Color MouseOverColor = SE.Color.White;

        /// <summary>
        /// Gets or sets the color used to multiply the image pixels when control is disabled.
        /// </summary>
        public Color DisabledTint = SE.Color.Gray;

        /// <summary>
        /// Gets or sets the color used to multiply the image pixels.
        /// </summary>
        public Color Color = SE.Color.White;

        /// <summary>
        /// Gets or sets a value indicating whether keep aspect ratio when drawing the image.
        /// </summary>
        public bool KeepAspectRatio = true;

        /// <summary>
        /// Occurs when mouse clicks on the image.
        /// </summary>
        public Action<Image, MouseButton> Clicked;

        public Image() : base(0, 0, 64, 64)
        {
            AutoFocus = false;
        }

        /// <inheritdoc />
        public Image(float x, float y, float width, float height) : base(x, y, width, height)
        {
            AutoFocus = false;
        }

        /// <inheritdoc />
        public Image(Float2 location, Float2 size) : base(location, size)
        {
            AutoFocus = false;
        }

        /// <inheritdoc />
        public Image(Rectangle bounds) : base(bounds)
        {
            AutoFocus = false;
        }

        /// <inheritdoc />
        public override void DrawSelf()
        {
            base.DrawSelf();

            if (Brush == null)
            {
                return;
            }

            Rectangle rect;
            if (KeepAspectRatio)
            {
                // Figure out the ratio
                Float2 size = Size;
                Float2 imageSize = Brush.Size;
                if (imageSize.LengthSquared < 1)
                {
                    return;
                }

                Float2 ratio = size / imageSize;
                Float2 aspectRatio = new Float2(ratio.MinValue);

                // Get the new height and width
                Float2 newSize = imageSize * aspectRatio;

                // Calculate the X,Y position of the upper-left corner
                // (one of these will always be zero)
                Float2 newPos = (size - newSize) / new Float2(2);

                rect = new Rectangle(newPos, newSize);
            }
            else
            {
                rect = new Rectangle(Float2.Zero, Size);
            }

            Margin.ShrinkRectangle(ref rect);

            SE.Color color = IsMouseOver || IsNavFocused ? MouseOverColor : Color;
            if (!Enabled)
            {
                color *= DisabledTint;
            }

            Brush.Draw(rect, color);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (base.OnMouseUp(location, button))
            {
                return true;
            }

            if (Clicked != null)
            {
                Clicked(this, button);
                return true;
            }

            return false;
        }
    }
}

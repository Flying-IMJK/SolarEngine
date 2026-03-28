#pragma once

#include "Runtime/UI/GUI/ContainerControl.h"

namespace SE
{
    class IBrush;

    class SE_API_RUNTIME Image : public ContainerControl
    {
        SE_CLASS(Image, ContainerControl)
    public:
        /// <summary>
        /// Gets or sets the image source.
        /// </summary>
        IBrush* Brush;

        /// <summary>
        /// Gets or sets the margin for the image.
        /// </summary>
        Margin Margin;

        /// <summary>
        /// Gets or sets the color used to multiply the image pixels when mouse is over the image.
        /// </summary>
        Color MouseOverColor = Colors::White;

        /// <summary>
        /// Gets or sets the color used to multiply the image pixels when control is disabled.
        /// </summary>
        Color DisabledTint = Colors::Gray;

        /// <summary>
        /// Gets or sets the color used to multiply the image pixels.
        /// </summary>
        Color Color = Colors::White;

        /// <summary>
        /// Gets or sets a value indicating whether keep aspect ratio when drawing the image.
        /// </summary>
        bool KeepAspectRatio  = true;

        /// <summary>
        /// Occurs when mouse clicks on the image.
        /// </summary>
        Delegate<Image*, MouseButton> Clicked;

        Image();

        /// <inheritdoc />
        Image(float x, float y, float width, float height);

        /// <inheritdoc />
        Image(Float2 location, Float2 size);

        /// <inheritdoc />
        Image(Rectangle bounds);

        /// <inheritdoc />
        void DrawSelf() override;

        /// <inheritdoc />
        bool OnMouseUp(Float2 location, MouseButton button) override;
    };
} // SE


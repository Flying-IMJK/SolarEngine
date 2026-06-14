#pragma once

#include "Runtime/UI/GUI/ContainerControl.h"

namespace SE
{
    class IBrush;
    class Font;
    class MaterialBase;

    class SE_API_RUNTIME Button : public ContainerControl
    {
        SE_DEFINE_CLASS(Button, ContainerControl)
    public:
        /// <summary>
        /// The default height for the buttons.
        /// </summary>
        static float DefaultHeight;

        /// <summary>
        /// True if button is being pressed (by mouse or touch).
        /// </summary>
        bool isPressed;

        /// <summary>
        /// The font.
        /// </summary>
        Font* Font;

        /// <summary>
        /// The text.
        /// </summary>
        String Text;

        /// <summary>
        /// Gets or sets the custom material used to render the text. It must has domain set to GUI and have a public texture parameter named Font used to sample font atlas texture with font characters data.
        /// </summary>
        MaterialBase* TextMaterial;

        /// <summary>
        /// Gets or sets the color used to draw button text.
        /// </summary>
        Color TextColor;

        /// <summary>
        /// Gets or sets the brush used for background drawing.
        /// </summary>
        IBrush* BackgroundBrush;

        /// <summary>
        /// Gets or sets the background color when button is highlighted.
        /// </summary>
        Color BackgroundColorHighlighted;

        /// <summary>
        /// Gets or sets the background color when button is selected.
        /// </summary>
        Color BackgroundColorSelected;

        /// <summary>
        /// Gets or sets whether the button has a border.
        /// </summary>
        bool HasBorder = true;

        /// <summary>
        /// Gets or sets the border thickness.
        /// </summary>
        float BorderThickness = 1.0f;

        /// <summary>
        /// Gets or sets the color of the border.
        /// </summary>
        Color BorderColor;

        /// <summary>
        /// Gets or sets the border color when button is highlighted.
        /// </summary>
        Color BorderColorHighlighted;

        /// <summary>
        /// Gets or sets the border color when button is selected.
        /// </summary>
        Color BorderColorSelected;

        /// <summary>
        /// Event fired when user clicks on the button.
        /// </summary>
        Action Clicked;

        /// <summary>
        /// Event fired when user clicks on the button.
        /// </summary>
        Delegate<Button*> ButtonClicked;

        /// <summary>
        /// Event fired when users mouse enters the control.
        /// </summary>
        Action HoverBegin;

        /// <summary>
        /// Event fired when users mouse leaves the control.
        /// </summary>
        Action HoverEnd;

        Button();

        /// <summary>
        /// Initializes a new instance of the <see cref="Button"/> class.
        /// </summary>
        /// <param name="x">Position X coordinate</param>
        /// <param name="y">Position Y coordinate</param>
        /// <param name="width">Width</param>
        /// <param name="height">Height</param>
        Button(float x, float y, float width = 120, float height = DefaultHeight);

        /// <summary>
        /// Initializes a new instance of the <see cref="Button"/> class.
        /// </summary>
        /// <param name="location">Position</param>
        /// <param name="size">Size</param>
        Button(Float2 location, Float2 size);

    protected:
        /// <summary>
        /// Called when mouse or touch clicks the button.
        /// </summary>
        virtual void OnClick();

        /// <summary>
        /// Called when button starts to be pressed by the used (via mouse or touch).
        /// </summary>
        virtual void OnPressBegin();

        /// <summary>
        /// Called when button ends to be pressed by the used (via mouse or touch).
        /// </summary>
        virtual void OnPressEnd();

    public:
        /// <summary>
        /// Sets the button colors palette based on a given main color.
        /// </summary>
        /// <param name="color">The main color.</param>
        virtual void SetColors(Color color);

        /// <inheritdoc />
        void ClearState() override;

        /// <inheritdoc />
        void DrawSelf() override;

        /// <inheritdoc />
        void OnMouseEnter(Float2 location) override;

        /// <inheritdoc />
        void OnMouseLeave() override;

        /// <inheritdoc />
        bool OnMouseDown(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        bool OnMouseUp(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        bool OnMouseDoubleClick(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        bool OnTouchDown(Float2 location, int pointerId) override;

        /// <inheritdoc />
        bool OnTouchUp(Float2 location, int pointerId) override;

        /// <inheritdoc />
        void OnTouchLeave() override;

        /// <inheritdoc />
        void OnLostFocus() override;

        /// <inheritdoc />
        void OnSubmit() override;
    };
} // SE


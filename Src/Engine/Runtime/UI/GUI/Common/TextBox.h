#pragma once
#include "Label.h"
#include "TextBoxBase.h"

namespace SE
{
    SE_CLASS(Reflect)
    class SE_API_RUNTIME TextBox : public TextBoxBase
    {
        SE_DEFINE_CLASS(TextBox, TextBoxBase)
    public:
        /// <summary>
        /// Gets or sets the watermark text to show grayed when textbox is empty.
        /// </summary>
        // [EditorOrder(20), Tooltip("The watermark text to show grayed when textbox is empty.")]
        PRO_REF(WatermarkText, TextBox, String, __GetWatermarkText, __SetWatermarkText);

        /// <summary>
        /// The text case.
        /// </summary>
        // [EditorDisplay("Text Style"), EditorOrder(2000), Tooltip("The case of the text.")]
        TextCaseOptions CaseOption = TextCaseOptions::None;

        /// <summary>
        /// Whether to bold the text.
        /// </summary>
        // [EditorDisplay("Text Style"), EditorOrder(2001), Tooltip("Bold the text.")]
        bool Bold = false;

        /// <summary>
        /// Whether to italicize the text.
        /// </summary>
        // [EditorDisplay("Text Style"), EditorOrder(2002), Tooltip("Italicize the text.")]
        bool Italic = false;

        /// <summary>
        /// The vertical alignment of the text.
        /// </summary>
        // [EditorDisplay("Text Style"), EditorOrder(2023), Tooltip("The vertical alignment of the text.")]
        PRO(VerticalAlignment, TextBox, TextAlignment, __GetVerticalAlignment, __SetVerticalAlignment);

        /// <summary>
        /// The vertical alignment of the text.
        /// </summary>
        // [EditorDisplay("Text Style"), EditorOrder(2024), Tooltip("The horizontal alignment of the text.")]
        PRO(HorizontalAlignment, TextBox, TextAlignment, __GetHorizontalAlignment, __SetHorizontalAlignment);

        /// <summary>
        /// Gets or sets the text wrapping within the control bounds.
        /// </summary>
        // [EditorDisplay("Text Style"), EditorOrder(2025), Tooltip("The text wrapping within the control bounds.")]
        PRO(Wrapping, TextBox, TextWrapping, __GetWrapping, __SetWrapping);

        /// <summary>
        /// Gets or sets the font.
        /// </summary>
        // [EditorDisplay("Text Style"), EditorOrder(2026)]
        FontReference Font;

        /// <summary>
        /// Gets or sets the custom material used to render the text. It must has domain set to GUI and have a public texture parameter named Font used to sample font atlas texture with font characters data.
        /// </summary>
        // [EditorDisplay("Text Style"), EditorOrder(2027), Tooltip("Custom material used to render the text. It must has domain set to GUI and have a public texture parameter named Font used to sample font atlas texture with font characters data.")]
        MaterialBase* TextMaterial;

        /// <summary>
        /// Gets or sets the color of the text.
        /// </summary>
        // [EditorDisplay("Text Style"), EditorOrder(2020), Tooltip("The color of the text."), ExpandGroups]
        Color TextColor;

        /// <summary>
        /// Gets or sets the color of the text.
        /// </summary>
        // [EditorDisplay("Text Style"), EditorOrder(2021), Tooltip("The color of the watermark text.")]
        Color WatermarkTextColor;

        /// <summary>
        /// Gets or sets the color of the selection (Transparent if not used).
        /// </summary>
        // [EditorDisplay("Text Style"), EditorOrder(2022), Tooltip("The color of the selection (Transparent if not used).")]
        Color SelectionColor;

        TextBox();

        /// <summary>
        /// Init
        /// </summary>
        /// <param name="isMultiline">Enable/disable multiline text input support</param>
        /// <param name="x">Position X coordinate</param>
        /// <param name="y">Position Y coordinate</param>
        /// <param name="width">Width</param>
        TextBox(bool isMultiline, float x, float y, float width = 120);

        /// <inheritdoc />
        Float2 GetTextSize() override;

        /// <inheritdoc />
        Float2 GetCharPosition(int index, float& height) override;

        /// <inheritdoc />
        int HitTestText(Float2 location) override;

        /// <inheritdoc />
        void DrawSelf() override;

        /// <inheritdoc />
        bool OnMouseDoubleClick(Float2 location, MouseButton button) override;

    protected:
        /// <inheritdoc />
        void OnIsMultilineChanged() override;

        /// <inheritdoc />
        void OnSizeChanged() override;

        /// <summary>
        /// The watermark text.
        /// </summary>
        String _watermarkText;

    private:

        TextLayoutOptions _layout;

        ::SE::Font* GetFont();

        String ConvertedText();

        String& __GetWatermarkText()
        {
            return _watermarkText;
        }
        void __SetWatermarkText(String& value)
        {
            _watermarkText = value;
        }

        TextWrapping __GetWrapping()
        {
            return _layout.TextWrapping;
        }
        void __SetWrapping(TextWrapping value)
        {
            _layout.TextWrapping = value;
        }

        TextAlignment __GetHorizontalAlignment() { return _layout.HorizontalAlignment; }
        void __SetHorizontalAlignment(TextAlignment value)
        {
            _layout.HorizontalAlignment = value;
        }

        TextAlignment __GetVerticalAlignment() { return _layout.VerticalAlignment; }
        void __SetVerticalAlignment(TextAlignment value)
        {
            _layout.VerticalAlignment = value;
        }
    };

} // SE

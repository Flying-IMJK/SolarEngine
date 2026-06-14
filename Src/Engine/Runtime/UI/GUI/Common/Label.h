#pragma once

#include "Runtime/Render/2D/FontReference.h"
#include "Runtime/Render/2D/TextLayoutOptions.h"
#include "Runtime/UI/GUI/ContainerControl.h"

namespace SE
{
	class MaterialBase;

	/// <summary>
	/// Options for text case
	/// </summary>
	enum class TextCaseOptions
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
		Lowercase
	};

	class SE_API_RUNTIME Label : public ContainerControl
    {
		SE_DEFINE_CLASS(Label, ContainerControl)
	protected:
        /// <summary>
        /// The text.
        /// </summary>
	    String _text;
	    /// <summary>
	    /// The font.
	    /// </summary>
	    FontReference _font;

	private:
        bool m_AutoWidth;
        bool m_AutoHeight;
        bool m_AutoFitText;
        Float2 m_TextSize;

	public:

		/// <summary>
		/// Gets or sets the text.
		/// </summary>
		PRO_REF(Text, Label, String, __GetText, __SetText);

        /// <summary>
        /// The text case.
        /// </summary>
        TextCaseOptions CaseOption = TextCaseOptions::None;

        /// <summary>
        /// Whether to bold the text.
        /// </summary>
	    bool Bold  = false;

        /// <summary>
        /// Whether to italicize the text.
        /// </summary>
	    bool Italic = false;

        /// <summary>
        /// Gets or sets the color of the text.
        /// </summary>
        Color TextColor;

        /// <summary>
        /// Gets or sets the color of the text when it is highlighted (mouse is over).
        /// </summary>
        Color TextColorHighlighted;

        /// <summary>
        /// Gets or sets the horizontal text alignment within the control bounds.
        /// </summary>
        TextAlignment HorizontalAlignment = TextAlignment::Center;

        /// <summary>
        /// Gets or sets the vertical text alignment within the control bounds.
        /// </summary>
	    TextAlignment VerticalAlignment = TextAlignment::Center;

        /// <summary>
        /// Gets or sets the text wrapping within the control bounds.
        /// </summary>
	    TextWrapping Wrapping = TextWrapping::NoWrap;

        /// <summary>
        /// Gets or sets the text wrapping within the control bounds.
        /// </summary>
	    float BaseLinesGapScale = 1.0f;

        /// <summary>
        /// Gets or sets the font.
        /// </summary>
		PRO(Font, Label, FontReference, __GetFont, __SetFont);

        /// <summary>
        /// Gets or sets the custom material used to render the text. It has to have domain set to GUI and have a public texture parameter named Font used to sample font atlas texture with font characters data.
        /// </summary>
	    MaterialBase* Material;

        /// <summary>
        /// Gets or sets the margin for the text within the control bounds.
        /// </summary>
	    Margin Margin;

        /// <summary>
        /// Gets or sets a value indicating whether clip text during rendering.
        /// </summary>
	    bool ClipText;

        /// <summary>
        /// Gets or sets a value indicating whether set automatic width based on text contents.
        /// </summary>
		bool GetAutoWidth() const { return m_AutoWidth; }
		void SetAutoWidth(bool value);

        /// <summary>
        /// Gets or sets a value indicating whether set automatic height based on text contents.
        /// </summary>
		bool GetAutoHeight() const { return m_AutoHeight; }
		void SetAutoHeight(bool value);

        /// <summary>
        /// Gets or sets a value indicating whether scale text to fit the label control bounds. Disables using text alignment, automatic width and height.
        /// </summary>
		bool GetAutoFitText() const	{ return m_AutoFitText; }
		void SetAutoFitText(bool value);

        /// <summary>
        /// Gets or sets the text scale range (min and max) for automatic fit text option. Can be used to constraint the text scale adjustment.
        /// </summary>
        // [VisibleIf(nameof(AutoFitText))]
        // [EditorOrder(110), DefaultValue(typeof(Float2), "0.1, 100"), Tooltip("The text scale range (min and max) for automatic fit text option. Can be used to constraint the text scale adjustment.")]
	    Float2 AutoFitTextRange = Float2(0.1f, 100.0f);

		Label();

        /// <inheritdoc />
	    Label(float x, float y, float width, float height);

        /// <inheritdoc />
	    void DrawSelf() override;

	private:
	    ::SE::Font* GetFont();

	    String ConvertedText();

		String& __GetText() { return _text; }
		void __SetText(String & value);
		FontReference __GetFont() { return _font; }
		void __SetFont(FontReference value);


	protected:
        /// <inheritdoc />
	    void PerformLayoutBeforeChildren() override;
	};

} // SE


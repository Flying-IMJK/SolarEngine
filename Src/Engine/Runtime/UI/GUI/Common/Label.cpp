
#include "Label.h"

#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/Style.h"

namespace SE
{
	void Label::SetAutoWidth(bool value)
	{
		if (m_AutoWidth != value)
		{
			m_AutoWidth = value;
			PerformLayout();
		}
	}

	void Label::SetAutoHeight(bool value)
	{
		if (m_AutoHeight != value)
		{
			m_AutoHeight = value;
			PerformLayout();
		}
	}

	void Label::SetAutoFitText(bool value)
	{
		if (m_AutoFitText != value)
		{
			m_AutoFitText = value;
			PerformLayout();
		}
	}

	Label::Label() : ContainerControl(0, 0, 100, 20)
	{
		AutoFocus = false;
		Style* style = Style::Current;
		if (style != nullptr)
		{
			_font = FontReference(style->FontMedium);
			TextColor = style->TextColor;
			TextColorHighlighted = style->TextColor;
		}
	}

	Label::Label(float x, float y, float width, float height) : ContainerControl(x, y, width, height)
	{
		AutoFocus = false;
		Style* style = Style::Current;
		if (style != nullptr)
		{
			_font = FontReference(style->FontMedium);
			TextColor = style->TextColor;
			TextColorHighlighted = style->TextColor;
		}
	}

	void Label::DrawSelf()
	{
		ContainerControl::DrawSelf();

		Rectangle rect = Rectangle(Float2(Margin.Left, Margin.Top),  Size.operator->() - Margin.GetSize());

		if (ClipText)
			Render2D::PushClip(Rectangle(Float2::Zero, Size));

		Color color = IsMouseOver || IsNavFocused ? TextColorHighlighted : TextColor;

		if (!EnabledInHierarchy())
			color *= 0.6f;

		float scale = 1.0f;
		TextAlignment hAlignment = HorizontalAlignment;
		TextAlignment wAlignment = VerticalAlignment;
		if (m_AutoFitText)
		{
			if (!m_TextSize.IsZero())
			{
				scale = (rect.Size / m_TextSize).MinValue();
				scale = Math::Clamp(scale, AutoFitTextRange.x, AutoFitTextRange.y);
			}
		}

		::SE::Font* font = GetFont();
		String text = ConvertedText();

		Render2D::RenderText(font, Material, text, rect, color, hAlignment, wAlignment, Wrapping, BaseLinesGapScale, scale);

		if (ClipText)
			Render2D::PopClip();
	}

	Font* Label::GetFont()
	{
		::SE::Font* font;
		if (Bold)
			font = Italic ? _font.GetBold().GetItalic().GetFont() : _font.GetBold().GetFont();
		else if (Italic)
			font = _font.GetItalic().GetFont();
		else
			font = _font.GetFont();
		return font;
	}

	String Label::ConvertedText()
	{
		String text = _text;
		switch (CaseOption)
		{
		case TextCaseOptions::Uppercase:
			text = text.ToUpper();
			break;
		case TextCaseOptions::Lowercase:
			text = text.ToLower();
			break;
		}
		return text;
	}

	void Label::__SetText(String& value)
	{
		if (_text != value)
		{
			_text = value;
			m_TextSize = Float2::Zero;
			PerformLayout();
		}
	}

	void Label::__SetFont(FontReference value)
	{
		if (_font != value)
		{
			_font = value;

			if (m_AutoWidth || m_AutoHeight || m_AutoFitText)
			{
				m_TextSize = Float2::Zero;
				PerformLayout();
			}
		}
	}

	void Label::PerformLayoutBeforeChildren()
	{
		if (m_AutoWidth || m_AutoHeight || m_AutoFitText)
		{
			::SE::Font* font = GetFont();
			String text = ConvertedText();
			if (font)
			{
				// Calculate text size
				TextLayoutOptions layout = TextLayoutOptions::Default();
				layout.TextWrapping = Wrapping;
				if (m_AutoHeight && !m_AutoWidth)
					layout.Bounds.Size.x = Width - Margin.GetWidth();
				else if (m_AutoWidth && !m_AutoHeight)
					layout.Bounds.Size.y = Height - Margin.GetHeight();
				m_TextSize = font->MeasureText(text, layout);
				m_TextSize.y *= BaseLinesGapScale;

				// Check if size is controlled via text
				if (m_AutoWidth || m_AutoHeight)
				{
					Float2 size = Size;
					if (m_AutoWidth)
						size.x = m_TextSize.x + Margin.GetWidth();
					if (m_AutoHeight)
						size.y = m_TextSize.y + Margin.GetHeight();
					Size = size;
				}
			}
		}

		ContainerControl::PerformLayoutBeforeChildren();
	}
} // SE
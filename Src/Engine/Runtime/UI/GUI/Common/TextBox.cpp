
#include "TextBox.h"

#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/Style.h"

namespace SE
{
	TextBox::TextBox()
	{
	}
	
	TextBox::TextBox(bool isMultiline, float x, float y, float width) : TextBoxBase(isMultiline, x, y, width)
	{
		_layout = TextLayoutOptions::Default();
		_layout.VerticalAlignment = IsMultiline ? TextAlignment::Near : TextAlignment::Center;
		_layout.TextWrapping = TextWrapping::NoWrap;
		_layout.Bounds = Rectangle(DefaultMargin, 1, Width - 2 * DefaultMargin, Height - 2);

		Style* style = Style::Current;
		Font = FontReference(style->FontMedium);
		TextColor = style->Foreground;
		WatermarkTextColor = style->ForegroundDisabled;
		SelectionColor = style->BackgroundSelected;
	}

	Float2 TextBox::GetTextSize()
	{
		::SE::Font* font = GetFont();
		if (font == nullptr)
		{
			return Float2::Zero;
		}

		return font->MeasureText(ConvertedText(), _layout);
	}
	
	Float2 TextBox::GetCharPosition(int index, float& height)
	{
		::SE::Font* font = GetFont();
		if (font == nullptr)
		{
			height = Height;
			return Float2::Zero;
		}

		height = font->GetHeight() / GetDpiScale();
		return font->GetCharPosition(ConvertedText(), index, _layout);
	}
	
	int TextBox::HitTestText(Float2 location)
	{
		::SE::Font* font = GetFont();
		if (font == nullptr)
		{
			return 0;
		}

		return font->HitTestText(ConvertedText(), location, _layout);
	}
	
	void TextBox::DrawSelf()
	{
		// Cache data
		Rectangle rect = Rectangle(Float2::Zero, Size);
		bool enabled = EnabledInHierarchy();
		::SE::Font* font = GetFont();
		if (!font)
			return;

		// Background
		Color backColor = BackgroundColor;
		if (IsMouseOver || IsNavFocused)
			backColor = BackgroundSelectedColor;
		Render2D::FillRectangle(rect, backColor);
		if (HasBorder)
			Render2D::DrawRectangle(rect, IsFocused ? BorderSelectedColor : BorderColor, BorderThickness);

		// Apply view offset and clip mask
		if (ClipText)
			Render2D::PushClip(GetTextClipRectangle());
		bool useViewOffset = !_viewOffset.IsZero();
		if (useViewOffset)
			Render2D::PushTransform(Matrix3x3::Translation2D(-_viewOffset));

		String text = ConvertedText();

		// Check if sth is selected to draw selection
		if (HasSelection)
		{
			Float2 leftEdge = font->GetCharPosition(text, SelectionLeft, _layout);
			Float2 rightEdge = font->GetCharPosition(text, SelectionRight, _layout);
			float fontHeight = font->GetHeight();
			float textHeight = fontHeight / GetDpiScale();

			// Draw selection background
			float alpha = Math::Min(1.0f, Math::Cos(_animateTime * BackgroundSelectedFlashSpeed) * 0.5f + 1.3f);
			alpha *= alpha;
			Color selectionColor = SelectionColor * alpha;
			//
			int selectedLinesCount = 1 + Math::FloorToInt((rightEdge.y - leftEdge.y) / textHeight);
			if (selectedLinesCount == 1)
			{
				// Selected is part of single line
				Rectangle r1 = Rectangle(leftEdge.x, leftEdge.y, rightEdge.x - leftEdge.x, fontHeight);
				Render2D::FillRectangle(r1, selectionColor);
			}
			else
			{
				float leftMargin = _layout.Bounds.Location.x;

				// Selected is more than one line
				Rectangle r1 =  Rectangle(leftEdge.x, leftEdge.y, 1000000000, fontHeight);
				Render2D::FillRectangle(r1, selectionColor);
				//
				for (int i = 3; i <= selectedLinesCount; i++)
				{
					leftEdge.y += textHeight;
					Rectangle r =  Rectangle(leftMargin, leftEdge.y, 1000000000, fontHeight);
					Render2D::FillRectangle(r, selectionColor);
				}
				//
				Rectangle r2 = Rectangle(leftMargin, rightEdge.y, rightEdge.x - leftMargin, fontHeight);
				Render2D::FillRectangle(r2, selectionColor);
			}
		}

		// Text or watermark
		if (text.Length() > 0)
		{
			Color color = TextColor;
			if (!enabled)
				color *= 0.6f;
			Render2D::RenderText(font, text, color, _layout, TextMaterial);
		}
		else if (!_watermarkText.IsEmpty())
		{
			Render2D::RenderText(font, _watermarkText, WatermarkTextColor, _layout, TextMaterial);
		}

		// Caret
		if (IsFocused && CaretPosition > -1)
		{
			float alpha = Math::Saturate(Math::Cos(_animateTime * CaretFlashSpeed) * 0.5f + 0.7f);
			alpha = alpha * alpha * alpha * alpha * alpha * alpha;
			if (CaretPosition == 0)
			{
				Rectangle bounds = CaretBounds;
				if (_layout.VerticalAlignment == TextAlignment::Center)
					bounds.SetY(_layout.Bounds.GetY() + _layout.Bounds.GetHeight() * 0.5f - bounds.GetHeight() * 0.5f);
				else if (_layout.VerticalAlignment == TextAlignment::Far)
					bounds.SetY(_layout.Bounds.GetY() + _layout.Bounds.GetHeight() - bounds.GetHeight());

				if (_layout.HorizontalAlignment == TextAlignment::Center)
					bounds.SetX(_layout.Bounds.GetX() + _layout.Bounds.GetWidth() * 0.5f - bounds.GetWidth() * 0.5f);
				else if (_layout.HorizontalAlignment == TextAlignment::Far)
					bounds.SetX(_layout.Bounds.GetX() + _layout.Bounds.GetWidth() - bounds.GetWidth());
				Render2D::FillRectangle(bounds, CaretColor * alpha);
			}
			else
			{
				Render2D::FillRectangle(CaretBounds, CaretColor * alpha);
			}

		}

		// Restore rendering state
		if (useViewOffset)
			Render2D::PopTransform();
		if (ClipText)
			Render2D::PopClip();
	}
	
	bool TextBox::OnMouseDoubleClick(Float2 location, MouseButton button)
	{
		if (IsSelectable)
		{
			SelectAll();
		}
		return TextBoxBase::OnMouseDoubleClick(location, button);
	}
	
	void TextBox::OnIsMultilineChanged()
	{
		TextBoxBase::OnIsMultilineChanged();

		_layout.VerticalAlignment = IsMultiline ? TextAlignment::Near : TextAlignment::Center;
	}
	
	void TextBox::OnSizeChanged()
	{
		TextBoxBase::OnSizeChanged();

		_layout.Bounds = TextRectangle;
	}

	::SE::Font* TextBox::GetFont()
	{
		::SE::Font* font;
		if (Bold)
			font = Italic ? Font.GetBold().GetItalic().GetFont() : Font.GetBold().GetFont();
		else if (Italic)
			font = Font.GetItalic().GetFont();
		else
			font = Font.GetFont();
		return font;
	}

	String TextBox::ConvertedText()
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
} // SE
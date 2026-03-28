

#include "NavigationButton.h"

#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/Style.h"

namespace SE::Editor
{
	NavigationButton::NavigationButton() : m_ValidDragOver(false)
	{
		Height = 0;
	}

	NavigationButton::NavigationButton(float x, float y, float height) : Button(x, y, 2 * DefaultMargin), m_ValidDragOver(false)
	{
		Height = height;
	}

	void NavigationButton::Draw()
	{
		// Cache data
		Style* style = Style::Current;
		Rectangle clientRect = Rectangle(Float2::Zero, Size);
		Rectangle textRect = Rectangle(4, 0, clientRect.GetWidth() - 4, clientRect.GetHeight());

		// Draw background
		if (IsDragOver() && m_ValidDragOver)
		{
			Render2D::FillRectangle(clientRect, style->Selection);
			Render2D::DrawRectangle(clientRect, style->SelectionBorder);
		}
		else if (isPressed)
		{
			Render2D::FillRectangle(clientRect, style->BackgroundSelected);
		}
		else if (IsMouseOver)
		{
			Render2D::FillRectangle(clientRect, style->BackgroundHighlighted);
		}

		// Draw text
		Render2D::RenderText(style->FontMedium, Text, textRect, EnabledInHierarchy() ? style->Foreground : style->ForegroundDisabled,
			TextAlignment::Near, TextAlignment::Center);
	}
	
	void NavigationButton::PerformLayout(bool force)
	{
		Style* style = Style::Current;

		if (style->FontMedium)
		{
			Width = style->FontMedium->MeasureText(Text).x + 2 * DefaultMargin;
		}
	}
} // SE
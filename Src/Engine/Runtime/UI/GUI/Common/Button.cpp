
#include "Button.h"

#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/Style.h"
#include "Runtime/UI/GUI/Brushes/IBrush.h"

namespace SE
{
	float Button::DefaultHeight = 24.0f;

	Button::Button() : ContainerControl(0, 0)
	{
	}

	Button::Button(float x, float y, float width, float height) : ContainerControl(x, y, width, height)
	{
		Style* style = Style::Current;
		if (style != nullptr)
		{
			Font = style->FontMedium;
			TextColor = style->Foreground;
			BackgroundColor = style->BackgroundNormal;
			BorderColor = style->BorderNormal;
			BackgroundColorSelected = style->BackgroundSelected;
			BorderColorSelected = style->BorderSelected;
			BackgroundColorHighlighted = style->BackgroundHighlighted;
			BorderColorHighlighted = style->BorderHighlighted;
		}
	}

	Button::Button(Float2 location, Float2 size) : ContainerControl(location.x, location.y, size.x, size.y)
	{
	}

	void Button::OnClick()
	{
		Clicked();
		ButtonClicked(this);
	}

	void Button::OnPressBegin()
	{
		isPressed = true;
		if (AutoFocus)
			Focus();
	}

	void Button::OnPressEnd()
	{
		isPressed = false;
	}

	void Button::SetColors(Color color)
	{
		BackgroundColor = color;
		BorderColor = color.RGBMultiplied(0.5f);
		BackgroundColorSelected = color.RGBMultiplied(0.8f);
		BorderColorSelected = BorderColor;
		BackgroundColorHighlighted = color.RGBMultiplied(1.2f);
		BorderColorHighlighted = BorderColor;
	}

	void Button::ClearState()
	{
		ContainerControl::ClearState();

		if (isPressed)
			OnPressEnd();
	}

	void Button::DrawSelf()
	{
		// Cache data
		Rectangle clientRect = Rectangle(Float2::Zero, Size);
		bool enabled = EnabledInHierarchy();
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
		if (BackgroundBrush != nullptr)
			BackgroundBrush->Draw(clientRect, backgroundColor);
		else
			Render2D::FillRectangle(clientRect, backgroundColor);
		if (HasBorder)
			Render2D::DrawRectangle(clientRect, borderColor, BorderThickness);

		// Draw text
		Render2D::RenderText(Font, TextMaterial, Text, clientRect, textColor, TextAlignment::Center, TextAlignment::Center);
	}

	void Button::OnMouseEnter(Float2 location)
	{
		ContainerControl::OnMouseEnter(location);

		HoverBegin();
	}

	void Button::OnMouseLeave()
	{
		if (isPressed)
		{
			OnPressEnd();
		}

		HoverEnd();

		ContainerControl::OnMouseLeave();
	}

	bool Button::OnMouseDown(Float2 location, MouseButton button)
	{
		if (ContainerControl::OnMouseDown(location, button))
			return true;

		if (button == MouseButton::Left && !isPressed)
		{
			OnPressBegin();
			return true;
		}
		return false;
	}

	bool Button::OnMouseUp(Float2 location, MouseButton button)
	{
		if (ContainerControl::OnMouseUp(location, button))
			return true;

		if (button == MouseButton::Left && isPressed)
		{
			OnPressEnd();
			OnClick();
			return true;
		}
		return false;
	}

	bool Button::OnMouseDoubleClick(Float2 location, MouseButton button)
	{
		if (ContainerControl::OnMouseDoubleClick(location, button))
			return true;

		if (button == MouseButton::Left && isPressed)
		{
			OnPressEnd();
			OnClick();
			return true;
		}

		if (button == MouseButton::Left && !isPressed)
		{
			OnPressBegin();
			return true;
		}

		return false;
	}

	bool Button::OnTouchDown(Float2 location, int pointerId)
	{
		if (ContainerControl::OnTouchDown(location, pointerId))
			return true;

		if (!isPressed)
		{
			OnPressBegin();
			return true;
		}
		return false;
	}

	bool Button::OnTouchUp(Float2 location, int pointerId)
	{
		if (ContainerControl::OnTouchUp(location, pointerId))
			return true;

		if (isPressed)
		{
			OnPressEnd();
			OnClick();
			return true;
		}
		return false;
	}

	void Button::OnTouchLeave()
	{
		if (isPressed)
		{
			OnPressEnd();
		}

		ContainerControl::OnTouchLeave();
	}

	void Button::OnLostFocus()
	{
		if (isPressed)
		{
			OnPressEnd();
		}

		ContainerControl::OnLostFocus();
	}

	void Button::OnSubmit()
	{
		OnClick();

		ContainerControl::OnSubmit();
	}
} // SE
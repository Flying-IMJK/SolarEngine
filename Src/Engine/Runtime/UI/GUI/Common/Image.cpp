
#include "Image.h"

#include "Runtime/UI/GUI/Brushes/IBrush.h"

namespace SE
{
	Image::Image() : ContainerControl(0, 0, 64, 64), Brush()
	{
		AutoFocus = false;
	}

	Image::Image(float x, float y, float width, float height) : ContainerControl(x, y, width, height), Brush()
	{
		AutoFocus = false;
	}

	Image::Image(Float2 location, Float2 size) : ContainerControl(location, size), Brush()
	{
		AutoFocus = false;
	}

	Image::Image(Rectangle bounds) : ContainerControl(bounds), Brush()
	{
		AutoFocus = false;
	}

	void Image::DrawSelf()
	{
		ContainerControl::DrawSelf();

		if (Brush == nullptr)
			return;

		Rectangle rect;
		if (KeepAspectRatio)
		{
			// Figure out the ratio
			Float2 size = Size;
			Float2 imageSize = Brush->Size;
			if (imageSize.LengthSquared() < 1)
				return;
			Float2 ratio = size / imageSize;
			Float2 aspectRatio = ratio.MinValue();

			// Get the new height and width
			Float2 newSize = imageSize * aspectRatio;

			// Calculate the X,Y position of the upper-left corner
			// (one of these will always be zero)
			Float2 newPos = (size - newSize) / Float2(2);

			rect = Rectangle(newPos, newSize);
		}
		else
		{
			rect = Rectangle(Float2::Zero, Size);
		}

		Margin.ShrinkRectangle(rect);

		::SE::Color color = IsMouseOver || IsNavFocused ? MouseOverColor : Color;
		if (!Enabled)
			color *= DisabledTint;
		Brush->Draw(rect, color);
	}

	bool Image::OnMouseUp(Float2 location, MouseButton button)
	{
		if (ContainerControl::OnMouseUp(location, button))
			return true;

		if (Clicked.IsBinded())
		{
			Clicked(this, button);
			return true;
		}

		return false;
	}
} // SE
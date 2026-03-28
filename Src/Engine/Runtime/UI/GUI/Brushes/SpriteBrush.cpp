
#include "SpriteBrush.h"

#include "Runtime/Render/2D/Render2D.h"

namespace SE
{
	void SpriteBrush::Draw(Rectangle rect, Color color)
	{
		if (Filter == BrushFilter::Point)
			Render2D::DrawSpritePoint(Sprite, rect, color);
		else
			Render2D::DrawSprite(Sprite, rect, color);
	}

	Float2 SpriteBrush::__GetSize()
	{
		return Sprite.IsValid() ? Sprite.Size : Float2::Zero;
	}
} // SE
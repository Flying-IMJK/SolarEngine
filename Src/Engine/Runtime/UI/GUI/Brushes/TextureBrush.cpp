
#include "TextureBrush.h"
#include "Runtime/Core/Math/Vector2.h"
#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/Render/Assets/Texture/Texture.h"

namespace SE
{
	TextureBrush::TextureBrush(AssetRef<::SE::Texture> texture)
	{
		Texture = texture;
	}

	void TextureBrush::Draw(Rectangle rect, Color color)
	{
		if (Filter == BrushFilter::Point)
			Render2D::DrawTexturePoint(Texture->GetTexture(), rect, color);
		else
			Render2D::DrawTexture(Texture, rect, color);
	}

	Float2 TextureBrush::__GetSize()
	{
		return Texture != nullptr && !Texture->WaitForLoaded() ? Texture->Size() : Float2::Zero;
	}

	Texture9SlicingBrush::Texture9SlicingBrush(AssetRef<::SE::Texture> texture)
	{
		Texture = texture;
	}

	void Texture9SlicingBrush::Draw(Rectangle rect, Color color)
	{
		if (Texture == nullptr)
			return;
		Margin border = Border;
		Float4 borderUV = *(Float4*)&border;
		Float4 borderSize = borderUV * Float4(BorderSize, BorderSize, BorderSize, BorderSize);
		if (Filter == BrushFilter::Point)
			Render2D::Draw9SlicingTexturePoint(Texture, rect, borderSize, borderUV, color);
		else
			Render2D::Draw9SlicingTexture(Texture, rect, borderSize, borderUV, color);
#if SE_EDITOR
		if (ShowBorders)
		{
			Rectangle bordersRect = rect;
			bordersRect.Location.x += borderSize.x;
			bordersRect.Location.y += borderSize.z;
			bordersRect.Size.x -= borderSize.x + borderSize.y;
			bordersRect.Size.y -= borderSize.z + borderSize.w;
			Render2D::DrawRectangle(bordersRect, Colors::YellowGreen, 2.0f);
		}
#endif
	}

	Float2 Texture9SlicingBrush::__GetSize()
	{
		return Texture != nullptr ? Texture->Size() : Float2::Zero;
	}
} // SE
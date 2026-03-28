
#include "TexturePreview.h"

#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/Render/Assets/Texture/Texture.h"
#include "Runtime/UI/GUI/Style.h"

namespace SE::Editor
{
	void TexturePreviewBase::CenterView()
	{
		m_ViewScale = 1.0f;
		m_ViewPos = Float2::Zero;
	}
	
	void TexturePreviewBase::Draw()
	{
		Render2D::PushClip(Rectangle(Float2::Zero, Size));

		// Calculate texture view rectangle
		UpdateTextureRect();
		Rectangle textureRect = GetTextureViewRect();

		// Call drawing
		DrawTexture(textureRect);

		// Add overlay during debugger breakpoint hang
		/*if (Editor.Instance.Simulation.IsDuringBreakpointHang)
		{
			Rectangle bounds = Rectangle(Float2::Zero, Size);
			Render2D::FillRectangle(bounds,  Color(0.0f, 0.0f, 0.0f, 0.2f));
			Render2D::RenderText(Style::Current->FontLarge, SE_TEXT("Debugger breakpoint hit..."), bounds, Colors::White, TextAlignment::Center, TextAlignment::Center);
		}*/

		Render2D::PopClip();

		ContainerControl::Draw();
	}
	
	void TexturePreviewBase::OnMouseEnter(Float2 location)
	{
		// Store mouse position
		m_LastMousePos = location;

		ContainerControl::OnMouseEnter(location);
	}
	
	void TexturePreviewBase::OnMouseMove(Float2 location)
	{
		// Check if mouse is down
		if (m_IsMouseDown)
		{
			// Calculate mouse delta
			Float2 delta = location - m_LastMousePos;

			// Move view
			m_ViewPos += delta;
		}

		// Store mouse position
		m_LastMousePos = location;

		ContainerControl::OnMouseMove(location);
	}
	
	void TexturePreviewBase::OnMouseLeave()
	{
		// Clear flag
		m_IsMouseDown = false;
		Cursor = CursorType::Default;

		ContainerControl::OnMouseLeave();
	}
	
	bool TexturePreviewBase::OnMouseWheel(Float2 location, float delta)
	{
		if (ContainerControl::OnMouseWheel(location, delta))
			return true;

		// Zoom
		float prevScale = m_ViewScale;
		m_ViewScale = Math::Clamp(m_ViewScale + delta * 0.24f, 0.001f, 20.0f);

		// Compensate for the Rectangle.MakeScaled
		Float2 sizeDelta = (m_ViewScale - prevScale) * m_TextureRect.Size * 0.5f;
		m_ViewPos += sizeDelta * 0.5f;

		// Move to zoom position
		Float2 locationOnTexture = (location - m_TextureRect.Location) / m_TextureRect.Size;
		m_ViewPos -= sizeDelta * locationOnTexture;

		return true;
	}
	
	bool TexturePreviewBase::OnMouseDown(Float2 location, MouseButton button)
	{
		if (ContainerControl::OnMouseDown(location, button))
			return true;

		// Set flag
		m_IsMouseDown = true;
		m_LastMousePos = location;
		Cursor = CursorType::SizeAll;

		return true;
	}
	
	bool TexturePreviewBase::OnMouseUp(Float2 location, MouseButton button)
	{
		if (ContainerControl::OnMouseUp(location, button))
			return true;

		// Clear flag
		m_IsMouseDown = false;
		Cursor = CursorType::Default;

		return true;
	}

	TexturePreviewBase::TexturePreviewBase()
	{
		AnchorPreset = AnchorPresets::StretchAll;
		Offsets = Margin::Zero;
	}

	void TexturePreviewBase::UpdateTextureRect()
	{
		CalculateTextureRect(m_TextureRect);
	}

	void TexturePreviewBase::CalculateTextureRect(Float2 textureSize, Float2 viewSize, Rectangle& result)
	{
		Float2 size = Float2::Max(textureSize, Float2::One);
		float aspectRatio = size.x / size.y;
		float h = viewSize.x / aspectRatio;
		float w = viewSize.y * aspectRatio;
		if (w > h)
		{
			float diff = (viewSize.y - h) * 0.5f;
			result = Rectangle(0, diff, viewSize.x, h);
		}
		else
		{
			float diff = (viewSize.x - w) * 0.5f;
			result = Rectangle(diff, 0, w, viewSize.y);
		}
	}

	Rectangle TexturePreviewBase::GetTextureViewRect()
	{
		return (m_TextureRect + m_ViewPos) * m_ViewScale;
	}

	void TexturePreviewBase::OnSizeChanged()
	{
		ContainerControl::OnSizeChanged();

		// Update texture rectangle and move view to the center
		UpdateTextureRect();
		CenterView();
	}

	void SimpleTexturePreview::CalculateTextureRect(Rectangle& rect)
	{
		TexturePreviewBase::CalculateTextureRect(m_Asset != nullptr ? m_Asset->Size() : Float2(100), Size, rect);
	}
	
	void SimpleTexturePreview::DrawTexture(Rectangle& rect)
	{
		// Background
		Render2D::FillRectangle(rect, Colors::Gray);

		// Check if has loaded asset
		if (m_Asset && m_Asset->IsLoaded())
		{
			Render2D::DrawTexture(m_Asset, rect, Color);
		}
	}

	Texture* SimpleTexturePreview::__GetAsset()
	{
		return m_Asset;
	}

	void SimpleTexturePreview::__SetAsset(Texture* value)
	{
		if (m_Asset != value)
		{
			m_Asset = value;
			UpdateTextureRect();
		}
	}

	void SimpleSpriteAtlasPreview::CalculateTextureRect(Rectangle& rect)
	{
		TexturePreviewBase::CalculateTextureRect(_asset != nullptr ? _asset->Size() : Float2(100), Size, rect);
	}

	void SimpleSpriteAtlasPreview::DrawTexture(Rectangle& rect)
	{
		// Background
		Render2D::FillRectangle(rect, Colors::Gray);

		// Check if has loaded asset
		if (_asset && _asset->IsLoaded())
		{
			Render2D::DrawTexture(_asset, rect, Color);
		}
	}

	SpriteAtlas* SimpleSpriteAtlasPreview::__GetAsset()
	{
		return _asset;
	}

	void SimpleSpriteAtlasPreview::__SetAsset(SpriteAtlas* value)
	{
		if (_asset != value)
		{
			_asset = value;
			UpdateTextureRect();
		}
	}

	/*void TexturePreviewCustomBase::OnDestroy()
	{
		Object.Destroy(_previewMaterial);

		TexturePreviewBase::OnDestroy();
	}*/
} // SE


#include "ToolStripButton.h"

#include "ToolStrip.h"
#include "ContextMenu/ContextMenu.h"
#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/Style.h"

namespace SE::Editor
{
	ToolStripButton::ToolStripButton(float height, SpriteHandle icon) : Control(0, 0, height, height)
	{
		_icon = icon;
	}

	ToolStripButton* ToolStripButton::SetAutoCheck(bool value)
	{
		AutoCheck = value;
		return this;
	}

	void ToolStripButton::Draw()
	{
		Control::Draw();

		// Cache data
		Style* style = Style::Current;
		float iconSize = Height - DefaultMargin;
		Rectangle clientRect = Rectangle(Float2::Zero, Size);
		Rectangle iconRect = Rectangle(DefaultMargin, DefaultMargin, iconSize, iconSize);
		Rectangle textRect = Rectangle(DefaultMargin, 0, 0, Height);
		bool enabled = EnabledInHierarchy();
		bool mouseButtonDown = _primaryMouseDown || _secondaryMouseDown;

		// Draw background
		if (enabled && (IsMouseOver || IsNavFocused || Checked))
		{
			Color color = style->Background * 1.3f;
			if (Checked)
			{
				color = style->BackgroundSelected;
			}else if (mouseButtonDown)
			{
				color = style->BackgroundHighlighted;
			}
			Render2D::FillRectangle(clientRect, color);
		}

		// Draw icon
		if (_icon.IsValid())
		{
			Render2D::DrawSprite(_icon, iconRect, enabled ? style->Foreground : style->ForegroundDisabled);
			textRect.Location.x += iconSize + DefaultMargin;
		}

		// Draw text
		if (_text.Length() > 0)
		{
			textRect.Size.x = Width - DefaultMargin - textRect.GetLeft();
			Render2D::RenderText(style->FontMedium, _text, textRect, enabled ? style->TextColor : style->ForegroundDisabled,
				TextAlignment::Near, TextAlignment::Center);
		}
	}
	
	void ToolStripButton::PerformLayout(bool force)
	{
		Style* style = Style::Current;
		float iconSize = Height - DefaultMargin;
		bool hasSprite = _icon.IsValid();
		float width = DefaultMargin * 2;

		if (hasSprite)
			width += iconSize;
		if (_text.Length() > 0 && style->FontMedium)
			width += style->FontMedium->MeasureText(_text).x + (hasSprite ? DefaultMargin : 0);

		Width = width;
	}
	
	bool ToolStripButton::OnMouseDown(Float2 location, MouseButton button)
	{
		if (button == MouseButton::Left)
		{
			_primaryMouseDown = true;
			Focus();
			return true;
		}
		if (button == MouseButton::Right)
		{
			_secondaryMouseDown = true;
			Focus();
			return true;
		}

		return Control::OnMouseDown(location, button);
	}
	
	bool ToolStripButton::OnMouseUp(Float2 location, MouseButton button)
	{
		if (button == MouseButton::Left && _primaryMouseDown)
		{
			_primaryMouseDown = false;
			if (AutoCheck)
				Checked = !Checked;
			Clicked();
			ToolStrip* toolStrip;
			if (TypeTryCast<ToolStrip>(Parent, toolStrip))
			{
				toolStrip->OnButtonClicked(this);
			}

			return true;
		}
		if (button == MouseButton::Right && _secondaryMouseDown)
		{
			_secondaryMouseDown = false;
			SecondaryClicked();
			ToolStrip* toolStrip;
			if (TypeTryCast<ToolStrip>(Parent, toolStrip))
			{
				toolStrip->OnButtonClicked(this);
			}
			if (ContextMenu != nullptr)
			{
				ContextMenu->Show(this, Float2(0, Height));
			}
			return true;
		}

		return Control::OnMouseUp(location, button);
	}
	
	void ToolStripButton::OnMouseLeave()
	{
		_primaryMouseDown = false;
		_secondaryMouseDown = false;

		Control::OnMouseLeave();
	}
	
	void ToolStripButton::OnLostFocus()
	{
		_primaryMouseDown = false;
		_secondaryMouseDown = false;

		Control::OnLostFocus();
	}
} // SE
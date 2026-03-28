
#include "ContextMenuButton.h"

#include "ContextMenu.h"
#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/Render/Assets/Texture/Texture.h"
#include "Runtime/UI/GUI/Style.h"

namespace SE::Editor
{

	ContextMenuButton::ContextMenuButton(ContextMenu* parent, String text, String shortKeys) : ContextMenuItem(parent, 8, 22)
	{
		Text = text;
		ShortKeys = shortKeys;
	}
	
	ContextMenuButton* ContextMenuButton::SetAutoCheck(bool value)
	{
		AutoCheck = value;
		return this;
	}
	
	ContextMenuButton* ContextMenuButton::SetChecked(bool value)
	{
		Checked = value;
		return this;
	}
	
	void ContextMenuButton::Click()
	{
		if (CloseMenuOnClick && ParentContextMenu != nullptr)
		{
			// Close topmost context menu
			ParentContextMenu->TopmostCM->Hide();
		}

		// Auto check logic
		if (AutoCheck)
		{
			Checked = !Checked;
		}

		// Fire event
		Clicked();
		ButtonClicked(this);
		if (ParentContextMenu != nullptr)
		{
			ParentContextMenu->OnButtonClicked(this);
		}
	}
	
	void ContextMenuButton::Draw()
	{
		Style* style = Style::Current;
		Rectangle backgroundRect = Rectangle(-X + 3, 0, Parent->Width - 6, Height);
		Rectangle textRect = Rectangle(0, 0, Width - 8, Height);
		Color textColor = Enabled ? style->TextColor : style->TextColor;

		// Draw background
		if (IsMouseOver && Enabled)
			Render2D::FillRectangle(backgroundRect, style->LightBackground);
		else if (IsFocused)
			Render2D::FillRectangle(backgroundRect, style->LightBackground);

		ContextMenuItem::Draw();

		// Draw text
		Render2D::RenderText(style->FontMedium, Text, textRect, textColor, TextAlignment::Near, TextAlignment::Center);

		if (ShortKeys.Length() > 0)
		{
			// Draw short keys
			Render2D::RenderText(style->FontMedium, ShortKeys, Rectangle(textRect.GetX() + ExtraAdjustmentAmount, textRect.GetY(), textRect.GetWidth(), textRect.GetHeight()),
				textColor, TextAlignment::Far, TextAlignment::Center);
		}

		// Draw icon
		const float iconSize = 14;
		SpriteHandle icon = Checked ? style->CheckBoxTick : Icon;
		if (icon.IsValid())
		{
			Render2D::DrawSprite(icon, Rectangle(-iconSize - 1, (Height - iconSize) / 2, iconSize, iconSize), textColor);
		}
	}
	
	void ContextMenuButton::OnMouseLeave()
	{
		_isMouseDown = false;

		ContextMenuItem::OnMouseLeave();
	}
	
	bool ContextMenuButton::OnMouseDown(Float2 location, MouseButton button)
	{
		if (ContextMenuItem::OnMouseDown(location, button))
			return true;

		_isMouseDown = true;
		return true;
	}
	
	bool ContextMenuButton::OnMouseUp(Float2 location, MouseButton button)
	{
		if (ContextMenuItem::OnMouseUp(location, button))
			return true;

		if (_isMouseDown)
		{
			_isMouseDown = false;
			Click();
			return true;
		}

		return false;
	}
	
	bool ContextMenuButton::OnKeyDown(KeyboardKeys key)
	{
		if (ContextMenuItem::OnKeyDown(key))
			return true;

		if (key == KeyboardKeys::ArrowUp)
		{
			for (int i = IndexInParent - 1; i >= 0; i--)
			{
				Control* c = ParentContextMenu->ItemsContainer->Children()[i];
				ContextMenuButton* item;
				if (TypeTryCast<ContextMenuButton>(c, item) && item->Visible && item->Enabled)
				{
					item->Focus();
					ParentContextMenu->ItemsContainer->ScrollViewTo(item);
					return true;
				}
			}
		}
		else if (key == KeyboardKeys::ArrowDown)
		{
			List<Control*>& children = ParentContextMenu->ItemsContainer->Children();

			for (int i = IndexInParent + 1; i < children.Count(); i++)
			{
				Control* c = children[i];
				ContextMenuButton* item;
				if (TypeTryCast<ContextMenuButton>(c, item) && item->Visible && item->Enabled)
				{
					item->Focus();
					ParentContextMenu->ItemsContainer->ScrollViewTo(item);
					return true;
				}
			}
		}
		else if (key == KeyboardKeys::Return)
		{
			Click();
		}
		else if (key == KeyboardKeys::Escape)
		{
			ParentContextMenu->Hide();
		}

		return false;
	}
	
	void ContextMenuButton::OnLostFocus()
	{
		_isMouseDown = false;

		ContextMenuItem::OnLostFocus();
	}
	
	float ContextMenuButton::__GetMinimumWidth()
	{
		Style* style = Style::Current;
		float width = 20;
		if (style->FontMedium)
		{
			width += style->FontMedium->MeasureText(Text).x;
			if (ShortKeys.Length() > 0)
				width += 40 + style->FontMedium->MeasureText(ShortKeys).x;
		}

		return Math::Max(width, ContextMenuItem::__GetMinimumWidth());
	}
} // SE
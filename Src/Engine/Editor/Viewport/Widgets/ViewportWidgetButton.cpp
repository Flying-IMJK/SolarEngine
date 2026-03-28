//
// Created by 10303 on 25-12-26.
//

#include "ViewportWidgetButton.h"

#include "ViewportWidgetsContainer.h"
#include "Editor/GUI/Contextmenu/Contextmenu.h"
#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/Style.h"

namespace SE::Editor
{
	ViewportWidgetButton::ViewportWidgetButton(StringView text, SpriteHandle icon, ContextMenu* contextMenu, bool autoCheck, float textWidth)
		: Control(0, 0, CalculateButtonWidth(textWidth, icon.IsValid()), ViewportWidgetsContainer::WidgetsHeight)
	{
		_text = text;
		Icon = icon;
		_cm = contextMenu;
		_autoCheck = autoCheck;
		_forcedTextWidth = textWidth;

		if (_cm != nullptr)
		{
			_cm->VisibleChanged.BindUnique<ViewportWidgetButton, &ViewportWidgetButton::CmOnVisibleChanged>(this);
		}
	}

	void ViewportWidgetButton::Draw()
	{
		// Cache data
		Style* style = Style::Current;
		const float iconSize = ViewportWidgetsContainer::WidgetsIconSize;
		Rectangle iconRect = Rectangle(0, (Height - iconSize) / 2, iconSize, iconSize);
		Rectangle textRect = Rectangle(0, 0, Width + 1, Height + 1);

		// Check if is checked or mouse is over and auto check feature is enabled
		if (_checked)
			Render2D::FillRectangle(textRect, style->BackgroundSelected * (IsMouseOver ? 0.9f : 0.6f));
		else if (_autoCheck && IsMouseOver)
			Render2D::FillRectangle(textRect, style->BackgroundHighlighted);

		// Check if has icon
		if (Icon.IsValid())
		{
			// Draw icon
			Render2D::DrawSprite(Icon, iconRect, style->ForegroundViewport);

			// Update text rectangle
			textRect.Location.x += iconSize;
			textRect.Size.x -= iconSize;
		}

		// Draw text
		Render2D::RenderText(style->FontMedium, _text, textRect, style->ForegroundViewport * (IsMouseOver ? 1.0f : 0.9f), TextAlignment::Center, TextAlignment::Center);
	}

	bool ViewportWidgetButton::OnMouseDown(Float2 location, MouseButton button)
	{
		if (button == MouseButton::Left)
		{
			_isMosueDown = true;
		}
		if (_autoCheck)
		{
			// Toggle
			Checked = !_checked;
			if (Toggled.IsBinded())
			{
				Toggled(this);
			}
		}

		if (_cm != nullptr)
		{
			_cm->Show(this, Float2(-1, Height + 2));
		}

		return Control::OnMouseDown(location, button);
	}

	bool ViewportWidgetButton::OnMouseUp(Float2 location, MouseButton button)
	{
		if (button == MouseButton::Left && _isMosueDown)
		{
			_isMosueDown = false;
			if (Clicked.IsBinded())
			{
				Clicked(this);
			}
		}

		return Control::OnMouseUp(location, button);
	}

	void ViewportWidgetButton::PerformLayout(bool force)
	{
		Style* style = Style::Current;

		if (style != nullptr && style->FontMedium)
			Width = CalculateButtonWidth(_forcedTextWidth > 0.0f ? _forcedTextWidth : style->FontMedium->MeasureText(_text).x, Icon.IsValid());
	}

	void ViewportWidgetButton::CmOnVisibleChanged(Control* control)
	{
		if (_cm != nullptr && !_cm->IsOpened)
		{
			if (HasParent() && Parent->HasParent())
			{
				// Focus viewport
				Parent->Parent->Focus();
			}
		}
	}

	float ViewportWidgetButton::CalculateButtonWidth(float textWidth, bool hasIcon)
	{
		return (hasIcon ? ViewportWidgetsContainer::WidgetsIconSize : 0) + (textWidth > 0 ? textWidth + 8 : 0);
	}

	void ViewportWidgetButton::__SetText(String& value)
	{
		_text = value;
		PerformLayout();
	}
} // SE

#include "ContextMenuChildMenu.h"

#include "ContextMenu.h"
#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/Style.h"

namespace SE::Editor
{

	ContextMenuChildMenu::ContextMenuChildMenu(::SE::Editor::ContextMenu* parent, String text) : ContextMenuButton(parent, text)
	{
		ContextMenu = New<::SE::Editor::ContextMenu>();
		Text = text;
		CloseMenuOnClick = false;
	}

	void ContextMenuChildMenu::Draw()
	{
		Style* style = Style::Current;
		Rectangle backgroundRect = Rectangle(-X + 3, 0, Parent->Width - 6, Height);
		bool isCMopened = ContextMenu->IsOpened;

		// Draw background
		if (isCMopened)
			Render2D::FillRectangle(backgroundRect, style->LightBackground);

		ContextMenuButton::Draw();

		// Draw arrow
		if (ContextMenu->HasChildren())
		{
			Rectangle rectangle = Rectangle(Width - 15 + ExtraAdjustmentAmount, (Height - 12) / 2, 12, 12);
			Render2D::DrawSprite(style->ArrowRight, rectangle, Enabled ? isCMopened ? style->BackgroundSelected : style->Foreground : style->ForegroundDisabled);
		}
	}

	void ContextMenuChildMenu::OnMouseEnter(Float2 location)
	{
		// Skip if has no children
		if (ContextMenu->HasChildren() == false)
			return;

		// Skip if already shown
		::SE::Editor::ContextMenu* parentContextMenu = ParentContextMenu;
		if (parentContextMenu == ContextMenu)
			return;
		if (ContextMenu->IsOpened)
			return;

		ContextMenuButton::OnMouseEnter(location);

		ShowChild(parentContextMenu);
	}

	bool ContextMenuChildMenu::OnMouseUp(Float2 location, MouseButton button)
	{
		// Skip if already shown
		::SE::Editor::ContextMenu* parentContextMenu = ParentContextMenu;
		if (parentContextMenu == ContextMenu)
			return true;
		if (ContextMenu->IsOpened)
			return true;

		ShowChild(parentContextMenu);
		return ContextMenuButton::OnMouseUp(location, button);
	}

	void ContextMenuChildMenu::ShowChild(::SE::Editor::ContextMenu* parentContextMenu)
	{
		// Hide parent CM popups and set itself as child
		float vAlign = parentContextMenu->ItemsAreaMargin.operator->().Top;
		Float2 location = Float2(Width, -vAlign);
		location = PointToParent(parentContextMenu, location);
		parentContextMenu->ShowChild(ContextMenu, location);
	}
} // SE
#include "Runtime/Render/2D/Render2D.h"
#include "ContextMenuSeparator.h"

#include "Runtime/UI/GUI/Style.h"

namespace SE::Editor
{

	ContextMenuSeparator::ContextMenuSeparator(ContextMenu* parent) : ContextMenuItem(parent, 8, 4)
	{
	}

	void ContextMenuSeparator::Draw()
	{
		ContextMenuItem::Draw();

		// Draw separator line
		Render2D::FillRectangle(Rectangle(0, 1, Width - 4, 1), Style::Current->LightBackground);
	}
} // SE
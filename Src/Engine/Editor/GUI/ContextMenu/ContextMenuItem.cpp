

#include "ContextMenuItem.h"

#include "ContextMenu.h"

namespace SE::Editor
{
	ContextMenuItem::ContextMenuItem()
	{
		AutoFocus = false;
		ParentContextMenu = nullptr;
	}

	void ContextMenuItem::OnMouseEnter(Float2 location)
	{
		ParentContextMenu->HideChild();

		ContainerControl::OnMouseEnter(location);
	}

	float ContextMenuItem::__GetMinimumWidth()
	{
		float width = 0;

		for (int i = 0; i < m_Children.Count(); i++)
		{
			Control* c = m_Children[i];
			if (c->Visible)
			{
				width = Math::Max(width, c->Right + 4.0f);
			}
		}

		return width;
	}
} // SE


#include "NavigationBar.h"

#include "ToolStrip.h"
#include "ToolStripButton.h"

namespace SE::Editor
{
	NavigationBar::NavigationBar() : Panel(ScrollBars::Horizontal)
	{
	}

	void NavigationBar::UpdateBounds(ToolStrip* toolstrip)
	{
		if (toolstrip == nullptr)
			return;
		ToolStripButton* lastToolstripButton = toolstrip->LastButton;
		Float2 parentSize = Parent->Size;
		Bounds = Rectangle(lastToolstripButton->Right + 8.0f, 0, parentSize.x - X - 8.0f, toolstrip->Height);
	}

	void NavigationBar::Arrange()
	{
		Panel::Arrange();

		// Arrange buttons
		float x = DefaultButtonsMargin;
		for (int i = 0; i < m_Children.Count(); i++)
		{
			Control* child = m_Children[i];
			if (child->IsScrollable)
			{
				child->X = x;
				x += child->Width + DefaultButtonsMargin;
			}
		}
	}
} // SE
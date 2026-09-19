

#include "VerticalPanel.h"

namespace SE
{
	void VerticalPanel::PerformLayoutBeforeChildren()
	{
		PanelWithMargins::PerformLayoutBeforeChildren();

		// Pre-set width of all controls
		float w = Width - _margin.GetWidth();
		for (int i = 0; i < m_Children.Count(); i++)
		{
			Control* c = m_Children[i];
			if (c->Visible && Math::IsZero(c->AnchorMin.operator->().X) && Math::IsZero(c->AnchorMax.operator->().X))
			{
				c->Width = w;
			}
		}
	}

	void VerticalPanel::PerformLayoutAfterChildren()
	{
		// Sort controls vertically
		float top = _margin.Top;
		float bottom = _margin.Bottom;
		float w = Width - _margin.GetWidth();
		bool hasAnyTop = false, hasAnyBottom = false;
		for (int i = 0; i < m_Children.Count(); i++)
		{
			Control* c = m_Children[i];
			if (c->Visible)
			{
				float h = c->Height;
				if (Math::IsZero(c->AnchorMin.operator->().Y) && Math::IsZero(c->AnchorMax.operator->().Y))
				{
					c->Bounds = Rectangle(_margin.Left + _offset.X, top + _offset.Y, w, h);
					top = c->Bottom + _spacing;
					hasAnyTop = true;
				}
				else if (Math::IsOne(c->AnchorMin.operator->().Y) && Math::IsOne(c->AnchorMax.operator->().Y))
				{
					bottom += h + _spacing;
					c->Bounds = Rectangle(_margin.Left + _offset.X, Height - bottom + _offset.Y, w, h);
					hasAnyBottom = true;
				}
			}
		}
		if (hasAnyTop)
			top -= _spacing;
		if (hasAnyBottom)
			bottom -= _spacing;

		// Update size
		if (_autoSize)
			Height = top + bottom;
	}
} // SE

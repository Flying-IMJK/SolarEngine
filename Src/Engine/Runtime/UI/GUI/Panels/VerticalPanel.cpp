

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
			if (c->Visible && Math::IsZero(c->AnchorMin.operator->().x) && Math::IsZero(c->AnchorMax.operator->().x))
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
				if (Math::IsZero(c->AnchorMin.operator->().y) && Math::IsZero(c->AnchorMax.operator->().y))
				{
					c->Bounds = Rectangle(_margin.Left + _offset.x, top + _offset.y, w, h);
					top = c->Bottom + _spacing;
					hasAnyTop = true;
				}
				else if (Math::IsOne(c->AnchorMin.operator->().y) && Math::IsOne(c->AnchorMax.operator->().y))
				{
					bottom += h + _spacing;
					c->Bounds = Rectangle(_margin.Left + _offset.x, Height - bottom + _offset.y, w, h);
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
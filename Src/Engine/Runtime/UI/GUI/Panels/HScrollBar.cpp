
#include "HScrollBar.h"

namespace SE
{
	HScrollBar::HScrollBar()
	{
	}

	HScrollBar::HScrollBar(ContainerControl* parent, float y, float width, float height)
		: ScrollBar(Orientation::Horizontal)
	{
		AnchorPreset = AnchorPresets::HorizontalStretchBottom;;
		Parent = parent;
		Bounds = Rectangle(0, y, width, height);
	}

	float HScrollBar::TrackSize()
	{
		return Width;
	}
} // SE
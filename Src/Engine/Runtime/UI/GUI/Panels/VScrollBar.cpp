
#include "VScrollBar.h"

namespace SE
{
	VScrollBar::VScrollBar()
	{
	}

	VScrollBar::VScrollBar(ContainerControl* parent, float x, float height, float width)
		: ScrollBar(Orientation::Vertical)
	{
		AnchorPreset = AnchorPresets::VerticalStretchRight;;
		Parent = parent;
		Bounds = Rectangle(x, 0, width, height);
	}

	float VScrollBar::TrackSize()
	{
		return Height;
	}
} // SE
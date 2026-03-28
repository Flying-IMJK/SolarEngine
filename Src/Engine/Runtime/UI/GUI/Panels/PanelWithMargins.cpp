

#include "PanelWithMargins.h"

namespace SE
{
	PanelWithMargins::PanelWithMargins() : ContainerControl(0, 0, 64, 64)
	{
		AutoFocus = false;
	}

	void PanelWithMargins::OnChildResized(Control* control)
	{
		ContainerControl::OnChildResized(control);

		PerformLayout();
	}

	bool PanelWithMargins::ContainsPoint(Float2& location, bool precise)
	{
		if (precise && BackgroundColor.a <= 0.0f) // Go through transparency
			return false;
		return ContainerControl::ContainsPoint(location, precise);
	}
} // SE
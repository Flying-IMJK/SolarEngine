
#include "StatusBar.h"

#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/WindowRootControl.h"

namespace SE::Editor
{
	StatusBar::StatusBar()
	{
		AutoFocus = false;
		AnchorPreset = AnchorPresets::HorizontalStretchBottom;
	}

	void StatusBar::Draw()
	{
		ContainerControl::Draw();

		Style* style = Style::Current;

		// Draw size grip
		WindowRootControl* window = TypeTryCast<WindowRootControl>(Root);
		if (window != nullptr && !window->IsMaximized())
			Render2D::DrawSprite(style->StatusBarSizeGrip, Rectangle(Width - 12, 10, 12, 12), style->Foreground);

		// Draw status text
		Render2D::RenderText(style->FontSmall, Text, Rectangle(4, 0, Width - 20, Height), TextColor, TextAlignment::Near, TextAlignment::Center);
	}
} // SE
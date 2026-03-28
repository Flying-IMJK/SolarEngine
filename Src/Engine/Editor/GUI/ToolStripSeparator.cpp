

#include "ToolStripSeparator.h"

#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/Style.h"

namespace SE::Editor
{

	ToolStripSeparator::ToolStripSeparator(float height) : Control(0, 0, 4, height)
	{
		AutoFocus = false;
	}

	void ToolStripSeparator::Draw()
	{
		Control::Draw();

		// Draw the separator line
		Render2D::FillRectangle(Rectangle((Width - 4) / 2, 2, 1, Height - 4), Style::Current->LightBackground * 1.3f);
	}
} // SE

#include "ViewportWidgetsContainer.h"

#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/Style.h"

namespace SE::Editor
{
	ViewportWidgetsContainer::ViewportWidgetsContainer(ViewportWidgetLocation location) : ContainerControl(0, WidgetsMargin, 64, WidgetsHeight + 2)
	{
		AutoFocus = false;
		WidgetLocation = location;
	}

	void ViewportWidgetsContainer::Draw()
	{
		// Cache data
		Style* style = Style::Current;
		Rectangle clientRect = Rectangle(Float2::Zero, Size);

		// Draw background
		Render2D::FillRectangle(clientRect, style->LightBackground * (IsMouseOver ? 0.3f : 0.2f));

		ContainerControl::Draw();

		// Draw frame
		Render2D::DrawRectangle(clientRect, style->BackgroundSelected * (IsMouseOver ? 1.0f : 0.6f));
	}

	void ViewportWidgetsContainer::ArrangeWidgets(ContainerControl control)
	{
		// Arrange viewport widgets
		const float margin = ViewportWidgetsContainer::WidgetsMargin;
		float left = margin;
		float right = control.Width - margin;
		List<Control*>& childList = control.Children();
		for (int i = 0; i < control.ChildrenCount(); i++)
		{
			ViewportWidgetsContainer* widget;
			if (TypeTryCast(childList[i], widget) && widget->Visible)
			{
				float x;
				switch (widget->WidgetLocation)
				{
				case ViewportWidgetLocation::UpperLeft:
					x = left;
					left += widget->Width + margin;
					break;
				case ViewportWidgetLocation::UpperRight:
					x = right - widget->Width;
					right = x - margin;
					break;
				default:
					x = 0;
					break;
				}
				widget->Location = Float2(x, margin);
			}
		}
	}

	void ViewportWidgetsContainer::PerformLayoutBeforeChildren()
	{
		ContainerControl::PerformLayoutBeforeChildren();

		float x = 1;
		for (int i = 0; i < m_Children.Count(); i++)
		{
			Control* c = m_Children[i];
			float w = c->Width;

			c->Bounds = Rectangle(x, 1, w, Height - 2);

			x += w;
		}

		Width = x + 1;
	}
} // SE

#include "ScrollableControl.h"

#include "Runtime/Render/2D/Render2D.h"

namespace SE
{
	void ScrollableControl::DrawChildren()
	{
		// Draw all visible child controls
		bool hasViewOffset = !_viewOffset.IsZero();
		for (int i = 0; i < m_Children.Count(); i++)
		{
			Control* child = m_Children[i];

			if (child->Visible)
			{
				Matrix3x3 transform = child->GetCachedTransform();
				if (hasViewOffset && child->IsScrollable)
				{
					transform.M31 += _viewOffset.x;
					transform.M32 += _viewOffset.y;
				}

				Render2D::PushTransform(transform);
				child->Draw();
				Render2D::PopTransform();
			}
		}
	}

	bool ScrollableControl::IntersectsChildContent(Control* child, Float2 location, Float2& childSpaceLocation)
	{
		// Apply offset on scrollable controls
		if (child->IsScrollable)
			location -= _viewOffset;

		return child->IntersectsContent( location, childSpaceLocation);
	}

	bool ScrollableControl::IntersectsContent(Float2& locationParent, Float2& location)
	{
		// Little workaround to prevent applying offset when performing intersection test with this scrollable control.
		// Note that overriden PointFromParent applies view offset.
		location = ContainerControl::PointFromParent(locationParent);
		return ContainsPoint(location);
	}

	Float2 ScrollableControl::PointToParent(Float2 location)
	{
		return ContainerControl::PointToParent(location) + _viewOffset;
	}

	Float2 ScrollableControl::PointFromParent(Float2 location)
	{
		return ContainerControl::PointFromParent(location) - _viewOffset;
	}
} // SE

#include "ClickableLabel.h"

namespace SE
{
	bool ClickableLabel::OnMouseDoubleClick(Float2 location, MouseButton button)
	{
		if (DoubleClick.IsBinded())
		{
			DoubleClick();
		}

		return Label::OnMouseDoubleClick(location, button);
	}

	bool ClickableLabel::OnMouseDown(Float2 location, MouseButton button)
	{
		if (button == MouseButton::Left)
			m_LeftClick = true;
		else if (button == MouseButton::Right)
			m_IsRightDown = true;

		return Label::OnMouseDown(location, button);
	}

	bool ClickableLabel::OnMouseUp(Float2 location, MouseButton button)
	{
		if (button == MouseButton::Left && m_LeftClick)
		{
			m_LeftClick = false;
			if (LeftClick.IsBinded())
			{
				LeftClick();
			}
		}
		else if (button == MouseButton::Right && m_IsRightDown)
		{
			m_IsRightDown = false;
			if (RightClick.IsBinded())
			{
				RightClick();
			}
		}

		return Label::OnMouseUp(location, button);
	}

	void ClickableLabel::OnMouseLeave()
	{
		m_LeftClick = false;
		m_IsRightDown = false;

		Label::OnMouseLeave();
	}
} // SE
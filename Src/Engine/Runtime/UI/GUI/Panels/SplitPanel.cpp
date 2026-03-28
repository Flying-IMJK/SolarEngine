
#include "SplitPanel.h"

#include "Panel.h"
#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/Style.h"

namespace SE
{
	void SplitPanel::UpdateSplitRect()
	{
		if (_orientation == Orientation::Horizontal)
		{
			float split = Math::RoundToInt(_splitterValue * Width);
			_splitterRect = Rectangle(Math::Clamp(split - SplitterSizeHalf, 0.0f, Width.operator->()), 0, SplitterSize, Height);
		}
		else
		{
			float split = Math::RoundToInt(_splitterValue * Height);
			_splitterRect = Rectangle(0, Math::Clamp(split - SplitterSizeHalf, 0.0f, Height.operator->()), Width, SplitterSize);
		}
	}

	void SplitPanel::StartTracking()
	{
		// Start move
		_splitterClicked = true;

		// Start capturing mouse
		StartMouseCapture();
	}

	void SplitPanel::EndTracking()
	{
		if (_splitterClicked)
		{
			// Clear flag
			_splitterClicked = false;

			// End capturing mouse
			EndMouseCapture();
		}
	}

	SplitPanel::SplitPanel(::SE::Orientation orientation, ScrollBars panel1Scroll, ScrollBars panel2Scroll)
	{
		AutoFocus = false;

		_orientation = orientation;
		_splitterValue = 0.5f;

		Panel1 = New<Panel>(panel1Scroll);
		Panel2 = New<Panel>(panel2Scroll);

		Panel1->Parent = this;
		Panel2->Parent = this;

		UpdateSplitRect();
	}

	void SplitPanel::Draw()
	{
		ContainerControl::Draw();

		// Draw splitter
		Style* style = Style::Current;
		Render2D::FillRectangle(_splitterRect, _splitterClicked ? style->BackgroundSelected : _mouseOverSplitter ? style->BackgroundHighlighted : style->LightBackground);
	}

	void SplitPanel::OnLostFocus()
	{
		EndTracking();

		ContainerControl::OnLostFocus();
	}

	void SplitPanel::OnMouseMove(Float2 location)
	{
		_mouseOverSplitter = _splitterRect.Contains(location);

		if (_splitterClicked)
		{
			SplitterValue = _orientation == Orientation::Horizontal ? location.x / Width : location.y / Height;
			Cursor = _orientation == Orientation::Horizontal ? CursorType::SizeWE : CursorType::SizeNS;
			_cursorChanged = true;
		}
		else if (_mouseOverSplitter)
		{
			Cursor = _orientation == Orientation::Horizontal ? CursorType::SizeWE : CursorType::SizeNS;
			_cursorChanged = true;
		}
		else if (_cursorChanged)
		{
			Cursor = CursorType::Default;
			_cursorChanged = false;
		}

		ContainerControl::OnMouseMove(location);
	}

	bool SplitPanel::OnMouseDown(Float2 location, MouseButton button)
	{
		if (button == MouseButton::Left && _splitterRect.Contains(location))
		{
			// Start moving splitter
			StartTracking();
			Focus();
			return true;
		}

		return ContainerControl::OnMouseDown(location, button);
	}

	bool SplitPanel::OnMouseUp(Float2 location, MouseButton button)
	{
		if (_splitterClicked)
		{
			EndTracking();
			return true;
		}

		return ContainerControl::OnMouseUp(location, button);
	}

	void SplitPanel::OnMouseLeave()
	{
		// Clear flag
		_mouseOverSplitter = false;
		if (_cursorChanged)
		{
			Cursor = CursorType::Default;
			_cursorChanged = false;
		}

		ContainerControl::OnMouseLeave();
	}

	void SplitPanel::OnEndMouseCapture()
	{
		EndTracking();
	}

	void SplitPanel::OnSizeChanged()
	{
		ContainerControl::OnSizeChanged();

		UpdateSplitRect();
		PerformLayout();
	}

	void SplitPanel::PerformLayoutBeforeChildren()
	{
		ContainerControl::PerformLayoutBeforeChildren();

		if (_orientation == Orientation::Horizontal)
		{
			float split = Math::RoundToInt(_splitterValue * Width);
			Panel1->Bounds = Rectangle(0, 0, split - SplitterSizeHalf, Height);
			Panel2->Bounds = Rectangle(split + SplitterSizeHalf, 0, Width - split - SplitterSizeHalf, Height);
		}
		else
		{
			float split = Math::RoundToInt(_splitterValue * Height);
			Panel1->Bounds = Rectangle(0, 0, Width, split - SplitterSizeHalf);
			Panel2->Bounds = Rectangle(0, split + SplitterSizeHalf, Width, Height - split - SplitterSizeHalf);
		}
	}

	void SplitPanel::__SetOrientation(::SE::Orientation value)
	{
		if (_orientation != value)
		{
			_orientation = value;
			UpdateSplitRect();
			PerformLayout();
		}
	}

	void SplitPanel::__SetSplitterValue(float value)
	{
		value = Math::Saturate(value);
		if (!Math::IsNearEqual(_splitterValue, value))
		{
			// Set new value
			_splitterValue = value;

			// Calculate rectangle and update panels
			UpdateSplitRect();
			PerformLayout();
		}
	}
} // SE
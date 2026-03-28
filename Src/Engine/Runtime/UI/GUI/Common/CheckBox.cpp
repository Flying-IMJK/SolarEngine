

#include "CheckBox.h"

#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/Style.h"
#include "Runtime/UI/GUI/Brushes/SpriteBrush.h"

namespace SE
{
	CheckBox::CheckBox(float x, float y, bool isChecked, float size) : Control(x, y, size, size)
	{
		_state = isChecked ? CheckBoxState::Checked : CheckBoxState::Default;
		_boxSize = Math::Min(16.0f, size);

		Style* style = Style::Current;
		ImageColor = style->BorderSelected * 1.2f;
		BorderColor = style->BorderNormal;
		BorderColorHighlighted = style->BorderSelected;
		CheckedImage = New<SpriteBrush>(style->CheckBoxTick);
		IntermediateImage = New<SpriteBrush>(style->CheckBoxIntermediate);

		CacheBox();
	}

	CheckBox::~CheckBox()
	{
		Delete(CheckedImage);
		Delete(IntermediateImage);
	}

	void CheckBox::Draw()
	{
		Control::Draw();

		bool enabled = EnabledInHierarchy();

		// Border
		if (HasBorder)
		{
			Color borderColor = BorderColor;
			if (!enabled)
				borderColor *= 0.5f;
			else if (_isPressed || _mouseOverBox || IsNavFocused)
				borderColor = BorderColorHighlighted;
			Render2D::DrawRectangle(_box.MakeExpanded(-2.0f), borderColor, BorderThickness);
		}

		// Icon
		if (_state != CheckBoxState::Default)
		{
			Color color = ImageColor;
			if (!enabled)
				color *= 0.6f;

			if (_state == CheckBoxState::Checked)
			{
				if (CheckedImage != nullptr)
				{
					CheckedImage->Draw(_box, color);
				}
			}
			else
			{
				if (IntermediateImage != nullptr)
				{
					IntermediateImage->Draw(_box, color);
				}
			}
		}
	}

	bool CheckBox::ContainsPoint(Float2& location, bool precise)
	{
		if (precise) // Precise check for checkbox element
			return _box.Contains(location);
		return Control::ContainsPoint(location, precise);
	}

	void CheckBox::OnMouseMove(Float2 location)
	{
		Control::OnMouseMove(location);

		_mouseOverBox = _box.Contains(location);
	}

	bool CheckBox::OnMouseDown(Float2 location, MouseButton button)
	{
		if (button == MouseButton::Left && !_isPressed)
		{
			OnPressBegin();
			return true;
		}

		return Control::OnMouseDown(location, button);
	}

	bool CheckBox::OnMouseDoubleClick(Float2 location, MouseButton button)
	{
		if (button == MouseButton::Left && !_isPressed)
		{
			OnPressBegin();
			return true;
		}

		if (button == MouseButton::Left && _isPressed)
		{
			OnPressEnd();
			if (_box.Contains(location))
			{
				OnClick();
				return true;
			}
		}
		return Control::OnMouseDoubleClick(location, button);
	}

	bool CheckBox::OnMouseUp(Float2 location, MouseButton button)
	{
		if (button == MouseButton::Left && _isPressed)
		{
			OnPressEnd();
			if (_box.Contains(location))
			{
				OnClick();
				return true;
			}
		}

		return Control::OnMouseUp(location, button);
	}

	void CheckBox::OnMouseLeave()
	{
		if (_isPressed)
			OnPressEnd();
		_mouseOverBox = false;

		Control::OnMouseLeave();
	}

	bool CheckBox::OnTouchDown(Float2 location, int pointerId)
	{
		if (!_isPressed)
		{
			OnPressBegin();
			return true;
		}

		return Control::OnTouchDown(location, pointerId);
	}

	bool CheckBox::OnTouchUp(Float2 location, int pointerId)
	{
		if (_isPressed)
		{
			OnPressEnd();
			if (_box.Contains(location))
			{
				OnClick();
				return true;
			}
		}

		return Control::OnTouchUp(location, pointerId);
	}

	void CheckBox::OnTouchLeave()
	{
		if (_isPressed)
			OnPressEnd();

		Control::OnTouchLeave();
	}

	void CheckBox::OnLostFocus()
	{
		if (_isPressed)
			OnPressEnd();

		Control::OnLostFocus();
	}

	void CheckBox::OnSubmit()
	{
		OnClick();

		Control::OnSubmit();
	}

	void CheckBox::__SetState(CheckBoxState value)
	{
		if (_state != value)
		{
			_state = value;

			if (StateChanged.IsBinded())
			{
				StateChanged(this);
			}
		}
	}

	void CheckBox::__SetChecked(bool value)
	{
		State = value ? CheckBoxState::Checked : CheckBoxState::Default;
	}

	void CheckBox::__SetIntermediate(bool value)
	{
		State = value ? CheckBoxState::Intermediate : CheckBoxState::Default;
	}

	void CheckBox::__SetBoxSize(float value)
	{
		_boxSize = value;
		CacheBox();
	}
} // SE
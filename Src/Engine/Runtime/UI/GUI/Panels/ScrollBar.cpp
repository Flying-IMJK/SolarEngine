
#include "ScrollBar.h"

#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/RootControl.h"
#include "Runtime/UI/GUI/ScrollableControl.h"

#include "Core/Types/Delegate.h"

namespace SE
{
	bool ScrollBar::UseSmoothing() const
	{
		return EnableSmoothing && !Math::IsZero(ScrollAnimationDuration);
	}

	void ScrollBar::SetMinimum(float value)
	{
		if (value > _maximum)
		{
			value = _maximum;
		}
		_minimum = value;
		if (value < _minimum)
			value = _minimum;
	}

	void ScrollBar::SetMaximum(float value)
	{
		if (value < _minimum)
			value = _minimum;

		_maximum = value;
		if (value > _maximum)
			value = _maximum;
	}

	void ScrollBar::SetValue(float value)
	{
		value = Math::Clamp(value, _minimum, _maximum);
		if (!Math::IsNearEqual(value, _targetValue))
		{
			_targetValue = value;
			_startValue = _value;
			_scrollAnimationProgress = 0.0f;

			// Check if skip smoothing
			if (!UseSmoothing())
			{
				_value = value;
				_startValue = value;
				_scrollAnimationProgress = 1.0f;
				OnValueChanged();
			}
			else
			{
				SetUpdate(&_update, &m_OnUpdate);
			}
		}
	}

	void ScrollBar::SetTargetValue(float value)
	{
		value = Math::Clamp(value, _minimum, _maximum);
		if (!Math::IsNearEqual(value, _targetValue))
		{
			_targetValue = value;
			_value = value;
			SetUpdate(&_update, nullptr);
			OnValueChanged();
		}
	}

	void ScrollBar::FastScroll()
	{
		if (!Math::IsNearEqual(_value, _targetValue))
		{
			_value = _targetValue = _startValue;
			_scrollAnimationProgress = 0.0f;
			SetUpdate(&_update, nullptr);
			OnValueChanged();
		}
	}

	void ScrollBar::ScrollViewTo(float min, float max, bool fastScroll)
	{
		// Check if we need to change view
		float viewMin = _value;
		float viewSize = TrackSize();
		float viewMax = viewMin + viewSize;
		if (Math::RangeExclusive(min, viewMin, viewMax))
		{
			if (fastScroll)
				SetTargetValue(min);
			else
				SetValue(min);
		}
		/*else if (Mathf.IsNotInRange(max, viewMin, viewMax))
		{
			Value = max - viewSize;
		}*/
	}

	void ScrollBar::SetScrollRange(float minimum, float maximum)
	{
		ENGINE_ASSERT_M(minimum <= maximum, SE_TEXT("ArgumentOutOfRange"));

		_minimum = minimum;
		_maximum = maximum;

		if (_value < minimum)
			SetValue(minimum);
		else if (_value > maximum)
			_value = maximum;

		UpdateThumb();
	}

	void ScrollBar::Draw()
	{
		Control::Draw();

		// var style = Style.Current;
		Render2D::FillRectangle(_trackRect, TrackColor * _thumbOpacity);
		Render2D::FillRectangle(_thumbRect, (_thumbClicked ? ThumbSelectedColor : ThumbColor) * _thumbOpacity);
	}

	void ScrollBar::OnLostFocus()
	{
		EndTracking();

		Control::OnLostFocus();
	}

	void ScrollBar::OnMouseMove(Float2 location)
	{
		if (_thumbClicked)
		{
			Float2 slidePosition = location + Root->GetTrackingMouseOffset();
			ScrollableControl* panel;
			if (TypeTryCast<ScrollableControl>(Parent, panel))
			{
				slidePosition += panel->GetViewOffset(); // Hardcoded fix
			}
			float mousePosition = _orientation == Orientation::Vertical ? slidePosition.y : slidePosition.x;

			float percentage = (mousePosition - _mouseOffset - _thumbSize / 2) / (TrackSize() - _thumbSize);
			SetTargetValue(_minimum + percentage * (_maximum - _minimum));
		}
	}

	bool ScrollBar::OnMouseWheel(Float2 location, float delta)
	{
		if (ThumbEnabled)
		{
			// Scroll
			SetValue(_targetValue - delta * _scrollChange);
		}
		return true;
	}

	bool ScrollBar::OnMouseDown(Float2 location, MouseButton button)
	{
		if (button == MouseButton::Left && ThumbEnabled)
		{
			// Remove focus
			RootControl* root = Root;
			if (root->GetFocusedControl() != nullptr)
			{
				root->GetFocusedControl()->Defocus();
			}

			float mousePosition = _orientation == Orientation::Vertical ? location.y : location.x;

			if (_thumbRect.Contains(location))
			{
				// Start moving thumb
				_thumbClicked = true;
				_mouseOffset = mousePosition - _thumbCenter;

				// Start capturing mouse
				StartMouseCapture();
			}
			else
			{
				// Click change
				SetValue(_value + (mousePosition < _thumbCenter ? -1 : 1) * _clickChange);
			}
		}

		return Control::OnMouseDown(location, button);
	}

	bool ScrollBar::OnMouseUp(Float2 location, MouseButton button)
	{
		EndTracking();

		return Control::OnMouseUp(location, button);
	}

	void ScrollBar::OnEndMouseCapture()
	{
		EndTracking();
	}

	void ScrollBar::OnMouseEnter(Float2 location)
	{
		Control::OnMouseEnter(location);

		SetUpdate(&_update, &m_OnUpdate);
	}

	void ScrollBar::OnMouseLeave()
	{
		Control::OnMouseLeave();

		SetUpdate(&_update, &m_OnUpdate);
	}

	void ScrollBar::Reset()
	{
		_value = _targetValue = _startValue = 0;
		_scrollAnimationProgress = 0.0f;
	}

	void ScrollBar::UpdateThumb()
	{
		// Cache data
		float width = Width;
		float height = Height;
		float trackSize = TrackSize();
		float range = _maximum - _minimum;
		_thumbSize = Math::Min(trackSize - 10, Math::Max(trackSize / range * 100.0f, 50.0f));
		float pixelRange = trackSize - _thumbSize;
		float percentage = (_value - _minimum) / range;
		float thumbPosition = (int)(percentage * pixelRange);
		_thumbCenter = thumbPosition + _thumbSize / 2;
		_thumbRect = _orientation == Orientation::Vertical
					 ? Rectangle((width - ThumbThickness) / 2, thumbPosition + 4, ThumbThickness, _thumbSize - 8)
					 : Rectangle(thumbPosition + 4, (height - ThumbThickness) / 2, _thumbSize - 8, ThumbThickness);
		_trackRect = _orientation == Orientation::Vertical
					 ? Rectangle((width - TrackThickness) / 2, 4, TrackThickness, height - 8)
					 : Rectangle(4, (height - TrackThickness) / 2, width - 8, TrackThickness);
	}

	void ScrollBar::EndTracking()
	{
		// Check flag
		if (_thumbClicked)
		{
			// Clear flag
			_thumbClicked = false;

			// End capturing mouse
			EndMouseCapture();
		}
	}

	void ScrollBar::OnUpdate(float deltaTime)
	{
		bool isDeltaSlow = deltaTime > (1 / 20.0f);

		// Opacity smoothing
		float targetOpacity = IsMouseOver ? 1.0f : DefaultMinimumOpacity;
		_thumbOpacity = isDeltaSlow ? targetOpacity : Math::Lerp(_thumbOpacity, targetOpacity, deltaTime * 10.0f);
		bool needUpdate = Math::Abs(_thumbOpacity - targetOpacity) > 0.001f;

		// Ensure scroll bar is visible and smoothing is required
		if (Visible && Math::Abs(_targetValue - _value) > 0.01f)
		{
			// Interpolate or not if running slow
			float value;
			if (!isDeltaSlow && UseSmoothing())
			{
				// percentage of scroll from 0 to _scrollChange, ex. 0.5 at _scrollChange / 2
				float minScrollChangeRatio = Math::Clamp(Math::Abs(_targetValue - _startValue) / _scrollChange, 0.0f, 1.0f);

				// shorten the duration if we scrolled less than _scrollChange
				float actualDuration = ScrollAnimationDuration * minScrollChangeRatio;
				float step = deltaTime / actualDuration;

				float progress = _scrollAnimationProgress;
				progress = Math::Clamp(progress + step, 0.0f, 1.0f);

				// https://easings.net/#easeOutSine
				float easedProgress = Math::Sin((progress * Math::PI) / 2);
				value = Math::Lerp(_startValue, _targetValue, easedProgress);

				_scrollAnimationProgress = progress;
			}
			else
			{
				value = _targetValue;
				_startValue = _targetValue;
				_scrollAnimationProgress = 0.0f;
			}

			_value = value;
			OnValueChanged();
			needUpdate = true;
		}

		// End updating if all animations are done
		if (!needUpdate)
		{
			SetUpdate(&_update, nullptr);
		}
	}

	void ScrollBar::OnValueChanged()
	{
		UpdateThumb();

		ValueChanged();
	}

	void ScrollBar::OnSizeChanged()
	{
		Control::OnSizeChanged();

		UpdateThumb();
	}

	ScrollBar::ScrollBar()
	{
	}

	ScrollBar::ScrollBar(Orientation orientation)
	{
		m_OnUpdate.BindUnique(CreateFunc<ScrollBar, &ScrollBar::OnUpdate>(this));

		AutoFocus = false;

		_orientation = orientation;
		// var style = Style.Current;
		// TrackColor = style.BackgroundHighlighted;
		// ThumbColor = style.BackgroundNormal;
		// ThumbSelectedColor = style.BackgroundSelected;
	}

	void ScrollBar::AddUpdateCallbacks(RootControl* root)
	{
		Control::AddUpdateCallbacks(root);

		if (_update.IsBinded())
		{
			root->UpdateCallbacksToAdd.Add(&_update);
		}
	}

	void ScrollBar::RemoveUpdateCallbacks(RootControl* root)
	{
		Control::RemoveUpdateCallbacks(root);

		if (_update.IsBinded())
		{
			root->UpdateCallbacksToRemove.Add(&_update);
		}
	}
} // SE

#include "FloatValueBox.h"

namespace SE::Editor
{
	void FloatValueBox::SetValue(float value)
	{
		value = Math::Clamp(value, _min, _max);
		if (Math::Abs(_value - value) > Math::EPSILON)
		{
			_value = value;
			UpdateText();
			OnValueChanged();
		}
	}
	
	void FloatValueBox::SetMinValue(float value)
	{
		if (!Math::IsNearEqual(_min, value))
		{
			ENGINE_ASSERT(value <= _max)
			_min = value;
			SetValue(GetValue());
		}
	}

	void FloatValueBox::SetMaxValue(float value)
	{
		if (!Math::IsNearEqual(_max, value))
		{
			ENGINE_ASSERT(value >= _min)
			_max = value;
			SetValue(GetValue());
		}
	}

	FloatValueBox::FloatValueBox(float value, float x, float y, float width, float min, float max, float slideSpeed)
		: ValueBox(Math::Clamp(value, min, max), x, y, width, min, max, slideSpeed)
	{
		TryUseAutoSliderSpeed();
		UpdateText();
	}

	void FloatValueBox::SetLimits(float min, float max)
	{
		_min = min;
		_max = Math::Max(_min, max);
		SetValue(GetValue());
	}

	void FloatValueBox::SetSpeed(float value)
	{
		_slideSpeed = value;
		TryUseAutoSliderSpeed();
	}

	void FloatValueBox::SetLimits(FloatValueBox other)
	{
		_min = other._min;
		_max = other._max;
		_slideSpeed = other._slideSpeed;
		SetValue(GetValue());
	}

	void FloatValueBox::TryUseAutoSliderSpeed()
	{
		float range = _max - _min;
		if (Math::IsOne(_slideSpeed) && range > Math::EPSILON * 200.0f && range < 1000000.0f)
		{
			_slideSpeed = range * 0.001f;
		}
	}

	void FloatValueBox::UpdateText()
	{
		SetText(String::Format(SE_TEXT("{0}"), _value));
	}

	void FloatValueBox::TryGetValue()
	{
		float value;
		StringUtils::Parse(Text.operator->().Get(), &value);
		SetValue(value);
	}
} // SE
#include "LayoutElements.h"

namespace SE::Editor
{
	Control* ButtonElement::GetControl()
	{
		return reinterpret_cast<Control*>(Button);
	}

	void SpaceElement::Init(float height)
	{
		// TODO: Set spacer height when Spacer class is available
		// if (Spacer)
		//     Spacer->SetHeight(height);
	}

	Control* SpaceElement::GetControl()
	{
		return reinterpret_cast<Control*>(Spacer);
	}

	Control* TextBoxElement::GetControl()
	{
		return reinterpret_cast<Control*>(TextBox);
	}

	Control* CheckBoxElement::GetControl()
	{
		return reinterpret_cast<Control*>(CheckBox);
	}

	Control* DoubleValueElement::GetControl()
	{
		return reinterpret_cast<Control*>(ValueBox);
	}

	Control* IntegerValueElement::GetControl()
	{
		return reinterpret_cast<Control*>(ValueBox);
	}

	Control* SliderElement::GetControl()
	{
		return reinterpret_cast<Control*>(Slider);
	}

	Control* ComboBoxElement::GetControl()
	{
		return reinterpret_cast<Control*>(ComboBox);
	}

	Control* EnumElement::GetControl()
	{
		return reinterpret_cast<Control*>(ComboBox);
	}

	Control* ImageElement::GetControl()
	{
		return reinterpret_cast<Control*>(Image);
	}

} // namespace SE::Editor

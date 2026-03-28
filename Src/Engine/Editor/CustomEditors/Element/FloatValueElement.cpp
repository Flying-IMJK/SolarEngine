
#include "FloatValueElement.h"

namespace SE::Editor
{
	FloatValueElement::FloatValueElement()
	{
		ValueBox = New<FloatValueBox>(0);
	}


	Control* FloatValueElement::GetControl()
	{
		return ValueBox;
	}

	float FloatValueElement::GetValue()
	{
		return ValueBox->GetValue();
	}

	void FloatValueElement::SetValue(float value)
	{
		ValueBox->SetValue(value);
	}

	bool FloatValueElement::IsSliding()
	{
		return ValueBox->IsSliding;
	}
} // SE
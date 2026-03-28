

#include "PropertiesListElement.h"

namespace SE::Editor
{
	void PropertiesListElement::OnAddProperty(const String& name, const String& tooltip)
	{
		// TODO: Create PropertyNameLabel and add to list
		LOG_WARNING("Editor", "PropertiesListElement::OnAddProperty not fully implemented yet");
	}

	void PropertiesListElement::OnAddProperty(PropertyNameLabel* label, const String& tooltip)
	{
		if (label)
		{
			Labels.Add(label);
		}
	}

	ContainerControl* PropertiesListElement::GetContainerControl()
	{
		return reinterpret_cast<ContainerControl*>(Properties);
	}

	Control* PropertiesListElement::GetControl()
	{
		return reinterpret_cast<Control*>(Properties);
	}

} // SE
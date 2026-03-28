#pragma once
#include "Editor/CustomEditors/LayoutElementsContainer.h"

namespace SE::Editor
{
	/// <summary>
	/// Properties list element.
	/// </summary>
	class PropertiesListElement : public LayoutElementsContainer
	{
		SE_CLASS_DEFAULT(PropertiesListElement, LayoutElementsContainer)
	public:
		void* Properties = nullptr; // PropertiesList*
		List<PropertyNameLabel*> Labels;

		void OnAddProperty(const String& name, const String& tooltip = String::Empty);
		void OnAddProperty(PropertyNameLabel* label, const String& tooltip = String::Empty);

		ContainerControl* GetContainerControl() override;
		Control* GetControl() override;
	};

} // SE


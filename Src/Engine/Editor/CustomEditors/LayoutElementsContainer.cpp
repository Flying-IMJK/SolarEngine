#include "LayoutElementsContainer.h"
#include "Element/LayoutElements.h"
#include "Element/LabelElement.h"
#include "CustomEditor.h"
#include "CustomEditorFactory.h"
#include "CustomEditorPresenter.h"
#include "Element/FloatValueElement.h"
#include "Element/Container/PropertiesListElement.h"
#include "Values/ValueContainer.h"

namespace SE::Editor
{
	CustomEditorPresenter* LayoutElementsContainer::GetPresenter()
	{
		CustomEditorPresenter* result = nullptr;
		LayoutElementsContainer* container = this;
		do
		{
			result = TypeCast<CustomEditorPresenter>(container);
			container = container->parent;
		} while (container != nullptr);
		return result;
	}

	// Grouping and panels
	GroupElement* LayoutElementsContainer::Group(const String& title, bool useTransparentHeader)
	{
		GroupElement* element = New<GroupElement>();
		element->parent = this;
		element->isRootGroup = false;
		// TODO: Create actual DropPanel when GUI system is ready
		// element->Panel = New<DropPanel>();
		OnAddElement(element);
		return element;
	}

	GroupElement* LayoutElementsContainer::Group(const String& title, CustomEditor* linkedEditor, bool useTransparentHeader)
	{
		GroupElement* element = Group(title, useTransparentHeader);
		if (linkedEditor)
		{
			// TODO: Link editor to group
		}
		return element;
	}

	HorizontalPanelElement* LayoutElementsContainer::HorizontalPanel()
	{
		HorizontalPanelElement* element = New<HorizontalPanelElement>();
		element->parent = this;
		// TODO: Create actual HorizontalPanel when GUI system is ready
		OnAddElement(element);
		return element;
	}

	VerticalPanelElement* LayoutElementsContainer::VerticalPanel()
	{
		VerticalPanelElement* element = New<VerticalPanelElement>();
		element->parent = this;
		// TODO: Create actual VerticalPanel when GUI system is ready
		OnAddElement(element);
		return element;
	}

	// Basic controls
	LabelElement* LayoutElementsContainer::Label(const String& text, TextAlignment alignment)
	{
		LabelElement* element = New<LabelElement>();
		// TODO: Create actual Label control when GUI system is ready
		// element->Label = New<Label>();
		// element->Label->Text = text;
		// element->Label->HorizontalAlignment = alignment;
		OnAddElement(element);
		return element;
	}

	LabelElement* LayoutElementsContainer::Header(const String& text)
	{
		// Header is just a label with larger font
		return Label(text, TextAlignment::Near);
	}

	ButtonElement* LayoutElementsContainer::Button(const String& text, const String& tooltip)
	{
		ButtonElement* element = New<ButtonElement>();
		// TODO: Create actual Button control when GUI system is ready
		OnAddElement(element);
		return element;
	}

	SpaceElement* LayoutElementsContainer::Space(float height)
	{
		SpaceElement* element = New<SpaceElement>();
		element->Init(height);
		OnAddElement(element);
		return element;
	}

	// Input controls
	TextBoxElement* LayoutElementsContainer::TextBox(bool isMultiline)
	{
		TextBoxElement* element = New<TextBoxElement>();
		// TODO: Create actual TextBox control when GUI system is ready
		OnAddElement(element);
		return element;
	}

	CheckBoxElement* LayoutElementsContainer::Checkbox()
	{
		CheckBoxElement* element = New<CheckBoxElement>();
		// TODO: Create actual CheckBox control when GUI system is ready
		OnAddElement(element);
		return element;
	}

	FloatValueElement* LayoutElementsContainer::FloatValue()
	{
		FloatValueElement* element = New<FloatValueElement>();
		// TODO: Create actual FloatValueBox control when GUI system is ready
		OnAddElement(element);
		return element;
	}

	DoubleValueElement* LayoutElementsContainer::DoubleValue()
	{
		DoubleValueElement* element = New<DoubleValueElement>();
		// TODO: Create actual DoubleValueBox control when GUI system is ready
		OnAddElement(element);
		return element;
	}

	IntegerValueElement* LayoutElementsContainer::IntegerValue()
	{
		IntegerValueElement* element = New<IntegerValueElement>();
		// TODO: Create actual IntegerValueBox control when GUI system is ready
		OnAddElement(element);
		return element;
	}

	SliderElement* LayoutElementsContainer::Slider()
	{
		SliderElement* element = New<SliderElement>();
		// TODO: Create actual Slider control when GUI system is ready
		OnAddElement(element);
		return element;
	}

	ComboBoxElement* LayoutElementsContainer::ComboBox()
	{
		ComboBoxElement* element = New<ComboBoxElement>();
		// TODO: Create actual ComboBox control when GUI system is ready
		OnAddElement(element);
		return element;
	}

	EnumElement* LayoutElementsContainer::Enum(TypeID enumType)
	{
		EnumElement* element = New<EnumElement>();
		// TODO: Create actual EnumComboBox control when GUI system is ready
		OnAddElement(element);
		return element;
	}

	// Image
	ImageElement* LayoutElementsContainer::Image(SpriteHandle* sprite)
	{
		ImageElement* element = New<ImageElement>();
		// TODO: Create actual Image control when GUI system is ready
		OnAddElement(element);
		return element;
	}

	ImageElement* LayoutElementsContainer::Image(Texture* texture)
	{
		ImageElement* element = New<ImageElement>();
		// TODO: Create actual Image control when GUI system is ready
		OnAddElement(element);
		return element;
	}

	ImageElement* LayoutElementsContainer::Image(GPUTexture* texture)
	{
		ImageElement* element = New<ImageElement>();
		// TODO: Create actual Image control when GUI system is ready
		OnAddElement(element);
		return element;
	}

	// Property and object editors
	CustomEditor* LayoutElementsContainer::Property(const String& name, ValueContainer* values,
	                                                CustomEditor* overrideEditor,
	                                                const String& tooltip)
	{
		CustomEditor* editor = CustomEditorFactory::CreateEditor(values, overrideEditor);


		PropertiesListElement* property = AddPropertyItem(name, tooltip);

		return property->Object(values, editor);
	}

	CustomEditor* LayoutElementsContainer::Object(ValueContainer* values, CustomEditor* overrideEditor)
	{
		CustomEditor* editor = CustomEditorFactory::CreateEditor(values, overrideEditor);

		OnAddEditor(editor);
		editor->Initialize(CustomEditor::CurrentCustomEditor->GetPresenter(), this, values);

		return editor;
	}

	CustomEditor* LayoutElementsContainer::Object(const String& name, ValueContainer* values,
	                                              CustomEditor* overrideEditor,
	                                              const String& tooltip)
	{
		PropertiesListElement* property = AddPropertyItem(name, tooltip);
		return property->Object(values, overrideEditor);
	}

	PropertiesListElement* LayoutElementsContainer::AddPropertyItem()
	{
		PropertiesListElement* element = nullptr;

		if (!(Children.Count() > 0 && TypeTryCast<PropertiesListElement>(Children[Children.Count() - 1], element)))
		{
			element = New<PropertiesListElement>();
			OnAddElement(element);
		}

		return element;
	}

	// Property items
	PropertiesListElement* LayoutElementsContainer::AddPropertyItem(const String& name, const String& tooltip)
	{
		PropertiesListElement* element = AddPropertyItem();
		element->OnAddProperty(name, tooltip);
		return element;
	}

	PropertiesListElement* LayoutElementsContainer::AddPropertyItem(PropertyNameLabel* label, const String& tooltip)
	{
		if (label == nullptr)
			return nullptr;

		PropertiesListElement* element = AddPropertyItem();
		element->OnAddProperty(label, tooltip);
		return element;
	}

	// Element management
	void LayoutElementsContainer::AddElement(LayoutElement* element)
	{
		OnAddElement(element);
	}

	void LayoutElementsContainer::ClearLayout()
	{
		// Clear all children
		for (LayoutElement* child : Children)
		{
			Delete(child);
		}
		Children.Clear();

		// Clear all editors
		for (CustomEditor* editor : Editors)
		{
			editor->Cleanup();
			Delete(editor);
		}
		Editors.Clear();
	}

	void LayoutElementsContainer::OnAddElement(LayoutElement* element)
	{
		if (!element)
			return;

		// Set parent control if available
		if (Control* control = element->GetControl())
		{
			control->Parent = GetContainerControl();
		}

		Children.Add(element);
	}

	void LayoutElementsContainer::OnAddEditor(CustomEditor* editor)
	{
		if (!editor)
			return;

		Editors.Add(editor);
	}

	// Container element implementations
	ContainerControl* GroupElement::GetContainerControl()
	{
		return static_cast<ContainerControl*>(Panel);
	}

	Control* GroupElement::GetControl()
	{
		return static_cast<Control*>(Panel);
	}

	ContainerControl* HorizontalPanelElement::GetContainerControl()
	{
		return static_cast<ContainerControl*>(Panel);
	}

	Control* HorizontalPanelElement::GetControl()
	{
		return static_cast<Control*>(Panel);
	}

	ContainerControl* VerticalPanelElement::GetContainerControl()
	{
		return static_cast<ContainerControl*>(Panel);
	}

	Control* VerticalPanelElement::GetControl()
	{
		return static_cast<Control*>(Panel);
	}
} // namespace SE::Editor
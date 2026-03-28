#pragma once
#include "LayoutElement.h"
#include "Core/Types/Collections/List.h"
#include "Runtime/Render/2D/TextLayoutOptions.h"

namespace SE
{
	class ContainerControl;
	class SpriteHandle;
	class Texture;
	class GPUTexture;
}

namespace SE::Editor
{
	class LabelElement;
	class ButtonElement;
	class SpaceElement;
	class TextBoxElement;
	class CheckBoxElement;
	class FloatValueElement;
	class DoubleValueElement;
	class IntegerValueElement;
	class SliderElement;
	class ComboBoxElement;
	class EnumElement;
	class ImageElement;
	class GroupElement;
	class HorizontalPanelElement;
	class VerticalPanelElement;
	class PropertiesListElement;
	class PropertyNameLabel;
	class CustomEditor;
	class CustomEditorPresenter;
	class ValueContainer;

	class LayoutElementsContainer : public LayoutElement
	{
		SE_CLASS_DEFAULT(LayoutElementsContainer, LayoutElement)
	public:
		/// <summary>
		/// Helper flag that is set to true if this container is in root presenter area, otherwise it's one of child groups.
		/// It's used to collapse all child groups and open the root ones by auto.
		/// </summary>
		bool isRootGroup = true;

		/// <summary>
		/// Parent container who created this one.
		/// </summary>
		LayoutElementsContainer* parent = nullptr;

		/// <summary>
		/// The children.
		/// </summary>
		List<LayoutElement*> Children = List<LayoutElement*>();

		/// <summary>
		/// The child custom editors.
		/// </summary>
		List<CustomEditor*> Editors = List<CustomEditor*>();

		/// <summary>
		/// Gets the control represented by this element.
		/// </summary>
		virtual ContainerControl* GetContainerControl() = 0;

		/// <summary>
		/// Gets the Custom Editors layout presenter.
		/// </summary>
		CustomEditorPresenter* GetPresenter();

		// Grouping and panels
		GroupElement* Group(const String& title, bool useTransparentHeader = false);
		GroupElement* Group(const String& title, CustomEditor* linkedEditor, bool useTransparentHeader = false);
		HorizontalPanelElement* HorizontalPanel();
		VerticalPanelElement* VerticalPanel();

		// Basic controls
		LabelElement* Label(const String& text, TextAlignment alignment = TextAlignment::Near);
		LabelElement* Header(const String& text);
		ButtonElement* Button(const String& text, const String& tooltip = String::Empty);
		SpaceElement* Space(float height);

		// Input controls
		TextBoxElement* TextBox(bool isMultiline = false);
		CheckBoxElement* Checkbox();
		FloatValueElement* FloatValue();
		DoubleValueElement* DoubleValue();
		IntegerValueElement* IntegerValue();
		SliderElement* Slider();
		ComboBoxElement* ComboBox();
		EnumElement* Enum(TypeID enumType);

		// Image
		ImageElement* Image(SpriteHandle* sprite);
		ImageElement* Image(Texture* texture);
		ImageElement* Image(GPUTexture* texture);

		// Custom controls
		template<typename T>
		T* Custom();

		template<typename T>
		T* CustomContainer();

		// Property and object editors
		CustomEditor* Property(const String& name, ValueContainer* values, 
		                      CustomEditor* overrideEditor = nullptr, 
		                      const String& tooltip = String::Empty);
		CustomEditor* Object(ValueContainer* values, CustomEditor* overrideEditor = nullptr);
		CustomEditor* Object(const String& name, ValueContainer* values, 
		                    CustomEditor* overrideEditor = nullptr, 
		                    const String& tooltip = String::Empty);

		// Property items
		PropertiesListElement* AddPropertyItem(const String& name, const String& tooltip = String::Empty);
		PropertiesListElement* AddPropertyItem(PropertyNameLabel* label, const String& tooltip = String::Empty);

		// Element management
		void AddElement(LayoutElement* element);
		void ClearLayout();

		// Accessors
		const List<LayoutElement*>& GetChildren() const { return Children; }
		const List<CustomEditor*>& GetEditors() const { return Editors; }

	protected:
		/// <summary>
		/// Called when element is added to the layout.
		/// </summary>
		virtual void OnAddElement(LayoutElement* element);

		/// <summary>
		/// Called when editor is added to the layout.
		/// </summary>
		virtual void OnAddEditor(CustomEditor* editor);

	private:
		PropertiesListElement* AddPropertyItem();
	};

	/// <summary>
	/// Group element - collapsible panel.
	/// </summary>
	class GroupElement : public LayoutElementsContainer
	{
		SE_CLASS_DEFAULT(GroupElement, LayoutElementsContainer)
	public:
		void* Panel = nullptr; // DropPanel*
		ContainerControl* GetContainerControl() override;
		Control* GetControl() override;
	};

	/// <summary>
	/// Horizontal panel element.
	/// </summary>
	class HorizontalPanelElement : public LayoutElementsContainer
	{
		SE_CLASS_DEFAULT(HorizontalPanelElement, LayoutElementsContainer)
	public:
		void* Panel = nullptr; // HorizontalPanel*
		ContainerControl* GetContainerControl() override;
		Control* GetControl() override;
	};

	/// <summary>
	/// Vertical panel element.
	/// </summary>
	class VerticalPanelElement : public LayoutElementsContainer
	{
		SE_CLASS_DEFAULT(VerticalPanelElement, LayoutElementsContainer)
	public:
		void* Panel = nullptr; // VerticalPanel*
		ContainerControl* GetContainerControl() override;
		Control* GetControl() override;
	};



	// Template implementations
	template<typename T>
	T* LayoutElementsContainer::Custom()
	{
		T* element = New<T>();
		OnAddElement(element);
		return element;
	}

	template<typename T>
	T* LayoutElementsContainer::CustomContainer()
	{
		T* element = New<T>();
		element->parent = this;
		OnAddElement(element);
		return element;
	}

} // namespace SE::Editor

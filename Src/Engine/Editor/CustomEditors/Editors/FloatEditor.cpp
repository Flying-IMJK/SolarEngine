
#include "FloatEditor.h"

#include "Editor/CustomEditors/CustomEditorFactory.h"
#include "Editor/CustomEditors/LayoutElementsContainer.h"
#include "Editor/CustomEditors/Element/FloatValueElement.h"
#include "Editor/CustomEditors/Values/ValueContainer.h"

// SE_CUSTOM_EDITOR(SE::Editor::FloatEditor, float);

namespace SE::Editor
{
	void FloatEditor::OnValueChanged()
	{
		bool isSliding = m_Element->IsSliding();
		auto token = isSliding ? this : nullptr;
		SetValue(Variant(m_Element->GetValue()), token);
	}

	void FloatEditor::OnInitialize(LayoutElementsContainer* layout)
	{
		m_Element = nullptr;
		/*var attributes = Values.GetAttributes();
		var range = (RangeAttribute)attributes?.FirstOrDefault(x => x is RangeAttribute);
		if (range != null)
		{
			// Use slider
			var slider = layout.Slider();
			slider.Slider.SetLimits(range);
			slider.Slider.ValueChanged += OnValueChanged;
			slider.Slider.SlidingEnd += ClearToken;
			m_Element = slider;
			return;
		}*/

		auto floatValue = layout->FloatValue();
		// floatValue->ValueBox->ValueChanged.Bind<FloatEditor, FloatEditor::OnValueChanged>(this);
		// floatValue->ValueBox->SlidingEnd.Bind<FloatEditor, FloatEditor::ClearToken>(this);
		m_Element = floatValue;
		/*if (attributes != null)
		{
			var limit = (LimitAttribute)attributes.FirstOrDefault(x => x is LimitAttribute);
			floatValue.SetLimits(limit);
			var valueCategory = ((ValueCategoryAttribute)attributes.FirstOrDefault(x => x is ValueCategoryAttribute))?.Category ?? Utils.ValueCategory.None;
			if (valueCategory != Utils.ValueCategory.None)
			{
				floatValue.SetCategory(valueCategory);
				if (LinkedLabel != null)
				{
					LinkedLabel.SetupContextMenu += (label, menu, editor) =>
					{
						menu.AddSeparator();
						var mb = menu.AddButton("Show formatted", bt => { floatValue.SetCategory(bt.Checked ? valueCategory : Utils.ValueCategory.None); });
						mb.AutoCheck = true;
						mb.Checked = floatValue.ValueBox.Category != Utils.ValueCategory.None;
					};
				}
			}
		}*/
	}

	void FloatEditor::RefreshInternal()
	{
		if (HasDifferentValues())
		{
			// TODO: support different values for ValueBox<T>
		}
		else
		{
			ValueContent value = GetValues()->At(0);
			if (value.GetType().IsCoreType())
			{
				if (value.GetType() == Typeof<float>())
					m_Element->SetValue(*value.property->GetPropertyAddress<float>(value.instance));
				else if (value.GetType() == Typeof<double>())
					m_Element->SetValue(*value.property->GetPropertyAddress<double>(value.instance));
				else if (value.GetType() == Typeof<int>())
					m_Element->SetValue(*value.property->GetPropertyAddress<int>(value.instance));
			}
		}
	}
} // SE
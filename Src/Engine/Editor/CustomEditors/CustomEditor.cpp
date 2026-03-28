#include "CustomEditor.h"
#include "CustomEditorPresenter.h"
#include "LayoutElementsContainer.h"
#include "Values/ValueContainer.h"

namespace SE::Editor
{
	CustomEditor* CustomEditor::CurrentCustomEditor = nullptr;
	bool CustomEditor::IsSettingValue = false;

	void CustomEditor::Initialize(CustomEditorPresenter* presenter, LayoutElementsContainer* layout, ValueContainer* values)
	{
		m_layout = layout;
		m_presenter = presenter;
		m_values = values;

		CustomEditor* prev = CurrentCustomEditor;
		CurrentCustomEditor = this;

		m_isSetBlocked = true;
		OnInitialize(layout);
		Refresh();
		m_isSetBlocked = false;

		CurrentCustomEditor = prev;
	}

	void CustomEditor::Deinitialize()
	{
		// Override in derived classes for cleanup
	}

	void CustomEditor::Cleanup()
	{
		Deinitialize();

		for (int i = 0; i < m_children.Count(); i++)
		{
			m_children[i]->Cleanup();
		}

		m_children.Clear();
		m_presenter = nullptr;
		m_parent = nullptr;
		m_hasValueDirty = false;
		m_valueToSet = Variant();
		m_values = nullptr;
		m_isSetBlocked = false;
		m_rebuildOnRefresh = false;
		LinkedLabel = nullptr;
	}

	void CustomEditor::Refresh()
	{
		// Update linked label highlight if present
		if (LinkedLabel)
		{
			// TODO: Update highlight color based on default/reference value status
			// Color color = Color::Transparent;
			// if (m_values->HasReferenceValue() && CanRevertReferenceValue())
			//     color = Style::Current.BackgroundSelected;
			// else if (m_values->HasDefaultValue() && CanRevertDefaultValue())
			//     color = Color::Yellow * 0.8f;
			// LinkedLabel->HighlightStripColor = color;
		}
	}

	void CustomEditor::RefreshInternal()
	{
		if (m_values == nullptr)
			return;

		// Check if need to update value
		if (m_hasValueDirty)
		{
			IsSettingValue = true;
			
			// Cleanup (won't retry update in case of exception)
			/*void* val = m_valueToSet;
			m_hasValueDirty = false;
			m_valueToSet = nullptr;*/

			// Assign value
			// SynchronizeValue(val);

			// Propagate values up (eg. when member of structure gets modified, also structure should be updated)
			CustomEditor* obj = m_parent;
			while (obj && obj->m_parent != nullptr)
			{
				// TODO: Check if parent is SyncPointEditor
				obj->m_values->Set(obj->m_parent->m_values, obj->m_values);
				obj = obj->m_parent;
			}

			OnUnDirty();
			IsSettingValue = false;
		}
		else
		{
			// Update values from parent
			if (m_parent)
			{
				m_values->Refresh(m_parent->m_values);
			}
		}

		// Update itself
		m_isSetBlocked = true;
		Refresh();
		m_isSetBlocked = false;

		// Update children
		if (m_skipChildrenRefresh)
		{
			m_skipChildrenRefresh = false;
			return;
		}

		for (int i = 0; i < m_children.Count(); i++)
		{
			m_children[i]->RefreshInternal();
		}

		// Rebuild if flag is set
		if (m_rebuildOnRefresh)
		{
			m_rebuildOnRefresh = false;
			RebuildLayout();
		}
	}

	void CustomEditor::RefreshRoot()
	{
		for (int i = 0; i < m_children.Count(); i++)
		{
			m_children[i]->RefreshInternal();
		}
	}

	void CustomEditor::RebuildLayout()
	{
		// Skip rebuilding during init
		if (CurrentCustomEditor == this)
			return;

		// Special case for root objects to run normal layout build
		if (m_presenter != nullptr && m_presenter->Selection == m_values)
		{
			m_presenter->BuildLayout();
			return;
		}

		ValueContainer* values = m_values;
		CustomEditorPresenter* presenter = m_presenter;
		LayoutElementsContainer* layout = m_layout;
		
		if (layout->Editors.Count() > 1)
		{
			// There are more editors using the same layout so rebuild parent editor
			if (m_parent != nullptr)
			{
				m_parent->RebuildLayout();
			}
			return;
		}

		ContainerControl* control = layout->GetContainerControl();
		CustomEditor* parent = m_parent;

		// TODO: Save and restore scroll position
		// float parentScrollV = -1;

		// Lock layout and clear
		// TODO: Implement when ContainerControl is available
		// control->SetIsLayoutLocked(true);
		// control->DisposeChildren();

		layout->ClearLayout();
		Cleanup();

		// Rebuild
		m_parent = parent;
		Initialize(presenter, layout, values);

		// Unlock layout
		// TODO: Implement when ContainerControl is available
		// control->SetIsLayoutLocked(false);
		// control->PerformLayout();

		// TODO: Restore scroll position
	}

	void CustomEditor::RebuildLayoutOnRefresh()
	{
		m_rebuildOnRefresh = true;
	}

	void CustomEditor::SkipChildrenRefresh()
	{
		m_skipChildrenRefresh = true;
	}

	void CustomEditor::RevertToDefaultValue()
	{
		if (m_values && m_values->HasDefaultValue())
		{
			void* defaultValue = m_values->GetDefaultValue();
			SetValue(defaultValue);
		}
	}

	void CustomEditor::RevertToReferenceValue()
	{
		if (m_values && m_values->HasReferenceValue())
		{
			void* referenceValue = m_values->GetReferenceValue();
			SetValue(referenceValue);
		}
	}

	void CustomEditor::SetValueToDefault()
	{
		if (m_values && m_values->HasDefaultValue())
		{
			void* defaultValue = m_values->GetDefaultValue();
			/*if (m_parent)
			{
				m_values->Set(m_parent->m_values, defaultValue);
			}*/
		}
	}

	void CustomEditor::SetValueToReference()
	{
		if (m_values && m_values->HasReferenceValue())
		{
			/*void* referenceValue = m_values->GetReferenceValue();
			if (m_parent)
			{
				m_values->Set(m_parent->m_values, referenceValue);
			}*/
		}
	}

	void CustomEditor::Copy()
	{
		// TODO: Implement copy to clipboard using serialization
		LOG_WARNING("Editor", "CustomEditor::Copy not implemented yet");
	}

	void CustomEditor::Paste()
	{
		// TODO: Implement paste from clipboard using deserialization
		LOG_WARNING("Editor", "CustomEditor::Paste not implemented yet");
	}

	bool CustomEditor::CanPaste() const
	{
		// TODO: Check if clipboard contains compatible data
		return false;
	}

	bool CustomEditor::IsSingleObject() const
	{
		return m_values && m_values->IsSingleObject();
	}

	bool CustomEditor::HasDifferentValues() const
	{
		return m_values && m_values->HasDifferentValues();
	}

	bool CustomEditor::HasDifferentTypes() const
	{
		return m_values && m_values->HasDifferentTypes();
	}

	bool CustomEditor::CanRevertDefaultValue() const
	{
		return m_values && m_values->HasDefaultValue() && m_values->IsDefaultValueModified();
	}

	bool CustomEditor::CanRevertReferenceValue() const
	{
		return m_values && m_values->HasReferenceValue() && m_values->IsReferenceValueModified();
	}

	void CustomEditor::SetValue(Variant value, void* token)
	{
		if (m_isSetBlocked)
			return;

		m_valueToSet = value;
		m_hasValueDirty = true;

		// Propagate dirty state up
		if (m_parent)
		{
			m_parent->OnDirty(this, value, token);
		}
	}

	bool CustomEditor::OnDirty(CustomEditor* editor, Variant value, void* token)
	{
		// Mark as dirty and propagate up
		m_hasValueDirty = true;
		
		if (m_parent)
		{
			return m_parent->OnDirty(this, value, token);
		}
		
		return true;
	}

	void CustomEditor::OnUnDirty()
	{
		m_hasValueDirty = false;
		m_valueToSet = Variant();
	}

	void CustomEditor::ClearToken()
	{
		// Override in derived classes if needed
	}

	void CustomEditor::SynchronizeValue(void* value)
	{
		if (m_values && m_parent)
		{
			// m_values->Set(m_parent->m_values, value);
		}
	}

	void CustomEditor::InitializeInternal(CustomEditorPresenter* presenter,
	                                      LayoutElementsContainer* layout,
	                                      ValueContainer* values)
	{
		Initialize(presenter, layout, values);
	}

	void CustomEditor::OnChildCreated(CustomEditor* child)
	{
		if (child)
		{
			child->m_parent = this;
			m_children.Add(child);
		}
	}

} // namespace SE::Editor
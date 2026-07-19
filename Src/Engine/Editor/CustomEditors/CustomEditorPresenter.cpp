
#include "CustomEditorPresenter.h"

#include "Element/LabelElement.h"
#include "CustomEditorFactory.h"
#include "Runtime/Core/Types/Object.h"
#include "Runtime/UI/GUI/Common/ClickableLabel.h"
#include "Runtime/UI/GUI/Panels/Panel.h"
#include "Values/ValueContainer.h"
#include "Runtime/Core/Types/Collections/Span.h"

namespace SE::Editor
{
	CustomEditorPresenter::PresenterPanel::PresenterPanel(CustomEditorPresenter* presenter)
	{
		_presenter = presenter;
		AnchorPreset = AnchorPresets::StretchAll;
		Offsets = Margin::Zero;
		IsScrollable = true;
	}

	void CustomEditorPresenter::PresenterPanel::Update(float deltaTime)
	{
		// Update editors
		_presenter->Update();
		VerticalPanel::Update(deltaTime);
	}

	void CustomEditorPresenter::PresenterPanel::OnDestroy()
	{
		VerticalPanel::OnDestroy();

		_presenter = nullptr;
	}

	CustomEditorPresenter* CustomEditorPresenter::PresenterPanel::__GetPresenter()
	{
		return _presenter;
	}

	CustomEditorPresenter::RootEditor::RootEditor(StringView noSelectionText)
	{
		if (noSelectionText.IsEmpty())
		{
			NoSelectionText = SE_TEXT("No selection");
		}
		else
		{
			NoSelectionText = noSelectionText;
		}
	}

	void CustomEditorPresenter::RootEditor::Setup(CustomEditorPresenter* presenter)
	{
		Cleanup();
		Initialize(presenter, presenter, nullptr);
	}

	void CustomEditorPresenter::RootEditor::OnInitialize(LayoutElementsContainer* layout)
	{
		CustomEditorPresenter* presenter = GetPresenter();

		if (presenter->BeforeLayout.IsBinded())
		{
			presenter->BeforeLayout(layout);
		}

		ValueContainer* selection = presenter->Selection;
		selection->ClearReferenceValue();
		if (selection->Count() > 0)
		{
			if (_overrideEditor != nullptr)
			{
				Editor = _overrideEditor;
			}
			else
			{
				// Create editor based on selection type
				Editor = CustomEditorFactory::CreateEditor(selection, nullptr);
			}

			if (Editor)
			{
				Editor->Initialize(presenter, presenter, selection);
				OnChildCreated(Editor);
			}
		}
		else
		{
			LabelElement* label = layout->Label(NoSelectionText, TextAlignment::Center);
			label->Label->Height = 20.0f;
		}

		SyncPointEditor::OnInitialize(layout);

		if (presenter->AfterLayout.IsBinded())
		{
			presenter->AfterLayout(layout);
		}
	}

	CustomEditor* CustomEditorPresenter::RootEditor::GetOverrideEditor()
	{
		return _overrideEditor;
	}

	void CustomEditorPresenter::RootEditor::SetOverrideEditor(CustomEditor* value)
	{
		_overrideEditor = value;
		RebuildLayout();
	}

	void CustomEditorPresenter::RootEditor::OnModified()
	{
		GetPresenter()->OnModified();

		SyncPointEditor::OnModified();
	}


	CustomEditorPresenter::CustomEditorPresenter(StringView noSelectionText, IPresenterOwner* owner)
	{
		// Undo = undo;
		Owner = owner;
		_noSelectionText = noSelectionText.IsEmpty() ? SE_TEXT("No selection") : String(noSelectionText);
		Selection = New<ValueContainer>();
		Panel = new PresenterPanel(this);
		Editor = new RootEditor(noSelectionText);
		Editor->Initialize(this, this, nullptr);
	}

	CustomEditorPresenter::~CustomEditorPresenter()
	{
		if (Editor)
		{
			Editor->Cleanup();
			Delete(Editor);
			Editor = nullptr;
		}

		if (Selection)
		{
			Delete(Selection);
			Selection = nullptr;
		}

		// Panel will be destroyed by the GUI system
	}

	int CustomEditorPresenter::GetSelectionCount() const
	{
		return Selection ? Selection->Count() : 0;
	}

	CustomEditor* CustomEditorPresenter::GetOverrideEditor() const
	{
		return Editor ? Editor->GetOverrideEditor() : nullptr;
	}

	void CustomEditorPresenter::SetOverrideEditor(CustomEditor* editor)
	{
		if (Editor)
		{
			Editor->SetOverrideEditor(editor);
		}
	}

	void CustomEditorPresenter::SetReadOnly(bool readOnly)
	{
		if (_readOnly == readOnly)
			return;

		_readOnly = readOnly;
		UpdateReadOnly();
	}

	Control* CustomEditorPresenter::GetControl()
	{
		return Panel;
	}

	ContainerControl* CustomEditorPresenter::GetContainerControl()
	{
		return Panel;
	}

	void CustomEditorPresenter::SetNoSelectionText(const String& text)
	{
		_noSelectionText = text;
		if (Editor)
		{
			Editor->NoSelectionText = text;
		}
	}

	void CustomEditorPresenter::Deselect()
	{
		if (Selection->Count() == 0)
			return;

		Selection->Clear();
		OnSelectionChanged();
	}

	void CustomEditorPresenter::OpenAllGroups()
	{
		// TODO: Implement when GroupElement is available
		LOG_INFO("Editor", "CustomEditorPresenter::OpenAllGroups - Not implemented yet");
	}

	void CustomEditorPresenter::CloseAllGroups()
	{
		// TODO: Implement when GroupElement is available
		LOG_INFO("Editor", "CustomEditorPresenter::CloseAllGroups - Not implemented yet");
	}

	void CustomEditorPresenter::BuildLayoutOnUpdate()
	{
		_buildOnUpdate = true;
	}

	void CustomEditorPresenter::BuildLayout()
	{
		// Clear layout
		float parentScrollV = -1;
		::SE::Panel* panel = TypeCast<::SE::Panel>(Panel->Parent.Get());
		if (panel != nullptr && panel->vScrollBar != nullptr)
		{
			parentScrollV = panel->vScrollBar->GetValue();
		}

		Panel->SetIsLayoutLocked(true);
		Panel->DisposeChildren();

		ClearLayout();
		Editor->Setup(this);

		Panel->SetIsLayoutLocked(false);
		Panel->PerformLayout();

		// Restore scroll value
		if (parentScrollV > -1 && panel && panel->vScrollBar)
		{
			panel->vScrollBar->SetValue(parentScrollV);
		}

		if (_readOnly)
		{
			UpdateReadOnly();
		}
	}

	void CustomEditorPresenter::ClearLayout()
	{
		Children.Clear();
		Editors.Clear();
	}

	void CustomEditorPresenter::Select(::SE::Object* obj)
	{
		if (obj == nullptr)
		{
			Deselect();
			return;
		}

		if (Selection->Count() == 1 && Selection->At(0).instance == obj)
		{
			return;
		}

		Selection->Clear();
		Selection->Add(obj);

		OnSelectionChanged();
	}

	void CustomEditorPresenter::Select(const Span<::SE::Object*>& objs)
	{
		if (objs.Length() <= 0)
		{
			Deselect();
			return;
		}

		if (objs.Length() == Selection->Count())
		{
			bool sameSelection = true;
			for (int i = 0; i < Selection->Count(); i++)
			{
				if (objs[i] != Selection->At(i).instance)
				{
					sameSelection = false;
					break;
				}
			}

			if (sameSelection)
			{
				return;
			}
		}

		Selection->Clear();
		for (int i = 0; i < objs.Length(); i++)
		{
			Selection->Add(objs[i]);
		}

		OnSelectionChanged();
	}

	void CustomEditorPresenter::Update()
	{
		if (_buildOnUpdate)
		{
			_buildOnUpdate = false;
			BuildLayout();
		}

		// Skip refresh if in batch update mode
		if (_batchUpdateMode)
			return;

		if (Editor != nullptr)
		{
			Editor->RefreshInternal();
		}
	}

	void CustomEditorPresenter::OnSelectionChanged()
	{
		BuildLayout();
		if (SelectionChanged.IsBinded())
		{
			SelectionChanged();
		}
	}

	void CustomEditorPresenter::UpdateReadOnly()
	{
		// TODO: Implement recursive read-only update when Control supports it
		LOG_INFO("Editor", "CustomEditorPresenter::UpdateReadOnly - Not fully implemented yet");
		
		// Recursively set all controls to read-only
		// This would need to traverse all child editors and their controls
	}
} // SE
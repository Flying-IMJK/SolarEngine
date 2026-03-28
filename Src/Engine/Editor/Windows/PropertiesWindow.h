#pragma once
#include "EditorWindow.h"
#include "Editor/CustomEditors/CustomEditorPresenter.h"

namespace SE::Editor
{
	class CustomEditorPresenter;

	class PropertiesWindow : public EditorWindow, public IPresenterOwner
	{
	public:
		/// <summary>
		/// The editor.
		/// </summary>
		CustomEditorPresenter* Presenter;

		PropertiesWindow();

		EditorViewport* GetPresenterViewport() override;
		void Select(List<ScenesGraphNode*> nodes) override;

	private:
		void OnSelectionChanged();
	};

} // SE

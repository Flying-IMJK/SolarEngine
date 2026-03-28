

#include "PropertiesWindow.h"

#include "EditSceneWindow.h"
#include "Editor/EditorApp.h"
#include "Editor/CustomEditors/CustomEditorPresenter.h"
#include "Editor/Modules/SceneModule.h"
#include "Editor/Modules/WindowsModule.h"
#include "Editor/Viewport/EditorGizmoViewport.h"

namespace SE::Editor
{
	PropertiesWindow::PropertiesWindow() : EditorWindow(&EditorApp::Ins(), true, ScrollBars::Vertical)
	{
		Title = SE_TEXT("Properties");
		AutoFocus = true;

		Presenter = New<CustomEditorPresenter>(/*editor.Undo, */StringView::Empty, this);
		Presenter->Panel->Parent = this;
		/*Presenter->GetUndoObjects += GetUndoObjects;
		Presenter->Features |= FeatureFlags.CacheExpandedGroups;*/

		EditorApp::Ins().sceneModule->SelectionChanged.BindUnique<PropertiesWindow, &PropertiesWindow::OnSelectionChanged>(this);
	}

	EditorViewport* PropertiesWindow::GetPresenterViewport()
	{
		return editor->windowsModule->EditSceneWin->Viewport;
	}

	void PropertiesWindow::Select(List<ScenesGraphNode*> nodes)
	{
		editor->sceneModule->Select(nodes);
	}

	void PropertiesWindow::OnSelectionChanged()
	{
		/*if (LockObjects)
			return;*/

		// Update selected objects
		// TODO: use cached collection for less memory allocations
		// undoRecordObjects = Editor.SceneEditing.Selection.ConvertAll(x => x.UndoRecordObject).Distinct();

		List<Object*> types;
		for (ScenesGraphNode* graphNode : editor->sceneModule->Selection)
		{
			types.Add(graphNode->GetEditableObject());
		}
		Presenter->Select(Span<Object*>(types.Get(), types.Count()));
	}
} // SE
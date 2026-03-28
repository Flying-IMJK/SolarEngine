
#include "EditorWindow.h"

#include "Editor/EditorApp.h"
#include "Editor/Modules/UIModule.h"
#include "Editor/Modules/WindowsModule.h"
#include "Runtime/UI/GUI/RootControl.h"

namespace SE::Editor
{

	EditorWindow::EditorWindow(const EditorApp* editor, bool hideOnClose, ScrollBars scrollBars) : DockWindow(editor->uiModule->GetMasterPanel(), hideOnClose, scrollBars)
	{
		AutoFocus = true;
		this->editor = editor;

		/*
		InputActions.Add(options => options.ContentFinder, () =>
		{
			if (CanOpenContentFinder)
			{
				Editor.ContentFinding.ShowFinder(RootWindow);
			}
		});
		*/

		// Register
		editor->windowsModule->OnWindowAdd(this);
	}

	bool EditorWindow::OnKeyDown(KeyboardKeys key)
	{
		/*// Prevent closing the editor window when using RMB + Ctrl + W to slow down the camera flight
		if (Editor.Options.Options.Input.CloseTab.Process(this, key))
		{
			if (Root->GetMouseButton(MouseButton::Right))
				return true;
		}*/

		if (DockWindow::OnKeyDown(key))
			return true;

		switch (key)
		{
		case KeyboardKeys::Return:
			if (CanUseNavigation() && Root != nullptr && Root->GetFocusedControl() != nullptr)
			{
				Root->SubmitFocused();
				return true;
			}
			break;
		case KeyboardKeys::Tab:
			if (CanUseNavigation() && Root != nullptr)
			{
				bool shiftDown = Root->GetKey(KeyboardKeys::Shift);
				Root->Navigate(shiftDown ? NavDirection::Previous : NavDirection::Next);
				return true;
			}
			break;
		}
		return false;
	}

	void EditorWindow::OnDestroy()
	{
		if (IsDisposing)
			return;
		OnExit();

		// Unregister
		editor->windowsModule->OnWindowRemove(this);

		DockWindow::OnDestroy();
	}
} // SE
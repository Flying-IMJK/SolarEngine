

#include "WindowsModule.h"

#include "Editor/Windows/EditorWindow.h"
#include "Editor/Windows/LogWindow.h"
#include "Editor/Windows/ContentWindow.h"
#include "Editor/Windows/SceneHierarchyWindow.h"
#include "Editor/Windows/EditSceneWindow.h"
#include "Editor/Windows/PropertiesWindow.h"

namespace SE::Editor
{
	inline int WindowsModule::InitOrder()
	{
		return -90;
	}

	EditorWindow* WindowsModule::FindEditor(ContentItem* item)
	{
		if (item == nullptr)
			return nullptr;
		for (int i = 0; i < m_Windows.Count(); i++)
		{
			EditorWindow* win = m_Windows[i];
			if (win->IsEditingItem(item))
			{
				return win;
			}
		}
		return nullptr;
	}

	void WindowsModule::CloseAllEditors(ContentItem* item)
	{
		for (int i = 0; i < m_Windows.Count(); i++)
		{
			EditorWindow* win = m_Windows[i];
			if (win->IsEditingItem(item))
			{
				win->Close();
				i--;
			}
		}
	}

	WindowsModule::WindowsModule(EditorApp* editor) : EditorModule(editor)
	{

	}

	void WindowsModule::OnInit()
	{
		DebugLogWin = New<LogWindow>();
		ContentWin = New<ContentWindow>();
		SceneHierarchyWin = New<SceneHierarchyWindow>();
		EditSceneWin = New<EditSceneWindow>();
		PropertiesWin = New<PropertiesWindow>();
	}

	void WindowsModule::OnEndInit()
	{
		// UpdateWindowTitle();

		// Initialize windows
		for (int i = 0; i < m_Windows.Count(); i++)
		{
			m_Windows[i]->OnInit();
		}

		// Load current workspace layout
		/*if (!LoadLayout(_windowsLayoutPath))
		{
			LoadDefaultLayout();
		}*/

		// Clear timer flag
		// _lastLayoutSaveTime = DateTime.UtcNow;
	}

	void WindowsModule::OnUpdate()
	{
		// Update editor windows
		for (int i = 0; i < m_Windows.Count(); i++)
		{
			m_Windows[i]->OnUpdate();
		}
	}

	void WindowsModule::OnWindowAdd(EditorWindow* window)
	{
		m_Windows.Add(window);
		WindowAdded(window);
	}

	void WindowsModule::OnWindowRemove(EditorWindow* window)
	{
		m_Windows.Remove(window);
		WindowRemoved(window);
	}
}

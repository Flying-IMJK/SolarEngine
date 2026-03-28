#pragma once
#include "EditorModule.h"
#include "Core/Types/Delegate.h"
#include "Core/Types/Collections/List.h"

namespace SE::Editor
{
	class PropertiesWindow;
	class EditSceneWindow;
	class ContentWindow;
	class ContentItem;
	class LogWindow;
	class EditorWindow;
	class SceneHierarchyWindow;

	class WindowsModule : public EditorModule
	{
		friend class EditorWindow;
	public:

		/// <summary>
		/// The debug log window.
		/// </summary>
		LogWindow* DebugLogWin = nullptr;

		/// <summary>
		/// The content window.
		/// </summary>
		ContentWindow* ContentWin = nullptr;

		SceneHierarchyWindow* SceneHierarchyWin = nullptr;

		PropertiesWindow* PropertiesWin = nullptr;

		EditSceneWindow* EditSceneWin = nullptr;

		/// <summary>
		/// Occurs when new window gets opened and added to the editor windows list.
		/// </summary>
		Delegate<EditorWindow*> WindowAdded;

		/// <summary>
		/// Occurs when new window gets closed and removed from the editor windows list.
		/// </summary>
		Delegate<EditorWindow*> WindowRemoved;

		/// <summary>
		/// Finds the first window that is using given element to view/edit it.
		/// </summary>
		/// <param name="item">The item.</param>
		/// <returns>Editor window or null if cannot find any window.</returns>
		EditorWindow* FindEditor(ContentItem* item);

		/// <summary>
		/// Closes all windows that are using given element to view/edit it.
		/// </summary>
		/// <param name="item">The item.</param>
		void CloseAllEditors(ContentItem* item);


		WindowsModule(EditorApp* editor);

		int InitOrder() override;

		void OnInit() override;

		void OnEndInit() override;

		void OnUpdate() override;

	private:

		/// <summary>
		/// List with all created editor windows.
		/// </summary>
		List<EditorWindow*> m_Windows = List<EditorWindow*>(32);

		void OnWindowAdd(EditorWindow* window);

		void OnWindowRemove(EditorWindow* window);

	};

} // SE


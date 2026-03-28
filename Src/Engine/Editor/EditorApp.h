#pragma once

#include "Editor/API.h"
#include "Runtime/App.h"

#include "EditorContext.h"

namespace SE::Editor
{
	class SceneGraphModule;
	class SceneModule;
}

namespace SE
{
	class ProjectInfo;
}

namespace SE::Editor
{
	class AssetImportingModule;
	class AssetDatabaseModule;
	class UIModule;
	class WindowsModule;
	class EditorModule;
	class SettingModule;

	class SE_API_EDITOR EditorApp : public App
	{
		constexpr static float s_windowControlButtonWidth = 45;
		constexpr static float s_minimumDraggableGap = 24; // Minimum open gap left open to allow dragging
		constexpr static float s_sectionPadding = 8;       // Padding between the window frame/window controls and the menu/control sections

		static float GetWindowsControlsWidth()
		{
			return s_windowControlButtonWidth * 3;
		}
	public:

		static EditorApp& Ins();

		/// <summary>
		/// Information about the loaded game project.
		/// </summary>
		ProjectInfo* Project;
		SettingModule* settingModule;
		UIModule* uiModule;
		SceneModule* sceneModule;
		SceneGraphModule* sceneGraphModule;
		WindowsModule* windowsModule;
		AssetDatabaseModule* databaseModule;
		AssetImportingModule* importingModule;

	public:
		int32 LoadProduct() override;
		GraphicWindow* CreateMainWindow()  override;
		GraphicWindow* GetMainWindow() override;
		bool Init()  override;
		void BeforeRun()  override;
		void BeforeExit()  override;

		void OnUpdate() override;
		void OnLateUpdate() override;
		void OnRender() override;

	private:

		bool _isAfterInit = false, _areModulesInited = false, _areModulesAfterInitEnd = false;

		/*void BorderlessWindowHitTest(const Float2& mouse, WindowHitCodes& hit, bool& handled);
		void DrawToolBar();
		void DrawWindowControls();*/

		void RegisterModule(EditorModule* module);

		List<EditorModule*> m_Modules = List<EditorModule*>(16);
		GraphicWindow* m_Window = nullptr;

		EditorContext m_Context;
	};
}

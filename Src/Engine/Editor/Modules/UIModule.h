#pragma once

#include "EditorModule.h"

namespace SE
{
	class RootControl;
}

namespace SE::Editor
{
	class Thumbnails;
	class ToolStrip;
	class StatusBar;
	class MasterDockPanel;
	class MainMenu;

	class UIModule : public EditorModule
	{
	public:
		int InitOrder() override;

		explicit UIModule(EditorApp* editor);

		Thumbnails* thumbnails;

		/// <summary>
		/// The main menu control.
		/// </summary>
		MainMenu* MainMenu = nullptr;

		void OnInit() override;
		void OnUpdate() override;
		void OnExit() override;

		MasterDockPanel* GetMasterPanel();

	private:
		void InitMainMenu(RootControl* mainWindow);
		void InitToolstrip(RootControl* mainWindow);
		void InitStatusBar(RootControl* mainWindow);
		void InitDockPanel(RootControl* mainWindow);

		/// <summary>
		/// The master dock panel for all Editor windows.
		/// </summary>
		MasterDockPanel* m_MasterPanel;
		/// <summary>
		/// The status strip control.
		/// </summary>
		StatusBar* m_StatusBar;

		/// <summary>
		/// The tool strip control.
		/// </summary>
		ToolStrip* m_ToolStrip;
	};

} // SE

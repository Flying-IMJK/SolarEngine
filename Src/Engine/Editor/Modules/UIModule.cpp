
#include "UIModule.h"

#include "WindowsModule.h"
#include "Editor/EditorApp.h"
#include "Editor/GUI/MainMenu.h"
#include "editor/gui/mainmenubutton.h"
#include "Editor/GUI/StatusBar.h"
#include "Editor/GUI/ToolStrip.h"
#include "Editor/GUI/ContextMenu/ContextMenu.h"
#include "Editor/GUI/Docking/MasterDockPanel.h"
#include "Editor/Resource/Thumbnails/ThumbnailRequest.h"
#include "Editor/Resource/Thumbnails/Thumbnails.h"
#include "Editor/Windows/ContentWindow.h"
#include "Runtime/UI/GUI/Style.h"
#include "Runtime/UI/GUI/WindowRootControl.h"

#include "Editor/Test.h"

namespace SE::Editor
{
	int UIModule::InitOrder()
	{
		return -95;
	}

	UIModule::UIModule(EditorApp* editor) : EditorModule(editor)
	{
		thumbnails = New<Thumbnails>(editor);
	}

	void UIModule::OnInit()
	{
		m_MasterPanel = New<MasterDockPanel>();
		auto mainWindow = editor->GetMainWindow()->GetGUI();

		// Update window background
		mainWindow->BackgroundColor = Style::Current->Background;

		thumbnails->OnInit();

		InitMainMenu(mainWindow);
		InitToolstrip(mainWindow);
		InitStatusBar(mainWindow);
		InitDockPanel(mainWindow);
	}

	void UIModule::OnUpdate()
	{
		thumbnails->OnUpdate();
	}
	void UIModule::OnExit()
	{
		thumbnails->OnExit();
	}

	MasterDockPanel* UIModule::GetMasterPanel()
	{
		return m_MasterPanel;
	}

	void UIModule::InitMainMenu(RootControl* mainWindow)
	{
		MainMenu = New<Editor::MainMenu>(mainWindow);
		MainMenu->Parent = mainWindow;

		ContextMenu* contextMenu = nullptr;

		// Window
		MainMenuButton* menuWindow = MainMenu->AddButton(SE_TEXT("Window"));
		contextMenu = menuWindow->ContextMenu;
		// contextMenu->VisibleChanged += OnMenuWindowVisibleChanged;
		contextMenu->AddButton(SE_TEXT("Content"), CreateFunc([this]()
		{
			editor->windowsModule->ContentWin->FocusOrShow();
		}));
		contextMenu->AddButton(SE_TEXT("TestScene"), CreateFunc([this]()
		{
			CreateTestScene();
		}));
		/*contextMenu->AddButton(SE_TEXT("Scene"), Editor.Windows.SceneWin.FocusOrShow);
		contextMenu->AddButton(SE_TEXT("Toolbox"), Editor.Windows.ToolboxWin.FocusOrShow);
		contextMenu->AddButton(SE_TEXT("Properties"), Editor.Windows.PropertiesWin.FocusOrShow);
		contextMenu->AddButton(SE_TEXT("Game"), Editor.Windows.GameWin.FocusOrShow);
		contextMenu->AddButton(SE_TEXT("Editor"), Editor.Windows.EditWin.FocusOrShow);
		contextMenu->AddButton(SE_TEXT("Debug Log"), Editor.Windows.DebugLogWin.FocusOrShow);
		contextMenu->AddButton(SE_TEXT("Output Log"), Editor.Windows.OutputLogWin.FocusOrShow);
		contextMenu->AddButton(SE_TEXT("Graphics Quality"), Editor.Windows.GraphicsQualityWin.FocusOrShow);
		contextMenu->AddButton(SE_TEXT("Game Cooker"), Editor.Windows.GameCookerWin.FocusOrShow);
		contextMenu->AddButton(SE_TEXT("Profiler"), inputOptions.ProfilerWindow, Editor.Windows.ProfilerWin.FocusOrShow);
		contextMenu->AddButton(SE_TEXT("Content Search"), Editor.ContentFinding.ShowSearch);
		contextMenu->AddButton(SE_TEXT("Visual Script Debugger"), Editor.Windows.VisualScriptDebuggerWin.FocusOrShow);*/
		contextMenu->AddSeparator();
		// contextMenu->AddButton("Save window layout", Editor.Windows.SaveLayout);
		/*_menuWindowApplyWindowLayout = contextMenu->AddChildMenu("Window layouts");
		cm.AddButton("Restore default layout", Editor.Windows.LoadDefaultLayout);*/
	}

	void UIModule::InitToolstrip(RootControl* mainWindow)
	{
		m_ToolStrip = New<ToolStrip>(34.0f, MainMenu->Bottom);
		m_ToolStrip->Parent = mainWindow;
	}

	void UIModule::InitStatusBar(RootControl* mainWindow)
	{
		// Status Bar
		m_StatusBar = New<StatusBar>();
		m_StatusBar->Text = "Loading...";
		m_StatusBar->Parent = mainWindow;
		m_StatusBar->Offsets = Margin(0, 0, -StatusBar::DefaultHeight, StatusBar::DefaultHeight);
	}

	void UIModule::InitDockPanel(RootControl* mainWindow)
	{
		// Dock Panel
		m_MasterPanel->AnchorPreset = AnchorPresets::StretchAll;
		m_MasterPanel->Parent = mainWindow;
		m_MasterPanel->Offsets = Margin(0, 0, m_ToolStrip->Bottom, m_StatusBar->Height);
	}
} // SE
#include "EditorApp.h"

#include "ManagedEditor.h"
#include "Modules/EditorModule.h"
#include "Modules/SettingModule.h"
#include "Modules/UIModule.h"
#include "Modules/WindowsModule.h"
#include "Modules/AssetDatabaseModule.h"
#include "Windows/LogWindow.h"

#include "Runtime/EngineContext.h"
#include "Runtime/Settings/GlobalSettings_Resource.h"
#include "Runtime/Graphics/GraphicWindow.h"
#include "Runtime/Project/ProjectInfo.h"

#include "Editor/GUI/Docking/MasterDockPanel.h"
#include "Core/Types/Collections/Sorting.h"
#include "Core/Utilities/IniFile.h"
#include "Core/Platform/FileSystem.h"
#include "Modules/AssetImportingModule.h"
#include "Modules/SceneGraphModule.h"
#include "Modules/SceneModule.h"
#include "Windows/ContentWindow.h"
#include "Windows/EditSceneWindow.h"
#include "Windows/PropertiesWindow.h"
#include "Windows/SceneHierarchyWindow.h"

namespace SE::Editor
{
	EditorApp& EditorApp::Ins()
	{
		static EditorApp Instance;

		return Instance;
	}

	int32 EditorApp::LoadProduct()
	{
		String ProjectPath = EngineContext::StartupFolder / SE_TEXT("Project");
		bool NewProject = true;


		// Gather project directory from the command line
		String projectPath = ProjectPath.TrimTrailing();
		const int32 startIndex = projectPath.StartsWith('\"') || projectPath.StartsWith('\'') ? 1 : 0;
		const int32 length = projectPath.Length() - (projectPath.EndsWith('\"') || projectPath.EndsWith('\'') ? 1 : 0) - startIndex;
		if (length > 0)
		{
			projectPath = projectPath.Substring(startIndex, length - startIndex);
			FileSystem::PathRemoveRelativeParts(projectPath);
			if (FileSystem::IsRelative(projectPath))
			{
				projectPath = Platform::GetWorkingDirectory() / projectPath;
				FileSystem::PathRemoveRelativeParts(projectPath);
			}
			if (projectPath.EndsWith(SE_TEXT(".flaxproj")))
			{
				projectPath = FileSystem::GetDirectoryName(projectPath);
			}
		}
		else
		{
			projectPath.Clear();
		}

		// Create new project option
		if (NewProject)
		{
			List<String> projectFiles;
			FileSystem::DirectoryGetFiles(projectFiles, projectPath, SE_TEXT("*.flaxproj"), DirectorySearchOption::TopOnly);
			if (projectFiles.Count() > 1)
			{
				Platform::Fatal(SE_TEXT("Too many project files."));
				return -2;
			}
			else if (projectFiles.Count() == 1)
			{
				LOG_INFO("App", "Skip creating new project because it already exists");
			}
			else
			{
				List<String> files;
				FileSystem::DirectoryGetFiles(files, projectPath, SE_TEXT("*"), DirectorySearchOption::TopOnly);
				if (files.Count() > 0)
				{
					Platform::Fatal(String::Format(SE_TEXT("Target project folder '{0}' is not empty."), projectPath));
					return -1;
				}
			}
		}

		if (NewProject)
		{
			if (projectPath.IsEmpty())
			{
				projectPath = Platform::GetWorkingDirectory();
			}
			else if (!FileSystem::DirectoryExists(projectPath))
			{
				FileSystem::CreateDirectory(projectPath);
			}
			FileSystem::NormalizePath(projectPath);
			String folderName = FileSystem::GetFileName(projectPath);
			String tmpName;
			for (int32 i = 0; i < folderName.Length(); i++)
			{
				Char c = folderName[i];
				if (StringUtils::IsAlnum(c) && c != ' ' && c != '.')
				{
					tmpName += c;
				}
			}

			// Create project file
			ProjectInfo newProject;
			newProject.Name = MoveTemp(tmpName);
			newProject.ProjectPath = projectPath / newProject.Name + SE_TEXT(".flaxproj");
			newProject.ProjectFolderPath = projectPath;
			newProject.Version = VersionInfo(1, 0);
			newProject.Company = SE_TEXT("My Company");
			newProject.MinEngineVersion = VersionInfo(1, 0);;
			newProject.GameTarget = SE_TEXT("GameTarget");
			newProject.EditorTarget = SE_TEXT("GameEditorTarget");
			/*auto& flaxRef = newProject.References.AddOne();
			flaxRef.Name = SE_TEXT("$(EnginePath)/Flax.flaxproj");
			flaxRef.Project = nullptr;*/
			if (!newProject.SaveProject())
				return 10;

			// Generate source files
			if (!FileSystem::CreateDirectory(projectPath / SE_TEXT("Content")))
				return 11;
			if (!FileSystem::CreateDirectory(projectPath / SE_TEXT("Source/Game")))
				return 11;

			/*if (failed)
				return 12;*/
		}

		// Missing project case
		if (projectPath.IsEmpty())
		{
/*#if PLATFORM_HAS_HEADLESS_MODE
			if (CommandLine::Options.Headless)
			{
				Platform::Fatal(TEXT("Missing project path."));
				return -1;
			}
#endif*/

			// Ask user to pick a project to open
			List<String> files;
			if (!FileSystem::ShowOpenFileDialog(
				nullptr,
				StringView::Empty,
				SE_TEXT("Project files (*.flaxproj)\0*.flaxproj\0All files (*.*)\0*.*\0"),
				false,
				SE_TEXT("Select project to open in Editor"),
				files) || files.Count() != 1)
			{
				return -1;
			}
			if (!FileSystem::FileExists(files[0]))
			{
				Platform::Fatal(SE_TEXT("Cannot opoen selected project file because it doesn't exist."));
				return -1;
			}
			projectPath = FileSystem::GetDirectoryName(files[0]);
			FileSystem::PathRemoveRelativeParts(projectPath);
		}

		// Check folder with project exists
		if (!FileSystem::DirectoryExists(projectPath))
		{
			Platform::Fatal(String::Format(SE_TEXT("Project folder '{0}' is missing"), projectPath));
			return -1;
		}

		EngineContext::ProjectFolder = projectPath;
		ENGINE_ASSERT(!FileSystem::IsRelative(EngineContext::ProjectFolder));

		// Load project
		Project = New<ProjectInfo>();
		List<String> projectFiles;
		FileSystem::DirectoryGetFiles(projectFiles, projectPath, SE_TEXT("*.flaxproj"), DirectorySearchOption::TopOnly);
		if (projectFiles.Count() == 0)
		{
			Platform::Fatal(SE_TEXT("Missing project file (*.flaxproj)."));
			return -2;
		}
		if (projectFiles.Count() > 1)
		{
			Platform::Fatal(SE_TEXT("Too many project files."));
			return -2;
		}
		const bool loadResult = Project->LoadProject(projectFiles[0]);
		if (!loadResult)
		{
			Platform::Fatal(SE_TEXT("Cannot load project."));
			return -2;
		}

		HashSet<ProjectInfo*> projects;
		Project->GetAllProjects(projects);

		return 0;
	}

	GraphicWindow* EditorApp::CreateMainWindow()
	{
		CreateWindowSettings createWindowSettings;
		createWindowSettings.Title = "SE Editor";
		createWindowSettings.HasBorder = false;
		createWindowSettings.AllowDragAndDrop = true;

		m_Window = New<GraphicWindow>(createWindowSettings);

		return m_Window;
	}

	GraphicWindow* EditorApp::GetMainWindow()
	{
		return m_Window;
	}

	bool EditorApp::Init()
	{
		ResourceGlobalSettings globalSettings;
		IniFile settings;
		globalSettings.LoadSettings(settings);

		managedEditor = New<ManagedEditor>();
		return true;
	}

	void EditorApp::BeforeRun()
	{
		managedEditor->Init();

		RegisterModule(settingModule = New<SettingModule>(this));
		RegisterModule(uiModule = New<UIModule>(this));
		RegisterModule(sceneModule = New<SceneModule>(this));
		RegisterModule(sceneGraphModule = New<SceneGraphModule>(this));
		RegisterModule(windowsModule = New<WindowsModule>(this));
		RegisterModule(databaseModule = New<AssetDatabaseModule>(this));
		RegisterModule(importingModule = New<AssetImportingModule>(this));

		// Note: we don't sort modules before Init (optimized)
		const auto sortFunc = CreateFunc([](EditorModule* const &a, EditorModule* const &b)
		{
			return a->InitOrder() < b->InitOrder();
		});
		Sorting::QuickSort(m_Modules, sortFunc);
		_isAfterInit = true;

		// Initialize modules (from front to back)
		for (int i = 0; i < m_Modules.Count(); i++)
		{
			EditorModule* module = m_Modules[i];
			module->OnInit();
		}
		_areModulesInited = true;


		// Initialize modules (from front to back)
		for (int i = 0; i < m_Modules.Count(); i++)
		{
			EditorModule* module = m_Modules[i];
			module->OnEndInit();
		}

		DockPanel* dockWindow = uiModule->GetMasterPanel();
		windowsModule->DebugLogWin->Show(DockState::DockFill, dockWindow);
		windowsModule->ContentWin->Show(DockState::DockFill, dockWindow);
		windowsModule->SceneHierarchyWin->Show(DockState::DockFill, dockWindow);
		windowsModule->PropertiesWin->Show(DockState::DockFill, dockWindow);
		windowsModule->EditSceneWin->Show(DockState::DockFill, dockWindow);
	}

	void EditorApp::BeforeExit()
	{
		m_Window->Close();
		Delete(m_Window);
	}

	void EditorApp::OnUpdate()
	{
		// Update modules
		for (int i = 0; i < m_Modules.Count(); i++)
		{
			m_Modules[i]->OnUpdate();
		}
	}

	void EditorApp::OnLateUpdate()
	{

	}

	void EditorApp::OnRender()
	{

	}

	void EditorApp::RegisterModule(EditorModule* module)
	{
		LOG_INFO("Editor", "Register Editor module ");

		m_Modules.Add(module);
		if (_isAfterInit)
		{
			Sorting::QuickSort(m_Modules, CreateFunc([](EditorModule* const &a, EditorModule* const &b) {
				return a->InitOrder() < b->InitOrder();
			}));
		}
		if (_areModulesInited)
			module->OnInit();
		if (_areModulesAfterInitEnd)
			module->OnEndInit();
	}
}

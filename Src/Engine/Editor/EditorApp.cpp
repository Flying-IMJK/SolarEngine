#include "EditorApp.h"

#include "Runtime/EngineContext.h"
#include "Runtime/Settings/GlobalSettings_Resource.h"
#include "Runtime/Graphics/GraphicWindow.h"
#include "Runtime/Project/ProjectInfo.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRClass.h"
#include "Runtime/Core/Scripting/Binary/NativeBinaryModule.h"
#include "Runtime/Core/Types/Collections/Sorting.h"
#include "Runtime/Core/IniFile.h"
#include "Runtime/Core/Platform/FileSystem.h"

#include "Runtime/Core/Scripting/ManagedCLR/CLRMethod.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRUtils.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRException.h"

#include "Runtime/Render/2D/SpriteAtlasPacker.h"
#include "Runtime/Render/2D/SpriteAtlas.h"
#include "Runtime/Render/Assets/Texture/Texture.h"
#include "Runtime/Resource/Factories/IAssetFactory.h"
#include "Runtime/Resource/Importers/AssetsImportingSystem.h"
#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/Graphics/GPUDevice.h"


#include "Editor/EditorIcons.h"
#include "Editor/EditorAssets.h"

#include "Test.h"

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
			if (projectPath.EndsWith(SE_TEXT(".solarproj")))
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
			FileSystem::DirectoryGetFiles(projectFiles, projectPath, SE_TEXT("*.solarproj"), DirectorySearchOption::TopOnly);
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
			newProject.ProjectPath = projectPath / newProject.Name + SE_TEXT(".solarproj");
			newProject.ProjectFolderPath = projectPath;
			newProject.Version = VersionInfo(1, 0);
			newProject.Company = SE_TEXT("My Company");
			newProject.MinEngineVersion = VersionInfo(1, 0);;
			newProject.GameTarget = SE_TEXT("GameTarget");
			newProject.EditorTarget = SE_TEXT("GameEditorTarget");
			/*auto& flaxRef = newProject.References.AddOne();
			flaxRef.Name = SE_TEXT("$(EnginePath)/Flax.solarproj");
			flaxRef.Project = nullptr;*/
			if (!newProject.SaveProject())
            {
				return 10;
            }

			// Generate source files
			if (!FileSystem::CreateDirectory(projectPath / SE_TEXT("Content")))
            {
				return 11;
            }
			if (!FileSystem::CreateDirectory(projectPath / SE_TEXT("Source/Game")))
            {
				return 11;
            }

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
				SE_TEXT("Project files (*.solarproj)\0*.solarproj\0All files (*.*)\0*.*\0"),
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
		FileSystem::DirectoryGetFiles(projectFiles, projectPath, SE_TEXT("*.solarproj"), DirectorySearchOption::TopOnly);
		if (projectFiles.Count() == 0)
		{
			Platform::Fatal(SE_TEXT("Missing project file (*.solarproj)."));
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

	Window* EditorApp::CreateMainWindow()
	{
		return managedEditor ? managedEditor->GetMainWindow() : nullptr;
	}

	Window* EditorApp::GetMainWindow()
	{
		return managedEditor ? managedEditor->GetMainWindow() : nullptr;
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
		// Asset import and icon loading require the initialized engine systems.
		ImportAsset();
		LoadIcon();

		// The native modules now provide only background services. Initialize the
		// managed presentation after those services are ready.
		managedEditor->Init();
	}

	void EditorApp::BeforeExit()
	{
		if (managedEditor != nullptr)
		{
			managedEditor->Exit();
			Delete(managedEditor);
			managedEditor = nullptr;
		}

		// m_Window->Close();
		// Delete(m_Window);
	}

	void EditorApp::OnUpdate()
	{
		if (!m_IconAtlasUpload)
        {
			UploadIcon();
        }

		if (managedEditor != nullptr)
		{
			managedEditor->Update();
		}
	}

	void EditorApp::OnLateUpdate()
	{
		if (managedEditor != nullptr)
		{
			managedEditor->LateUpdate();
		}
	}

	void EditorApp::OnRender()
	{
		if (managedEditor != nullptr)
		{
			managedEditor->Render();
		}
	}

    void EditorApp::ImportAsset()
    {
    	String assetsPath = String::Format(SE_TEXT("{0}/{1}"), EngineContext::ProjectCacheFolder, EditorAssets::AssetsPath);

    	List<String> file;
    	FileSystem::DirectoryGetFiles(file, assetsPath, nullptr);
		for (String fileName : file)
		{
			String fileExtension = FileSystem::GetExtension(fileName);
			if (fileExtension == ASSET_FILES_EXTENSION)
			{
				continue;
			}

			auto importer = AssetsImporting::GetImporter(fileExtension);
			if (importer == nullptr)
			{
				continue;
			}

			String outPath = fileName;
			FileSystem::ReplaceExtension(outPath, ASSET_FILES_EXTENSION);
			AssetsImporting::ImportIfEdited(fileName, outPath);
		}
    }

    void EditorApp::LoadIcon()
	{
	    List<IconPack*> icons;
		StringBuilder path;
    	// Find icons
    	for (IconPack &iconDefine : EditorIcons::GetIconDefine())
    	{
    		path.Clear();
    		path.Append(EditorAssets::IconsPath);
    		path.Append("/");
    		path.Append(iconDefine.name);
    		auto icon = AssetContent::LoadAsyncInternal<Texture>(path.ToString());

    		if (icon == nullptr)
    		{
    			LOG_WARNING("Setting", "not find icon \'{0}\'.", iconDefine.name);
    			continue;
    		}

    		iconDefine.texture = icon;

    		icons.Add(&iconDefine);
    	}

		m_LoadedIcons.Clear();
    	List<IconPack*> loadFailedIcons;
    	while (icons.Count() > 0)
    	{
    		for (int i = 0; i < icons.Count(); i++)
    		{
    			IconPack* iconPack = icons[i];

    			if (!iconPack->texture->IsLoaded())
    			{
    				continue;
    			}

    			if (iconPack->texture->LastLoadFailed())
    			{
    				LOG_WARNING("Setting", "Failed to load icon \'{0}\'.", iconPack->name);
    				loadFailedIcons.Add(iconPack);
    			}
    			else
    			{
    				m_LoadedIcons.Add(iconPack);
    			}

    			icons.RemoveAt(i);
    			i--;
    		}
    	}

		m_IconAtlas = AssetContent::CreateVirtualAsset<SpriteAtlas>();

		TextureInitData* initData = TextureInitData::Create(2048, 2048, 1, PixelFormat::BC3_UNorm);
		m_IconAtlas->Init(initData);

		// BC3 copies need block-aligned destinations; icon dimensions and spacing are multiples of four.
		SpriteAtlasPacker atlasPacker = SpriteAtlasPacker(initData->Width, initData->Height, 4);

		for (int i = 0; i < m_LoadedIcons.Count(); i++)
		{
			IconPack* iconPack = m_LoadedIcons[i];

			atlasPacker.InsertRectangle(iconPack->texture->Width(), iconPack->texture->Height(), i);
		}

		const int packedCount = atlasPacker.PackRectangles();
		if (packedCount != m_LoadedIcons.Count())
		{
			LOG_ERROR("Setting", "Icon atlas is too small: packed {0} of {1} icons.", packedCount, m_LoadedIcons.Count());
			// Do not publish incomplete sprites or copy icons into missing rectangles.
			m_IconAtlasUpload = true;
			return;
		}

		Float2 atlasSize = m_IconAtlas->Size();
		for (int i = 0; i < m_LoadedIcons.Count(); i++)
		{
			IconPack* iconPack = m_LoadedIcons[i];

			SpriteAtlasPacker::Rect rectangle = atlasPacker.GetRectangle(i);

			Sprite sprite;
			sprite.Area = Rectangle(rectangle.x / atlasSize.X, rectangle.y / atlasSize.Y, rectangle.width / atlasSize.X, rectangle.height / atlasSize.Y);
			sprite.Name = iconPack->name;
			SpriteHandle handle = m_IconAtlas->AddSprite(sprite);

			*iconPack->value = handle;
			iconPack->position = Int2(rectangle.x, rectangle.y);
		}
    }

    void EditorApp::UploadIcon()
	{
		for (int i = 0; i < m_LoadedIcons.Count(); i++)
		{
			IconPack* iconPack = m_LoadedIcons[i];

			if (iconPack->texture->GetResidentMipLevels() < iconPack->texture->GetMipLevels())
			{
				return;
			}
		}

		if (m_IconAtlas->GetResidentMipLevels() <= 0)
		{
			return;
		}

		m_IconAtlasUpload = true;

		GPUContext* gpuContext = GPUDevice::instance->GetMainContext();
		for (int i = 0; i < m_LoadedIcons.Count(); i++)
		{
			IconPack* iconPack = m_LoadedIcons[i];
			gpuContext->CopyTexture(m_IconAtlas->GetTexture(), 0, iconPack->position.X, iconPack->position.Y, 0,
				iconPack->texture->GetTexture(), 0);
		}
	}



	UID ManagedEditor::ObjectID(0x91970b4e, 0x99634f61, 0x84723632, 0x54c776af);


	ManagedEditor::ManagedEditor(): ScriptingObject(SpawnParams(ObjectID, ManagedEditor::TypeInitializer))
	{
		auto editor = static_cast<NativeBinaryModule*>(GetBinaryModuleSEEditor())->Assembly;
		editor->Loaded.Bind<ManagedEditor, &ManagedEditor::OnEditorAssemblyLoaded>(this);

	}

	ManagedEditor::~ManagedEditor()
	{
	}

	void ManagedEditor::Init()
	{
		InvokeManagedMethod("Init");
	}

	void ManagedEditor::Update()
	{
		InvokeManagedMethod("Update");
	}

	void ManagedEditor::LateUpdate()
	{
		InvokeManagedMethod("LateUpdate");
	}

	void ManagedEditor::Render()
	{
		InvokeManagedMethod("Render");
	}

	void ManagedEditor::Exit()
	{
		InvokeManagedMethod("Exit");
	}


	Window* ManagedEditor::GetMainWindow()
	{
		CLRClass* clrClass = GetClass();
		if (clrClass == nullptr)
		{
			LOG_ERROR("Scripts", "Invalid Editor assembly while requesting the main window.");
			return nullptr;
		}

		CLRMethod* method = clrClass->GetMethod("GetMainWindowPtr");
		if (method == nullptr)
		{
			LOG_ERROR("Scripts", "Invalid Editor assembly! Missing GetMainWindowPtr.");
			return nullptr;
		}

		CLRObject* instance = GetOrCreateManagedInstance();
		if (instance == nullptr)
		{
			LOG_ERROR("Scripts", "Failed to create managed editor instance while requesting the main window.");
			return nullptr;
		}

		CLRObject* exception = nullptr;
		CLRObject* result = method->Invoke(instance, nullptr, &exception);
		if (exception)
		{
			CLRException(exception).Log(Log::Severity::Error, SE_TEXT("Editor"));
			return nullptr;
		}
		return (Window*)CLRUtils::Unbox<void*>(result);
    }

    ProjectInfo* ManagedEditor::GetProjectInfo() const { return EditorApp::Ins().Project; }

    void ManagedEditor::CreateDemoScene() 
	{ 
		CreateTestScene();
	}

	void ManagedEditor::InvokeManagedMethod(const char* methodName)
	{
		CLRClass* clrClass = GetClass();
		if (clrClass == nullptr)
		{
			LOG_FATAL("Scripts", "Invalid Editor assembly! Missing class");
			return;
		}

		const auto method = clrClass->GetMethod(methodName);

		if (method == nullptr)
		{
			LOG_FATAL("Scripts", "Invalid Editor assembly! Missing method");
			return;
		}

		CLRObject* instance = GetOrCreateManagedInstance();
		if (instance == nullptr)
		{
			LOG_FATAL("Scripts", "Failed to create editor instance");
			return;
		}

		CLRObject* exception = nullptr;
		method->Invoke(instance, nullptr, &exception);
		if (exception)
		{
			CLRException(exception).Log(Log::Severity::Error, SE_TEXT("Editor"));
		}
	}

	void ManagedEditor::OnEditorAssemblyLoaded(CLRAssembly* assembly)
	{
		ENGINE_ASSERT(!HasManagedInstance());

		// Editor.CSharp.dll has been loaded, let's create managed object for C# editor
		CreateManaged();
	}

}

#include "Engine.h"

#include "Runtime/Core/Profiler/Profiler.h"
#include "Runtime/Core/Platform/FileSystem.h"
#include "Runtime/Core/IniFile.h"
#include "Runtime/Core/Thread/ThreadRegistry.h"
#include "Runtime/Core/Thread/TaskGraph.h"
#include "Runtime/Core/Logging/LoggingSystem.h"
#include "Runtime/Core/Platform/Platform.h"
#include "Runtime/Core/Types/DateTime.h"
#include "Runtime/Core/Systems.h"

#include "EngineContext.h"
#include "Graphics/GPUContext.h"
#include "Graphics/Async/GPUTasksSystem.h"
#include "Utilities/Time.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Render/RenderTask.h"

namespace SE
{
	namespace EngineImpl
	{
		bool IsReady = false;
#if !SE_EDITOR
		bool RunInBackground = false;
#endif
		String CommandLine = String::Empty;
		int32 Fps = 0, FpsAccumulatedFrames = 0;
		double FpsAccumulated = 0.0;

		void InitLog();
		void InitPaths();
		void InitMainWindow(App* application);
		void InitGraphic();
	}

	DateTime Engine::StartupTime;
	bool Engine::HasFocus = false;
	uint64 Engine::UpdateCount = 0;
	uint64 Engine::FrameCount = 0;
	Action Engine::UpdateCall;
	Action Engine::LateUpdateCall;
	App* Engine::pApp = nullptr;
	Threading::TaskGraph* Engine::UpdateGraph = nullptr;
	Action Engine::RenderCall;
/*	Action Engine::Pause;
	Action Engine::Unpause;*/


	int32 Engine::Main(const Char* cmdLine, App* application)
	{
		pApp = application;

		Types::InitTypeSystem();

		// 设置主线程ID
		Threading::SetMainThreadID(Platform::GetCurrentThreadID());

		Systems::Sort();
//		bc7_enc_settings encoderSettings;
		if (Platform::Init())
		{
			Platform::Fatal(SE_TEXT("Cannot init platform."));
			return -1;
		}

		Platform::SetHighDpiAwarenessEnabled(true);
		EngineContext::StartupFolder = EngineContext::BinariesFolder = Platform::GetMainDirectory();

		FileSystem::PathRemoveRelativeParts(EngineContext::StartupFolder);
		FileSystem::NormalizePath(EngineContext::BinariesFolder);

		FileSystem::GetSpecialFolderPath(SpecialFolder::Temporary, EngineContext::TemporaryFolder);
		if (EngineContext::TemporaryFolder.IsEmpty())
		{
			Platform::Fatal(SE_TEXT("Failed to gather temporary folder directory."));
		}
		EngineContext::TemporaryFolder /= UID::New().ToString(UID::FormatType::D);

		EngineImpl::InitPaths();
		EngineImpl::InitLog();

		// Load game info or project info
		const int32 result = application->LoadProduct();
		if (result != 0)
		{
			return result;
		}

		// 启动多线程
		Threading::SetMainThreadID(Platform::GetCurrentThreadID());
		Threading::Init();

		if (!application->Init())
		{
			return -10;
		}

		EngineImpl::InitGraphic();
		EngineImpl::InitMainWindow(application);

		UpdateGraph = New<Threading::TaskGraph>();
		Systems::Initialize();

		Platform::BeforeRun();
		application->BeforeRun();

		Time::Synchronize();
		EngineImpl::IsReady = true;

		// Main engine loop
		const bool useSleep = true; // TODO: this should probably be a platform setting
		while (!EngineContext::IsRequestingExit)
		{
			// Reduce CPU usage by introducing idle time if the engine is running very fast and has enough time to spend
			if ((useSleep && Time::UpdateFPS > Math::ZeroTolerance) || !Platform::GetHasFocus())
			{
				double nextTick = Time::GetNextTick();
				double timeToTick = nextTick - Platform::GetTimeSeconds();

				// Sleep less than needed, some platforms may sleep slightly more than requested
				if (timeToTick > 0.002)
				{
					PROFILE_CPU_NAMED("Idle");
					Platform::Sleep(1);
				}
			}

			// App paused logic
			if (Platform::GetIsPaused())
			{
				OnPause();
				do
				{
					Platform::Sleep(10);
					Platform::Tick();
				} while (Platform::GetIsPaused() && !EngineContext::IsRequestingExit);
				if (EngineContext::IsRequestingExit)
				{
					break;
				}
				OnUnpause();
			}

			// Use the same time for all ticks to improve synchronization
			const double time = Platform::GetTimeSeconds();

			// Update game logic
			if (Time::OnBeginUpdate(time))
			{
				OnUpdate();
				OnLateUpdate();
				Time::OnEndUpdate();
			}
			// Draw frame
			if (Time::OnBeginDraw(time))
			{
				OnRender();
				Time::OnEndRender();
				FrameMark;
			}
		}

		// Call on exit event
		OnExit();

		// Delete temporary directory only if Engine is closing normally (after crash user/developer can restore some data)
		if (FileSystem::DirectoryExists(EngineContext::TemporaryFolder))
		{
			FileSystem::DeleteDirectory(EngineContext::TemporaryFolder);
		}

		return EngineContext::ExitCode;
	}

	void Engine::Exit(int32 exitCode)
	{
		ENGINE_ASSERT(Threading::IsMainThread());

		// Call on exit event
		OnExit();

		// Exit application
		exit(exitCode);
	}

	void Engine::OnUpdate()
	{
		PROFILE_CPU_NAMED("Update");

		UpdateCount++;

		// Update application (will gather data and other platform related events)
		{
			PROFILE_CPU_NAMED("Platform.Tick");
			Platform::Tick();
		}

		const auto mainWindow = pApp->GetMainWindow();

		// Determine if application has focus (flag used by the other parts of the engine)
		HasFocus = (mainWindow && mainWindow->IsFocused()) || Platform::GetHasFocus();

		Threading::MainThreadTask::RunAll(Time::Update.UnscaledDeltaTime.GetTotalSeconds());

		// Update services
		Systems::Update();
		UpdateCall();

		pApp->OnUpdate();

		UpdateGraph->Execute();
	}

	void Engine::OnLateUpdate()
	{
		PROFILE_CPU_NAMED("Late Update");

		// Call event
		LateUpdateCall();
		pApp->OnLateUpdate();

		// Update services
		Systems::LateUpdate();
	}

	void Engine::OnRender()
	{
		PROFILE_CPU_NAMED("Draw");

		// Begin frame rendering
		FrameCount++;
		const double time = Platform::GetTimeSeconds();
		auto device = GPUDevice::instance;
		device->locker.Lock();
		ProfilerGPU::BeginFrame();

		device->Draw([](GPUContext* context){
		  	pApp->OnRender();
			Engine::RenderCall();
			Systems::Render();
		  	RenderTask::RenderAll();
		});

		// End frame rendering
		ProfilerGPU::EndFrame();
		device->locker.Unlock();

		// Calculate FPS
		EngineImpl::FpsAccumulatedFrames++;
		if (time - EngineImpl::FpsAccumulated >= 1.0)
		{
			EngineImpl::Fps = EngineImpl::FpsAccumulatedFrames;
			EngineImpl::FpsAccumulatedFrames = 0;
			EngineImpl::FpsAccumulated = time;
		}
	}
	
	void Engine::OnExit()
	{
		EngineImpl::IsReady = false;

		pApp->BeforeExit();
		Platform::BeforeExit();

		if (GPUDevice::instance)
		{
			// Start disposing
			GPUDevice::instance->GetTasksSystem()->Dispose();
		}

		Systems::ShutDown();

		Threading::Shutdown();
		// Kill all remaining threads
		Threading::ThreadRegistry::KillEmAll();

		ProfilerCPU::Dispose();
		ProfilerGPU::Dispose();

		Log::System::Shutdown();
		Platform::Exit();
	}

	void Engine::OnPause()
	{

	}

	void Engine::OnUnpause()
	{

	}
	bool Engine::ShouldExit()
	{
		return EngineContext::IsRequestingExit;
	}

	void EngineImpl::InitLog()
	{
		// Initialize logger
		Log::System::Initialize();

		// Log platform into
		Platform::LogInfo();
	}

	void EngineImpl::InitPaths()
	{
		// Cache other global paths
		FileSystem::GetSpecialFolderPath(SpecialFolder::LocalAppData, EngineContext::ProductLocalFolder);
		if (EngineContext::ProductLocalFolder.IsEmpty())
		{
			Platform::Fatal(SE_TEXT("Failed to gather local app data folder directory."));
		}

/*#if SE_EDITOR
		EngineContext::EngineContentFolder = EngineContext::StartupFolder / SE_TEXT("Content");
#else
#if USE_MONO
		EngineContext::MonoPath = EngineContext::StartupFolder / SE_TEXT("Mono");
#endif
#endif*/

#if SE_EDITOR
		EngineContext::ProjectFolder = EngineContext::StartupFolder;
		EngineContext::ProjectSourceFolder = EngineContext::ProjectFolder / SE_TEXT("Source");
    	EngineContext::ProjectCacheFolder = EngineContext::ProjectFolder / SE_TEXT("Cache");
#endif

		EngineContext::ProjectContentFolder = EngineContext::ProjectFolder / SE_TEXT("Content");

#if !PLATFORM_SWITCH
		// Setup directories
		if (FileSystem::DirectoryExists(EngineContext::TemporaryFolder))
		{
			FileSystem::DeleteDirectory(EngineContext::TemporaryFolder);
		}
		if (FileSystem::CreateDirectory(EngineContext::TemporaryFolder))
		{
			// Try one more time (Explorer may block it)
			Platform::Sleep(10);
			if (FileSystem::CreateDirectory(EngineContext::TemporaryFolder))
			{
				Platform::Fatal(SE_TEXT("Cannot create temporary directory."));
			}
		}
#endif

#if SE_EDITOR
		if (!FileSystem::DirectoryExists(EngineContext::ProjectContentFolder))
		{
			FileSystem::CreateDirectory(EngineContext::ProjectContentFolder);
		}
		if (!FileSystem::DirectoryExists(EngineContext::ProjectSourceFolder))
		{
			FileSystem::CreateDirectory(EngineContext::ProjectSourceFolder);
		}
/*		if (CommandLine::Options.ClearCache)
		{
			FileSystem::DeleteDirectory(EngineContext::ProjectCacheFolder, true);
		}
		else if (CommandLine::Options.ClearCookerCache)
		{
			FileSystem::DeleteDirectory(EngineContext::ProjectCacheFolder / SE_TEXT("Cooker"), true);
		}*/
		if (!FileSystem::DirectoryExists(EngineContext::ProjectCacheFolder))
		{
			FileSystem::CreateDirectory(EngineContext::ProjectCacheFolder);
		}
#endif

		// Setup current working directory to the project root
		Platform::SetWorkingDirectory(EngineContext::ProjectFolder);
	}

	void EngineImpl::InitMainWindow(App* application)
	{
#if PLATFORM_HAS_HEADLESS_MODE
		// Try to use headless mode
/*		if (CommandLine::Options.Headless.IsTrue())
		{
			LOG_INFO("Engine", "Running in headless mode.");
			return;
		}*/
#endif
		PROFILE_CPU_NAMED("Engine::InitMainWindow");

		// Create window
		Window* window = application->CreateMainWindow();
		if (!window)
		{
			Platform::Fatal(SE_TEXT("No main window created."));
			return;
		}

		window->Show();

		Window::SetMainWindow(window);
	}

	void EngineImpl::InitGraphic()
	{
		ENGINE_ASSERT(GPUDevice::instance == nullptr);

		// Create and initialize graphics device
		// Log::Logger::WriteFloor();
		LOG_INFO("Graphic", "Creating Graphics Device...");


		GPUGlobalSettings globalSettings;
		globalSettings.validationMode = RHIValidationMode::Disabled;
		if (!GPUDevice::Create(globalSettings))
		{
			LOG_FATAL("Graphic", "Cannot create rendering device.");
		}

		GPUDevice* device = GPUDevice::instance;

		LOG_INFO("Graphic",
			"Graphics Device created! Adapter: \'{0}\', Renderer: {1}, Shader Profile: {2}, Feature Level: {3}",
			SE_TEXT(""), /*device->GetDescription(),*/
			SE_TEXT(""),/*::ToString(device->GetRendererType()),*/
			ToString(device->GetShaderProfile()),
			ToString(device->GetFeatureLevel())
		);



		// Log::Logger::WriteFloor();
	}

}
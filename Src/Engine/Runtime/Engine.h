#pragma once

#include "Core/CoreModule.h"
#include "Core/Types/Delegate.h"

#include "App.h"
//-------------------------------------------------------------------------

namespace SE
{
	namespace Threading
	{
		class TaskGraph;
	}

	class SE_API_RUNTIME Engine
    {
	public:
		/// <summary>
		/// The engine start time (local time).
		/// </summary>
		static DateTime StartupTime;

		/// <summary>
		/// True if app has focus (one of the windows is being focused).
		/// </summary>
		static bool HasFocus;

		/// <summary>
		/// Gets the current update counter since the start of the game.
		/// </summary>
		static uint64 UpdateCount;

		/// <summary>
		/// Gets the current frame (drawing) count since the start of the game.
		/// </summary>
		static uint64 FrameCount;

	public:

		/// <summary>
		/// Event called on engine update.
		/// </summary>
		static Action UpdateCall;

		/// <summary>
		/// Event called after engine update.
		/// </summary>
		static Action LateUpdateCall;

		/// <summary>
		/// Event called during frame rendering and can be used to invoke custom rendering with GPUDevice.
		/// </summary>
		static Action RenderCall;

    public:
		static App* pApp;
		static Threading::TaskGraph* UpdateGraph;
	public:

		/// <summary>
		/// The main engine function (must be called from platform specific entry point).
		/// </summary>
		/// <param name="cmdLine">The input application command line arguments.</param>
		/// <returns>The application exit code.</returns>
		static int32 Main(const Char* cmdLine, App* application);

		/// <summary>
		/// Exits the engine.
		/// </summary>
		/// <param name="exitCode">The exit code.</param>
		static void Exit(int32 exitCode = -1);

		static bool ShouldExit();

	public:
		/// <summary>
		/// Updates game and all engine services.
		/// </summary>
		static void OnUpdate();

		/// <summary>
		/// Late update callback.
		/// </summary>
		static void OnLateUpdate();

		/// <summary>
		/// Draw callback.
		/// </summary>
		static void OnRender();

		/// <summary>
		/// Called when engine exists. Disposes engine services and shuts down the engine.
		/// </summary>
		static void OnExit();

	private:
		static void OnPause();
		static void OnUnpause();

    protected:
        Function<bool(String const&)>                   m_fatalErrorHandler;

        // Modules
        //-------------------------------------------------------------------------
        CoreModule                                      m_CoreModule;

        // Application data
        //-------------------------------------------------------------------------
//        ResPath                                    m_startupMap;
        bool                                       m_exitRequested = false;
    };
}
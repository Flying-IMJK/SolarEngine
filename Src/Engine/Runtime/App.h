#pragma once

#include "Runtime/Graphics/GraphicWindow.h"
#include "Runtime/API.h"

namespace SE
{
	struct RenderContext;

	class SE_API_RUNTIME App
	{
	public:
		virtual ~App() = default;

		/// <summary>
		/// Loads the application info.
		/// </summary>
		/// <returns>The error code or 0 if succeed.</returns>
		virtual int32 LoadProduct() = 0;

		/// <summary>
		/// Creates the main window of the application.
		/// </summary>
		/// <returns>The main window (null if failed).</returns>
		virtual GraphicWindow* CreateMainWindow() = 0;

		virtual GraphicWindow* GetMainWindow() = 0;

		/// <summary>
		/// Initializes the application. Called after initialization of all engine services.
		/// </summary>
		/// <returns>True if failed, otherwise false.</returns>
		virtual bool Init() = 0;

		/// <summary>
		/// Called just before main engine loop start after full engine initialization.
		/// </summary>
		virtual void BeforeRun() = 0;

		/// <summary>
		/// Called just before engine shutdown.
		/// </summary>
		virtual void BeforeExit() = 0;


		virtual void OnUpdate() {};

		virtual void OnLateUpdate() {};

		virtual void OnRender() {};
	};
}
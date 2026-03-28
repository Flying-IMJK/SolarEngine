#pragma once

#include "EditorModule.h"

namespace SE::Editor
{
	class MainMenu;

	class ToolbarBarModule : public EditorModule
	{
	public:
		/// <summary>
		/// The main menu control.
		/// </summary>
		MainMenu* MainMenu;
	};

} // SE

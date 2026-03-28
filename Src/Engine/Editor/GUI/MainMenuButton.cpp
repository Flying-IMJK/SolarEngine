
#include "MainMenuButton.h"

#include "MainMenu.h"
#include "ContextMenu/ContextMenu.h"
#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/Style.h"

namespace SE::Editor
{

	MainMenuButton::MainMenuButton(String text) : Control(0, 0, 32, 16)
	{
		ContextMenu = New<::SE::Editor::ContextMenu>();

		Text = text;

		Style* style = Style::Current;
#if PLATFORM_WINDOWS
		if (true)
		{
			BackgroundColorMouseOver = style->BackgroundHighlighted;
			BackgroundColorMouseOverOpened = style->Background;
		}
		else
#endif
		{
			BackgroundColorMouseOver = BackgroundColorMouseOverOpened = style->LightBackground * 1.3f;
		}
	}

	void MainMenuButton::Draw()
	{
		// Cache data
		Style* style = Style::Current;
		Rectangle clientRect = Rectangle(0, 0, Width, Height);
		bool hasChildItems = ContextMenu->HasChildren();
		bool isOpened = ContextMenu->IsOpened;
		bool enabled = EnabledInHierarchy();

		// Draw background
		if (enabled && hasChildItems && (isOpened || IsMouseOver))
		{
			Render2D::FillRectangle(clientRect, isOpened ? BackgroundColorMouseOverOpened : BackgroundColorMouseOver);
		}

		// Draw text
		Render2D::RenderText(style->FontMedium, Text, clientRect, enabled && hasChildItems ? style->Foreground : style->ForegroundDisabled,
			TextAlignment::Center, TextAlignment::Center);
	}

	bool MainMenuButton::OnMouseDown(Float2 location, MouseButton button)
	{
		Focus();

		MainMenu* mainMenu = TypeTryCast<MainMenu>(Parent);
		if (mainMenu != nullptr)
			mainMenu->Selected = this;

		return true;
	}

	void MainMenuButton::OnMouseEnter(Float2 location)
	{
		Control::OnMouseEnter(location);

		MainMenu* mainMenu = TypeTryCast<MainMenu>(Parent);
		if (mainMenu != nullptr && mainMenu->Selected != nullptr)
			mainMenu->Selected = this;
	}

	void MainMenuButton::PerformLayout(bool force)
	{
		Style* style = Style::Current;
		float width = 18;

		if (style->FontMedium)
			width += style->FontMedium->MeasureText(Text).x;

		Width = width;
	}
} // SE
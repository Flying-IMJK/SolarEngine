
#include "MainMenu.h"

#include "MainMenuButton.h"
#include "ContextMenu/ContextMenu.h"
#include "Editor/EditorAssets.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/GraphicWindow.h"
#include "Runtime/Render/2D/FontAsset.h"
#include "Runtime/Render/Assets/Texture/Texture.h"
#include "Runtime/UI/GUI/RootControl.h"
#include "Runtime/UI/GUI/Style.h"
#include "Runtime/UI/GUI/WindowRootControl.h"
#include "Runtime/UI/GUI/Brushes/TextureBrush.h"
#include "Runtime/UI/GUI/Common/Button.h"
#include "Runtime/UI/GUI/Common/Image.h"
#include "Runtime/UI/GUI/Common/Label.h"

namespace SE::Editor
{
	void MainMenu::OnSelectedContextMenuVisibleChanged(Control* control)
	{
		if (_selected != nullptr)
			Selected = nullptr;
	}
	
	MainMenuButton* MainMenu::GetRightButton()
	{
		MainMenuButton* b = nullptr;
		for (Control* control : Children())
		{
			bool isMenuButton = TypeAs<MainMenuButton>(control);
			if (b == nullptr && isMenuButton)
				b = static_cast<MainMenuButton*>(control);

			if (isMenuButton && b != nullptr && control->Right > b->Right)
				b = static_cast<MainMenuButton*>(control);
		}
		return b;
	}

#if PLATFORM_WINDOWS
	void MainMenu::OnWindowClosed()
	{
		if (_window != nullptr)
		{
			_window->HitTestEvent.UnbindAll();
			_window = nullptr;
		}
	}
	WindowHitCodes MainMenu::OnHitTest(Float2 mouse)
	{
		float dpiScale = _window->GetDpiScale();

		if (_window->IsMinimized())
			return WindowHitCodes::NoWhere;

		if (!_window->IsMaximized())
		{
			Float2 pos = _window->ScreenToClient(mouse * dpiScale); // pos is not DPI adjusted
			Float2 winSize = _window->GetSize();

			// Distance from which the mouse is considered to be on the border/corner
			float distance = 5.0f * dpiScale;

			if (pos.y > winSize.y - distance && pos.x < distance)
				return WindowHitCodes::BottomLeft;

			if (pos.x > winSize.x - distance && pos.y > winSize.y - distance)
				return WindowHitCodes::BottomRight;

			if (pos.y < distance && pos.x < distance)
				return WindowHitCodes::TopLeft;

			if (pos.y < distance && pos.x > winSize.x - distance)
				return WindowHitCodes::TopRight;

			if (pos.x > winSize.x - distance)
				return WindowHitCodes::Right;

			if (pos.x < distance)
				return WindowHitCodes::Left;

			if (pos.y < distance)
				return WindowHitCodes::Top;

			if (pos.y > winSize.y - distance)
				return WindowHitCodes::Bottom;
		}

		Float2 mousePos = PointFromScreen(mouse * dpiScale);
		Control* controlUnderMouse = GetChildAt(mousePos);
		bool isMouseOverSth = controlUnderMouse != nullptr && controlUnderMouse != _title;

		MainMenuButton* rb = GetRightButton();
		if (rb == nullptr)
		{
			Float2 upperLeft = _title->UpperLeft;
			Rectangle rectangle = Rectangle(upperLeft, {_minimizeButton->BottomLeft.operator->().x - upperLeft.x, _title->Height} );
			if (rectangle.Contains(mousePos) && !isMouseOverSth)
			{
				return WindowHitCodes::Caption;
			}
		}
		else if (_minimizeButton != nullptr &&
			Rectangle(rb->UpperRight, _minimizeButton->BottomLeft.operator->() - rb->UpperRight).Contains(mousePos) && !isMouseOverSth)
		{
			return WindowHitCodes::Caption;
		}

		/*if (_minimizeButton->Bounds.operator->().Contains(mousePos))
		{
			return WindowHitCodes::Client;
		}

		if (_maximizeButton->Bounds.operator->().Contains(mousePos))
		{
			return WindowHitCodes::Client;
		}

		if (_closeButton->Bounds.operator->().Contains(mousePos))
		{
			return WindowHitCodes::Client;
		}*/

		return WindowHitCodes::Client;
	}
#endif

	MainMenu::MainMenu(RootControl* mainWindow) : ContainerControl(0, 0, 0, 20)
	{
		AutoFocus = false;
		AnchorPreset = AnchorPresets::HorizontalStretchTop;;

#if PLATFORM_WINDOWS
		_useCustomWindowSystem = true;
		if (_useCustomWindowSystem)
		{
			BackgroundColor = Style::Current->LightBackground;
			Height = 28;

			Texture* windowIcon = AssetContent::LoadAsyncInternal<Texture>(EditorAssets::WindowIcon);
			FontAsset* windowIconsFont = AssetContent::LoadAsyncInternal<FontAsset>(EditorAssets::WindowIconsFont);

			windowIcon->WaitForLoaded();
			windowIconsFont->WaitForLoaded();

			Font* iconFont = windowIconsFont->CreateFont(9);
			m_hitTestCall = CreateFunc([this](const Float2& mousePosition, WindowHitCodes& result, bool& handled)
			{
				Float2 pos = mousePosition / _window->GetDpiScale();
				result = OnHitTest(pos);
				handled = true;
			});


			_window = mainWindow->RootWindow()->Window();
			_window->HitTestEvent.BindUnique(m_hitTestCall);
			_window->ClosedEvent.BindUnique<MainMenu, &MainMenu::OnWindowClosed>(this);

			// ScriptsBuilder.GetBinariesConfiguration(out _, out _, out _, out var configuration);

			_icon = New<Image>();
			_icon->Margin = Margin(6, 6, 6, 6),
			_icon->Brush = New<TextureBrush>(windowIcon),
			_icon->Color = Style::Current->Foreground,
			_icon->KeepAspectRatio = false,
			// _icon->TooltipText = String::Format(SE_TEXT("{0}\nVersion {1}\nConfiguration {3}\nGraphics {2}"), _window->GetTitle(), /*Globals.EngineVersion*/ "0.1", GPUDevice::instance->GetRendererType(), /*configuration*/ ""),
			_icon->Parent = this,

			_title = New<Label>(0, 0, Width, Height);
			_title->Text = _window->GetTitle();
			_title->HorizontalAlignment = TextAlignment::Center;
			_title->VerticalAlignment = TextAlignment::Center;
			_title->ClipText = true;
			_title->TextColor = Style::Current->ForegroundGrey;
			_title->TextColorHighlighted = Style::Current->ForegroundGrey;
			_title->Parent = this;

			_closeButton = New<Button>(0, 0, 46, Height);
			_closeButton->Text = EditorAssets::SegMDL2Icons::ChromeClose;
			_closeButton->Font = iconFont;
			_closeButton->TextColor = Style::Current->Foreground;
			_closeButton->BackgroundColor = Colors::Transparent;
			_closeButton->BackgroundColorHighlighted = Colors::Red;
			_closeButton->BackgroundColorSelected = Colors::Red.RGBMultiplied(1.3f);
			_closeButton->BorderColor = Colors::Transparent;
			_closeButton->BorderColorHighlighted = Colors::Transparent;
			_closeButton->BorderColorSelected = Colors::Transparent;
			_closeButton->Parent = this;
			_closeButton->Clicked.BindUnique([this]()
			{
				_window->Close(ClosingReason::User);
			});

			_minimizeButton = New<Button>(0, 0, 46, Height);
			_minimizeButton->Text = EditorAssets::SegMDL2Icons::ChromeMinimize;
			_minimizeButton->Font = iconFont;
			_minimizeButton->TextColor = Style::Current->Foreground;
			_minimizeButton->BackgroundColor = Colors::Transparent;
			_minimizeButton->BackgroundColorHighlighted = Style::Current->LightBackground.RGBMultiplied(0.8f);
			_minimizeButton->BorderColor = Colors::Transparent;
			_minimizeButton->BorderColorHighlighted = Colors::Transparent;
			_minimizeButton->BorderColorSelected = Colors::Transparent;
			_minimizeButton->Parent = this;
			_minimizeButton->Clicked.BindUnique([this]()
			{
				_window->Minimize();
			});

			_maximizeButton = New<Button>(0, 0, 46, Height);
			_maximizeButton->Text = _window->IsMaximized() ? EditorAssets::SegMDL2Icons::ChromeRestore : EditorAssets::SegMDL2Icons::ChromeMaximize;
			_maximizeButton->TextColor = Style::Current->Foreground;
			_maximizeButton->Font = iconFont;
			_maximizeButton->BackgroundColor = Colors::Transparent;
			_maximizeButton->BackgroundColorHighlighted = Style::Current->LightBackground.RGBMultiplied(0.8f);
			_maximizeButton->BorderColor = Colors::Transparent;
			_maximizeButton->BorderColorHighlighted = Colors::Transparent;
			_maximizeButton->BorderColorSelected = Colors::Transparent;
			_maximizeButton->Parent = this;
			_maximizeButton->Clicked.BindUnique([this]{
				if (_window->IsMaximized())
					_window->Restore();
				else
					_window->Maximize();
			});
		}
		else
#endif
		{
			BackgroundColor = Style::Current->LightBackground;
		}
	}
	

#if PLATFORM_WINDOWS
	void MainMenu::Update(float deltaTime)
	{
		ContainerControl::Update(deltaTime);

		if (_maximizeButton != nullptr)
		{
			_maximizeButton->Text = _window->IsMaximized() ? EditorAssets::SegMDL2Icons::ChromeRestore : EditorAssets::SegMDL2Icons::ChromeMaximize;
		}
	}
#endif

	MainMenuButton* MainMenu::AddButton(String text)
	{
		return AddChild(New<MainMenuButton>(text));
	}

	MainMenuButton* MainMenu::GetButton(String text)
	{
		MainMenuButton* result = nullptr;
		for (int i = 0; i < ChildrenCount(); i++)
		{
			Control* control = Children()[i];
			MainMenuButton* button = TypeTryCast<MainMenuButton>(control);
			if (button != nullptr && button->Text == text)
			{
				result = button;
				break;
			}
		}
		return result;
	}

	bool MainMenu::OnMouseDoubleClick(Float2 location, MouseButton button)
	{
		if (ContainerControl::OnMouseDoubleClick(location, button))
			return true;

#if PLATFORM_WINDOWS
		Control* child = GetChildAtRecursive(location);
		if (_useCustomWindowSystem && !TypeAs<Button>(child) && !TypeAs<MainMenuButton>(child))
		{
			if (_window->IsMaximized())
				_window->Restore();
			else
				_window->Maximize();
		}
#endif

		return true;
	}

	bool MainMenu::OnKeyDown(KeyboardKeys key)
	{
		if (ContainerControl::OnKeyDown(key))
			return true;

		// Fallback to the edit window for shortcuts
		// var editor = Editor.Instance;
		return false;// editor.Windows.EditWin.InputActions.Process(editor, this, key);
	}

#if PLATFORM_WINDOWS
	void MainMenu::OnDestroy()
	{
		ContainerControl::OnDestroy();

		if (_window != nullptr)
		{
			_window->ClosedEvent.BindUnique<MainMenu, &MainMenu::OnWindowClosed>(this);
			OnWindowClosed();
		}
	}
#endif

	void MainMenu::PerformLayoutAfterChildren()
	{
		float x = 0;

#if PLATFORM_WINDOWS
		if (_useCustomWindowSystem)
		{
			// Icon
			_icon->X = x;
			_icon->Size = Float2(Height);
			x += _icon->Width;
		}
#endif

		// Arrange controls
		MainMenuButton* rightMostButton = nullptr;
		for (int i = 0; i < m_Children.Count(); i++)
		{
			Control* c = m_Children[i];
			MainMenuButton* b = TypeTryCast<MainMenuButton>(c);
			if (b != nullptr && c->Visible)
			{
				b->Bounds = Rectangle(x, 0, b->Width, Height);

				if (rightMostButton == nullptr)
					rightMostButton = b;
				else if (rightMostButton->X < b->X)
					rightMostButton = b;

				x += b->Width;
			}
		}

#if PLATFORM_WINDOWS
		if (_useCustomWindowSystem)
		{
			// Buttons
			float width = _closeButton->Width;
			_closeButton->Height = Height;
			_closeButton->X = Width - width;
			_maximizeButton->Height = Height;
			_maximizeButton->X = _closeButton->X - _maximizeButton->Width;
			_minimizeButton->Height = Height;
			_minimizeButton->X = (_maximizeButton->X - _minimizeButton->Width);

			// Title
			_title->Bounds = Rectangle(x + 2, 0, _minimizeButton->Left - x - 4, Height);
			_title->Text = /*_title->Width < 300.0f ? Editor.Instance.ProjectInfo.Name : */_window->GetTitle();
		}
#endif
	}

	void MainMenu::__SetSelected(MainMenuButton* value)
	{
		if (_selected == value)
			return;

		if (_selected != nullptr)
		{
			_selected->ContextMenu->VisibleChanged.Unbind<MainMenu, &MainMenu::OnSelectedContextMenuVisibleChanged>(this);
			_selected->ContextMenu->Hide();
		}

		_selected = value;

		if (_selected != nullptr && _selected->ContextMenu->HasChildren())
		{
			_selected->ContextMenu->Show(_selected, Float2(0, _selected->Height));
			_selected->ContextMenu->VisibleChanged.BindUnique<MainMenu, &MainMenu::OnSelectedContextMenuVisibleChanged>(this);
		}
	}
} // SE
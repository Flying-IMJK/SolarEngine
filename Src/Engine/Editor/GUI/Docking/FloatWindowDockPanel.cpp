
#include "FloatWindowDockPanel.h"

#include "DockHintWindow.h"
#include "DockWindow.h"
#include "MasterDockPanel.h"
#include "Runtime/Graphics/GraphicWindow.h"
#include "Runtime/UI/GUI/WindowRootControl.h"

namespace SE::Editor
{
	void FloatWindowDockPanel::OnLeftButtonHit(WindowHitCodes hitTest, bool& result)
	{
		if (hitTest == WindowHitCodes::Caption)
		{
			BeginDrag();
			result = true;
		}
		else
		{
			result = false;
		}
	}

	void FloatWindowDockPanel::OnClosing(ClosingReason reason, bool& cancel)
	{
		// Close all docked windows
		while (Tabs.operator->().Count() > 0)
		{
			if (!Tabs.operator->()[0]->Close(reason))
			{
				// Cancel
				cancel = true;
				return;
			}
		}

		// Unlink
		_window->Window()->ClosingEvent.Unbind(_ClosingCall);;
		_window->Window()->LeftButtonHitEvent.UnbindAll();
		_window = nullptr;

		// Remove object
		ENGINE_ASSERT(TabsCount == 0 && ChildPanelsCount == 0);
		Dispose();
	}

	FloatWindowDockPanel::FloatWindowDockPanel(MasterDockPanel* masterPanel, RootControl* window) : DockPanel(nullptr)
	{
		_ClosingCall.Bind<FloatWindowDockPanel, &FloatWindowDockPanel::OnClosing>(this);
		_LeftButtonHitCall.Bind<FloatWindowDockPanel, &FloatWindowDockPanel::OnLeftButtonHit>(this);
		_masterPanel = masterPanel;
		_window = static_cast<WindowRootControl*>(window);

		// Link
		_masterPanel->FloatingPanels.Add(this);
		Parent = window;
		_window->Window()->ClosingEvent.BindUnique(_ClosingCall);;
		_window->Window()->LeftButtonHitEvent.BindUnique(_LeftButtonHitCall);
	}

	void FloatWindowDockPanel::BeginDrag()
	{
		// Filter invalid events
		if (_window == nullptr)
			return;

		// Create docking hint window
		DockHintWindow::Create(this);
	}

	GraphicWindow* FloatWindowDockPanel::CreateFloatWindow(RootControl* parent, Float2 location, Float2 size, WindowStartPosition startPosition, String title)
	{
		// Setup initial window settings
		CreateWindowSettings settings =  DefaultWindowSettings();
		WindowRootControl* t;
		if (TypeTryCast<WindowRootControl>(parent, t))
		{
			settings.Parent = t->Window();
		}
		settings.Parent = nullptr;
		settings.Title = title;
		settings.Size = size;
		settings.Position = location;
		settings.MinimumSize = Float2(1);
		settings.MaximumSize = Float2::Zero; // Unlimited size
		settings.Fullscreen = false;
		settings.HasBorder = true;
		settings.SupportsTransparency = false;
		settings.ActivateWhenFirstShown = true;
		settings.AllowInput = true;
		settings.AllowMinimize = true;
		settings.AllowMaximize = true;
		settings.AllowDragAndDrop = true;
		settings.IsTopmost = false;
		settings.IsRegularWindow = true;
		settings.HasSizingFrame = true;
		settings.ShowAfterFirstPaint = false;
		settings.ShowInTaskbar = true;
		settings.StartPosition = startPosition;

		// Create window
		return New<GraphicWindow>(settings);
	}

	DockState FloatWindowDockPanel::TryGetDockState(float& splitterValue)
	{
		splitterValue = 0.5f;
		return DockState::Float;
	}

	void FloatWindowDockPanel::OnDestroy()
	{
		if (_masterPanel != nullptr)
		{
			_masterPanel->FloatingPanels.Remove(this);
		}

		DockPanel::OnDestroy();
	}

	void FloatWindowDockPanel::OnLastTabRemoved()
	{
		// Close window
		if (_window != nullptr)
		{
			_window->Close();
		}
	}

	void FloatWindowDockPanel::OnSelectedTabChanged()
	{
		DockPanel::OnSelectedTabChanged();

		if (_window != nullptr && SelectedTab != nullptr)
			_window->SetTitle(SelectedTab->Title.operator->());
	}
} // SE

#include "MasterDockPanel.h"

#include "DockPanelProxy.h"
#include "DockWindow.h"
#include "FloatWindowDockPanel.h"

namespace SE::Editor
{
	void MasterDockPanel::ResetLayout()
	{
		// Close all windows
		for (int i = 0; i < Windows.Count(); i++)
			Windows[i]->Close();

		// Ensure that has no docked windows
		List<DockPanel*>& childPanels = ChildPanels;
		for (int i = 0; i < childPanels.Count(); i++)
			childPanels[i]->Dispose();

		// Delete remaining controls (except tabs proxy)
		if (TabsProxy != nullptr)
			TabsProxy->Parent = nullptr;
		DisposeChildren();
		if (TabsProxy != nullptr)
			TabsProxy->Parent = this;
	}
	
	DockPanel* MasterDockPanel::HitTest(Float2& position, FloatWindowDockPanel* excluded)
	{
		// Check all floating windows
		// TODO: gather windows order and take it into account when performing test
		for (int i = 0; i < FloatingPanels.Count(); i++)
		{
			FloatWindowDockPanel* win = FloatingPanels[i];
			if (win->Visible && win != excluded)
			{
				DockPanel* result = win->HitTest(position);
				if (result != nullptr)
					return result;
			}
		}

		// Base
		return DockPanel::HitTest(position);
	}
	
	void MasterDockPanel::LinkWindow(::SE::Editor::DockWindow* window)
	{
		// Add to the windows list
		Windows.Add(window);
	}
	
	void MasterDockPanel::UnlinkWindow(Editor::DockWindow* window)
	{
		// Call event to the window
		window->OnUnlink();

		// Prevent this action during disposing (we don't want to modify Windows list)
		if (IsDisposing)
			return;

		// Remove from the windows list
		Windows.Remove(window);
	}

	void MasterDockPanel::OnDestroy()
	{
		DockPanel::OnDestroy();

		Windows.Clear();
		FloatingPanels.Clear();
	}
	
	DockState MasterDockPanel::TryGetDockState(float& splitterValue)
	{
		splitterValue = 0.5f;
		return DockState::DockFill;
	}
	
	bool MasterDockPanel::__GetIsMaster()
	{
		return true;
	}
	
	int MasterDockPanel::__GetVisibleWindowsCount()
	{
		int result = 0;
		for (int i = 0; i < Windows.Count(); i++)
		{
			if (Windows[i]->Visible)
				result++;
		}
		return result;
	}
} // SE
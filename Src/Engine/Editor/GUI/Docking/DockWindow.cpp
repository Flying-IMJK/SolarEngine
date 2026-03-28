

#include "DockWindow.h"

#include "FloatWindowDockPanel.h"
#include "MasterDockPanel.h"
#include "Runtime/Graphics/GraphicWindow.h"
#include "Runtime/UI/GUI/Style.h"
#include "Runtime/UI/GUI/WindowRootControl.h"

namespace SE::Editor
{
	void DockWindow::OnUnlink()
	{
		_masterPanel = nullptr;
	}
	void DockWindow::Undock()
	{
		// Defocus itself
		if (ContainsFocus())
		{
			Focus();
		}
		Defocus();

		// Call undock
		if (_dockedTo != nullptr)
		{
			_dockedTo->UndockWindowInternal(this);
			ENGINE_ASSERT(_dockedTo == nullptr);
		}
	}

	bool DockWindow::OnClosing(ClosingReason reason)
	{
		// Allow
		return false;
	}

	void DockWindow::PerformLayoutBeforeChildren()
	{
		// Cache window title dimensions
		if (_titleSize.x <= 0)
		{
			Style* style = Style::Current;
			if (style != nullptr && style->FontMedium != nullptr)
				_titleSize = style->FontMedium->MeasureText(_title);
		}

		Panel::PerformLayoutBeforeChildren();
	}

	DockWindow::DockWindow(MasterDockPanel* masterPanel, bool hideOnClose, ScrollBars scrollBars) : Panel(scrollBars)
	{
		_masterPanel = masterPanel;
		HideOnClose = hideOnClose;
		AnchorPreset = AnchorPresets::StretchAll;
		Offsets = Margin::Zero;

		// Bind navigation shortcuts
		/*InputActions.Add(options => options.CloseTab, () => Close(ClosingReason::User));
		InputActions.Add(options => options.PreviousTab, () =>
		{
			if (_dockedTo != null)
			{
				var index = _dockedTo.SelectedTabIndex;
				index = index == 0 ? _dockedTo.TabsCount - 1 : index - 1;
				_dockedTo.SelectedTabIndex = index;
			}
		});
		InputActions.Add(options => options.NextTab, () =>
		{
			if (_dockedTo != null)
			{
				var index = _dockedTo.SelectedTabIndex;
				index = (index + 1) % _dockedTo.TabsCount;
				_dockedTo.SelectedTabIndex = index;
			}
		});*/

		// Link to the master panel
		if (_masterPanel != nullptr)
		{
			_masterPanel->LinkWindow(this);
		}
	}

	void DockWindow::ShowFloating(Float2 location, Float2 size, WindowStartPosition position)
	{
		Undock();

		// Create window
		Float2 winSize = size.LengthSquared() > 4 ? size : DefaultSize;
		GraphicWindow* window = FloatWindowDockPanel::CreateFloatWindow(_masterPanel->Root, location, winSize, position, _title);
		WindowRootControl* windowGUI = window->GetGUI();

		// Create dock panel for the window
		FloatWindowDockPanel* dockPanel = New<FloatWindowDockPanel>(_masterPanel, windowGUI);
		dockPanel->DockWindowInternal(DockState::DockFill, this);

		// Perform layout
		Visible = true;
		windowGUI->UnlockChildrenRecursive();
		windowGUI->PerformLayout();

		// Show
		window->Show();
		window->BringToFront();
		window->Focus();
		OnShow();

		// Perform layout again
		windowGUI->PerformLayout();
	}

	void DockWindow::Show(DockState state, DockPanel* toDock, bool autoSelect, float splitterValue)
	{
		if (state == DockState::Hidden)
		{
			Hide();
		}
		else if (state == DockState::Float)
		{
			ShowFloating();
		}
		else
		{
			Visible = true;

			// Undock first
			Undock();

			// Then dock
			if (toDock != nullptr)
			{
				toDock->DockWindowInternal(state, this, autoSelect, splitterValue);
			}else
			{
				_masterPanel->DockWindowInternal(state, this, autoSelect, splitterValue);
			}

			OnShow();
			PerformLayout();
		}
	}

	void DockWindow::Show(DockState state, DockWindow* toDock)
	{
		DockPanel* panel = nullptr;
		if (toDock != nullptr)
		{
			panel = toDock->ParentDockPanel;
		}

		Show(state, panel);
	}

	void DockWindow::FocusOrShow()
	{
		FocusOrShow((DockState)DockState::DockFill);
	}

	void DockWindow::FocusOrShow(DockState state)
	{
		if (Visible)
		{
			SelectTab();
			Focus();
		}
		else
		{
			Show(state);
		}
	}

	void DockWindow::Hide()
	{
		// Undock
		Undock();

		Visible = false;

		// Ensure that dock panel has no parent
		ENGINE_ASSERT(HasParent() == true);
	}

	bool DockWindow::Close(ClosingReason reason)
	{
		// Fire events
		if (!OnClosing(reason))
			return false;
		OnClose();

		// Check if window should be hidden on close event
		if (HideOnClose)
		{
			// Hide
			Hide();
		}
		else
		{
			// Undock
			Undock();

			// Delete itself
			Dispose();
		}

		// Done
		return true;
	}

	void DockWindow::SelectTab(bool autoFocus)
	{
		if (_dockedTo != nullptr)
		{
			_dockedTo->SelectTab(this, autoFocus);
		}
	}

	void DockWindow::BringToFront()
	{
		if (_dockedTo != nullptr && _dockedTo->RootWindow() != nullptr)
		{
			_dockedTo->RootWindow()->BringToFront();
		}
	}

	void DockWindow::OnDestroy()
	{
		// Auto undock from non-disposing parent (user wants to remove only the dock window)
		if (HasParent() && !Parent->IsDisposing)
			Undock();

		// Unlink from the master panel
		if (_masterPanel != nullptr)
		{
			_masterPanel->UnlinkWindow(this);
		}

		Panel::OnDestroy();
	}

	void DockWindow::Focus()
	{
		Panel::Focus();

		SelectTab();
		BringToFront();
	}

	bool DockWindow::OnKeyDown(KeyboardKeys key)
	{
		// Base
		if (Panel::OnKeyDown(key))
			return true;

		// Custom input events
		return false;// InputActions.Process(Editor.Instance, this, key);
	}

	bool DockWindow::__GetIsSelected()
	{
		if (_dockedTo != nullptr)
		{
			return _dockedTo->SelectedTab == this;
		}
		return false;
	}

	String DockWindow::__GetSerializationTypename()
	{
		return SE_TEXT("::") + GetTypeInfo()->friendlyName;
	}

	void DockWindow::__SetTitle(String& value)
	{
		_title = value;

		// Invalidate cached title size
		_titleSize = Float2(-1);
		PerformLayoutBeforeChildren();


		// Check if is docked to the floating window and is selected so update window title
		if (IsSelected)
		{
			FloatWindowDockPanel* floatPanel;
			if (TypeTryCast<FloatWindowDockPanel>(this, floatPanel))
			{
				floatPanel->Window->SetTitle(Title.operator->());
			}
		}
	}
} // SE
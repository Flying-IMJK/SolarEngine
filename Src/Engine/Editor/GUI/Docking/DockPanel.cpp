
#include "DockPanel.h"

#include "DockPanelProxy.h"
#include "DockWindow.h"
#include "Runtime/UI/GUI/RootControl.h"
#include "Runtime/UI/GUI/Panels/SplitPanel.h"

namespace SE::Editor
{
	DockPanel::DockPanel(DockPanel* parentPanel)
	{
		AutoFocus = false;

		_parentPanel = parentPanel;
		if (_parentPanel != nullptr)
		{
			_parentPanel->_childPanels.Add(this);
		}

		AnchorPreset = AnchorPresets::StretchAll;
		Offsets = Margin::Zero;
	}

	bool DockPanel::CloseAll(ClosingReason reason)
	{
		while (_tabs.Count() > 0)
		{
			if (!_tabs[0]->Close(reason))
				return false;
		}

		return true;
	}

	DockWindow* DockPanel::GetTab(int tabIndex)
	{
		return _tabs[tabIndex];
	}

	int DockPanel::GetTabIndex(Editor::DockWindow* tab)
	{
		return _tabs.Find(tab);
	}

	bool DockPanel::ContainsTab(Editor::DockWindow* tab)
	{
		return _tabs.Contains(tab);
	}

	void DockPanel::SelectTab(int tabIndex)
	{
		::SE::Editor::DockWindow* tab = nullptr;
		if (tabIndex >= 0 && _tabs.Count() > tabIndex && _tabs.Count() > 0)
			tab = _tabs[tabIndex];
		SelectTab(tab);
	}

	void DockPanel::SelectTab(Editor::DockWindow* tab, bool autoFocus)
	{
		// Check if tab will change
		if (_selectedTab != tab)
		{
			// Change
			ContainerControl* proxy;
			if (_selectedTab != nullptr)
			{
				proxy = _selectedTab->Parent;
				_selectedTab->Parent = nullptr;
			}
			else
			{
				CreateTabsProxy();
				proxy = _tabsProxy;
			}
			_selectedTab = tab;
			if (_selectedTab != nullptr)
			{
				_selectedTab->UnlockChildrenRecursive();
				_selectedTab->Parent = proxy;
				if (autoFocus)
					_selectedTab->Focus();
			}
			OnSelectedTabChanged();
		}
		else if (autoFocus && _selectedTab != nullptr && !_selectedTab->ContainsFocus())
		{
			_selectedTab->Focus();
		}
	}

	DockPanel* DockPanel::HitTest(Float2& position)
	{
		// Get parent window and transform point position into local coordinates system
		RootControl* parentWin = Root;
		if (parentWin == nullptr)
			return nullptr;
		auto clientPos = parentWin->PointFromScreen(position);
		auto localPos = PointFromWindow(clientPos);

		// Early out
		if (localPos.x < 0 || localPos.y < 0)
			return nullptr;
		if (localPos.x > Width || localPos.y > Height)
			return nullptr;

		// Test all docked controls (find the smallest one)
		float sizeLengthSquared = Max_float;
		DockPanel* result = nullptr;
		for (int i = 0; i < _childPanels.Count(); i++)
		{
			auto panel = _childPanels[i]->HitTest(position);
			if (panel != nullptr)
			{
				auto sizeLen = panel->Size.operator->().LengthSquared();
				if (sizeLen < sizeLengthSquared)
				{
					sizeLengthSquared = sizeLen;
					result = panel;
				}
			}
		}
		if (result != nullptr)
			return result;

		// Itself
		return this;
	}

	DockState DockPanel::TryGetDockState(float& splitterValue)
	{
		DockState result = DockState::Unknown;
		splitterValue = DefaultSplitterValue;

		if (HasParent())
		{
			SplitPanel* splitter;
			if (TypeTryCast<SplitPanel>(Parent->Parent, splitter))
			{
				splitterValue = splitter->SplitterValue;
				if (Parent == splitter->Panel1)
				{
					if (splitter->Orientation == Orientation::Horizontal)
						result = DockState::DockLeft;
					else
						result = DockState::DockTop;
				}
				else
				{
					if (splitter->Orientation == Orientation::Horizontal)
						result = DockState::DockRight;
					else
						result = DockState::DockBottom;
					splitterValue = 1 - splitterValue;
				}
			}
		}

		return result;
	}

	DockPanel* DockPanel::CreateChildPanel(DockState state, float splitterValue)
	{
		CreateTabsProxy();

		// Create child dock panel
		DockPanel* dockPanel = New<DockPanel>(this);

		// Switch dock mode
		Control* c1 = nullptr;
		Control* c2 = nullptr;
		Orientation o;
		switch (state)
		{
		case DockState::DockTop:
		{
			o = Orientation::Vertical;
			c1 = dockPanel;
			c2 = _tabsProxy;
			break;
		}
		case DockState::DockBottom:
		{
			splitterValue = 1 - splitterValue;
			o = Orientation::Vertical;
			c1 = _tabsProxy;
			c2 = dockPanel;
			break;
		}
		case DockState::DockLeft:
		{
			o = Orientation::Horizontal;
			c1 = dockPanel;
			c2 = _tabsProxy;
			break;
		}
		case DockState::DockRight:
		{
			splitterValue = 1 - splitterValue;
			o = Orientation::Horizontal;
			c1 = _tabsProxy;
			c2 = dockPanel;
			break;
		}
		}

		// Create splitter and link controls
		ContainerControl* parent = _tabsProxy->Parent;
		SplitPanel* splitter = New<SplitPanel>(o, ScrollBars::None, ScrollBars::None);
		splitter->AnchorPreset = AnchorPresets::StretchAll;
		splitter->Offsets = Margin::Zero;
		splitter->SplitterValue = splitterValue;

		splitter->Panel1->AddChild(c1);
		splitter->Panel2->AddChild(c2);
		parent->AddChild(splitter);

		// Update
		splitter->UnlockChildrenRecursive();
		splitter->PerformLayout();

		return dockPanel;
	}

	void DockPanel::MoveTabLeft(int index)
	{
		if (index > 0)
		{
			auto tab = _tabs[index];
			_tabs.RemoveAt(index);
			_tabs.Insert(index - 1, tab);
		}
	}
	void DockPanel::MoveTabRight(int index)
	{
		if (index < _tabs.Count() - 2)
		{
			auto tab = _tabs[index];
			_tabs.RemoveAt(index);
			_tabs.Insert(index + 1, tab);
		}
	}

	void DockPanel::OnDestroy()
	{
		if (_parentPanel != nullptr)
		{
			_parentPanel->_childPanels.Remove(this);
		}

		ContainerControl::OnDestroy();

		// Clear other tabs (not in the view)
		for (int i = 0; i < _tabs.Count(); i++)
		{
			if (!_tabs[i]->IsDisposing)
			{
				_tabs[i]->Dispose();
			}
		}
	}

	void DockPanel::OnLastTabRemoved()
	{
		// Check if dock panel is linked to the split panel
		if (HasParent())
		{
			SplitPanel* splitter;
			if (TypeTryCast<SplitPanel>(Parent->Parent, splitter))
			{
				// Check if there is another nested dock panel inside this dock panel and extract it here
				auto childPanels = _childPanels;
				if (childPanels.Count() != 0)
				{
					// Move tabs from child panels into this one
					Editor::DockWindow* selectedTab = nullptr;
					for (DockPanel* childPanel : childPanels)
					{
						List<Editor::DockWindow*>& childPanelTabs = childPanel->Tabs;
						for (int i = 0; i < childPanelTabs.Count(); i++)
						{
							Editor::DockWindow* childPanelTab = childPanelTabs[i];
							if (selectedTab == nullptr && childPanelTab->IsSelected)
								selectedTab = childPanelTab;
							childPanel->UndockWindow(childPanelTab);
							AddTab(childPanelTab, false);
						}
					}
					if (selectedTab != nullptr)
						SelectTab(selectedTab);
				}
				else
				{
					// Unlink splitter
					ContainerControl* splitterParent = splitter->Parent;
					ENGINE_ASSERT(splitterParent != nullptr);
					splitter->Parent = nullptr;

					// Move controls from second split panel to the split panel parent
					Panel* scrPanel = Parent == splitter->Panel2 ? splitter->Panel1 : splitter->Panel2;
					int srcPanelChildrenCount = scrPanel->ChildrenCount();
					for (int i = srcPanelChildrenCount - 1; i >= 0 && scrPanel->ChildrenCount() > 0; i--)
					{
						scrPanel->GetChild(i)->Parent = splitterParent;
					}

					ENGINE_ASSERT(scrPanel->ChildrenCount() == 0);
					ENGINE_ASSERT(splitterParent->ChildrenCount() == srcPanelChildrenCount);

					// Delete
					splitter->Dispose();
					Delete(splitter);
				}
			}
			else if (!IsMaster)
			{
				ENGINE_UNREACHABLE_CODE()
			}
		}
		else if (!IsMaster)
		{
			ENGINE_UNREACHABLE_CODE()
		}
	}

	void DockPanel::DockWindow(DockState state, Editor::DockWindow* window, bool autoSelect, float splitterValue)
	{
		CreateTabsProxy();

		// Check if dock like a tab or not
		if (state == DockState::DockFill)
		{
			// Add tab
			AddTab(window, autoSelect);
		}
		else
		{
			// Create child panel
			auto dockPanel = CreateChildPanel(state, splitterValue != 0.0 ? splitterValue : DefaultSplitterValue);

			// Dock window as a tab in a child panel
			dockPanel->DockWindow(DockState::DockFill, window);
		}
	}

	void DockPanel::UndockWindow(::SE::Editor::DockWindow* window)
	{
		int index = GetTabIndex(window);
		ENGINE_ASSERT(index >= 0);

		// Check if tab was selected
		if (window == SelectedTab)
		{
			// Change selection
			if (index == 0 && TabsCount > 1)
				SelectTab(1);
			else
				SelectTab(index - 1);
		}

		// Undock
		_tabs.RemoveAt(index);
		window->ParentDockPanel = nullptr;

		// Check if has no more tabs
		if (_tabs.Count() == 0)
		{
			OnLastTabRemoved();
		}
		else
		{
			// Update
			PerformLayout();
		}
	}

	void DockPanel::AddTab(Editor::DockWindow* window, bool autoSelect)
	{
		_tabs.Add(window);
		window->ParentDockPanel = this;
		if (autoSelect)
			SelectTab(window);
	}

	void DockPanel::CreateTabsProxy()
	{
		if (_tabsProxy == nullptr)
		{
			// Create proxy and make set simple full dock
			_tabsProxy = New<DockPanelProxy>(this);
			_tabsProxy->Parent = this;

			_tabsProxy->UnlockChildrenRecursive();
		}
	}

	int DockPanel::__GetSelectedTabIndex()
	{
		return _tabs.Find(_selectedTab);
	}

	Rectangle DockPanel::__GetDockAreaBounds()
	{
		RootControl* parentWin = Root;
		ENGINE_ASSERT_M(parentWin != nullptr, SE_TEXT("Missing parent window."));

		Control* control = _tabsProxy != nullptr ? static_cast<Control*>(_tabsProxy) : this;
		Float2 clientPos = control->PointToWindow(Float2::Zero);
		return Rectangle(parentWin->PointToScreen(clientPos), Float2(GetDpiScale()) * control->Size);
	}
} // SE

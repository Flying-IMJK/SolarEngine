#pragma once
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/UI/GUI/ContainerControl.h"

namespace SE::Editor
{
    class DockWindow;
    class DockPanelProxy;

    /// <summary>
    /// Dockable window mode.
    /// </summary>
    enum class DockState
    {
        /// <summary>
        /// The unknown state.
        /// </summary>
        Unknown = 0,

        /// <summary>
        /// The floating window.
        /// </summary>
        Float = 1,

        //DockTopAutoHide = 2,
        //DockLeftAutoHide = 3,
        //DockBottomAutoHide = 4,
        //DockRightAutoHide = 5,

        /// <summary>
        /// The dock fill as a tab.
        /// </summary>
        DockFill = 6,

        /// <summary>
        /// The dock top.
        /// </summary>
        DockTop = 7,

        /// <summary>
        /// The dock left.
        /// </summary>
        DockLeft = 8,

        /// <summary>
        /// The dock bottom.
        /// </summary>
        DockBottom = 9,

        /// <summary>
        /// The dock right.
        /// </summary>
        DockRight = 10,

        /// <summary>
        /// The hidden state.
        /// </summary>
        Hidden = 11
    };

    /// <summary>
    /// Dockable panel control.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    SE_CLASS(Reflect)
    class DockPanel : public ContainerControl
    {
        SE_DEFINE_CLASS_DEFAULT(DockPanel, ContainerControl)
    public:
        /// <summary>
        /// The default dock tabs header height.
        /// </summary>
        static constexpr float DefaultHeaderHeight = 25;

        /// <summary>
        /// The default tabs header text left margin.
        /// </summary>
        static constexpr float DefaultTextMargin = 2;

        /// <summary>
        /// The default tabs header buttons size.
        /// </summary>
        static constexpr float DefaultButtonsSize = 12;

        /// <summary>
        /// The default tabs header buttons margin.
        /// </summary>
        static constexpr float DefaultButtonsMargin = 4;

        /// <summary>
        /// The default splitters value.
        /// </summary>
        static constexpr float DefaultSplitterValue = 0.25f;

    private:
        DockPanel* _parentPanel;
        List<DockPanel*> _childPanels;
        List<DockWindow*> _tabs;
        DockWindow* _selectedTab;
        DockPanelProxy* _tabsProxy;

    public:
        /// <summary>
        /// Returns true if this panel is a master panel.
        /// </summary>
        PRO_GET(IsMaster, DockPanel, bool, __GetIsMaster);

        /// <summary>
        /// Returns true if this panel is a floating window panel.
        /// </summary>
        PRO_GET(IsFloating, DockPanel, bool, __GetIsFloating);

        /// <summary>
        /// Gets docking area bounds (tabs rectangle) in a screen space.
        /// </summary>
        PRO_GET(DockAreaBounds, DockPanel, Rectangle, __GetDockAreaBounds);


        /// <summary>
        /// Gets the child panels.
        /// </summary>
        PRO_GET(ChildPanels, DockPanel, List<DockPanel*>&, __GetChildPanels);

        /// <summary>
        /// Gets the child panels count.
        /// </summary>
        PRO_GET(ChildPanelsCount, DockPanel, int, __GetChildPanelsCount);

        /// <summary>
        /// Gets the tabs.
        /// </summary>
        PRO_GET(Tabs, DockPanel, List<DockWindow*>&, __GetTabs);

        /// <summary>
        /// Gets amount of the tabs in a dock panel.
        /// </summary>
        PRO_GET(TabsCount, DockPanel, int, __GetTabsCount);

        /// <summary>
        /// Gets or sets the index of the selected tab.
        /// </summary>
        PRO(SelectedTabIndex, DockPanel, int, __GetSelectedTabIndex, __SetSelectedTabIndex);

        /// <summary>
        /// Gets the selected tab.
        /// </summary>
        PRO_GET(SelectedTab, DockPanel, DockWindow*, __GetSelectedTab);

        /// <summary>
        /// Gets the first tab.
        /// </summary>
        PRO_GET(FirstTab, DockPanel, DockWindow*, __GetFirstTab);

        /// <summary>
        /// Gets the last tab.
        /// </summary>
        PRO_GET(LastTab, DockPanel, DockWindow*, __GetLastTab);

        /// <summary>
        /// Gets the parent panel.
        /// </summary>
        PRO_GET(ParentDockPanel, DockPanel, DockPanel*, __GetParentDockPanel);

        /// <summary>
        /// Gets the tabs header proxy.
        /// </summary>
        PRO_GET(TabsProxy, DockPanel, DockPanelProxy*, __GetTabsProxy);

        /// <summary>
        /// Initializes a new instance of the <see cref="DockPanel"/> class.
        /// </summary>
        /// <param name="parentPanel">The parent panel.</param>
        DockPanel(DockPanel* parentPanel);

        /// <summary>
        /// Closes all the windows.
        /// </summary>
        /// <param name="reason">Window closing reason.</param>
        /// <returns>True if action has been cancelled (due to window internal logic).</returns>
        bool CloseAll(ClosingReason reason = ClosingReason::CloseEvent);

        /// <summary>
        /// Gets tab at the given index.
        /// </summary>
        /// <param name="tabIndex">The index of the tab page.</param>
        /// <returns>The tab.</returns>
        DockWindow* GetTab(int tabIndex);

        /// <summary>
        /// Gets tab at the given index.
        /// </summary>
        /// <param name="tab">The tab page.</param>
        /// <returns>The index of the given tab.</returns>
        int GetTabIndex(DockWindow* tab);

        /// <summary>
        /// Determines whether panel contains the specified tab.
        /// </summary>
        /// <param name="tab">The tab.</param>
        /// <returns><c>true</c> if panel contains the specified tab; otherwise, <c>false</c>.</returns>
        bool ContainsTab(DockWindow* tab);

        /// <summary>
        /// Selects the tab page.
        /// </summary>
        /// <param name="tabIndex">The index of the tab page to select.</param>
        void SelectTab(int tabIndex);

        /// <summary>
        /// Selects the tab page.
        /// </summary>
        /// <param name="tab">The tab page to select.</param>
        /// <param name="autoFocus">True if focus tab after selection change.</param>
        void SelectTab(DockWindow* tab, bool autoFocus = true);

        /// <summary>
        /// Performs hit test over dock panel
        /// </summary>
        /// <param name="position">Screen space position to test</param>
        /// <returns>Dock panel that has been hit or null if nothing found</returns>
        DockPanel* HitTest(Float2& position);

        /// <summary>
        /// Try get panel dock state
        /// </summary>
        /// <param name="splitterValue">Splitter value</param>
        /// <returns>Dock State</returns>
        virtual DockState TryGetDockState(float &splitterValue);

        /// <summary>
        /// Create child dock panel
        /// </summary>
        /// <param name="state">Dock panel state</param>
        /// <param name="splitterValue">Initial splitter value</param>
        /// <returns>Child panel</returns>
        DockPanel* CreateChildPanel(DockState state, float splitterValue);

        virtual void DockWindowInternal(DockState state, DockWindow* window, bool autoSelect = true, float splitterValue = 0)
        {
            DockWindow(state, window, autoSelect, splitterValue);
        }

        void RemoveIt()
        {
            OnLastTabRemoved();
        }

        void UndockWindowInternal(DockWindow* window)
        {
            UndockWindow(window);
        }

        void MoveTabLeft(int index);

        void MoveTabRight(int index);

        /// <inheritdoc />
        void OnDestroy() override;

    protected:
        /// <summary>
        /// Called when last tab gets removed.
        /// </summary>
        virtual void OnLastTabRemoved();

        /// <summary>
        /// Docks the window.
        /// </summary>
        /// <param name="state">The state.</param>
        /// <param name="window">The window.</param>
        /// <param name="autoSelect">Whether or not to automatically select the window after docking it.</param>
        /// <param name="splitterValue">The splitter value to use when docking to window.</param>
        virtual void DockWindow(DockState state, DockWindow* window, bool autoSelect = true, float splitterValue = 0);

        /// <summary>
        /// Undocks the window.
        /// </summary>
        /// <param name="window">The window.</param>
        virtual void UndockWindow(::SE::Editor::DockWindow* window);

        /// <summary>
        /// Adds the tab.
        /// </summary>
        /// <param name="window">The window to insert as a tab.</param>
        /// <param name="autoSelect">True if auto-select newly added tab.</param>
        virtual void AddTab(::SE::Editor::DockWindow* window, bool autoSelect = true);

        /// <summary>
        /// Called when selected tab gets changed.
        /// </summary>
        virtual void OnSelectedTabChanged()
        {
        }

        virtual bool __GetIsMaster() { return false; }
        virtual bool __GetIsFloating() { return false; }

    private:
        void CreateTabsProxy();

        DockPanelProxy* __GetTabsProxy() { return _tabsProxy; }

        DockPanel* __GetParentDockPanel() { return _parentPanel; }

        int __GetSelectedTabIndex();
        void __SetSelectedTabIndex(int value) { return SelectTab(value); }

        int __GetTabsCount() { return _tabs.Count(); }

        List<::SE::Editor::DockWindow*>& __GetTabs() { return _tabs; }

        List<DockPanel*>& __GetChildPanels() { return _childPanels; };
        int __GetChildPanelsCount() { return _childPanels.Count(); }

        ::SE::Editor::DockWindow* __GetSelectedTab(){ return _selectedTab; }
        ::SE::Editor::DockWindow* __GetFirstTab(){ return _tabs.Count() > 0 ? _tabs[0] : nullptr; }
        ::SE::Editor::DockWindow* __GetLastTab(){ return _tabs.Count() > 0 ? _tabs[_tabs.Count() - 1] : nullptr;}

        Rectangle __GetDockAreaBounds();

    };

} // SE


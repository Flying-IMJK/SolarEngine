#pragma once

#include "DockPanel.h"

namespace SE::Editor
{

    class DockWindow;
    class FloatWindowDockPanel;

    /// <summary>
    /// Master Dock Panel control used as a root control for dockable windows workspace.
    /// </summary>
    SE_CLASS(Reflect)
    class MasterDockPanel : public DockPanel
    {
        SE_DEFINE_CLASS_DEFAULT(MasterDockPanel, DockPanel)
    public:
        /// <summary>
        /// Array with all created dock windows for that master panel.
        /// </summary>
        List<::SE::Editor::DockWindow*> Windows = List<::SE::Editor::DockWindow*>(32);

        /// <summary>
        /// Array with all floating windows for that master panel.
        /// </summary>
        List<FloatWindowDockPanel*> FloatingPanels = List<FloatWindowDockPanel*>(4);

        /// <summary>
        /// Gets the visible windows count.
        /// </summary>
        /// <value>
        /// The visible windows count.
        /// </value>
        PRO_GET(VisibleWindowsCount, MasterDockPanel, int, __GetVisibleWindowsCount);

        /// <summary>
        /// Resets windows layout.
        /// </summary>
        void ResetLayout();

        /// <summary>
        /// Performs hit test over dock panel.
        /// </summary>
        /// <param name="position">Screen space position to test.</param>
        /// <param name="excluded">Floating window to omit during searching (and all docked to that one).</param>
        /// <returns>Dock panel that has been hit or null if nothing found.</returns>
        DockPanel* HitTest(Float2 &position, FloatWindowDockPanel* excluded);


        void LinkWindow(Editor::DockWindow* window);

        void UnlinkWindow(Editor::DockWindow* window);


        /// <inheritdoc />
        void OnDestroy() override;

        /// <inheritdoc />
        DockState TryGetDockState(float& splitterValue) override;

    protected:
        bool __GetIsMaster() override;

    private:
        int __GetVisibleWindowsCount();
    };

} // SE


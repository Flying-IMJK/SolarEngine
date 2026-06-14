#pragma once
#include "DockPanel.h"

namespace SE
{
    class GraphicWindow;
}

namespace SE::Editor
{
    class MasterDockPanel;

    /// <summary>
    /// Floating Window Dock Panel control.
    /// </summary>
    /// <seealso cref="DockPanel" />
    class FloatWindowDockPanel : public DockPanel
    {
        SE_DEFINE_CLASS_DEFAULT(FloatWindowDockPanel, DockPanel)
    private:
        MasterDockPanel* _masterPanel;
        WindowRootControl* _window;
        Function<void(ClosingReason, bool&)> _ClosingCall;
        Function<void(WindowHitCodes, bool& result)> _LeftButtonHitCall;

        void OnLeftButtonHit(WindowHitCodes hitTest, bool& result);

        void OnClosing(ClosingReason reason, bool &cancel);

    public:
        /// <summary>
        /// Gets the master panel.
        /// </summary>
        PRO_GET(MasterPanel, FloatWindowDockPanel, MasterDockPanel*, __GetMasterPanel);

        /// <summary>
        /// Gets the window.
        /// </summary>
        PRO_GET(Window, FloatWindowDockPanel, WindowRootControl*, __GetWindow);

        /// <summary>
        /// Initializes a new instance of the <see cref="FloatWindowDockPanel"/> class.
        /// </summary>
        /// <param name="masterPanel">The master panel.</param>
        /// <param name="window">The window.</param>
        FloatWindowDockPanel(MasterDockPanel* masterPanel, RootControl* window);

        /// <summary>
        /// Begin drag operation on the window
        /// </summary>
        void BeginDrag();

        /// <summary>
        /// Creates a floating window.
        /// </summary>
        /// <param name="parent">Parent window handle.</param>
        /// <param name="location">Client area location.</param>
        /// <param name="size">Window client area size.</param>
        /// <param name="startPosition">Window start position.</param>
        /// <param name="title">Initial window title.</param>
        static GraphicWindow* CreateFloatWindow(RootControl* parent, Float2 location, Float2 size, WindowStartPosition startPosition, String title);


        /// <inheritdoc />
        DockState TryGetDockState(float &splitterValue) override;

        /// <inheritdoc />
        void OnDestroy() override;

    protected:
        /// <inheritdoc />
        void OnLastTabRemoved() override;

        /// <inheritdoc />
        void OnSelectedTabChanged() override;

    private:
        MasterDockPanel* __GetMasterPanel() { return _masterPanel; }
        WindowRootControl* __GetWindow() { return _window; }

        /// <inheritdoc />
        bool __GetIsFloating() override { return true; }

    };

} // SE

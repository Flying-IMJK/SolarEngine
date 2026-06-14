#pragma once
#include "Runtime/UI/GUI/ContainerControl.h"

namespace SE::Editor
{
    class ContextMenuButton;
    class DockWindow;
    class DockPanel;
    
    /// <summary>
    /// Proxy control used for docking <see cref="DockWindow*"/> inside <see cref="DockPanel"/>.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    class DockPanelProxy : public ContainerControl
    {
        SE_DEFINE_CLASS_DEFAULT(DockPanelProxy, ContainerControl)
    public:
        /// <summary>
        /// The is mouse down flag (left button).
        /// </summary>
        bool IsMouseLeftButtonDown = false;

        /// <summary>
        /// The is mouse down flag (right button).
        /// </summary>
        bool IsMouseRightButtonDown = false;

        /// <summary>
        /// The is mouse down flag (middle button).
        /// </summary>
        bool IsMouseMiddleButtonDown = false;

        /// <summary>
        /// The is mouse down over cross button flag.
        /// </summary>
        bool IsMouseDownOverCross = false;

        /// <summary>
        /// The mouse down window.
        /// </summary>
        DockWindow* MouseDownWindow = nullptr;

        /// <summary>
        /// The mouse position.
        /// </summary>
        Float2 MousePosition = Float2::Minimum;

        /// <summary>
        /// The start drag asynchronous window.
        /// </summary>
        DockWindow* StartDragAsyncWindow = nullptr;

        /// <inheritdoc />
        void Draw() override;

        /// <inheritdoc />
        void OnLostFocus() override;
        
        /// <inheritdoc />
        void OnMouseEnter(Float2 location) override;

        /// <inheritdoc />
        bool OnMouseDoubleClick(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        bool OnMouseDown(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        bool OnMouseUp(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        void OnMouseMove(Float2 location) override;

        /// <inheritdoc />
        void OnMouseLeave() override;

        /// <inheritdoc />
        DragDropEffect OnDragEnter(const Float2& location, DragData* data) override;

        /// <inheritdoc />
        DragDropEffect OnDragMove(const Float2& location, DragData* data) override;

        /// <inheritdoc />
        void OnDragLeave() override;

        /// <inheritdoc />
        Rectangle GetDesireClientArea() override;

        /// <summary>
        /// Initializes a new instance of the <see cref="DockPanelProxy"/> class.
        /// </summary>
        /// <param name="panel">The panel.</param>
        DockPanelProxy(DockPanel* panel);

    private:
        DockPanel* _panel = nullptr;
        double _dragEnterTime = -1;

        Rectangle GetHeaderRectangle();

        DockWindow* GetTabAtPos(Float2 position, bool &closeButton);

        Rectangle GetTabRect(DockWindow* win);

        void StartDrag(DockWindow* win);

        void StartDragAsync();

        DragDropEffect TrySelectTabUnderLocation(const Float2& location);

        void ShowContextMenu(DockWindow* tab, Float2 &location);

        void OnTabMenuCloseClicked(ContextMenuButton* button);

        void OnTabMenuCloseAllClicked(ContextMenuButton* button);

        void OnTabMenuCloseAllButThisClicked(ContextMenuButton* button);

        void OnTabMenuCloseAllToTheRightClicked(ContextMenuButton* button);

        void OnTabMenuUndockClicked(ContextMenuButton* button);

        void OnTabMenuMaximizeClicked(ContextMenuButton* button);

        void OnTabMenuRestoreClicked(ContextMenuButton* button);
    };
} // SE

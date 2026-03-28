#pragma once

#include "Core/Math/Rectangle.h"
#include "Core/Types/Delegate.h"

namespace SE
{
    class GraphicWindow;
    enum class MouseButton;
}

namespace SE::Editor
{
    class DockWindow;
    class FloatWindowDockPanel;
    class DockPanel;
    enum class DockState;

    /// <summary>
    /// Helper class used to handle docking windows dragging and docking.
    /// </summary>
    class DockHintWindow
    {
    private:
        FloatWindowDockPanel* _toMove;

        Float2 _dragOffset;
        Float2 _defaultWindowSize;
        Rectangle _rectDock;
        Rectangle _rectWindow;
        Float2 _mouse;
        DockState _toSet;
        DockPanel* _toDock;
        bool _lateDragOffsetUpdate;

        Rectangle _rLeft, _rRight, _rBottom, _rUpper, _rCenter;

        Function<void(const Float2&, MouseButton)> _OnMouseUpCall;
        Function<void(const Float2&)> _OnMouseMoveCall;
        Function<void()> _OnLostFocusCall;


        void CalculateDragOffset(Float2 mouseScreenPosition);

        void UpdateRects();

        void OnMouseUp(const Float2 &location, MouseButton button);

        void OnMouseMove(const Float2 &mousePos);

        void OnLostFocus();

    public:

        DockHintWindow(FloatWindowDockPanel* toMove);

        /// <summary>
        /// Releases unmanaged and - optionally - managed resources.
        /// </summary>
        void Dispose();

        /// <summary>
        /// Creates the new dragging hit window.
        /// </summary>
        /// <param name="toMove">Floating dock panel to move.</param>
        /// <returns>The dock hint window object.</returns>
        static DockHintWindow* Create(FloatWindowDockPanel* toMove);

        /// <summary>
        /// Creates the new dragging hit window.
        /// </summary>
        /// <param name="toMove">Dock window to move.</param>
        /// <returns>The dock hint window object.</returns>
        static DockHintWindow* Create(DockWindow* toMove);

        /// <summary>
        /// Calculates window rectangle in the dock window.
        /// </summary>
        /// <param name="state">Window dock state.</param>
        /// <param name="rect">Dock panel rectangle.</param>
        /// <returns>Calculated window rectangle.</returns>
        static Rectangle CalculateDockRect(DockState state, Rectangle &rect);

        /// <summary>
        /// Contains helper proxy windows shared across docking panels. They are used to visualize docking window locations.
        /// </summary>
        class Proxy
        {
        public:
            /// <summary>
            /// The drag proxy window.
            /// </summary>
            static GraphicWindow* Window;

            /// <summary>
            /// The left hint proxy window.
            /// </summary>
            static GraphicWindow* Left;

            /// <summary>
            /// The right hint proxy window.
            /// </summary>
            static GraphicWindow* Right;

            /// <summary>
            /// The up hint proxy window.
            /// </summary>
            static GraphicWindow* Up;

            /// <summary>
            /// The down hint proxy window.
            /// </summary>
            static GraphicWindow* Down;

            /// <summary>
            /// The center hint proxy window.
            /// </summary>
            static GraphicWindow* Center;

            /// <summary>
            /// The hint windows size.
            /// </summary>
            constexpr static float HintWindowsSize = 32.0f;

            /// <summary>
            /// Initializes the hit proxy windows. Those windows are used to indicate drag target areas (left, right, top, bottom, etc.).
            /// </summary>
            static void InitHitProxy();

            /// <summary>
            /// Initializes the hint window.
            /// </summary>
            /// <param name="initSize">Initial size of the proxy window.</param>
            static void Init(Float2 &initSize);

            /// <summary>
            /// Hides proxy windows.
            /// </summary>
            static void Hide();

            /// <summary>
            /// Releases proxy data and windows.
            /// </summary>
            static void Dispose();

        private:
            static void CreateProxy(GraphicWindow* &win, StringView name);

            static void HideProxy(GraphicWindow* win);

            static void DisposeProxy(GraphicWindow* win);
        };
    };
} // SE


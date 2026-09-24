using System;
using SE.GUI;

namespace SE.Editor.GUI
{
    public sealed class DockHintWindow
    {
        private const float BorderMargin = 4.0f;
        private readonly FloatWindowDockPanel m_ToMove;
        private Float2 m_DragOffset;
        private Float2 m_DefaultWindowSize;
        private bool m_LateDragOffsetUpdate;
        private Rectangle m_DockRect;
        private Rectangle m_WindowRect;
        private Rectangle m_UpperRect;
        private Rectangle m_BottomRect;
        private Rectangle m_LeftRect;
        private Rectangle m_RightRect;
        private Rectangle m_CenterRect;

        private DockHintWindow(FloatWindowDockPanel toMove, Float2? initialMouseScreen = null)
        {
            m_ToMove = toMove ?? throw new ArgumentNullException(nameof(toMove));
            TargetState = DockState.Float;
            SE.Window? host = toMove.HostWindow;
            if (host == null)
                throw new InvalidOperationException("Floating dock panel has no host window.");

            m_ToMove.Focus();
            m_ToMove.Defocus();
            host.Focus();
            if (host.IsMaximized)
            {
                Float2 localMouse = host.MousePosition;
                Float2 restoredMouseScreen = host.ClientToScreen(localMouse);
                Float2 previousSize = host.Size;
                host.Restore();
                Float2 restoredSize = host.Size;
                host.Position = new Float2(
                    restoredMouseScreen.X - localMouse.X * restoredSize.X / Math.Max(previousSize.X, 1.0f),
                    restoredMouseScreen.Y - localMouse.Y * restoredSize.Y / Math.Max(previousSize.Y, 1.0f));
            }

            // Floating panels sample after focus, matching the native window mouse query.
            Float2 mouseScreen = initialMouseScreen ?? GetMouseScreenPosition(host);
            if (mouseScreen != Float2.Zero)
                CalculateDragOffset(mouseScreen);
            else
                m_LateDragOffsetUpdate = true;

            // The native implementation keeps the restored size for the whole drag operation.
            m_DefaultWindowSize = host.Size;
            Proxy.Init(m_DefaultWindowSize, host.DpiScale);
            Active = this;
            Proxy.Window?.StartTrackingMouse(false);
            Proxy.Window?.GUI.PerformLayout();
            Update(mouseScreen);
            host.Hide();
            Proxy.Show();
        }

        public static DockHintWindow? Active { get; private set; }
        public FloatWindowDockPanel ToMove => m_ToMove;
        public DockPanel? TargetPanel { get; private set; }
        public DockState TargetState { get; private set; }
        public Rectangle PreviewBounds => m_WindowRect;
        public bool IsDisposed { get; private set; }

        public static DockHintWindow Create(FloatWindowDockPanel toMove)
        {
            if (toMove == null)
                throw new ArgumentNullException(nameof(toMove));
            DockHintWindow hint = new DockHintWindow(toMove);
            return hint;
        }

        public static DockHintWindow Create(DockWindow toMove)
        {
            if (toMove == null)
            {
                throw new ArgumentNullException(nameof(toMove));
            }

            Float2 mouseScreen = toMove.Root is WindowRootControl sourceRoot
                ? GetMouseScreenPosition(sourceRoot.Window)
                : Float2.Zero;
            toMove.ShowFloating();
            FloatWindowDockPanel? floating = toMove.ParentDockPanel as FloatWindowDockPanel;
            if (floating == null)
            {
                throw new InvalidOperationException("The window did not create a floating dock panel.");
            }
            if (floating.HostWindow != null)
            {
                SE.Window host = floating.HostWindow;
                if (mouseScreen == Float2.Zero)
                    mouseScreen = GetMouseScreenPosition(host);
                host.Position = mouseScreen - new Float2(8.0f);
            }
            return new DockHintWindow(floating, mouseScreen);
        }

        public static Rectangle CalculateDockRect(DockState state, Rectangle rect)
        {
            return state switch
            {
                DockState.DockFill => new Rectangle(rect.X, rect.Y + DockPanel.DefaultHeaderHeight,
                    rect.Width, Math.Max(0.0f, rect.Height - DockPanel.DefaultHeaderHeight)),
                DockState.DockTop => new Rectangle(rect.X, rect.Y, rect.Width, rect.Height * DockPanel.DefaultSplitterValue),
                DockState.DockBottom => new Rectangle(rect.X, rect.Y + rect.Height * (1.0f - DockPanel.DefaultSplitterValue),
                    rect.Width, rect.Height * DockPanel.DefaultSplitterValue),
                DockState.DockLeft => new Rectangle(rect.X, rect.Y, rect.Width * DockPanel.DefaultSplitterValue, rect.Height),
                DockState.DockRight => new Rectangle(rect.X + rect.Width * (1.0f - DockPanel.DefaultSplitterValue), rect.Y,
                    rect.Width * DockPanel.DefaultSplitterValue, rect.Height),
                _ => rect,
            };
        }

        public void Update(Float2 mouseScreenPosition)
        {
            if (IsDisposed)
                return;

            TargetPanel = m_ToMove.MasterPanel.HitTest(mouseScreenPosition, m_ToMove);
            TargetState = DockState.Float;
            if (TargetPanel != null)
            {
                m_DockRect = TargetPanel.DockAreaBounds;
                float hintSize = Proxy.HintWindowsSize * (TargetPanel.Root?.DpiScale ?? 1.0f);
                Float2 center = m_DockRect.Location + m_DockRect.Size * 0.5f;
                m_UpperRect = new Rectangle(center.X - hintSize * 0.5f, m_DockRect.Y + BorderMargin, hintSize, hintSize);
                m_BottomRect = new Rectangle(center.X - hintSize * 0.5f, m_DockRect.Bottom - hintSize - BorderMargin, hintSize, hintSize);
                m_LeftRect = new Rectangle(m_DockRect.X + BorderMargin, center.Y - hintSize * 0.5f, hintSize, hintSize);
                m_RightRect = new Rectangle(m_DockRect.Right - hintSize - BorderMargin, center.Y - hintSize * 0.5f, hintSize, hintSize);
                m_CenterRect = new Rectangle(center.X - hintSize * 0.5f, center.Y - hintSize * 0.5f, hintSize, hintSize);

                if (m_ToMove.ChildPanelsCount == 0)
                {
                    if (m_UpperRect.Contains(mouseScreenPosition))
                        TargetState = DockState.DockTop;
                    else if (m_BottomRect.Contains(mouseScreenPosition))
                        TargetState = DockState.DockBottom;
                    else if (m_LeftRect.Contains(mouseScreenPosition))
                        TargetState = DockState.DockLeft;
                    else if (m_RightRect.Contains(mouseScreenPosition))
                        TargetState = DockState.DockRight;
                    if (m_CenterRect.Contains(mouseScreenPosition))
                        TargetState = DockState.DockFill;
                }
            }

            m_WindowRect = TargetState == DockState.Float
                ? new Rectangle(mouseScreenPosition - m_DragOffset, m_DefaultWindowSize)
                : CalculateDockRect(TargetState, m_DockRect);
            bool showHints = TargetPanel != null;
            bool canDockContents = showHints && m_ToMove.ChildPanelsCount == 0;
            Proxy.Update(m_WindowRect, m_UpperRect, m_BottomRect, m_LeftRect, m_RightRect, m_CenterRect,
                showHints, canDockContents, canDockContents);
        }

        public void OnMouseMove(Float2 mouseScreenPosition)
        {
            if (m_LateDragOffsetUpdate)
            {
                CalculateDragOffset(mouseScreenPosition);
                m_LateDragOffsetUpdate = false;
            }
            Update(mouseScreenPosition);
        }
        public void OnMouseUp(Float2 mouseScreenPosition)
        {
            Update(mouseScreenPosition);
            Dispose(true);
        }
        public void OnLostFocus() => Dispose(true);

        public void SetTarget(DockPanel? panel, DockState state)
        {
            TargetPanel = panel;
            TargetState = state;
        }

        public void Dispose() => Dispose(true);

        private void Dispose(bool commit)
        {
            if (IsDisposed)
                return;
            IsDisposed = true;
            Proxy.Window?.EndTrackingMouse();
            Proxy.Hide();

            if (commit && TargetPanel != null && TargetState != DockState.Float)
            {
                bool noChildren = m_ToMove.ChildPanelsCount == 0;
                if (noChildren && m_ToMove.TabsCount == 1)
                {
                    m_ToMove.GetTab(0).Show(TargetState, TargetPanel);
                }
                else if (noChildren && TargetState == DockState.DockFill)
                {
                    while (m_ToMove.TabsCount > 0)
                        m_ToMove.GetTab(0).Show(DockState.DockFill, TargetPanel);
                }
                else
                {
                    DockWindow? selected = m_ToMove.SelectedTab;
                    DockWindow first = m_ToMove.GetTab(0);
                    first.Show(TargetState, TargetPanel);
                    while (m_ToMove.TabsCount > 0)
                        m_ToMove.GetTab(0).Show(DockState.DockFill, first);
                    selected?.SelectTab();
                }
                if (TargetPanel.Root is WindowRootControl root)
                    root.Window.Focus();
            }
            else if (commit && m_ToMove.HostWindow != null)
            {
                Float2 mouseScreen = m_WindowRect.Location + m_DragOffset;
                if (Proxy.Window != null)
                    mouseScreen = Proxy.Window.ClientToScreen(Proxy.Window.GUI.MousePosition * Proxy.Window.DpiScale);
                m_ToMove.HostWindow.Position = mouseScreen - m_DragOffset;
                m_ToMove.HostWindow.Show();
            }

            TargetPanel = null;
            TargetState = DockState.Unknown;
            if (ReferenceEquals(Active, this))
                Active = null;
        }

        private void CalculateDragOffset(Float2 mouseScreenPosition)
        {
            if (m_ToMove.HostWindow != null)
                m_DragOffset = mouseScreenPosition - m_ToMove.HostWindow.Position;
        }

        private static Float2 GetMouseScreenPosition(SE.Window? window)
        {
            if (window == null)
                return Float2.Zero;
            Float2 mouse = window.MousePosition;
            return mouse != Float2.Zero && mouse != Float2.Minimum
                ? window.ClientToScreen(mouse)
                : Float2.Zero;
        }

        public static class Proxy
        {
            private static SE.Window? s_Window;
            private static SE.Window? s_Left;
            private static SE.Window? s_Right;
            private static SE.Window? s_Up;
            private static SE.Window? s_Down;
            private static SE.Window? s_Center;

            public const float HintWindowsSize = 32.0f;
            public static bool IsInitialized { get; private set; }
            public static Float2 Size { get; private set; }
            public static Rectangle PreviewBounds { get; private set; }
            public static SE.Window? Window => s_Window;
            public static SE.Window? Left => s_Left;
            public static SE.Window? Right => s_Right;
            public static SE.Window? Up => s_Up;
            public static SE.Window? Down => s_Down;
            public static SE.Window? Center => s_Center;

            public static void InitHitProxy()
            {
                InitHitProxy(1.0f);
            }

            private static void InitHitProxy(float dpiScale)
            {
                Float2 hintSize = new Float2(HintWindowsSize * dpiScale);
                s_Left ??= CreateProxy("DockHint.Left", false, hintSize);
                s_Right ??= CreateProxy("DockHint.Right", false, hintSize);
                s_Up ??= CreateProxy("DockHint.Up", false, hintSize);
                s_Down ??= CreateProxy("DockHint.Down", false, hintSize);
                s_Center ??= CreateProxy("DockHint.Center", false, hintSize);
                IsInitialized = true;
            }

            public static void Init(Float2 initSize)
            {
                Init(initSize, 1.0f);
            }

            internal static void Init(Float2 initSize, float dpiScale)
            {
                s_Window ??= CreateProxy("DockHint.Window", true, initSize);
                IsInitialized = true;
                Size = initSize;
                PreviewBounds = new Rectangle(Float2.Zero, initSize);
                s_Window.ClientSize = initSize;
                InitHitProxy(dpiScale);
            }

            public static void Show()
            {
                Window?.Show();
                Window?.Focus();
            }

            public static void Update(
                Rectangle preview,
                Rectangle upper,
                Rectangle bottom,
                Rectangle left,
                Rectangle right,
                Rectangle center,
                bool showHints,
                bool showBorders,
                bool showCenter)
            {
                PreviewBounds = preview;
                if (Window != null)
                    Window.ClientBounds = preview;
                SetHint(Up, upper, showHints && showBorders);
                SetHint(Down, bottom, showHints && showBorders);
                SetHint(Left, left, showHints && showBorders);
                SetHint(Right, right, showHints && showBorders);
                SetHint(Center, center, showHints && showCenter);
            }

            public static void Hide()
            {
                PreviewBounds = Rectangle.Empty;
                Window?.Hide();
                Left?.Hide();
                Right?.Hide();
                Up?.Hide();
                Down?.Hide();
                Center?.Hide();
            }

            public static void Dispose()
            {
                CloseProxy(ref s_Window);
                CloseProxy(ref s_Left);
                CloseProxy(ref s_Right);
                CloseProxy(ref s_Up);
                CloseProxy(ref s_Down);
                CloseProxy(ref s_Center);
                IsInitialized = false;
                PreviewBounds = Rectangle.Empty;
                Size = Float2.Zero;
            }

            private static SE.Window CreateProxy(string title, bool allowInput, Float2 size)
            {
                CreateWindowSettings settings = SE.Window.CreateDefaultSettings();
                settings.Title = title;
                settings.Size = size;
                settings.AllowInput = allowInput;
                settings.AllowMaximize = false;
                settings.AllowMinimize = false;
                settings.HasBorder = false;
                settings.HasSizingFrame = false;
                settings.IsRegularWindow = false;
                settings.SupportsTransparency = true;
                settings.ShowInTaskbar = false;
                settings.ActivateWhenFirstShown = allowInput;
                settings.IsTopmost = true;
                settings.ShowAfterFirstPaint = false;

                SE.Window window = SE.Window.Create(settings);
                window.Opacity = 0.6f;
                window.GUI.BackgroundColor = Style.Current.DragWindow;
                if (allowInput)
                {
                    window.MouseMove += OnMouseMove;
                    window.MouseUp += OnMouseUp;
                    window.LostFocus += OnLostFocus;
                }
                return window;
            }

            private static void SetHint(SE.Window? window, Rectangle bounds, bool visible)
            {
                if (window == null)
                    return;
                window.Position = bounds.Location;
                if (visible)
                    window.Show();
                else
                    window.Hide();
            }

            private static void CloseProxy(ref SE.Window? window)
            {
                if (window == null)
                    return;
                window.Close(SE.ClosingReason.User);
                window = null;
            }

            private static void OnMouseMove(ref Float2 localPosition)
            {
                if (Window != null)
                    Active?.OnMouseMove(Window.ClientToScreen(localPosition * Window.DpiScale));
            }

            private static void OnMouseUp(ref Float2 localPosition, MouseButton button, ref bool handled)
            {
                if (button == MouseButton.Left)
                {
                    handled = true;
                    if (Window != null)
                        Active?.OnMouseUp(Window.ClientToScreen(localPosition * Window.DpiScale));
                }
            }

            private static void OnLostFocus()
            {
                Active?.OnLostFocus();
            }
        }
    }
}

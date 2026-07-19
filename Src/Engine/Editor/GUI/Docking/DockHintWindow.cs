using SE.Editor.GUI;

namespace SE.Editor.GUI
{
    public sealed class DockHintWindow
    {
        private DockHintWindow(FloatWindowDockPanel toMove)
        {
            ToMove = toMove;
        }

        public FloatWindowDockPanel ToMove { get; }
        public DockPanel? TargetPanel { get; private set; }
        public DockState TargetState { get; private set; } = DockState.Unknown;
        public bool IsDisposed { get; private set; }

        public static DockHintWindow Create(FloatWindowDockPanel toMove)
        {
            return new DockHintWindow(toMove);
        }

        public static DockHintWindow Create(DockWindow toMove)
        {
            FloatWindowDockPanel floating = toMove.ParentDockPanel as FloatWindowDockPanel ?? new FloatWindowDockPanel(toMove.MasterPanel);
            return new DockHintWindow(floating);
        }

        public static GuiRect CalculateDockRect(DockState state, GuiRect rect)
        {
            return state switch
            {
                DockState.DockTop => new GuiRect(rect.X, rect.Y, rect.Width, rect.Height * DockPanel.DefaultSplitterValue),
                DockState.DockBottom => new GuiRect(rect.X, rect.Bottom - rect.Height * DockPanel.DefaultSplitterValue, rect.Width, rect.Height * DockPanel.DefaultSplitterValue),
                DockState.DockLeft => new GuiRect(rect.X, rect.Y, rect.Width * DockPanel.DefaultSplitterValue, rect.Height),
                DockState.DockRight => new GuiRect(rect.Right - rect.Width * DockPanel.DefaultSplitterValue, rect.Y, rect.Width * DockPanel.DefaultSplitterValue, rect.Height),
                _ => rect,
            };
        }

        public void SetTarget(DockPanel? panel, DockState state)
        {
            TargetPanel = panel;
            TargetState = state;
        }

        public void Dispose()
        {
            IsDisposed = true;
            TargetPanel = null;
            TargetState = DockState.Unknown;
        }

        public static class Proxy
        {
            public const float HintWindowsSize = 32.0f;

            public static bool IsInitialized { get; private set; }

            public static void InitHitProxy()
            {
                IsInitialized = true;
            }

            public static void Init(GuiSize initSize)
            {
                IsInitialized = true;
            }

            public static void Hide()
            {
            }

            public static void Dispose()
            {
                IsInitialized = false;
            }
        }
    }
}

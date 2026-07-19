using System.Collections.Generic;

namespace SE.Editor.GUI
{
    public sealed class MasterDockPanel : DockPanel
    {
        private readonly List<DockWindow> _windows = new List<DockWindow>(32);
        private readonly List<FloatWindowDockPanel> _floatingPanels = new List<FloatWindowDockPanel>(4);

        public MasterDockPanel()
            : base(null)
        {
        }

        public override bool IsMaster => true;
        public IReadOnlyList<DockWindow> Windows => _windows;
        public List<FloatWindowDockPanel> FloatingPanels => _floatingPanels;

        public int VisibleWindowsCount
        {
            get
            {
                int count = 0;
                foreach (DockWindow window in _windows)
                {
                    if (!window.IsHidden)
                        count++;
                }

                return count;
            }
        }

        public void ResetLayout()
        {
            foreach (DockWindow window in _windows)
            {
                DockWindowInternal(DockState.DockFill, window, _windows.IndexOf(window) == 0);
            }
        }

        public DockPanel? HitTest(GuiPoint position, FloatWindowDockPanel? excluded)
        {
            for (int i = _floatingPanels.Count - 1; i >= 0; i--)
            {
                FloatWindowDockPanel panel = _floatingPanels[i];
                if (!ReferenceEquals(panel, excluded))
                {
                    DockPanel? hit = panel.HitTest(position);
                    if (hit != null)
                        return hit;
                }
            }

            return HitTest(position);
        }

        public void LinkWindow(DockWindow window)
        {
            if (!_windows.Contains(window))
                _windows.Add(window);
        }

        public void UnlinkWindow(DockWindow window)
        {
            if (_windows.Remove(window))
                window.NotifyUnlinked();
        }

        internal FloatWindowDockPanel CreateFloatingPanel(GuiPoint location, GuiSize size, string title)
        {
            FloatWindowDockPanel floatingPanel = new FloatWindowDockPanel(this);
            _floatingPanels.Add(floatingPanel);

            if (Root is SE.GUI.WindowRootControl)
            {
                SE.Window window = SE.Window.CreateManaged(title, new Float2(size.Width, size.Height));
                Float2 position = new Float2(location.X, location.Y);
                window.SetPosition(ref position);
                floatingPanel.AttachHostWindow(window);
                floatingPanel.SetBounds(0.0f, 0.0f, size.Width, size.Height);
                window.GUI.AddChild(floatingPanel);
                window.Show();
            }
            else if (Root != null)
            {
                floatingPanel.SetBounds(location.X, location.Y, size.Width, size.Height);
                Root.AddChild(floatingPanel);
            }
            else
            {
                floatingPanel.SetBounds(location.X, location.Y, size.Width, size.Height);
            }

            return floatingPanel;
        }

        public override DockState TryGetDockState(out float splitterValue)
        {
            splitterValue = DefaultSplitterValue;
            return DockState.DockFill;
        }
    }
}

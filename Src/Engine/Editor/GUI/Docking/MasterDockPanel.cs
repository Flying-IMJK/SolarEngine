using System.Collections.Generic;

namespace SE.Editor.GUI
{
    public sealed class MasterDockPanel : DockPanel
    {
        private readonly List<DockWindow> m_Windows = new List<DockWindow>(32);
        private readonly List<FloatWindowDockPanel> m_FloatingPanels = new List<FloatWindowDockPanel>(4);

        public MasterDockPanel()
            : base(null)
        {
        }

        public override bool IsMaster => true;
        public IReadOnlyList<DockWindow> Windows => m_Windows;
        public List<FloatWindowDockPanel> FloatingPanels => m_FloatingPanels;

        public int VisibleWindowsCount
        {
            get
            {
                int count = 0;
                foreach (DockWindow window in m_Windows)
                {
                    if (!window.IsHidden)
                        count++;
                }

                return count;
            }
        }

        public void ResetLayout()
        {
            foreach (DockWindow window in m_Windows)
            {
                DockWindowInternal(DockState.DockFill, window, m_Windows.IndexOf(window) == 0);
            }
        }

        public DockPanel? HitTest(Float2 position, FloatWindowDockPanel? excluded)
        {
            for (int i = m_FloatingPanels.Count - 1; i >= 0; i--)
            {
                FloatWindowDockPanel panel = m_FloatingPanels[i];
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
            if (!m_Windows.Contains(window))
                m_Windows.Add(window);
        }

        public void UnlinkWindow(DockWindow window)
        {
            if (m_Windows.Remove(window))
                window.NotifyUnlinked();
        }

        internal FloatWindowDockPanel CreateFloatingPanel(Float2 location, Float2 size, string title)
        {
            FloatWindowDockPanel floatingPanel = new FloatWindowDockPanel(this);
            m_FloatingPanels.Add(floatingPanel);

            if (Root is SE.GUI.WindowRootControl)
            {
                SE.Window window = SE.Window.CreateManaged(title, new Float2(size.X, size.Y));
                Float2 position = new Float2(location.X, location.Y);
                window.SetPosition(ref position);
                floatingPanel.AttachHostWindow(window);
                floatingPanel.SetBounds(0.0f, 0.0f, size.X, size.Y);
                window.GUI.AddChild(floatingPanel);
                window.Show();
            }
            else if (Root != null)
            {
                floatingPanel.SetBounds(location.X, location.Y, size.X, size.Y);
                Root.AddChild(floatingPanel);
            }
            else
            {
                floatingPanel.SetBounds(location.X, location.Y, size.X, size.Y);
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

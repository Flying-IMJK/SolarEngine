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
                    if (window.Visible)
                        count++;
                }

                return count;
            }
        }

        public void ResetLayout()
        {
            // Native ResetLayout only clears the current tree. Default layout restoration is external.
            for (int index = 0; index < m_Windows.Count; index++)
                m_Windows[index].Close();

            // Preserve the native forward iteration over a list that disposal mutates.
            for (int index = 0; index < ChildPanels.Count; index++)
                ChildPanels[index].Dispose();

            DockPanelProxy? proxy = TabsProxy;
            proxy?.Parent?.RemoveChild(proxy);
            DisposeChildren();
            if (proxy != null)
                AddChild(proxy);
        }

        public DockPanel? HitTest(Float2 position, FloatWindowDockPanel? excluded)
        {
            for (int i = 0; i < m_FloatingPanels.Count; i++)
            {
                FloatWindowDockPanel panel = m_FloatingPanels[i];
                if (panel.Visible && !ReferenceEquals(panel, excluded))
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
            m_Windows.Add(window);
        }

        public void UnlinkWindow(DockWindow window)
        {
            window.NotifyUnlinked();
            if (!IsDisposing)
                m_Windows.Remove(window);
        }

        internal FloatWindowDockPanel CreateFloatingPanel(Float2 location, Float2 size, WindowStartPosition startPosition, string title)
        {
            SE.Window window = FloatWindowDockPanel.CreateFloatWindow(Root, location, size, startPosition, title);
            FloatWindowDockPanel floatingPanel = new FloatWindowDockPanel(this, window.GUI);
            floatingPanel.SetBounds(0.0f, 0.0f, size.X, size.Y);

            return floatingPanel;
        }

        public override DockState TryGetDockState(out float splitterValue)
        {
            splitterValue = 0.5f;
            return DockState.DockFill;
        }

        protected override void OnDispose()
        {
            base.OnDispose();
            m_Windows.Clear();
            m_FloatingPanels.Clear();
        }
    }
}

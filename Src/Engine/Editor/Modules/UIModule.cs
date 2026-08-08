using System;
using SE.Editor.GUI;
using SE.GUI;

namespace SE.Editor
{
    /// <summary>
    /// Owns the managed editor chrome and the master docking workspace.
    /// </summary>
    public sealed class UIModule : EditorModule
    {
        private WindowRootControl? m_Root;
        private float m_LastWidth = float.NaN;
        private float m_LastHeight = float.NaN;

        public UIModule(Editor editor) : base(editor)
        {
        }

        public MainMenu MainMenu { get; private set; } = null!;
        public ToolStrip ToolStrip { get; private set; } = null!;
        public StatusBar StatusBar { get; private set; } = null!;
        public MasterDockPanel MasterDockPanel { get; private set; } = null!;

        internal override int Order => -95;

        public override void OnInit()
        {
            m_Root = Editor.MainWindow.GUI;
            MainMenu = m_Root.AddChild(new MainMenu(Math.Max(m_Root.Width, 1.0f)));
            ToolStrip = m_Root.AddChild(new ToolStrip(ToolStrip.DefaultMarginV * 2.0f + 26.0f, MainMenu.DefaultHeight, Math.Max(m_Root.Width, 1.0f)));
            StatusBar = m_Root.AddChild(new StatusBar(0.0f, Math.Max(m_Root.Width, 1.0f)));
            MasterDockPanel = m_Root.AddChild(new MasterDockPanel());

            BuildMenus();
            BuildToolStrip();
            StatusBar.Text = "Ready";
            UpdateLayout(force: true);
        }

        public override void OnUpdate()
        {
            UpdateLayout(force: false);
            ThumbnailService.Instance.Update();
        }

        public override void OnDispose()
        {
            if (m_Root == null)
                return;

            DisposeControl(MasterDockPanel);
            DisposeControl(StatusBar);
            DisposeControl(ToolStrip);
            DisposeControl(MainMenu);
            m_Root = null;
        }

        private void BuildMenus()
        {
            MainMenuButton window = MainMenu.AddButton("Window");
            window.ContextMenu.AddButton("Content", () => Editor.Windows.Content.FocusOrShow());
            window.ContextMenu.AddButton("Scene Hierarchy", () => Editor.Windows.SceneHierarchy.FocusOrShow());
            window.ContextMenu.AddButton("Log", () => Editor.Windows.Log.FocusOrShow());
            window.ContextMenu.AddButton("Edit Scene", () => Editor.Windows.EditScene.FocusOrShow());
        }

        private void BuildToolStrip()
        {
            ToolStrip.AddButton("Content", () => Editor.Windows.Content.FocusOrShow());
            ToolStrip.AddButton("Scene", () => Editor.Windows.SceneHierarchy.FocusOrShow());
            ToolStrip.AddButton("Log", () => Editor.Windows.Log.FocusOrShow());
        }

        private void UpdateLayout(bool force)
        {
            if (m_Root == null || m_Root.IsDisposed)
                return;

            float width = Math.Max(m_Root.Width, 1.0f);
            float height = Math.Max(m_Root.Height, MainMenu.DefaultHeight + ToolStrip.Height + StatusBar.DefaultHeight + 1.0f);
            if (!force && width == m_LastWidth && height == m_LastHeight)
                return;

            m_LastWidth = width;
            m_LastHeight = height;
            MainMenu.SetBounds(0.0f, 0.0f, width, MainMenu.DefaultHeight);
            float toolStripTop = MainMenu.Y + MainMenu.Height;
            ToolStrip.SetBounds(0.0f, toolStripTop, width, ToolStrip.Height);
            StatusBar.SetBounds(0.0f, height - StatusBar.DefaultHeight, width, StatusBar.DefaultHeight);
            float workspaceTop = ToolStrip.Y + ToolStrip.Height;
            MasterDockPanel.SetBounds(0.0f, workspaceTop, width, Math.Max(1.0f, StatusBar.Y - workspaceTop));
            m_Root.PerformLayout(true);
        }

        private static void DisposeControl(Control? control)
        {
            if (control == null || control.IsDisposed)
                return;

            control.Parent?.RemoveChild(control);
            control.Dispose();
        }
    }
}

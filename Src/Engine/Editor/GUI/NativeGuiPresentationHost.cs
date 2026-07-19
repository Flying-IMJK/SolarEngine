// Managed editor GUI feature implementation.
using System.Collections.Generic;
using SE.Editor;
using SE.GUI;

namespace SE.Editor.GUI
{
    public sealed class NativeGuiPresentationHost : IEditorPresentationHost
    {
        private const float RootWidth = 1920.0f;
        private const float RootHeight = 1080.0f;
        private const float ToolStripHeight = 30.0f;

        private readonly Dictionary<EditorWindow, EditorWindowFrame> _windowViews = new Dictionary<EditorWindow, EditorWindowFrame>();
        private RootControl? _root;
        private MainMenu? _mainMenu;
        private ToolStrip? _toolStrip;
        private NavigationBar? _navigationBar;
        private Panel? _workspace;
        private StatusBar? _statusBar;

        public void Initialize(EditorApplication editor)
        {
            _root = new RootControl(new Rectangle(0, 0, RootWidth, RootHeight));

            _mainMenu = new MainMenu(RootWidth);
            _root.AddChild(_mainMenu);
            BuildMainMenu(editor, _mainMenu);

            _toolStrip = new ToolStrip(ToolStripHeight, MainMenu.DefaultHeight, RootWidth);
            _root.AddChild(_toolStrip);
            BuildToolStrip(editor, _toolStrip);

            _navigationBar = new NavigationBar(0, MainMenu.DefaultHeight, 320, ToolStripHeight);
            _navigationBar.AddButton("Project");
            _navigationBar.AddButton("Assets");
            _root.AddChild(_navigationBar);
            _navigationBar.UpdateBounds(_toolStrip, RootWidth);

            float workspaceY = MainMenu.DefaultHeight + ToolStripHeight;
            float workspaceHeight = RootHeight - workspaceY - StatusBar.DefaultHeight;
            _workspace = new Panel(new Rectangle(0, workspaceY, RootWidth, workspaceHeight));
            _root.AddChild(_workspace);

            _statusBar = new StatusBar(RootHeight - StatusBar.DefaultHeight, RootWidth);
            _statusBar.Text = "Ready";
            _root.AddChild(_statusBar);

            Debug.Log("C# Editor presentation host attached to Runtime/UI/GUI");
        }

        public void AddWindow(EditorWindow window)
        {
            if (_workspace == null || _windowViews.ContainsKey(window))
                return;

            int index = _windowViews.Count;
            EditorWindowFrame frame = new EditorWindowFrame(window.Title);
            frame.SetBounds(16 + (index % 3) * 330, 16 + (index / 3) * 220, 300, 180);
            _workspace.AddChild(frame);

            _windowViews.Add(window, frame);
            frame.Visible = window.IsVisible;
        }

        public void RemoveWindow(EditorWindow window)
        {
            if (!_windowViews.Remove(window, out EditorWindowFrame? view))
                return;

            view.Dispose();
        }

        public void FocusWindow(EditorWindow window)
        {
            ShowWindow(window);
        }

        public void ShowWindow(EditorWindow window)
        {
            if (_windowViews.TryGetValue(window, out EditorWindowFrame? view))
            {
                view.Visible = true;
            }
        }

        public void HideWindow(EditorWindow window)
        {
            if (_windowViews.TryGetValue(window, out EditorWindowFrame? view))
            {
                view.Visible = false;
            }
        }

        public void Update()
        {
        }

        public void Render()
        {
        }

        public void Shutdown()
        {
            foreach (EditorWindowFrame view in _windowViews.Values)
            {
                view.Dispose();
            }

            _windowViews.Clear();
            _workspace?.Dispose();
            _workspace = null;
            _navigationBar?.Dispose();
            _navigationBar = null;
            _toolStrip?.Dispose();
            _toolStrip = null;
            _mainMenu?.Dispose();
            _mainMenu = null;
            _statusBar?.Dispose();
            _statusBar = null;
            _root?.Dispose();
            _root = null;
        }

        private static void BuildMainMenu(EditorApplication editor, MainMenu mainMenu)
        {
            MainMenuButton file = mainMenu.AddButton("File");
            file.ContextMenu.AddButton("New Scene");
            file.ContextMenu.AddButton("Open");
            file.ContextMenu.AddSeparator();
            file.ContextMenu.AddButton("Exit", editor.Exit);

            MainMenuButton edit = mainMenu.AddButton("Edit");
            edit.ContextMenu.AddButton("Undo", "Ctrl+Z");
            edit.ContextMenu.AddButton("Redo", "Ctrl+Y");

            MainMenuButton view = mainMenu.AddButton("View");
            view.ContextMenu.AddButton("Content", () => editor.Windows.Content.Show());
            view.ContextMenu.AddButton("Scene", () => editor.Windows.SceneHierarchy.Show());
            view.ContextMenu.AddButton("Properties", () => editor.Windows.Properties.Show());
            view.ContextMenu.AddButton("Log", () => editor.Windows.Log.Show());

            mainMenu.AddButton("Window");
            mainMenu.AddButton("Help");
        }

        private static void BuildToolStrip(EditorApplication editor, ToolStrip toolStrip)
        {
            toolStrip.AddButton("Save");
            toolStrip.AddSeparator();
            toolStrip.AddButton("Content", () => editor.Windows.Content.Focus());
            toolStrip.AddButton("Scene", () => editor.Windows.SceneHierarchy.Focus());
            toolStrip.AddButton("Log", () => editor.Windows.Log.Focus());
        }
    }

    internal sealed class EditorWindowFrame : SE.GUI.ContainerControl
    {
        private readonly Label _title;

        public EditorWindowFrame(string title)
            : base(new Rectangle(0, 0, 300, 180))
        {
            _title = new Label(new Rectangle(10, 8, 280, 24), title)
            {
                AutoFocus = false,
                Enabled = false,
            };
            SetBounds(0, 0, 300, 180);
            AddChild(_title);
        }

        protected override void OnLayoutChildren()
        {
            _title.SetBounds(10, 8, Width - 20, 24);
        }
    }
}

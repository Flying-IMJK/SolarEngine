using System;
using System.Collections.Generic;
using SE.Editor.GUI;

namespace SE.Editor
{
    /// <summary>
    /// Registers managed editor windows and restores the default docking workspace.
    /// </summary>
    public sealed class WindowsModule : EditorModule
    {
        private readonly List<EditorWindow> m_Windows = new();

        public WindowsModule(Editor editor) : base(editor)
        {
        }

        public event Action<EditorWindow>? WindowAdded;
        public event Action<EditorWindow>? WindowRemoved;

        public LogWindow Log { get; private set; } = null!;
        public ContentWindow Content { get; private set; } = null!;
        public SceneHierarchyWindow SceneHierarchy { get; private set; } = null!;
        public EditSceneWindow EditScene { get; private set; } = null!;

        // Flax-compatible names for editor features that use the traditional
        // WindowsModule surface. The descriptive properties above remain the
        // preferred API for new Solar editor code.
        public LogWindow DebugLogWin => Log;
        public ContentWindow ContentWin => Content;
        public SceneHierarchyWindow SceneWin => SceneHierarchy;
        public EditSceneWindow EditWin => EditScene;

        public IReadOnlyList<EditorWindow> Windows => m_Windows;

        internal override int Order => -90;

        public bool TryGetWindow(string id, out EditorWindow? window)
        {
            foreach (EditorWindow candidate in m_Windows)
            {
                if (string.Equals(candidate.Name, id, StringComparison.Ordinal))
                {
                    window = candidate;
                    return true;
                }
            }

            window = null;
            return false;
        }

        public override void OnInit()
        {
            Log = new LogWindow(Editor);
            Content = new ContentWindow(Editor);
            SceneHierarchy = new SceneHierarchyWindow(Editor);
            EditScene = new EditSceneWindow(Editor);
        }

        public override void OnEndInit()
        {
            MasterDockPanel master = Editor.UI.MasterDockPanel;
            Content.Show(DockState.DockLeft, master, autoSelect: false, splitterValue: 0.24f);
            SceneHierarchy.Show(DockState.DockLeft, master, autoSelect: false, splitterValue: 0.24f);
            Log.Show(DockState.DockBottom, master, autoSelect: false, splitterValue: 0.25f);
            EditScene.Show(DockState.DockFill, master, autoSelect: true);

            foreach (EditorWindow window in new List<EditorWindow>(m_Windows))
            {
                window.OnInit();
            }
        }

        public override void OnUpdate()
        {
            foreach (EditorWindow window in new List<EditorWindow>(m_Windows))
            {
                window.OnUpdate();
            }
        }

        public override void OnDispose()
        {
            foreach (EditorWindow window in new List<EditorWindow>(m_Windows))
            {
                window.Dispose();
            }
            m_Windows.Clear();
            WindowAdded = null;
            WindowRemoved = null;
        }

        internal void RegisterWindow(EditorWindow window)
        {
            ArgumentNullException.ThrowIfNull(window);
            m_Windows.Add(window);
            WindowAdded?.Invoke(window);
        }

        internal void UnregisterWindow(EditorWindow window)
        {
            if (window != null && m_Windows.Remove(window))
            {
                WindowRemoved?.Invoke(window);
            }
        }
    }
}

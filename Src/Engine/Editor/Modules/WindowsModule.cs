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
        private readonly Dictionary<string, EditorWindow> m_Windows = new(StringComparer.Ordinal);

        public WindowsModule(Editor editor)
            : base(editor)
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

        public IReadOnlyDictionary<string, EditorWindow> Windows => m_Windows;

        internal override int Order => -90;

        public bool TryGetWindow(string id, out EditorWindow? window)
        {
            return m_Windows.TryGetValue(id, out window);
        }

        public override void OnInit()
        {
            Log = new LogWindow(Editor);
            Content = new ContentWindow(Editor);
            SceneHierarchy = new SceneHierarchyWindow(Editor);
            EditScene = new EditSceneWindow(Editor);

            SE.Level.SceneSaving += OnSceneSaving;
            SE.Level.SceneSaved += OnSceneSaved;
            SE.Level.SceneSaveError += OnSceneSaveError;
            SE.Level.SceneLoading += OnSceneLoading;
            SE.Level.SceneLoaded += OnSceneLoaded;
            SE.Level.SceneLoadError += OnSceneLoadError;
            SE.Level.SceneUnloading += OnSceneUnloading;
            SE.Level.SceneUnloaded += OnSceneUnloaded;
        }

        public override void OnEndInit()
        {
            MasterDockPanel master = Editor.UI.MasterDockPanel;
            Content.Show(DockState.DockLeft, master, autoSelect: false, splitterValue: 0.24f);
            SceneHierarchy.Show(DockState.DockLeft, master, autoSelect: false, splitterValue: 0.24f);
            Log.Show(DockState.DockBottom, master, autoSelect: false, splitterValue: 0.25f);
            EditScene.Show(DockState.DockFill, master, autoSelect: true);

            foreach (EditorWindow window in new List<EditorWindow>(m_Windows.Values))
            {
                window.OnInit();
            }
        }

        public override void OnUpdate()
        {
            foreach (EditorWindow window in new List<EditorWindow>(m_Windows.Values))
            {
                window.OnUpdate();
            }
        }

        public override void OnDispose()
        {
            SE.Level.SceneSaving -= OnSceneSaving;
            SE.Level.SceneSaved -= OnSceneSaved;
            SE.Level.SceneSaveError -= OnSceneSaveError;
            SE.Level.SceneLoading -= OnSceneLoading;
            SE.Level.SceneLoaded -= OnSceneLoaded;
            SE.Level.SceneLoadError -= OnSceneLoadError;
            SE.Level.SceneUnloading -= OnSceneUnloading;
            SE.Level.SceneUnloaded -= OnSceneUnloaded;

            foreach (EditorWindow window in new List<EditorWindow>(m_Windows.Values))
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
            if (!m_Windows.TryAdd(window.Id, window))
                throw new InvalidOperationException($"An editor window is already registered with id '{window.Id}'.");
            WindowAdded?.Invoke(window);
        }

        internal void UnregisterWindow(EditorWindow window)
        {
            if (window != null && m_Windows.Remove(window.Id))
                WindowRemoved?.Invoke(window);
        }

        private void OnSceneSaving(SE.Scene scene, Guid sceneId)
        {
            foreach (EditorWindow window in new List<EditorWindow>(m_Windows.Values))
            {
                window.OnSceneSaving(scene, sceneId);
            }
        }

        private void OnSceneSaved(SE.Scene scene, Guid sceneId)
        {
            foreach (EditorWindow window in new List<EditorWindow>(m_Windows.Values))
            {
                window.OnSceneSaved(scene, sceneId);
            }
        }

        private void OnSceneSaveError(SE.Scene scene, Guid sceneId)
        {
            foreach (EditorWindow window in new List<EditorWindow>(m_Windows.Values))
            {
                window.OnSceneSaveError(scene, sceneId);
            }
        }

        private void OnSceneLoading(SE.Scene scene, Guid sceneId)
        {
            foreach (EditorWindow window in new List<EditorWindow>(m_Windows.Values))
            {
                window.OnSceneLoading(scene, sceneId);
            }
        }

        private void OnSceneLoaded(SE.Scene scene, Guid sceneId)
        {
            foreach (EditorWindow window in new List<EditorWindow>(m_Windows.Values))
            {
                window.OnSceneLoaded(scene, sceneId);
            }
        }

        private void OnSceneLoadError(SE.Scene scene, Guid sceneId)
        {
            foreach (EditorWindow window in new List<EditorWindow>(m_Windows.Values))
            {
                window.OnSceneLoadError(scene, sceneId);
            }
        }

        private void OnSceneUnloading(SE.Scene scene, Guid sceneId)
        {
            foreach (EditorWindow window in new List<EditorWindow>(m_Windows.Values))
            {
                window.OnSceneUnloading(scene, sceneId);
            }
        }

        private void OnSceneUnloaded(SE.Scene scene, Guid sceneId)
        {
            foreach (EditorWindow window in new List<EditorWindow>(m_Windows.Values))
            {
                window.OnSceneUnloaded(scene, sceneId);
            }
        }
    }
}

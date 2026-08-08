using System;
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Base class for managed editor tool windows. It mirrors Flax EditorWindow:
    /// the window owns a focused editor feature and registers itself with the
    /// managed WindowsModule for lifecycle and docking management.
    /// </summary>
    public abstract class EditorWindow : DockWindow
    {
        private bool m_IsRegistered;

        protected EditorWindow(Editor editor, string id, string title, ScrollBars scrollBars = ScrollBars.None)
            : base((editor ?? throw new ArgumentNullException(nameof(editor))).UI.MasterDockPanel, hideOnClose: true, scrollBars)
        {
            Editor = editor;
            Id = id ?? throw new ArgumentNullException(nameof(id));
            Title = title ?? string.Empty;
            AutoFocus = true;

            m_IsRegistered = true;
            Editor.Windows.RegisterWindow(this);
        }

        /// <summary>
        /// Gets the managed editor root that owns this window.
        /// </summary>
        public Editor Editor { get; }

        /// <summary>
        /// Gets the stable layout and registration identifier.
        /// </summary>
        public string Id { get; }

        /// <summary>
        /// Determines whether this window is editing a given content item.
        /// </summary>
        public virtual bool IsEditingItem(ContentItem item)
        {
            _ = item;
            return false;
        }

        /// <summary>
        /// Called once after all default editor windows have been constructed.
        /// </summary>
        public virtual void OnInit()
        {
        }

        /// <summary>
        /// Called on every managed editor update.
        /// </summary>
        public virtual void OnUpdate()
        {
        }

        /// <summary>
        /// Called while the managed editor is shutting down.
        /// </summary>
        public virtual void OnExit()
        {
        }

        public virtual void OnSceneSaving(SE.Scene scene, Guid sceneId)
        {
            _ = scene;
            _ = sceneId;
        }

        public virtual void OnSceneSaved(SE.Scene scene, Guid sceneId)
        {
            _ = scene;
            _ = sceneId;
        }

        public virtual void OnSceneSaveError(SE.Scene scene, Guid sceneId)
        {
            _ = scene;
            _ = sceneId;
        }

        public virtual void OnSceneLoading(SE.Scene scene, Guid sceneId)
        {
            _ = scene;
            _ = sceneId;
        }

        public virtual void OnSceneLoaded(SE.Scene scene, Guid sceneId)
        {
            _ = scene;
            _ = sceneId;
        }

        public virtual void OnSceneLoadError(SE.Scene scene, Guid sceneId)
        {
            _ = scene;
            _ = sceneId;
        }

        public virtual void OnSceneUnloading(SE.Scene scene, Guid sceneId)
        {
            _ = scene;
            _ = sceneId;
        }

        public virtual void OnSceneUnloaded(SE.Scene scene, Guid sceneId)
        {
            _ = scene;
            _ = sceneId;
        }

        protected override void OnDispose()
        {
            OnExit();
            if (m_IsRegistered)
            {
                m_IsRegistered = false;
                Editor.Windows.UnregisterWindow(this);
            }
            base.OnDispose();
        }
    }
}

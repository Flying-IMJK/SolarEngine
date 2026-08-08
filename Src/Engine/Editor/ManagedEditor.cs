using System;
using System.Collections.Generic;

namespace SE.Editor
{
    /// <summary>
    /// The main managed editor class. Editor root object.
    /// </summary>
    public partial class Editor
    {
        /// <summary>
        /// Gets the Editor instance.
        /// </summary>
        public static Editor? Instance { get; private set; }

        private readonly List<EditorModule> m_Modules;
        private Window? m_MainWindow;
        private bool m_IsInitialized;
        private bool m_IsExiting;
        

        /// <summary>
        /// Gets the single editor main window, creating it on first use.
        /// </summary>
        public Window MainWindow => EnsureMainWindow();

        public UIModule UI { get; private set; }
        public SceneModule Scene { get; private set; }
        public WindowsModule Windows { get; private set; }
        public bool IsInitialized => m_IsInitialized;

        /// <summary>
        /// Called by the native editor bridge. It is intentionally idempotent
        /// because the native application may request the main window first.
        /// </summary>
        public void Init()
        {
            if (m_IsInitialized)
            {
                return;
            }

            if (m_IsExiting)
            {
                throw new ObjectDisposedException(nameof(Editor));
            }

            Instance = this;
            
            EnsureMainWindow();
            
            m_Modules.Add(UI = new UIModule(this));
            m_Modules.Add(Scene = new SceneModule(this));
            m_Modules.Add(Windows = new WindowsModule(this));

            
            m_Modules.Sort( (left, right) => left.Order.CompareTo(right.Order));
            
            
            foreach (EditorModule module in m_Modules)
            {
                module.OnInit();
            }
            
            foreach (EditorModule module in m_Modules)
            {
                module.OnEndInit();
            }
            
            m_IsInitialized = true;
        }

        public void Update()
        {
            if (!m_IsInitialized || m_IsExiting)
                return;

            foreach (EditorModule module in m_Modules)
            {
                module.OnUpdate();
            }
        }

        public void LateUpdate()
        {
            if (!m_IsInitialized || m_IsExiting)
                return;

            foreach (EditorModule module in m_Modules)
            {
                module.OnLateUpdate();
            }
        }

        public void Render()
        {
            if (!m_IsInitialized || m_IsExiting)
                return;

            foreach (EditorModule module in m_Modules)
            {
                module.OnRender();
            }
        }

        public void Exit()
        {
            if (m_IsExiting)
                return;

            m_IsExiting = true;
            for (int index = m_Modules.Count - 1; index >= 0; index--)
            {
                m_Modules[index].OnDispose();
            }

            m_IsInitialized = false;
            Instance = null;
        }

        /// <summary>
        /// Called from C++ before normal editor initialization if the application
        /// needs to create its platform window. This must not build editor UI.
        /// </summary>
        public IntPtr GetMainWindowPtr()
        {
            return Object.GetUnmanagedPtr(EnsureMainWindow());
        }

        private Window EnsureMainWindow()
        {
            if (m_MainWindow != null)
            {
                return m_MainWindow;
            }

            if (m_IsExiting)
            {
                throw new ObjectDisposedException(nameof(Editor));
            }

            CreateWindowSettings settings = Window.CreateDefaultSettings();
            settings.Title = "SE Editor";
            settings.HasBorder = false;
            settings.AllowDragAndDrop = true;
            m_MainWindow = Window.Create(settings) ?? throw new InvalidOperationException("Failed to create the editor main window.");
            return m_MainWindow;
        }
    }
}


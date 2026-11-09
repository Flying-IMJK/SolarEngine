using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;
using System.Xml;
using SE.Editor.GUI;
using SE.GUI;

namespace SE.Editor
{
    /// <summary>
    /// Registers managed editor windows and saves and restores the docking workspace.
    /// </summary>
    public sealed class WindowsModule : EditorModule
    {
        private readonly List<EditorWindow> m_Windows = new();
        private DateTime m_LastLayoutSaveTime;
        private string m_WindowsLayoutPath;
        private Window m_MainWindow;

        internal string LayoutsFolder => Path.Combine(EngineContext.ProjectCacheFolder, "LayoutsCache");

        public WindowsModule(Editor editor) : base(editor)
        {
        }

        public event Action<EditorWindow> WindowAdded;
        public event Action<EditorWindow> WindowRemoved;

        public LogWindow Log { get; private set; } = null!;
        public ContentWindow Content { get; private set; } = null!;
        public SceneHierarchyWindow SceneHierarchy { get; private set; } = null!;
        public EditSceneWindow EditScene { get; private set; } = null!;
        public PropertiesWindow Properties { get; private set; } = null!;
        public ProfilerWindow Profiler { get; private set; }

        // Flax-compatible names for editor features that use the traditional
        // WindowsModule surface. The descriptive properties above remain the
        // preferred API for new Solar editor code.
        public LogWindow DebugLogWin => Log;
        public ContentWindow ContentWin => Content;
        public SceneHierarchyWindow SceneWin => SceneHierarchy;
        public EditSceneWindow EditWin => EditScene;

        public IReadOnlyList<EditorWindow> Windows => m_Windows;

        internal override int Order => -90;

        public bool TryGetWindow(string id, out EditorWindow window)
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

        /// <summary>
        /// Saves the current workspace in the project cache. Call on the editor thread before window disposal.
        /// </summary>
        public void SaveCurrentLayout()
        {
            m_LastLayoutSaveTime = DateTime.UtcNow;
            SaveLayout(m_WindowsLayoutPath);
        }

        /// <summary>
        /// Restores the engine default XML layout. Missing or invalid defaults leave all tool windows closed.
        /// </summary>
        public void LoadDefaultLayout()
        {
            string path = Path.Combine(EngineContext.StartupFolder, EditorAssets.AssetsPath, "Config", "LayoutDefault.xml");
            if (!LoadLayout(path))
            {
                Editor.UI.MasterDockPanel.ResetLayout();
            }
        }

        /// <summary>
        /// Restores a layout on the editor thread. Invalid files are logged and return false.
        /// </summary>
        public bool LoadLayout(string path)
        {
            if (!File.Exists(path))
            {
                Debug.LogWarning($"Cannot load windows layout '{path}': file is missing.");
                return false;
            }
            MasterDockPanel master = Editor.UI.MasterDockPanel;
            bool reset = false;
            try
            {
                XmlDocument document = new XmlDocument { XmlResolver = null };
                document.Load(path);
                XmlElement root = document.DocumentElement;
                if (root == null || root.Name != "DockPanelLayout" || root.GetAttribute("Version") != "4")
                {
                    throw new InvalidDataException("Invalid or unsupported windows layout.");
                }

                XmlElement mainPanel = root["MasterPanel"];
                if (mainPanel == null)
                {
                    throw new InvalidDataException("Windows layout has no master panel.");
                }

                // Validate the file before replacing the current workspace.
                HashSet<DockWindow> windows = new HashSet<DockWindow>();
                ValidatePanel(mainPanel, windows);
                if (root["MainWindow"] is XmlElement mainWindow)
                {
                    LoadBounds(mainWindow);
                }
                foreach (XmlElement floating in root.SelectNodes("Float"))
                {
                    if (!LoadBounds(floating).HasValue)
                    {
                        throw new InvalidDataException("Floating window has no bounds.");
                    }
                    ValidatePanel(floating, windows);
                }

                reset = true;
                master.ResetLayout();
                if (root["MainWindow"] is XmlElement mainWindowNode)
                {
                    LoadWindow(Editor.MainWindow, mainWindowNode, false);
                }
                LoadPanel(mainPanel, master);
                foreach (XmlElement floating in root.SelectNodes("Float"))
                {
                    Rectangle bounds = LoadBounds(floating).Value;
                    Window window = FloatWindowDockPanel.CreateFloatWindow(Editor.MainWindow.GUI, bounds.Location, bounds.Size,
                        WindowStartPosition.Manual, string.Empty);
                    if (window == null)
                    {
                        throw new InvalidOperationException("Failed to create a floating layout window.");
                    }
                    FloatWindowDockPanel panel = new FloatWindowDockPanel(master, window.GUI);
                    LoadPanel(floating, panel);
                    if (panel.TabsCount == 0 && panel.ChildPanelsCount == 0)
                    {
                        window.Close();
                        continue;
                    }
                    window.GUI.UnlockChildrenRecursive();
                    window.GUI.PerformLayout();
                    window.Show();
                    LoadWindow(window, floating, true);
                    window.GUI.PerformLayout();
                }
                master.PerformLayout(true);
                return true;
            }
            catch (Exception ex)
            {
                Debug.LogWarning($"Failed to load windows layout '{path}'.");
                Debug.LogWarning(ex);
                if (reset)
                {
                    master.ResetLayout();
                }
                return false;
            }
        }

        /// <summary>
        /// Opens a modeless dialog to save a named layout in this project's layout cache.
        /// </summary>
        public void SaveLayout()
        {
            new LayoutNameDialog(this).Show();
        }

        /// <summary>
        /// Saves the workspace on the editor thread. Logs errors and returns false if saving fails.
        /// </summary>
        public bool SaveLayout(string path)
        {
            MasterDockPanel master = Editor.UI.MasterDockPanel;
            if (master == null || master.IsDisposed || m_MainWindow == null)
            {
                return false;
            }

            try
            {
                StringBuilder text = new StringBuilder();
                XmlWriterSettings settings = new XmlWriterSettings
                {
                    Indent = true,
                    IndentChars = "    ",
                    OmitXmlDeclaration = true,
                };
                using (XmlWriter writer = XmlWriter.Create(text, settings))
                {
                    writer.WriteStartElement("DockPanelLayout");
                    writer.WriteAttributeString("Version", "4");
                    writer.WriteStartElement("MainWindow");
                    SaveBounds(writer, m_MainWindow);
                    writer.WriteEndElement();
                    writer.WriteStartElement("MasterPanel");
                    SavePanel(writer, master);
                    writer.WriteEndElement();
                    foreach (FloatWindowDockPanel panel in master.FloatingPanels)
                    {
                        if (panel.Window == null)
                        {
                            continue;
                        }
                        writer.WriteStartElement("Float");
                        SavePanel(writer, panel);
                        SaveBounds(writer, panel.Window.Window);
                        writer.WriteEndElement();
                    }
                    writer.WriteEndElement();
                }

                Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(path)));
                File.WriteAllText(path, text.ToString(), new UTF8Encoding(false));
                return true;
            }
            catch (Exception ex)
            {
                Debug.LogWarning($"Failed to save windows layout '{path}'.");
                Debug.LogWarning(ex);
                return false;
            }
        }

        private void SavePanel(XmlWriter writer, DockPanel panel)
        {
            writer.WriteAttributeString("SelectedTab", panel.SelectedTabIndex.ToString(CultureInfo.InvariantCulture));
            foreach (DockWindow window in panel.Tabs)
            {
                writer.WriteStartElement("Window");
                writer.WriteAttributeString("Typename", window.SerializationTypename);
                if (window.UseLayoutData)
                {
                    writer.WriteStartElement("Data");
                    window.OnLayoutSerialize(writer);
                    writer.WriteEndElement();
                }
                writer.WriteEndElement();
            }
            foreach (DockPanel child in panel.ChildPanels)
            {
                if (child.TabsCount == 0 && child.ChildPanelsCount == 0)
                {
                    continue;
                }
                DockState state = child.TryGetDockState(out float splitterValue);
                writer.WriteStartElement("Panel");
                writer.WriteAttributeString("DockState", ((int)state).ToString(CultureInfo.InvariantCulture));
                writer.WriteAttributeString("SplitterValue", splitterValue.ToString(CultureInfo.InvariantCulture));
                SavePanel(writer, child);
                writer.WriteEndElement();
            }
        }

        private void ValidatePanel(XmlElement node, HashSet<DockWindow> windows)
        {
            int selectedTab = int.Parse(node.GetAttribute("SelectedTab"), CultureInfo.InvariantCulture);
            XmlNodeList tabs = node.SelectNodes("Window");
            if (selectedTab < -1 || selectedTab >= tabs.Count)
            {
                throw new InvalidDataException("Invalid selected layout tab.");
            }
            foreach (XmlElement tab in tabs)
            {
                string typename = tab.GetAttribute("Typename");
                DockWindow window = m_Windows.Find(candidate => candidate.SerializationTypename == typename);
                if (window == null || !windows.Add(window))
                {
                    throw new InvalidDataException($"Unknown or duplicate layout window '{typename}'.");
                }
            }
            foreach (XmlElement child in node.SelectNodes("Panel"))
            {
                DockState state = (DockState)int.Parse(child.GetAttribute("DockState"), CultureInfo.InvariantCulture);
                float splitterValue = float.Parse(child.GetAttribute("SplitterValue"), CultureInfo.InvariantCulture);
                if ((state != DockState.DockTop && state != DockState.DockBottom && state != DockState.DockLeft && state != DockState.DockRight)
                    || !float.IsFinite(splitterValue) || splitterValue <= 0.0f || splitterValue >= 1.0f)
                {
                    throw new InvalidDataException("Invalid layout panel split.");
                }
                ValidatePanel(child, windows);
            }
        }

        private void LoadPanel(XmlElement node, DockPanel panel)
        {
            foreach (XmlElement tab in node.SelectNodes("Window"))
            {
                string typename = tab.GetAttribute("Typename");
                EditorWindow window = m_Windows.Find(candidate => candidate.SerializationTypename == typename);
                if (tab["Data"] is XmlElement data)
                {
                    window.OnLayoutDeserialize(data);
                }
                else
                {
                    window.OnLayoutDeserialize();
                }
                window.Show(DockState.DockFill, panel, autoSelect: false);
            }
            foreach (XmlElement child in node.SelectNodes("Panel"))
            {
                DockState state = (DockState)int.Parse(child.GetAttribute("DockState"), CultureInfo.InvariantCulture);
                float splitterValue = float.Parse(child.GetAttribute("SplitterValue"), CultureInfo.InvariantCulture);
                DockPanel childPanel = panel.CreateChildPanel(state, splitterValue);
                LoadPanel(child, childPanel);
                if (childPanel.TabsCount == 0 && childPanel.ChildPanelsCount == 0)
                {
                    childPanel.RemoveIt();
                }
            }
            panel.SelectTab(int.Parse(node.GetAttribute("SelectedTab"), CultureInfo.InvariantCulture));
        }

        private static void SaveBounds(XmlWriter writer, Window window)
        {
            Rectangle bounds = window.ClientBounds;
            writer.WriteStartElement("Bounds");
            writer.WriteAttributeString("X", bounds.X.ToString(CultureInfo.InvariantCulture));
            writer.WriteAttributeString("Y", bounds.Y.ToString(CultureInfo.InvariantCulture));
            writer.WriteAttributeString("Width", bounds.Width.ToString(CultureInfo.InvariantCulture));
            writer.WriteAttributeString("Height", bounds.Height.ToString(CultureInfo.InvariantCulture));
            writer.WriteAttributeString("IsMaximized", window.IsMaximized.ToString());
            writer.WriteAttributeString("IsMinimized", window.IsMinimized.ToString());
            writer.WriteEndElement();
        }

        private static Rectangle? LoadBounds(XmlElement node)
        {
            XmlElement bounds = node["Bounds"];
            if (bounds == null)
            {
                throw new InvalidDataException("Window has no bounds element.");
            }
            bool.Parse(bounds.GetAttribute("IsMaximized"));
            bool.Parse(bounds.GetAttribute("IsMinimized"));
            // The default layout specifies window state without a fixed screen resolution.
            if (!bounds.HasAttribute("X") && !bounds.HasAttribute("Y") && !bounds.HasAttribute("Width") && !bounds.HasAttribute("Height"))
            {
                return null;
            }
            float x = float.Parse(bounds.GetAttribute("X"), CultureInfo.InvariantCulture);
            float y = float.Parse(bounds.GetAttribute("Y"), CultureInfo.InvariantCulture);
            float width = float.Parse(bounds.GetAttribute("Width"), CultureInfo.InvariantCulture);
            float height = float.Parse(bounds.GetAttribute("Height"), CultureInfo.InvariantCulture);
            if (!float.IsFinite(x) || !float.IsFinite(y) || !float.IsFinite(width) || !float.IsFinite(height) || width < 1.0f || height < 1.0f)
            {
                throw new InvalidDataException("Invalid window bounds.");
            }
            return new Rectangle(x, y, width, height);
        }

        private static void LoadWindow(Window window, XmlElement node, bool allowMinimized)
        {
            Rectangle? bounds = LoadBounds(node);
            if (window.IsMaximized || window.IsMinimized)
            {
                window.Restore();
            }
            if (bounds.HasValue)
            {
                Rectangle value = bounds.Value;
                Rectangle desktop = Platform.VirtualDesktopBounds;
                if (value.X < desktop.X || value.Y < desktop.Y || value.X >= desktop.Right || value.Y >= desktop.Bottom)
                {
                    value.Location = desktop.Location;
                }
                window.ClientBounds = value;
            }
            XmlElement state = node["Bounds"];
            if (bool.Parse(state.GetAttribute("IsMaximized")))
            {
                window.Maximize();
            }
            else if (allowMinimized && bool.Parse(state.GetAttribute("IsMinimized")))
            {
                window.Minimize();
            }
        }

        private sealed class LayoutNameDialog : Dialog
        {
            private readonly WindowsModule m_Module;
            private readonly TextBox m_TextBox;

            public LayoutNameDialog(WindowsModule module) : base("Save Window Layout")
            {
                m_Module = module;
                m_TextBox = new TextBox(false, 8, 8, 260)
                {
                    WatermarkText = "Enter layout name",
                    Parent = this,
                };
                Button save = new Button(218, m_TextBox.Bottom + 8, 50) { Text = "Save", Parent = this };
                save.Clicked += OnSubmit;
                Button cancel = new Button(160, save.Y, 50) { Text = "Cancel", Parent = this };
                cancel.Clicked += OnCancel;
                m_DialogSize = save.BottomRight + new Float2(8);
            }

            public override void OnSubmit()
            {
                string name = m_TextBox.Text.Trim();
                if (string.IsNullOrEmpty(name) || name.IndexOfAny(Path.GetInvalidFileNameChars()) >= 0 || name.EndsWith('.'))
                {
                    MessageBox.Show("Enter a valid layout name.", "Save Window Layout");
                    return;
                }
                string path = Path.Combine(m_Module.LayoutsFolder, "Layout_" + name + ".xml");
                if (File.Exists(path) && MessageBox.Show($"Replace layout '{name}'?", "Save Window Layout",
                    MessageBoxButtons.YesNo, MessageBoxIcon.Question) != DialogResult.Yes)
                {
                    return;
                }
                if (!m_Module.SaveLayout(path))
                {
                    MessageBox.Show("Failed to save the layout. See the log for details.", "Save Window Layout", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }
                base.OnSubmit();
            }
        }

        public override void OnInit()
        {
            m_WindowsLayoutPath = Path.Combine(EngineContext.ProjectCacheFolder, "WindowsLayout.xml");

            m_MainWindow = Editor.MainWindow;
            m_MainWindow.Closing += OnMainWindowClosing;
            m_MainWindow.Closed += OnMainWindowClosed;

            Log = new LogWindow(Editor);
            Content = new ContentWindow(Editor);
            SceneHierarchy = new SceneHierarchyWindow(Editor);
            EditScene = new EditSceneWindow(Editor);
            Properties = new PropertiesWindow(Editor);
            Profiler = new ProfilerWindow(Editor);
        }

        public override void OnEndInit()
        {
            foreach (EditorWindow window in new List<EditorWindow>(m_Windows))
            {
                window.OnInit();
            }

            if (!LoadLayout(m_WindowsLayoutPath))
            {
                LoadDefaultLayout();
            }

            m_LastLayoutSaveTime = DateTime.UtcNow;
        }

        public override void OnUpdate()
        {
            if (m_MainWindow != null && DateTime.UtcNow - m_LastLayoutSaveTime >= TimeSpan.FromSeconds(10))
            {
                SaveCurrentLayout();
            }

            foreach (EditorWindow window in new List<EditorWindow>(m_Windows))
            {
                window.OnUpdate();
            }
        }

        public override void OnDispose()
        {
            if (m_MainWindow != null)
            {
                SaveCurrentLayout();
                m_MainWindow.Closing -= OnMainWindowClosing;
                m_MainWindow.Closed -= OnMainWindowClosed;
                m_MainWindow = null;
            }

            foreach (EditorWindow window in new List<EditorWindow>(m_Windows))
            {
                window.Dispose();
            }

            m_Windows.Clear();
            WindowAdded = null;
            WindowRemoved = null;
        }

        private void OnMainWindowClosing(ClosingReason reason, ref bool cancel)
        {
            SaveCurrentLayout();
        }

        private void OnMainWindowClosed()
        {
            m_MainWindow.Closing -= OnMainWindowClosing;
            m_MainWindow.Closed -= OnMainWindowClosed;
            m_MainWindow = null;
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

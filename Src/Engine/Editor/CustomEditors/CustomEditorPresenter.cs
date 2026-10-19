using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using SE.GUI;

namespace SE.Editor
{
    [Flags]
    public enum FeatureFlags
    {
        None = 0,
        CacheExpandedGroups = 1,
        UsePrefab = 2,
        UseDefault = 4,
    }

    /// <summary>
    /// Owns the selection and editor tree for one properties surface. The owner parents Panel
    /// into a GUI tree for automatic updates; use Update to explicitly flush standalone hosts.
    /// </summary>
    [HideInEditor]
    public class CustomEditorPresenter : LayoutElementsContainer
    {
        public class PresenterPanel : VerticalPanel
        {
            private CustomEditorPresenter m_Presenter;
            public CustomEditorPresenter Presenter => m_Presenter;
            internal PresenterPanel(CustomEditorPresenter presenter) => m_Presenter = presenter;

            public override void Update(float deltaTime)
            {
                if (m_Presenter == null)
                {
                    return;
                }
                try
                {
                    m_Presenter.Update();
                }
                catch (Exception ex)
                {
                    SE.Debug.Log(ex);
                    m_Presenter.BuildLayout();
                }
                base.Update(deltaTime);
            }

            public override void OnDestroy()
            {
                CustomEditorPresenter presenter = m_Presenter;
                m_Presenter = null;
                if (presenter != null)
                {
                    presenter.m_Root.Cleanup();
                    presenter.m_Root.OverrideEditor = null;
                    presenter.Selection.Clear();
                    presenter.Selection.ClearDefaultValue();
                    presenter.Selection.ClearReferenceValue();
                    presenter.Selection.SetType(typeof(object));
                    presenter.ClearLayout();
                    presenter.BeforeLayout = null;
                    presenter.AfterLayout = null;
                    presenter.Modified = null;
                    presenter.SelectionChanged = null;
                }
                base.OnDestroy();
            }
        }

        private readonly RootEditor m_Root;
        private bool m_BuildOnUpdate;
        private bool m_ReadOnly;
        private bool m_BuildingLayout;
        private bool m_Initialized;
        private readonly Dictionary<string, bool> m_GroupClosed = new(StringComparer.Ordinal);
        private string m_GroupStateFile = string.Empty;
        private bool m_GroupStateLoaded;

        public readonly PresenterPanel Panel;
        /// <summary>
        /// Current selection values. Change them through Select or Deselect.
        /// </summary>
        /// 
        public readonly ValueContainer Selection = new();
        public string SearchText = string.Empty;
        public FeatureFlags Features = FeatureFlags.UsePrefab | FeatureFlags.UseDefault;
        public event Action SelectionChanged;
        public event Action Modified;
        public event Action<LayoutElementsContainer> BeforeLayout;
        public event Action<LayoutElementsContainer> AfterLayout;
        public int SelectionCount => Selection.Count;
        public CustomEditor Root => m_Root;
        public bool BuildOnUpdate => m_BuildOnUpdate;

        public bool ReadOnly
        {
            get => m_ReadOnly;
            set
            {
                m_ReadOnly = value;
                Panel.Enabled = !value;
            }
        }

        public override ContainerControl ContainerControl => Panel;

        public CustomEditorPresenter(string noSelectionText = null)
        {
            Panel = new PresenterPanel(this)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Pivot = Float2.Zero,
                IsScrollable = true,
                Spacing = 3.0f,
                Margin = new Margin(3.0f),
            };
            m_Root = new RootEditor();
            NoSelectionText = noSelectionText ?? "No selection";
            BuildLayout();
        }

        public string NoSelectionText { get; set; }

        /// <summary>
        /// Editor instance to use for the whole selection. The presenter initializes and cleans
        /// the supplied instance when it rebuilds the layout.
        /// </summary>
        public CustomEditor OverrideEditor
        {
            get => m_Root.OverrideEditor;
            set
            {
                if (m_Root.OverrideEditor == value)
                {
                    return;
                }
                m_Root.OverrideEditor = value;
                BuildLayout();
            }
        }

        /// <summary>
        /// Selects one object; null clears the selection.
        /// </summary>
        public void Select(object obj)
        {
            if (obj == null)
            {
                Deselect();
                return;
            }

            if (Selection.Count == 1 && Selection[0] == obj)
            {
                return;
            }

            Selection.Clear();
            Selection.Add(obj);
            Selection.SetType(obj.GetType());
            OnSelectionChanged();
        }

        /// <summary>
        /// Selects objects in order; null clears the selection.
        /// </summary>
        public void Select(IEnumerable<object> objects)
        {
            if (objects == null)
            {
                Deselect();
                return;
            }

            if (objects is not object[] array)
            {
                array = objects.ToArray();
            }

            if (array.SequenceEqual(Selection))
            {
                return;
            }

            Selection.Clear();
            Selection.AddRange(array);
            Selection.SetType(array.Length > 0 && !Selection.HasDifferentTypes
                ? array[0]?.GetType() ?? typeof(object) : typeof(object));
            OnSelectionChanged();
        }

        /// <summary>
        /// Clears the selection and its layout.
        /// </summary>
        public void Deselect()
        {
            if (Selection.Count == 0)
            {
                return;
            }
            Selection.Clear();
            Selection.SetType(typeof(object));
            OnSelectionChanged();
        }

        public virtual void BuildLayout()
        {
            float scrollValue = -1;
            Panel parentPanel = Panel.Parent as Panel;
            if (parentPanel != null && parentPanel.vScrollBar != null)
            {
                scrollValue = parentPanel.vScrollBar.GetValue();
            }


            if (m_BuildingLayout || Panel.IsDisposed)
            {
                return;
            }
            m_BuildingLayout = true;
            bool locked = Panel.IsLayoutLocked;
            Panel.IsLayoutLocked = true;
            try
            {
                Panel.DisposeChildren();
                ClearLayout();

                m_BuildOnUpdate = false;
                m_Initialized = true;

                BeforeLayout?.Invoke(this);

                m_Root.Cleanup();
                // The root coordinates editors; only its child presents the selected business object.
                m_Root.Initialize(this, this, null!);

                AfterLayout?.Invoke(this);

            }
            finally
            {
                Panel.IsLayoutLocked = locked;
                m_BuildingLayout = false;
            }

            Panel.PerformLayout();
            if (scrollValue >= 0 && parentPanel != null && parentPanel.vScrollBar != null)
            {
                parentPanel.vScrollBar.SetValue(scrollValue);
            }
            ApplySearchFilter(SearchText);
        }

        public void BuildLayoutOnUpdate() => m_BuildOnUpdate = true;

        public void OnModified() => Modified?.Invoke();

        internal bool GetGroupClosed(string path, bool defaultValue)
        {
            LoadGroupState();
            return m_GroupClosed.TryGetValue(path, out bool closed) ? closed : defaultValue;
        }

        internal void SetGroupClosed(string path, bool closed)
        {
            LoadGroupState();
            m_GroupClosed[path] = closed;

            if (m_GroupStateFile == null)
            {
                return;
            }
            try
            {
                Directory.CreateDirectory(Path.GetDirectoryName(m_GroupStateFile)!);
                File.WriteAllText(m_GroupStateFile, JsonSerializer.Serialize(m_GroupClosed));
            }
            catch (IOException ex)
            {
                SE.Debug.Log(ex.ToString());
            }
            catch (UnauthorizedAccessException ex)
            {
                SE.Debug.Log(ex.ToString());
            }
        }

        private void LoadGroupState()
        {
            if (m_GroupStateLoaded)
            {
                return;
            }
            m_GroupStateLoaded = true;
            string folder = EngineContext.ProjectCacheFolder;
            if (string.IsNullOrEmpty(folder))
            {
                return;
            }
            m_GroupStateFile = Path.Combine(folder, "CustomEditorGroups.json");
            if (!File.Exists(m_GroupStateFile))
            {
                return;
            }
            try
            {
                Dictionary<string, bool> values = JsonSerializer.Deserialize<Dictionary<string, bool>>(File
                    .ReadAllText(m_GroupStateFile));
                if (values != null)
                {
                    foreach (KeyValuePair<string, bool> pair in values)
                    {
                        m_GroupClosed[pair.Key] = pair.Value;
                    }
                }
            }
            catch (IOException ex)
            {
                SE.Debug.Log(ex.ToString());
            }
            catch (JsonException ex)
            {
                SE.Debug.Log(ex.ToString());
            }
        }

        public void OpenAllGroups() => SetAllGroups(this, false);
        public void ClosesAllGroups() => SetAllGroups(this, true);

        private static void SetAllGroups(LayoutElementsContainer container, bool closed)
        {
            if (container is GroupElement group)
            {
                if (closed)
                {
                    group.Panel.Close();
                }
                else
                {
                    group.Panel.Open();
                }
            }

            foreach (LayoutElement child in container.Children)
            {
                if (child is LayoutElementsContainer nested)
                {
                    SetAllGroups(nested, closed);
                }
            }
        }

        protected virtual void OnSelectionChanged()
        {
            if (m_Initialized)
            {
                BuildLayout();
            }
            else
            {
                BuildLayoutOnUpdate();
            }
            SelectionChanged?.Invoke();
        }

        /// <summary>Applies pending edits and refreshes visible values once per editor frame.</summary>
        public void Update()
        {
            if (Panel.IsDisposed || m_BuildingLayout)
            {
                return;
            }
            if (m_BuildOnUpdate)
            {
                BuildLayout();
            }
            if (Selection.Count != 0)
            {
                try { m_Root.RefreshInternal(); }
                catch (System.Reflection.TargetException ex)
                {
                    SE.Debug.Log(ex);
                    BuildLayoutOnUpdate();
                }
            }
            if (m_BuildOnUpdate)
            {
                BuildLayout();
            }
        }

        /// <summary>
        /// Applies search filter query to the presenter layout controls.
        /// </summary>
        public void ApplySearchFilter(string query)
        {
            SearchText = query;
            if (Root == null)
            {
                return;
            }

            bool isQueryEmpty = string.IsNullOrEmpty(query);
            Dictionary<DropPanel, bool> groupMatchCache = new Dictionary<DropPanel, bool>();
            UpdateFilter(Root, query, isQueryEmpty, groupMatchCache);
            UpdatePropertiesListsVisibility(Panel, query);
            UpdateGroupsVisibility(Panel, query);
            Panel.PerformLayout();
        }

        /// <summary>
        /// Updates the visibility of properties lists and drop panels based on the current search query.
        /// </summary>
        public void UpdateGroupsAndListsVisibility()
        {
            if (string.IsNullOrEmpty(SearchText))
            {
                RestoreVisibilities(Panel);
            }
            else
            {
                UpdatePropertiesListsVisibility(Panel, SearchText);
                UpdateGroupsVisibility(Panel, SearchText);
            }
            Panel.PerformLayout();
        }

        private void RestoreVisibilities(Control control)
        {
            if (control is DropPanel dropPanel)
            {
                dropPanel.SearchText = string.Empty;
                dropPanel.Visible = true;
            }
            else if (control is PropertiesList list)
            {
                list.Visible = true;
            }

            if (control is ContainerControl container)
            {
                for (int i = 0; i < container.ChildrenCount; i++)
                {
                    RestoreVisibilities(container.GetChild(i));
                }
            }
        }

        private void UpdatePropertiesListsVisibility(Control control, string query)
        {
            if (control is PropertiesList list)
            {
                if (string.IsNullOrEmpty(query))
                {
                    list.Visible = true;
                }
                else
                {
                    bool anyVisible = false;
                    foreach (PropertyNameLabel label in list.Element.Labels)
                    {
                        if (label.Visible)
                        {
                            anyVisible = true;
                            break;
                        }
                    }
                    list.Visible = anyVisible;
                }
            }

            if (control is ContainerControl container)
            {
                for (int i = 0; i < container.ChildrenCount; i++)
                {
                    UpdatePropertiesListsVisibility(container.GetChild(i), query);
                }
            }
        }

        private void UpdateGroupsVisibility(Control control, string query)
        {
            if (control is DropPanel dropPanel)
            {
                dropPanel.SearchText = query;
                for (int i = 0; i < dropPanel.ChildrenCount; i++)
                {
                    UpdateGroupsVisibility(dropPanel.GetChild(i), query);
                }

                if (string.IsNullOrEmpty(query))
                {
                    dropPanel.Visible = true;
                }
                else
                {
                    dropPanel.Visible = HasVisibleDescendants(dropPanel);
                }
            }
            else if (control is ContainerControl container)
            {
                for (int i = 0; i < container.ChildrenCount; i++)
                {
                    UpdateGroupsVisibility(container.GetChild(i), query);
                }
            }
        }

        private bool HasVisibleDescendants(ContainerControl container)
        {
            for (int i = 0; i < container.ChildrenCount; i++)
            {
                Control child = container.GetChild(i);
                if (child is PropertyNameLabel label && label.Visible)
                {
                    return true;
                }
                if (child is DropPanel dropPanel && dropPanel.Visible)
                {
                    return true;
                }
                if (child is ContainerControl subContainer && subContainer.Visible &&
                    HasVisibleDescendants(subContainer))
                {
                    return true;
                }
            }
            return false;
        }

        private bool IsInMatchingGroup(Control control, string query, Dictionary<DropPanel, bool> groupMatchCache)
        {
            if (string.IsNullOrEmpty(query))
            {
                return false;
            }

            ContainerControl p = control.Parent;
            while (p != null)
            {
                if (p is DropPanel dropPanel)
                {
                    if (!groupMatchCache.TryGetValue(dropPanel, out bool matches))
                    {
                        string headerText = dropPanel.HeaderText;
                        matches = headerText != null &&
                            headerText.IndexOf(query, StringComparison.OrdinalIgnoreCase) >= 0;
                        groupMatchCache[dropPanel] = matches;
                    }
                    if (matches)
                    {
                        return true;
                    }
                }
                p = p.Parent;
            }
            return false;
        }

        private bool UpdateFilter(CustomEditor editor, string query, bool forceVisible,
            Dictionary<DropPanel, bool> groupMatchCache)
        {
            bool isVisible = false;

            bool labelMatches = false;
            if (editor.LinkedLabel != null)
            {
                editor.LinkedLabel.SearchText = query;
                if (forceVisible)
                {
                    labelMatches = true;
                }
                else if (!string.IsNullOrEmpty(query))
                {
                    string labelText = editor.LinkedLabel.Text.ToString();
                    if (labelText.IndexOf(query, StringComparison.OrdinalIgnoreCase) >= 0)
                    {
                        labelMatches = true;
                    }
                    else if (IsInMatchingGroup(editor.LinkedLabel, query, groupMatchCache))
                    {
                        labelMatches = true;
                    }
                }
            }

            bool isThisEditorVisible = forceVisible || labelMatches;

            bool anyChildVisible = false;
            bool forceChildrenVisible = isThisEditorVisible;

            foreach (CustomEditor child in editor.ChildrenEditors)
            {
                if (UpdateFilter(child, query, forceChildrenVisible, groupMatchCache))
                {
                    anyChildVisible = true;
                }
            }

            isVisible = isThisEditorVisible || anyChildVisible;

            if (editor.LinkedLabel != null)
            {
                SetLabelAndControlsVisible(editor.LinkedLabel, isVisible);
            }

            if (editor.Style == DisplayStyle.Group && editor.Layout != null && editor.Layout.Control != null)
            {
                editor.Layout.Control.Visible = isVisible;
            }

            return isVisible;
        }

        private void SetLabelAndControlsVisible(PropertyNameLabel label, bool visible)
        {
            label.Visible = visible;
            PropertiesList container = label.FirstChildControlContainer ?? label.Parent as PropertiesList;
            if (container != null)
            {
                int startIndex = label.FirstChildControlIndex;
                if (startIndex >= 0)
                {
                    int endIndex = container.Children.Count;
                    List<PropertyNameLabel> labels = container.Element.Labels;
                    int labelIndex = labels.IndexOf(label);
                    if (labelIndex >= 0 && labelIndex < labels.Count - 1)
                    {
                        for (int i = labelIndex + 1; i < labels.Count; i++)
                        {
                            PropertiesList nextContainer = labels[i].FirstChildControlContainer ??
                                labels[i].Parent as PropertiesList;
                            if (nextContainer == container)
                            {
                                endIndex = labels[i].FirstChildControlIndex - 1;
                                break;
                            }
                        }
                    }

                    for (int i = startIndex; i < endIndex; i++)
                    {
                        if (i < container.Children.Count)
                        {
                            container.Children[i].Visible = visible;
                        }
                    }
                }
            }
        }

        private sealed class RootEditor : SyncPointEditor
        {
            public CustomEditor OverrideEditor;

            public override void Initialize(LayoutElementsContainer layout)
            {
                Presenter.Selection.ClearReferenceValue();
                if (Presenter.Selection.Count > 0)
                {
                    Type selectionType = Presenter.Selection.HasDifferentTypes
                        ? typeof(object)
                        : Presenter.Selection[0]?.GetType() ?? typeof(object);
                    ValueContainer editorTypeValues = new ValueContainer(selectionType, Presenter.Selection);
                    CustomEditor editor = CustomEditorsUtil.CreateEditor(editorTypeValues, OverrideEditor, false);
                    OnChildCreated(editor);
                    editor.Initialize(Presenter, layout, Presenter.Selection);
                }
                else if (!string.IsNullOrEmpty(Presenter.NoSelectionText))
                {
                    layout.Label(Presenter.NoSelectionText);
                }
                base.Initialize(layout);
            }
        }
    }
}

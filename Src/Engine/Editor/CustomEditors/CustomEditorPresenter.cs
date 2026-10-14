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
    /// into a window and calls Update once per editor frame; use Select or Deselect to rebuild it.
    /// </summary>
    [HideInEditor]
    public class CustomEditorPresenter : LayoutElementsContainer
    {
        private readonly RootEditor _root;
        private bool _buildOnUpdate;
        private bool _readOnly;
        private bool _buildingLayout;
        private bool _initialized;
        private readonly Dictionary<Control, bool> _conditionalVisibility = new();
        private readonly Dictionary<string, bool> _groupClosed = new(StringComparer.Ordinal);
        private string? _groupStateFile;
        private bool _groupStateLoaded;

        public readonly VerticalPanel Panel;
        /// <summary>Current selection values. Change them through Select or Deselect.</summary>
        public readonly ValueContainer Selection = new();
        public string SearchText = string.Empty;
        public FeatureFlags Features = FeatureFlags.UsePrefab | FeatureFlags.UseDefault;
        public event Action? SelectionChanged;
        public event Action? Modified;
        public event Action<LayoutElementsContainer>? BeforeLayout;
        public event Action<LayoutElementsContainer>? AfterLayout;
        public int SelectionCount => Selection.Count;
        public CustomEditor Root => _root;
        public bool BuildOnUpdate => _buildOnUpdate;

        public bool ReadOnly
        {
            get => _readOnly;
            set
            {
                _readOnly = value;
                Panel.Enabled = !value;
            }
        }

        public override ContainerControl ContainerControl => Panel;

        public CustomEditorPresenter(string? noSelectionText = null)
        {
            Panel = new VerticalPanel
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Pivot = Float2.Zero,
                IsScrollable = true,
                Spacing = 3.0f,
                Margin = new Margin(3.0f),
            };
            _root = new RootEditor();
            NoSelectionText = noSelectionText ?? "No selection";
            BuildLayout();
        }

        public string? NoSelectionText { get; set; }

        /// <summary>
        /// Editor instance to use for the whole selection. The presenter initializes and cleans
        /// the supplied instance when it rebuilds the layout.
        /// </summary>
        public CustomEditor? OverrideEditor
        {
            get => _root.OverrideEditor;
            set
            {
                if (ReferenceEquals(_root.OverrideEditor, value))
                    return;
                _root.OverrideEditor = value;
                BuildLayout();
            }
        }

        /// <summary>Selects one object; null clears the selection.</summary>
        public void Select(object? obj)
        {
            if (obj == null)
            {
                Deselect();
                return;
            }
            if (Selection.Count == 1 && Selection[0] == obj)
                return;
            Selection.Clear();
            Selection.Add(obj);
            Selection.SetType(obj.GetType());
            OnSelectionChanged();
        }

        /// <summary>Selects objects in order; null clears the selection.</summary>
        public void Select(IEnumerable<object?>? objects)
        {
            if (objects == null)
            {
                Deselect();
                return;
            }
            object?[] array = objects as object?[] ?? objects.ToArray();
            if (array.SequenceEqual(Selection))
                return;
            Selection.Clear();
            Selection.AddRange(array);
            Selection.SetType(array.GetType());
            OnSelectionChanged();
        }

        /// <summary>Clears the selection and its layout.</summary>
        public void Deselect()
        {
            if (Selection.Count == 0)
                return;
            Selection.Clear();
            Selection.SetType(typeof(object));
            OnSelectionChanged();
        }

        public virtual void BuildLayout()
        {
            float scrollValue = (Panel.Parent as Panel)?.vScrollBar?.GetValue() ?? -1;
            _buildingLayout = true;
            Panel.IsLayoutLocked = true;
            Panel.DisposeChildren();
            ClearLayout();
            _conditionalVisibility.Clear();
            _buildOnUpdate = false;
            _initialized = true;
            BeforeLayout?.Invoke(this);
            _root.Cleanup();
            _root.Initialize(this, this, Selection);
            AfterLayout?.Invoke(this);
            Panel.IsLayoutLocked = false;
            _buildingLayout = false;
            Panel.PerformLayout();
            if (scrollValue >= 0 && Panel.Parent is Panel parentPanel && parentPanel.vScrollBar != null)
                parentPanel.vScrollBar.SetValue(scrollValue);
            ApplySearchFilter(SearchText);
        }

        public void BuildLayoutOnUpdate() => _buildOnUpdate = true;
        public void OnModified() => Modified?.Invoke();
        internal bool GetGroupClosed(string path, bool defaultValue)
        {
            LoadGroupState();
            return _groupClosed.TryGetValue(path, out bool closed) ? closed : defaultValue;
        }

        internal void SetGroupClosed(string path, bool closed)
        {
            LoadGroupState();
            _groupClosed[path] = closed;
            if (_groupStateFile == null)
                return;
            try
            {
                Directory.CreateDirectory(Path.GetDirectoryName(_groupStateFile)!);
                File.WriteAllText(_groupStateFile, JsonSerializer.Serialize(_groupClosed));
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
            if (_groupStateLoaded)
                return;
            _groupStateLoaded = true;
            string folder = EngineContext.ProjectCacheFolder;
            if (string.IsNullOrEmpty(folder))
                return;
            _groupStateFile = Path.Combine(folder, "CustomEditorGroups.json");
            if (!File.Exists(_groupStateFile))
                return;
            try
            {
                var values = JsonSerializer.Deserialize<Dictionary<string, bool>>(File.ReadAllText(_groupStateFile));
                if (values != null)
                    foreach (var pair in values)
                        _groupClosed[pair.Key] = pair.Value;
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
            if (container is LayoutElementsContainer.GroupElement group)
            {
                if (closed) group.Close();
                else group.Open();
            }
            foreach (var child in container.Children)
                if (child is LayoutElementsContainer nested)
                    SetAllGroups(nested, closed);
        }

        protected virtual void OnSelectionChanged()
        {
            if (_initialized)
                BuildLayout();
            else
                BuildLayoutOnUpdate();
            SelectionChanged?.Invoke();
        }

        /// <summary>Applies pending edits and refreshes visible values once per editor frame.</summary>
        public void Update()
        {
            if (_buildOnUpdate)
                BuildLayout();
            if (Selection.Count != 0)
                _root.RefreshInternal();
            if (_buildOnUpdate)
                BuildLayout();
        }

        public void ApplySearchFilter(string query)
        {
            SearchText = query;
            foreach (var child in Panel.Children)
                UpdateFilter(child, Selection.Count == 0 ? string.Empty : query, false);
            Panel.PerformLayout();
        }

        internal void SetConditionalVisible(Control control, bool visible)
        {
            if (_conditionalVisibility.TryGetValue(control, out bool previous) && previous == visible)
                return;
            _conditionalVisibility[control] = visible;
            if (!_buildingLayout)
                ApplySearchFilter(SearchText);
        }

        private bool UpdateFilter(Control control, string query, bool forceShow)
        {
            bool conditionAllows = !_conditionalVisibility.TryGetValue(control, out bool condition) || condition;
            bool collapseAllows = true;
            if (control.Tag is LayoutElementsContainer.GroupElement group &&
                ReferenceEquals(control, group.ContainerControl))
                collapseAllows = !group.IsClosed;
            bool allowed = conditionAllows && collapseAllows;
            if (string.IsNullOrEmpty(query))
            {
                control.Visible = allowed;
                if (control is ContainerControl container)
                    foreach (var child in container.Children)
                        UpdateFilter(child, query, false);
                return control.Visible;
            }
            bool ownMatch = control is PropertyNameLabel label &&
                label.Text.Contains(query, StringComparison.OrdinalIgnoreCase) ||
                control.Tag is LayoutElementsContainer.GroupElement currentGroup &&
                ReferenceEquals(control, currentGroup.Control) &&
                currentGroup.Title.Contains(query, StringComparison.OrdinalIgnoreCase);
            bool childForced = forceShow || ownMatch;
            if (control is ContainerControl parent)
            {
                childForced |= parent.Children.Any(child =>
                    child is PropertyNameLabel directLabel &&
                    directLabel.Text.Contains(query, StringComparison.OrdinalIgnoreCase));
            }
            bool match = ownMatch || forceShow;
            if (control is ContainerControl parent2)
                foreach (var child in parent2.Children)
                    match |= UpdateFilter(child, query, childForced);
            control.Visible = match && allowed;
            return match && conditionAllows;
        }

        private sealed class RootEditor : SyncPointEditor
        {
            public CustomEditor? OverrideEditor;

            public override void Initialize(LayoutElementsContainer layout)
            {
                Presenter.Selection.ClearReferenceValue();
                if (Presenter.Selection.Count > 0)
                {
                    Type selectionType = Presenter.Selection.HasDifferentTypes
                        ? typeof(object)
                        : Presenter.Selection[0]?.GetType() ?? typeof(object);
                    var editorTypeValues = new ValueContainer(selectionType, Presenter.Selection);
                    CustomEditor editor = CustomEditorsUtil.CreateEditor(editorTypeValues, OverrideEditor);
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

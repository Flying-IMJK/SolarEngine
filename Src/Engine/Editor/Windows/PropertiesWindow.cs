using System;
using System.Collections.Generic;
using System.Linq;
using SE.Editor.SceneGraph;
using SE.GUI;

namespace SE.Editor.GUI
{
    public sealed class PropertiesWindow : SceneEditorWindow
    {
        private readonly SearchBox _searchBox;
        private readonly Button _lockButton;
        private readonly Panel _scrollingPanel;
        private readonly Dictionary<Guid, float> _actorScrollValues = new();
        private bool _lockSelection;

        public readonly CustomEditorPresenter Presenter;

        public bool LockSelection
        {
            get => _lockSelection;
            set
            {
                if (_lockSelection == value)
                    return;
                _lockSelection = value;
                _lockButton.Text = value ? "Unlock" : "Lock";
                if (!value)
                    OnSelectionChanged();
            }
        }

        public PropertiesWindow(Editor editor) : base(editor)
        {
            Title = "Properties";
            AutoFocus = true;

            var header = AddChild(new ContainerControl
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                BackgroundColor = Style.Current.Background,
                IsScrollable = false,
                Offsets = new Margin(0, 0, 0, 24)
            });
            _searchBox = header.AddChild(new SearchBox());
            _searchBox.AnchorPreset = AnchorPresets.HorizontalStretchTop;
            _searchBox.Offsets = new Margin(4, 56, 3, 18);
            _lockButton = header.AddChild(new Button
            {
                Text = "Lock",
                TooltipText = "Lock the current selection",
                AnchorPreset = AnchorPresets.TopRight,
                Bounds = new Rectangle(header.Width - 52, 3, 48, 18),
            });
            _lockButton.Clicked += () => LockSelection = !LockSelection;

            _scrollingPanel = AddChild(new Panel());
            _scrollingPanel.AnchorPreset = AnchorPresets.StretchAll;
            _scrollingPanel.Offsets = new Margin(0, 0, header.Bottom, 0);
            _scrollingPanel.SetScrollBars(ScrollBars.Vertical);
            if (_scrollingPanel.vScrollBar != null)
                _scrollingPanel.vScrollBar.ValueChanged += OnScrollValueChanged;

            Presenter = new CustomEditorPresenter();
            Presenter.Features |= FeatureFlags.CacheExpandedGroups;
            Presenter.Panel.Margin = new Margin(0, 0, 3, 3);
            Presenter.Panel.Parent = _scrollingPanel;
            _searchBox.TextChanged += () => Presenter.ApplySearchFilter(_searchBox.Text);
            Presenter.AfterLayout += _ => Presenter.ApplySearchFilter(_searchBox.Text);
            Editor.Scene.SelectionChanged += OnSelectionChanged;
            SE.Level.SceneLoaded += OnSceneLoaded;
        }

        public override void OnInit() => OnSelectionChanged();
        public override void OnUpdate() => Presenter.Update();

        public override void OnExit()
        {
            Editor.Scene.SelectionChanged -= OnSelectionChanged;
            SE.Level.SceneLoaded -= OnSceneLoaded;
        }

        private void OnSelectionChanged()
        {
            if (LockSelection)
                return;
            IEnumerable<object?> objects = Editor.Scene.Selection.Select(node => node.GetEditableObject()).Distinct();
            Presenter.Select(objects);
            if (Editor.Scene.SelectionCount == 1 && _scrollingPanel.vScrollBar != null)
                _scrollingPanel.vScrollBar.SetTargetValue(
                    _actorScrollValues.GetValueOrDefault(Editor.Scene.Selection[0].ID, 0));
        }

        private void OnScrollValueChanged()
        {
            if (Editor.Scene.SelectionCount != 1 || _scrollingPanel.vScrollBar == null)
                return;
            if (_actorScrollValues.Count >= 20)
            {
                int count = 0;
                foreach (Guid key in _actorScrollValues.Keys.ToArray())
                {
                    if (count++ >= 10)
                        break;
                    _actorScrollValues.Remove(key);
                }
            }
            _actorScrollValues[Editor.Scene.Selection[0].ID] = _scrollingPanel.vScrollBar.GetTargetValue();
        }

        private void OnSceneLoaded(SE.Scene scene, Guid sceneId)
        {
            if (SE.Level.ScenesCount > 1)
                return;
            _actorScrollValues.Clear();
            if (LockSelection)
            {
                LockSelection = false;
                Presenter.Deselect();
            }
        }
    }
}

using System;
using System.Collections.Generic;
using System.Linq;
using SE.Editor.SceneGraph;
using SE.Editor.GUI;
using SE.GUI;

namespace SE.Editor
{
    public sealed class PropertiesWindow : SceneEditorWindow
    {
        private readonly SearchBox m_SearchBox;
        private readonly Button m_LockButton;
        private readonly Panel m_ScrollingPanel;
        private readonly Dictionary<Guid, float> m_ActorScrollValues = new();
        private bool m_LockSelection;

        public readonly CustomEditorPresenter Presenter;

        public bool LockSelection
        {
            get => m_LockSelection;
            set
            {
                if (m_LockSelection == value)
                {
                    return;
                }
                m_LockSelection = value;
                m_LockButton.Text = value ? "Unlock" : "Lock";
                if (!value)
                {
                    OnSelectionChanged();
                }
            }
        }

        public PropertiesWindow(Editor editor) : base(editor)
        {
            Title = "Properties";
            AutoFocus = true;
            Icon = EditorIcons.PropertiesWindow32;

            ContainerControl header = AddChild(new ContainerControl
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                BackgroundColor = Style.Current.Background,
                IsScrollable = false,
                Offsets = new Margin(0, 0, 0, 24)
            });
            m_SearchBox = header.AddChild(new SearchBox());
            m_SearchBox.AnchorPreset = AnchorPresets.HorizontalStretchTop;
            m_SearchBox.Offsets = new Margin(4, 56, 3, 18);
            m_LockButton = header.AddChild(new Button
            {
                Text = "Lock",
                TooltipText = "Lock the current selection",
                AnchorPreset = AnchorPresets.TopRight,
                Bounds = new Rectangle(header.Width - 52, 3, 48, 18),
            });
            m_LockButton.Clicked += () => LockSelection = !LockSelection;

            m_ScrollingPanel = AddChild(new Panel());
            m_ScrollingPanel.AnchorPreset = AnchorPresets.StretchAll;
            m_ScrollingPanel.Offsets = new Margin(0, 0, header.Bottom, 0);
            m_ScrollingPanel.SetScrollBars(ScrollBars.Vertical);
            if (m_ScrollingPanel.vScrollBar != null)
            {
                m_ScrollingPanel.vScrollBar.ValueChanged += OnScrollValueChanged;
            }

            Presenter = new CustomEditorPresenter();
            Presenter.Features |= FeatureFlags.CacheExpandedGroups;
            Presenter.Panel.Margin = new Margin(0, 0, 3, 3);
            Presenter.Panel.Parent = m_ScrollingPanel;
            m_SearchBox.TextChanged += () => Presenter.ApplySearchFilter(m_SearchBox.Text);
            Presenter.AfterLayout += _ => Presenter.ApplySearchFilter(m_SearchBox.Text);
            Editor.Scene.SelectionChanged += OnSelectionChanged;
            SE.Level.SceneLoaded += OnSceneLoaded;
        }

        public override void OnInit() => OnSelectionChanged();

        public override void OnExit()
        {
            Editor.Scene.SelectionChanged -= OnSelectionChanged;
            SE.Level.SceneLoaded -= OnSceneLoaded;
        }

        private void OnSelectionChanged()
        {
            if (LockSelection)
            {
                return;
            }
            IEnumerable<object> objects = Editor.Scene.Selection.Select(node => node.GetEditableObject()).Distinct();
            Presenter.Select(objects);
            if (Editor.Scene.SelectionCount == 1 && m_ScrollingPanel.vScrollBar != null)
            {
                m_ScrollingPanel.vScrollBar.SetTargetValue(
                                    m_ActorScrollValues.GetValueOrDefault(Editor.Scene.Selection[0].ID, 0));
            }
        }

        private void OnScrollValueChanged()
        {
            if (Editor.Scene.SelectionCount != 1 || m_ScrollingPanel.vScrollBar == null)
            {
                return;
            }
            if (m_ActorScrollValues.Count >= 20)
            {
                int count = 0;
                foreach (Guid key in m_ActorScrollValues.Keys.ToArray())
                {
                    if (count++ >= 10)
                    {
                        break;
                    }
                    m_ActorScrollValues.Remove(key);
                }
            }
            m_ActorScrollValues[Editor.Scene.Selection[0].ID] = m_ScrollingPanel.vScrollBar.GetTargetValue();
        }

        private void OnSceneLoaded(SE.Scene scene, Guid sceneId)
        {
            if (SE.Level.ScenesCount > 1)
            {
                return;
            }
            m_ActorScrollValues.Clear();
            if (LockSelection)
            {
                LockSelection = false;
                Presenter.Deselect();
            }
        }
    }
}

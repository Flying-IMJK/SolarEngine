// Copyright (c) Wojciech Figat. All rights reserved.

using SE.Editor.GUI;
using SE.Editor.Utilities;
using SE.GUI;

namespace SE.Editor
{
    /// <summary>
    /// Displays custom editor property name.
    /// </summary>
    /// <seealso cref="SE.GUI.Label" />
    [HideInEditor]
    public class PropertyNameLabel : Label
    {
        // TODO: if name is too long to show -> use tooltip to show it

        /// <summary>
        /// Custom event delegate that can be used to extend the property name label with an additional functionality.
        /// </summary>
        /// <param name="label">The label.</param>
        /// <param name="menu">The menu.</param>
        /// <param name="linkedEditor">The linked editor. Can be null.</param>
        public delegate void SetupContextMenuDelegate(PropertyNameLabel label, ContextMenu menu,
            CustomEditor linkedEditor);

        private bool m_MouseDown;

        /// <summary>
        /// Helper value used by the <see cref="PropertiesList"/> to draw property names in a proper area.
        /// </summary>
        internal int FirstChildControlIndex;

        /// <summary>
        /// Helper value used by the <see cref="PropertiesList"/> to draw property names in a proper area.
        /// </summary>
        internal PropertiesList FirstChildControlContainer;

        /// <summary>
        /// The linked custom editor (shows the label property).
        /// </summary>
        internal CustomEditor LinkedEditor;

        /// <summary>
        /// The highlight strip color drawn on a side (transparent if skip rendering).
        /// </summary>
        public Color HighlightStripColor;

        /// <summary>
        /// The active search text query used to highlight matching parts of the label.
        /// </summary>
        public string SearchText = string.Empty;

        private string m_LastSearchText;
        private string m_LastText;
        private QueryFilterHelper.Range[] m_HighlightRanges;

        private void UpdateHighlights()
        {
            string text = Text?.ToString() ?? string.Empty;
            if (m_LastSearchText == SearchText && m_LastText == text)
            {
                return;
            }

            m_LastSearchText = SearchText;
            m_LastText = text;

            if (!string.IsNullOrEmpty(SearchText))
            {
                QueryFilterHelper.Match(SearchText, text, out m_HighlightRanges);
            }
            else
            {
                m_HighlightRanges = null;
            }
        }

        /// <summary>
        /// Occurs when label creates the context menu popup for th property. Can be used to add some custom logic per
        /// property editor.
        /// </summary>
        public event SetupContextMenuDelegate SetupContextMenu;

        /// <summary>
        /// Initializes a new instance of the <see cref="PropertyNameLabel"/> class.
        /// </summary>
        /// <param name="name">The name.</param>
        public PropertyNameLabel(string name)
        {
            Text = name;
            HorizontalAlignment = TextAlignment.Near;
            VerticalAlignment = TextAlignment.Center;
            Margin = new Margin(4, 0, 0, 0);
            ClipText = true;

            HighlightStripColor = Color.Transparent;
        }

        internal void LinkEditor(CustomEditor editor)
        {
            if (LinkedEditor == null)
            {
                LinkedEditor = editor;
                editor.LinkLabel(this);
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            if (HighlightStripColor.A > 0.0f)
            {
                Render2D.FillRectangle(new Rectangle(0, 0, 2, Height), HighlightStripColor);
            }

            UpdateHighlights();

            if (m_HighlightRanges != null && m_HighlightRanges.Length > 0)
            {
                string text = Text.ToString();
                Font font = Font.GetFont();
                if (font != null)
                {
                    Style style = Style.Current;
                    Color color = style.ProgressNormal * 0.6f;
                    Margin margin = Margin;
                    Float2 textSize = font.MeasureText(text, TextLayoutOptions.Default);
                    float textX = margin.Left;
                    float textY = margin.Top + (Height - margin.Height - textSize.Y) * 0.5f;

                    for (int i = 0; i < m_HighlightRanges.Length; i++)
                    {
                        Float2 start = font.GetCharPosition(text, m_HighlightRanges[i].StartIndex, TextLayoutOptions
                            .Default);
                        Float2 end = font.GetCharPosition(text, m_HighlightRanges[i].EndIndex, TextLayoutOptions
                            .Default);
                        Rectangle highlightRect = new Rectangle(start.X + textX, textY, end.X - start.X, textSize.Y);
                        Render2D.FillRectangle(highlightRect, color);
                    }
                }
            }
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            m_MouseDown = false;

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Right)
            {
                m_MouseDown = true;
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (base.OnMouseUp(location, button))
            {
                return true;
            }

            if (m_MouseDown && button == MouseButton.Right)
            {
                m_MouseDown = false;

                // Skip if is not extended
                CustomEditor linkedEditor = LinkedEditor;
                if (linkedEditor == null && SetupContextMenu == null)
                {
                    return false;
                }

                ContextMenu menu = new ContextMenu();

                if (linkedEditor != null)
                {
                    FeatureFlags features = linkedEditor.Presenter.Features;
                    if ((features & (FeatureFlags.UseDefault | FeatureFlags.UsePrefab)) != 0)
                    {
                        if ((features & FeatureFlags.UsePrefab) != 0)
                        {
                            menu.AddButton("Revert to Prefab", linkedEditor.RevertToReferenceValue).Enabled =
                                linkedEditor.CanRevertReferenceValue;
                        }
                        if ((features & FeatureFlags.UseDefault) != 0)
                        {
                            menu.AddButton("Reset to default", linkedEditor.RevertToDefaultValue).Enabled =
                                linkedEditor.CanRevertDefaultValue;
                        }
                        menu.AddSeparator();
                    }
                    menu.AddButton("Copy", linkedEditor.Copy);
                    ContextMenuButton paste = menu.AddButton("Paste", linkedEditor.Paste);
                    paste.Enabled = linkedEditor.CanPaste;
                }

                SetupContextMenu?.Invoke(this, menu, linkedEditor);

                menu.Show(this, location);

                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            m_MouseDown = false;

            base.OnLostFocus();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            SetupContextMenu = null;
            LinkedEditor = null;
            FirstChildControlContainer = null;

            base.OnDestroy();
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return Text.ToString();
        }
    }
}

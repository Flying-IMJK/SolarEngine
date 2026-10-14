#nullable disable annotations

using System;
using SE.Editor.GUI;
using SE.GUI;

namespace SE.Editor
{
    /// <summary>
    /// Displays custom editor property name with support for context menu and visual feedback.
    /// </summary>
    public class PropertyNameLabel : Label
    {
        /// <summary>
        /// Helper value used by the PropertiesList to draw property names in a proper area.
        /// </summary>
        public int FirstChildControlIndex = 0;

        /// <summary>
        /// The linked custom editor that shows the label property.
        /// </summary>
        public CustomEditor LinkedEditor;

        /// <summary>
        /// The highlight strip color drawn on a side, transparent if skipped.
        /// </summary>
        public Color HighlightStripColor = Color.Transparent;

        /// <summary>
        /// Event fired when right-click context menu is requested.
        /// </summary>
        public Action<PropertyNameLabel, ContextMenu, CustomEditor> SetupContextMenu;

        private bool m_mouseDown = false;

        /// <summary>
        /// Initializes a new instance of the PropertyNameLabel class.
        /// </summary>
        /// <param name="name">The property name to display.</param>
        public PropertyNameLabel(string name)
        {
            Text = name;
            HorizontalAlignment = TextAlignment.Near;
            // VerticalAlignment = TextAlignment.Center;
            // Margin = new Margin(4, 0, 0, 0);
            // ClipText = true;

            Name = name;
            Height = 18;
            HighlightStripColor = Color.Transparent;
        }

        /// <summary>Links this label to a custom editor.</summary>
        /// <param name="editor">The editor to link.</param>
        public void LinkEditor(CustomEditor editor)
        {
            if (LinkedEditor == null && editor != null)
            {
                LinkedEditor = editor;
                editor.LinkedLabel = this;
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
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            m_mouseDown = false;
            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Right)
            {
                m_mouseDown = true;
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
            if (m_mouseDown && button == MouseButton.Right)
            {
                m_mouseDown = false;
                if (LinkedEditor == null && SetupContextMenu == null)
                {
                    return false;
                }
                ShowContextMenu(location);
                return true;
            }
            return false;
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            m_mouseDown = false;
            base.OnLostFocus();
        }

        /// <summary>Shows the context menu at the specified location.</summary>
        /// <param name="location">The menu location.</param>
        protected virtual void ShowContextMenu(Float2 location)
        {
            var menu = new ContextMenu();
            if (LinkedEditor != null)
                PopulateEditorActions(menu, LinkedEditor);
            SetupContextMenu?.Invoke(this, menu, LinkedEditor);
            menu.Show(this, location);
        }

        internal static void PopulateEditorActions(ContextMenu menu, CustomEditor editor)
        {
            var revertReference = menu.AddButton("Revert to Prefab", editor.RevertToReferenceValue);
            revertReference.Enabled = editor.CanRevertReferenceValue;
            var revertDefault = menu.AddButton("Reset to default", editor.RevertToDefaultValue);
            revertDefault.Enabled = editor.CanRevertDefaultValue;
            menu.AddSeparator();
            menu.AddButton("Copy", editor.Copy);
            var paste = menu.AddButton("Paste", editor.Paste);
            paste.Enabled = editor.CanPaste;
        }
    }
}

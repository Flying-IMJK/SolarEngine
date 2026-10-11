#nullable disable annotations

using System;

namespace SE.Editor
{
    /// <summary>
    /// Checkable property name label with checkbox functionality.
    /// </summary>
    public class CheckablePropertyNameLabel : PropertyNameLabel
    {
        /// <summary>
        /// Gets or sets whether the label is checked.
        /// </summary>
        public bool Checked = false;

        /// <summary>
        /// Event fired when checked state changes.
        /// </summary>
        public Action<CheckablePropertyNameLabel, bool> CheckedChanged;

        public CheckablePropertyNameLabel(string name) : base(name)
        {
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // Native checkbox drawing is commented out.
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                Checked = !Checked;
                if (CheckedChanged != null)
                {
                    CheckedChanged(this, Checked);
                }
                return true;
            }
            return base.OnMouseUp(location, button);
        }
    }
}

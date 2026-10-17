using System;

using SE.GUI;

namespace SE.Editor.GUI
{

    /// <summary>
    /// ToolStrip separator control.
    /// </summary>
    public class ToolStripSeparator : Control
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ToolStripSeparator"/> class.
        /// </summary>
        /// <param name="height">The height.</param>
        public ToolStripSeparator(float height) : base(0, 0, 4, height)
        {
            AutoFocus = false;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // Draw the separator line
            Render2D.FillRectangle(new Rectangle((Width - 4) / 2, 2, 1, Height - 4), Style.Current.LightBackground * 1.3f);
        }
    }
}
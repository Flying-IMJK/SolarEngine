using System;
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// A data row displayed by <see cref="Table"/>.
    /// </summary>
    public class Row : Control
    {
        public Row(float height = 16.0f)
            : base(new Rectangle(0.0f, 0.0f, 100.0f, MathF.Max(16.0f, height)))
        {
            Depth = -1;
        }

        public object?[]? Values { get; set; }
        public Color[]? BackgroundColors { get; set; }
        public int Depth { get; set; }
        public Table? Table => Parent as Table;

        protected override void OnDraw()
        {
            base.OnDraw();
            Table? table = Table;
            object?[]? values = Values;
            if (table == null || values == null)
                return;

            if (IsMouseOver)
            {
                Rectangle highlight = ScreenBounds;
                Color color = Style.Current.BackgroundHighlighted;
                color.A *= 0.7f;
                Render2D.FillRectangle(ref highlight, ref color);
            }

            int count = Math.Min(values.Length, table.Columns.Count);
            float x = ScreenPos.X;
            for (int index = 0; index < count; index++)
            {
                ColumnDefinition column = table.Columns[index];
                float width = table.GetColumnWidth(index);
                Rectangle cell = new Rectangle(x, ScreenPos.Y, width, Height);
                if (BackgroundColors != null && index < BackgroundColors.Length && BackgroundColors[index].A > 0.0f)
                {
                    Color background = BackgroundColors[index];
                    Render2D.FillRectangle(ref cell, ref background);
                }

                string text = values[index] == null ? string.Empty : column.FormatValue?.Invoke(values[index]!) ?? values[index]!.ToString() ?? string.Empty;
                Font? font = Style.Current.FontMedium;
                if (!ReferenceEquals(font, null) && text.Length != 0)
                {
                    Rectangle textBounds = new Rectangle(cell.X + 4.0f, cell.Y, MathF.Max(0.0f, cell.Width - 8.0f), cell.Height);
                    Color color = Style.Current.Foreground;
                    Render2D.RenderText(font, text, ref textBounds, ref color, column.CellAlignment, TextAlignment.Center, TextWrapping.NoWrap);
                }
                x += width;
            }
        }
    }

    /// <summary>
    /// A row that reports pointer clicks.
    /// </summary>
    public class ClickableRow : Row
    {
        private bool _leftDown;
        private bool _rightDown;

        public event Action<ClickableRow>? DoubleClicked;
        public event Action<ClickableRow>? LeftClicked;
        public event Action<ClickableRow>? RightClicked;

        public override bool OnMouseDown(Float2 location, int button)
        {
            if (button == 1)
                _leftDown = true;
            else if (button == 3)
                _rightDown = true;
            return button == 1 || button == 3;
        }

        public override bool OnMouseUp(Float2 location, int button)
        {
            if (button == 1 && _leftDown)
            {
                _leftDown = false;
                LeftClicked?.Invoke(this);
                return true;
            }
            if (button == 3 && _rightDown)
            {
                _rightDown = false;
                RightClicked?.Invoke(this);
                return true;
            }
            return false;
        }

        public override bool OnMouseDoubleClick(Float2 location, int button)
        {
            if (button != 1)
                return false;
            DoubleClicked?.Invoke(this);
            return true;
        }

        public override void OnMouseLeave()
        {
            _leftDown = false;
            _rightDown = false;
        }
    }
}

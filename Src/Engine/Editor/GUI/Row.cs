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
            VisibleChanged += OnVisibleChanged;
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
                float leftDepthMargin = 0.0f;
                if (column.UseExpandCollapseMode && TryGetFirstChild(table, out Row? child))
                {
                    const float arrowSize = 12.0f;
                    leftDepthMargin = arrowSize * (Depth + 1);
                    DrawExpandCollapseArrow(cell.X + leftDepthMargin - arrowSize, cell.Y + (Height - arrowSize) * 0.5f, arrowSize, child.Visible);
                }

                Font? font = Style.Current.FontMedium;
                if (!ReferenceEquals(font, null) && text.Length != 0)
                {
                    Rectangle textBounds = new Rectangle(cell.X + 4.0f + leftDepthMargin, cell.Y, MathF.Max(0.0f, cell.Width - 8.0f - leftDepthMargin), cell.Height);
                    Color color = Style.Current.Foreground;
                    Render2D.RenderText(font, text, ref textBounds, ref color, column.CellAlignment, TextAlignment.Center, TextWrapping.NoWrap);
                }
                x += width;
            }
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            Table? table = Table;
            if (button == MouseButton.Left && Values != null && table != null)
            {
                float x = 0.0f;
                int count = Math.Min(Values.Length, table.Columns.Count);
                for (int index = 0; index < count; index++)
                {
                    ColumnDefinition column = table.Columns[index];
                    float width = table.GetColumnWidth(index);
                    if (column.UseExpandCollapseMode && TryGetFirstChild(table, out Row? child))
                    {
                        const float arrowSize = 12.0f;
                        float leftDepthMargin = arrowSize * (Depth + 1);
                        Rectangle arrowBounds = new Rectangle(x + leftDepthMargin - arrowSize, (Height - arrowSize) * 0.5f, arrowSize, arrowSize);
                        if (arrowBounds.Contains(location))
                        {
                            SetSubRowsVisible(table, !child.Visible);
                            return true;
                        }
                    }

                    x += width;
                }
            }

            return base.OnMouseUp(location, button);
        }

        private void OnVisibleChanged(Control control)
        {
            _ = control;
            if (!Visible && Table is Table table && Depth != -1)
                SetSubRowsVisible(table, false);
        }

        private bool TryGetFirstChild(Table table, out Row? child)
        {
            int nextIndex = IndexInParent + 1;
            if (nextIndex >= 0 && nextIndex < table.Children.Count && table.Children[nextIndex] is Row nextRow && nextRow.Depth == Depth + 1)
            {
                child = nextRow;
                return true;
            }

            child = null;
            return false;
        }

        private void SetSubRowsVisible(Table table, bool visible)
        {
            for (int index = IndexInParent + 1; index < table.Children.Count; index++)
            {
                if (table.Children[index] is not Row child)
                    continue;

                if (child.Depth == Depth + 1)
                    child.Visible = visible;
                else if (child.Depth <= Depth)
                    break;
            }

            table.PerformLayout();
        }

        private void DrawExpandCollapseArrow(float x, float y, float size, bool expanded)
        {
            float halfSize = size * 0.5f;
            Float2 first;
            Float2 middle;
            Float2 last;
            if (expanded)
            {
                first = new Float2(x + 2.0f, y + 4.0f);
                middle = new Float2(x + halfSize, y + size - 3.0f);
                last = new Float2(x + size - 2.0f, y + 4.0f);
            }
            else
            {
                first = new Float2(x + 4.0f, y + 2.0f);
                middle = new Float2(x + size - 3.0f, y + halfSize);
                last = new Float2(x + 4.0f, y + size - 2.0f);
            }

            Color color = IsMouseOver ? Style.Current.Foreground : Style.Current.ForegroundDisabled;
            Render2D.DrawLine(ref first, ref middle, ref color, 1.0f);
            Render2D.DrawLine(ref middle, ref last, ref color, 1.0f);
        }
    }

    /// <summary>
    /// A row that reports pointer clicks.
    /// </summary>
    public class ClickableRow : Row
    {
        private bool m_LeftDown;
        private bool m_RightDown;

        public event Action<ClickableRow>? DoubleClicked;
        public event Action<ClickableRow>? LeftClicked;
        public event Action<ClickableRow>? RightClicked;

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
                m_LeftDown = true;
            else if (button == MouseButton.Right)
                m_RightDown = true;

            return base.OnMouseDown(location, button);
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && m_LeftDown)
            {
                m_LeftDown = false;
                LeftClicked?.Invoke(this);
            }
            else if (button == MouseButton.Right && m_RightDown)
            {
                m_RightDown = false;
                RightClicked?.Invoke(this);
            }

            return base.OnMouseUp(location, button);
        }

        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            DoubleClicked?.Invoke(this);
            return base.OnMouseDoubleClick(location, button);
        }

        public override void OnMouseLeave()
        {
            m_LeftDown = false;
            m_RightDown = false;
            base.OnMouseLeave();
        }
    }
}

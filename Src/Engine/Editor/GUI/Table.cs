using System;
using System.Collections.Generic;
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// A managed table with resizable column headers and data rows.
    /// </summary>
    public class Table : ContainerControl
    {
        private readonly List<ColumnDefinition> m_Columns = new();
        private float[] m_Splits = Array.Empty<float>();
        private float m_HeaderHeight = 20.0f;
        private int m_MovingSplit = -1;

        public Table()
            : base(new Rectangle(0.0f, 0.0f, 240.0f, 160.0f))
        {
            AutoFocus = false;
        }

        public IReadOnlyList<ColumnDefinition> Columns => m_Columns;
        public IReadOnlyList<float> Splits => m_Splits;

        public float HeaderHeight
        {
            get => m_HeaderHeight;
            set
            {
                float clamped = MathF.Max(1.0f, value);
                if (MathF.Abs(m_HeaderHeight - clamped) <= float.Epsilon)
                    return;
                m_HeaderHeight = clamped;
                PerformLayout();
            }
        }

        public void SetColumns(IEnumerable<ColumnDefinition> columns)
        {
            ArgumentNullException.ThrowIfNull(columns);
            m_Columns.Clear();
            m_Columns.AddRange(columns);
            m_Splits = m_Columns.Count == 0 ? Array.Empty<float>() : CreateEqualSplits(m_Columns.Count);
            PerformLayout();
        }

        public void SetSplits(IReadOnlyList<float> splits)
        {
            ArgumentNullException.ThrowIfNull(splits);
            if (splits.Count != m_Columns.Count)
                throw new ArgumentException("The number of splits must match the number of columns.", nameof(splits));

            m_Splits = new float[splits.Count];
            for (int index = 0; index < m_Splits.Length; index++)
                m_Splits[index] = Math.Clamp(splits[index], 0.0f, 1.0f);
            NormalizeSplits();
            PerformLayout();
        }

        public T AddRow<T>(T row) where T : Row
        {
            return AddChild(row);
        }

        public float GetColumnWidth(int columnIndex)
        {
            if ((uint)columnIndex >= (uint)m_Splits.Length)
                throw new ArgumentOutOfRangeException(nameof(columnIndex));
            return m_Splits[columnIndex] * Width;
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button != MouseButton.Left)
                return false;

            int split = FindSplit(location);
            if (split < 0)
                return false;

            m_MovingSplit = split;
            Root?.StartTrackingMouse(this);
            return true;
        }

        public override void OnMouseMove(Float2 location)
        {
            if (m_MovingSplit < 0 || m_MovingSplit + 1 >= m_Splits.Length || Width <= 0.0f)
                return;

            float left = 0.0f;
            for (int index = 0; index < m_MovingSplit; index++)
                left += m_Splits[index];

            float total = m_Splits[m_MovingSplit] + m_Splits[m_MovingSplit + 1];
            float requested = Math.Clamp(location.X / Width - left, 0.0f, total);
            float first = m_Columns[m_MovingSplit].ClampColumnSize(requested, Width);
            float second = m_Columns[m_MovingSplit + 1].ClampColumnSize(total - first, Width);
            if (first + second > total && second > 0.0f)
                first = Math.Max(0.0f, total - second);

            m_Splits[m_MovingSplit] = first;
            m_Splits[m_MovingSplit + 1] = Math.Max(0.0f, total - first);
            PerformLayout();
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button != MouseButton.Left || m_MovingSplit < 0)
                return false;
            m_MovingSplit = -1;
            Root?.EndTrackingMouse();
            return true;
        }

        public override void ClearState()
        {
            m_MovingSplit = -1;
            base.ClearState();
        }

        public override void Draw()
        {
            base.Draw();
            if (!VisibleInHierarchy || m_Columns.Count == 0)
                return;

            float x = ScreenPos.X;
            for (int index = 0; index < m_Columns.Count; index++)
            {
                ColumnDefinition column = m_Columns[index];
                Rectangle header = new Rectangle(x, ScreenPos.Y, GetColumnWidth(index), HeaderHeight);
                Color background = column.TitleBackgroundColor;
                Render2D.FillRectangle(ref header, ref background);
                Font? font = column.TitleFont ?? Style.Current.FontMedium;
                if (!ReferenceEquals(font, null) && column.Title.Length != 0)
                {
                    Color foreground = column.TitleColor;
                    Render2D.RenderText(font, column.Title, ref header, ref foreground, TextAlignment.Center, TextAlignment.Center, TextWrapping.NoWrap);
                }
                x += header.Width;
            }
        }

        protected override void OnLayoutChildren()
        {
            float y = HeaderHeight;
            for (int index = 0; index < Children.Count; index++)
            {
                Control child = Children[index];
                if (!child.Visible)
                    continue;
                child.SetBounds(0.0f, y, Width, child.Height);
                y += child.Height + 1.0f;
            }
            Height = y;
        }

        private int FindSplit(Float2 location)
        {
            if (location.Y < 0.0f || location.Y > HeaderHeight)
                return -1;

            float x = 0.0f;
            for (int index = 0; index < m_Splits.Length - 1; index++)
            {
                x += GetColumnWidth(index);
                if (MathF.Abs(location.X - x) <= 3.0f)
                    return index;
            }
            return -1;
        }

        private static float[] CreateEqualSplits(int count)
        {
            var result = new float[count];
            float value = 1.0f / count;
            for (int index = 0; index < result.Length; index++)
                result[index] = value;
            return result;
        }

        private void NormalizeSplits()
        {
            float total = 0.0f;
            for (int index = 0; index < m_Splits.Length; index++)
                total += m_Splits[index];
            if (total <= float.Epsilon)
            {
                m_Splits = CreateEqualSplits(m_Splits.Length);
                return;
            }
            for (int index = 0; index < m_Splits.Length; index++)
                m_Splits[index] /= total;
        }
    }
}

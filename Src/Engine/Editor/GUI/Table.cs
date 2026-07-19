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
        private readonly List<ColumnDefinition> _columns = new();
        private float[] _splits = Array.Empty<float>();
        private float _headerHeight = 20.0f;
        private int _movingSplit = -1;

        public Table()
            : base(new Rectangle(0.0f, 0.0f, 240.0f, 160.0f))
        {
            AutoFocus = false;
        }

        public IReadOnlyList<ColumnDefinition> Columns => _columns;
        public IReadOnlyList<float> Splits => _splits;

        public float HeaderHeight
        {
            get => _headerHeight;
            set
            {
                float clamped = MathF.Max(1.0f, value);
                if (MathF.Abs(_headerHeight - clamped) <= float.Epsilon)
                    return;
                _headerHeight = clamped;
                PerformLayout();
            }
        }

        public void SetColumns(IEnumerable<ColumnDefinition> columns)
        {
            ArgumentNullException.ThrowIfNull(columns);
            _columns.Clear();
            _columns.AddRange(columns);
            _splits = _columns.Count == 0 ? Array.Empty<float>() : CreateEqualSplits(_columns.Count);
            PerformLayout();
        }

        public void SetSplits(IReadOnlyList<float> splits)
        {
            ArgumentNullException.ThrowIfNull(splits);
            if (splits.Count != _columns.Count)
                throw new ArgumentException("The number of splits must match the number of columns.", nameof(splits));

            _splits = new float[splits.Count];
            for (int index = 0; index < _splits.Length; index++)
                _splits[index] = Math.Clamp(splits[index], 0.0f, 1.0f);
            NormalizeSplits();
            PerformLayout();
        }

        public T AddRow<T>(T row) where T : Row
        {
            return AddChild(row);
        }

        public float GetColumnWidth(int columnIndex)
        {
            if ((uint)columnIndex >= (uint)_splits.Length)
                throw new ArgumentOutOfRangeException(nameof(columnIndex));
            return _splits[columnIndex] * Width;
        }

        public override bool OnMouseDown(Float2 location, int button)
        {
            if (button != 1)
                return false;

            int split = FindSplit(location);
            if (split < 0)
                return false;

            _movingSplit = split;
            Root?.StartTrackingMouse(this);
            return true;
        }

        public override void OnMouseMove(Float2 location)
        {
            if (_movingSplit < 0 || _movingSplit + 1 >= _splits.Length || Width <= 0.0f)
                return;

            float left = 0.0f;
            for (int index = 0; index < _movingSplit; index++)
                left += _splits[index];

            float total = _splits[_movingSplit] + _splits[_movingSplit + 1];
            float requested = Math.Clamp(location.X / Width - left, 0.0f, total);
            float first = _columns[_movingSplit].ClampColumnSize(requested, Width);
            float second = _columns[_movingSplit + 1].ClampColumnSize(total - first, Width);
            if (first + second > total && second > 0.0f)
                first = Math.Max(0.0f, total - second);

            _splits[_movingSplit] = first;
            _splits[_movingSplit + 1] = Math.Max(0.0f, total - first);
            PerformLayout();
        }

        public override bool OnMouseUp(Float2 location, int button)
        {
            if (button != 1 || _movingSplit < 0)
                return false;
            _movingSplit = -1;
            Root?.EndTrackingMouse();
            return true;
        }

        public override void ClearState()
        {
            _movingSplit = -1;
            base.ClearState();
        }

        public override void Draw()
        {
            base.Draw();
            if (!VisibleInHierarchy || _columns.Count == 0)
                return;

            float x = ScreenPos.X;
            for (int index = 0; index < _columns.Count; index++)
            {
                ColumnDefinition column = _columns[index];
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
            for (int index = 0; index < _splits.Length - 1; index++)
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
            for (int index = 0; index < _splits.Length; index++)
                total += _splits[index];
            if (total <= float.Epsilon)
            {
                _splits = CreateEqualSplits(_splits.Length);
                return;
            }
            for (int index = 0; index < _splits.Length; index++)
                _splits[index] /= total;
        }
    }
}

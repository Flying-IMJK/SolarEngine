using System;
using System.Linq;
using SE.Editor.GUI;
using SE.GUI;

namespace SE.Editor
{
    // Flax's two- and four-component editors use the same one-row grid and
    // leave mixed-value refresh unchanged until ValueBox supports it.
    public abstract class ComponentVectorEditor<TScalar> : CustomEditor
    {
        protected ValueBox<TScalar>[] Components = null!;
        protected abstract int ComponentCount { get; }
        protected virtual int Columns => ComponentCount;
        protected virtual int Rows => 1;
        protected abstract ValueBox<TScalar> CreateBox(LimitAttribute? limit);
        protected abstract TScalar[] ReadComponents(object value);
        protected abstract object CreateValue(TScalar[] values);

        public override DisplayStyle Style => DisplayStyle.Inline;

        public override void Initialize(LayoutElementsContainer layout)
        {
            var grid = new UniformGridPanel
            {
                Parent = layout.ContainerControl,
                Height = TextBox.DefaultHeight * Rows,
                SlotsHorizontally = Columns,
                SlotsVertically = Rows,
                ClipChildren = false,
            };
            var limit = Values.GetAttributes().OfType<LimitAttribute>().FirstOrDefault();
            Components = new ValueBox<TScalar>[ComponentCount];
            for (int i = 0; i < ComponentCount; i++)
            {
                var box = CreateBox(limit);
                box.Parent = grid;
                int index = i;
                box.ValueChanged += () => OnComponentChanged(index);
                box.SlidingEnd += () => OnSlidingEnd(index);
                Components[i] = box;
            }
        }

        protected virtual void OnSlidingEnd(int index) => ClearToken();

        protected virtual void OnComponentChanged(int index)
        {
            if (IsSetBlocked || Values == null)
                return;
            bool sliding = Components.Any(x => x.IsSliding);
            SetValue(CreateValue(Components.Select(x => x.GetValue()).ToArray()), sliding ? this : null);
        }

        public override void Refresh()
        {
            base.Refresh();
            if (HasDifferentValues || Values[0] == null)
                return;
            var values = ReadComponents(Values[0]!);
            for (int i = 0; i < Components.Length; i++)
                Components[i].SetValue(values[i]);
        }
    }

    public abstract class FloatComponentVectorEditor : ComponentVectorEditor<float>
    {
        protected override ValueBox<float> CreateBox(LimitAttribute? limit)
        {
            var box = new FloatValueBox(0);
            if (limit != null) box.SetLimits(limit);
            return box;
        }
    }

    public abstract class DoubleComponentVectorEditor : ComponentVectorEditor<double>
    {
        protected override ValueBox<double> CreateBox(LimitAttribute? limit)
        {
            var box = new DoubleValueBox(0);
            if (limit != null) box.SetLimits(limit);
            return box;
        }
    }

    public abstract class IntComponentVectorEditor : ComponentVectorEditor<int>
    {
        protected override ValueBox<int> CreateBox(LimitAttribute? limit)
        {
            var box = new IntValueBox();
            if (limit != null) box.SetLimits(limit);
            return box;
        }
    }

    [CustomEditor(typeof(Float2)), DefaultEditor]
    public sealed class Float2Editor : FloatComponentVectorEditor
    {
        protected override int ComponentCount => 2;
        protected override float[] ReadComponents(object value) { var v = (Float2)value; return new[] { v.X, v.Y }; }
        protected override object CreateValue(float[] v) => new Float2(v[0], v[1]);
    }

    [CustomEditor(typeof(Double2)), DefaultEditor]
    public sealed class Double2Editor : DoubleComponentVectorEditor
    {
        protected override int ComponentCount => 2;
        protected override double[] ReadComponents(object value) { var v = (Double2)value; return new[] { v.X, v.Y }; }
        protected override object CreateValue(double[] v) => new Double2(v[0], v[1]);
    }

    [CustomEditor(typeof(Int2)), DefaultEditor]
    public sealed class Int2Editor : IntComponentVectorEditor
    {
        protected override int ComponentCount => 2;
        protected override int[] ReadComponents(object value) { var v = (Int2)value; return new[] { v.X, v.Y }; }
        protected override object CreateValue(int[] v) => new Int2(v[0], v[1]);
    }

    [CustomEditor(typeof(Float4)), DefaultEditor]
    public sealed class Float4Editor : FloatComponentVectorEditor
    {
        protected override int ComponentCount => 4;
        protected override float[] ReadComponents(object value) { var v = (Float4)value; return new[] { v.X, v.Y, v.Z, v.W }; }
        protected override object CreateValue(float[] v) => new Float4(v[0], v[1], v[2], v[3]);
    }

    [CustomEditor(typeof(Double4)), DefaultEditor]
    public sealed class Double4Editor : DoubleComponentVectorEditor
    {
        protected override int ComponentCount => 4;
        protected override double[] ReadComponents(object value) { var v = (Double4)value; return new[] { v.X, v.Y, v.Z, v.W }; }
        protected override object CreateValue(double[] v) => new Double4(v[0], v[1], v[2], v[3]);
    }

    [CustomEditor(typeof(Int4)), DefaultEditor]
    public sealed class Int4Editor : IntComponentVectorEditor
    {
        protected override int ComponentCount => 4;
        protected override int[] ReadComponents(object value) { var v = (Int4)value; return new[] { v.X, v.Y, v.Z, v.W }; }
        protected override object CreateValue(int[] v) => new Int4(v[0], v[1], v[2], v[3]);
    }

    [CustomEditor(typeof(Int3)), DefaultEditor]
    public sealed class Int3Editor : IntComponentVectorEditor
    {
        protected override int ComponentCount => 3;
        protected override int[] ReadComponents(object value) { var v = (Int3)value; return new[] { v.X, v.Y, v.Z }; }
        protected override object CreateValue(int[] v) => new Int3(v[0], v[1], v[2]);
    }
}

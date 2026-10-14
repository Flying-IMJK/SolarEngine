using System.Linq;

namespace SE.Editor
{
    [CustomEditor(typeof(Matrix)), DefaultEditor]
    public sealed class MatrixEditor : FloatComponentVectorEditor
    {
        protected override int ComponentCount => 16;
        protected override int Columns => 4;
        protected override int Rows => 4;
        protected override float[] ReadComponents(object value)
        {
            var matrix = (Matrix)value;
            var result = new float[16];
            for (int i = 0; i < result.Length; i++) result[i] = matrix[i];
            return result;
        }
        protected override object CreateValue(float[] values)
        {
            var matrix = new Matrix();
            for (int i = 0; i < values.Length; i++) matrix[i] = values[i];
            return matrix;
        }
    }

    [CustomEditor(typeof(Margin)), DefaultEditor]
    public sealed class MarginEditor : GenericEditor
    {
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);
            var limit = Values.GetAttributes().OfType<LimitAttribute>().FirstOrDefault();
            if (limit == null) return;
            foreach (var child in ChildrenEditors)
                if (child is FloatEditor editor)
                    editor.Element?.SetLimits(limit);
        }
    }
}

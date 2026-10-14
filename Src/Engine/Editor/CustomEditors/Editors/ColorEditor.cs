using SE.Editor.GUI;

namespace SE.Editor
{
    [CustomEditor(typeof(Color)), DefaultEditor]
    public sealed class ColorEditor : CustomEditor
    {
        private ColorValueBox _box = null!;
        public override DisplayStyle Style => DisplayStyle.Inline;

        public override void Initialize(LayoutElementsContainer layout)
        {
            _box = new ColorValueBox { Parent = layout.ContainerControl };
            _box.ValueChanged += () => SetValue(_box.Value, _box.IsSliding ? this : null);
        }

        public override void Refresh()
        {
            base.Refresh();
            if (HasDifferentValues) return;
            object? value = Values[0];
            if (value is Color color) _box.Value = color;
            else if (value is Color32 color32) _box.Value = color32;
            else if (value is Float4 float4) _box.Value = float4;
            else if (value is Double4 double4) _box.Value = (Float4)double4;
        }
    }
}

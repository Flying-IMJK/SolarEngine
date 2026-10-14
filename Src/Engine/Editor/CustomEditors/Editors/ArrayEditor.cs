using System;
using System.Collections;

namespace SE.Editor
{
    [CustomEditor(typeof(Array)), DefaultEditor]
    public sealed class ArrayEditor : CollectionEditor
    {
        public override int Count => (Values[0] as Array)?.Length ?? 0;

        protected override IList Allocate(int size) => Array.CreateInstance(ElementType, size);

        protected override void Resize(int newSize)
        {
            var array = Values[0] as Array;
            int oldSize = array?.Length ?? 0;
            if (oldSize == newSize) return;
            var result = Array.CreateInstance(ElementType, newSize);
            int shared = Math.Min(oldSize, newSize);
            if (array != null && shared > 0)
            {
                Array.Copy(array, result, shared);
                object? fill = ElementType.IsValueType || NotNullItems
                    ? CloneValue(array.GetValue(oldSize - 1))
                    : DefaultValue();
                for (int i = oldSize; i < newSize; i++)
                    result.SetValue(ElementType.IsValueType || NotNullItems ? CloneValue(fill) : fill, i);
            }
            else
            {
                object? fill = DefaultValue();
                for (int i = 0; i < newSize; i++) result.SetValue(fill, i);
            }
            SetValue(result);
        }

        private object? DefaultValue() => ElementType.IsValueType ? Activator.CreateInstance(ElementType) : null;

        protected override IList CloneValues()
        {
            var source = Values[0] as Array;
            var result = Array.CreateInstance(ElementType, source?.Length ?? 0);
            if (source != null) Array.Copy(source, result, source.Length);
            return result;
        }
    }
}

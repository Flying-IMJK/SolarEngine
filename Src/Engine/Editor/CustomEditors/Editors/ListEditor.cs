using System;
using System.Collections;
using System.Collections.Generic;

namespace SE.Editor
{
    [CustomEditor(typeof(List<>)), DefaultEditor]
    public sealed class ListEditor : CollectionEditor
    {
        public override int Count => (Values[0] as IList)?.Count ?? 0;

        protected override IList Allocate(int size)
        {
            var list = (IList)Activator.CreateInstance(Values.Type)!;
            object? fill = ElementType.IsValueType ? Activator.CreateInstance(ElementType) : null;
            for (int i = 0; i < size; i++) list.Add(fill);
            return list;
        }

        protected override void Resize(int newSize)
        {
            var list = Values[0] as IList;
            int oldSize = list?.Count ?? 0;
            if (oldSize == newSize) return;
            var result = (IList)Activator.CreateInstance(Values.Type)!;
            int shared = Math.Min(oldSize, newSize);
            if (list != null && shared > 0)
            {
                for (int i = 0; i < shared; i++) result.Add(list[i]);
                object? fill = ElementType.IsValueType || NotNullItems
                    ? list[oldSize - 1]
                    : DefaultValue();
                for (int i = oldSize; i < newSize; i++) result.Add(fill);
            }
            else
            {
                object? fill = DefaultValue();
                for (int i = 0; i < newSize; i++) result.Add(fill);
            }
            SetValue(result);
        }

        private object? DefaultValue() => ElementType.IsValueType ? Activator.CreateInstance(ElementType) : null;

        protected override IList CloneValues()
        {
            var result = (IList)Activator.CreateInstance(Values.Type)!;
            if (Values[0] is IList source)
            {
                foreach (var item in source)
                {
                    result.Add(item);
                }
            }
            return result;
        }
    }
}

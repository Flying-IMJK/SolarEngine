using System;
using System.Collections;

namespace SE.Editor
{
    [HideInEditor]
    public sealed class ListValueContainer : ValueContainer
    {
        private readonly object[]? _attributes;
        public readonly int Index;

        public ListValueContainer(Type elementType, int index, ValueContainer values, object[]? attributes = null)
            : base(elementType)
        {
            Index = index;
            _attributes = attributes;
            Capacity = values.Count;
            foreach (var value in values)
                Add(((IList)value!)[index]);
            if (values.ReferenceValue is IList reference && reference.Count > index)
            {
                _referenceValue = reference[index];
                _hasReferenceValue = true;
            }
        }

        public override object[] GetAttributes() => _attributes ?? base.GetAttributes();

        public override void Refresh(ValueContainer instanceValues)
        {
            if (instanceValues.Count != Count) throw new ArgumentException();
            for (int i = 0; i < Count; i++) this[i] = ((IList)instanceValues[i]!)[Index];
        }

        public override void Set(ValueContainer instanceValues, object? value)
        {
            if (instanceValues.Count != Count) throw new ArgumentException();
            for (int i = 0; i < Count; i++)
            {
                ((IList)instanceValues[i]!)[Index] = value;
                this[i] = value;
            }
        }

        public override void Set(ValueContainer instanceValues, ValueContainer values)
        {
            if (instanceValues.Count != Count || values.Count != Count) throw new ArgumentException();
            for (int i = 0; i < Count; i++)
            {
                ((IList)instanceValues[i]!)[Index] = values[i];
                this[i] = values[i];
            }
        }

        public override void Set(ValueContainer instanceValues)
        {
            if (instanceValues.Count != Count) throw new ArgumentException();
            for (int i = 0; i < Count; i++) ((IList)instanceValues[i]!)[Index] = this[i];
        }

        public override void RefreshReferenceValue(object? instanceValue)
        {
            if (instanceValue is IList list && list.Count > Index)
            {
                _referenceValue = list[Index];
                _hasReferenceValue = true;
            }
        }
    }
}

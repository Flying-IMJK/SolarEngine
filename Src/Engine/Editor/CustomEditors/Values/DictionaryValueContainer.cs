using System;
using System.Collections;

namespace SE.Editor
{
    [HideInEditor]
    public sealed class DictionaryValueContainer : ValueContainer
    {
        private readonly object[]? _attributes;
        public readonly object Key;

        public DictionaryValueContainer(Type valueType, object key, ValueContainer values, object[]? attributes = null)
            : base(valueType)
        {
            Key = key;
            _attributes = attributes;
            Capacity = values.Count;
            foreach (var value in values) Add(((IDictionary)value!)[key]);
            if (values.ReferenceValue is IDictionary reference && reference.Contains(key))
            {
                _referenceValue = reference[key];
                _hasReferenceValue = true;
            }
        }

        public override object[] GetAttributes() => _attributes ?? base.GetAttributes();

        public override void Refresh(ValueContainer instanceValues)
        {
            if (instanceValues.Count != Count) throw new ArgumentException();
            for (int i = 0; i < Count; i++) this[i] = ((IDictionary)instanceValues[i]!)[Key];
        }

        public override void Set(ValueContainer instanceValues, object? value)
        {
            if (instanceValues.Count != Count) throw new ArgumentException();
            for (int i = 0; i < Count; i++)
            {
                ((IDictionary)instanceValues[i]!)[Key] = value;
                this[i] = value;
            }
        }

        public override void Set(ValueContainer instanceValues, ValueContainer values)
        {
            if (instanceValues.Count != Count || values.Count != Count) throw new ArgumentException();
            for (int i = 0; i < Count; i++)
            {
                ((IDictionary)instanceValues[i]!)[Key] = values[i];
                this[i] = values[i];
            }
        }

        public override void Set(ValueContainer instanceValues)
        {
            if (instanceValues.Count != Count) throw new ArgumentException();
            for (int i = 0; i < Count; i++) ((IDictionary)instanceValues[i]!)[Key] = this[i];
        }

        public override void RefreshReferenceValue(object? instanceValue)
        {
            if (instanceValue is IDictionary reference && reference.Contains(Key))
            {
                _referenceValue = reference[Key];
                _hasReferenceValue = true;
            }
        }
    }
}

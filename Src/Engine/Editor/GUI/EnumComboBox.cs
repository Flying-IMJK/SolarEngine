using System;
using System.Collections.Generic;

namespace SE.Editor.GUI
{
    /// <summary>
    /// A <see cref="ComboBox"/> that maps selections to a managed enum value.
    /// </summary>
    public class EnumComboBox : ComboBox
    {
        public readonly struct Entry
        {
            public Entry(string name, long value, string? tooltip = null)
            {
                Name = name;
                Value = value;
                Tooltip = tooltip;
            }

            public string Name { get; }
            public long Value { get; }
            public string? Tooltip { get; }
        }

        private readonly List<Entry> _entries = new();
        private long _value;

        public EnumComboBox(Type enumType, float x = 0.0f, float y = 0.0f, float width = 120.0f)
            : base(x, y, width)
        {
            if (enumType == null)
                throw new ArgumentNullException(nameof(enumType));
            if (!enumType.IsEnum)
                throw new ArgumentException("The type must be an enum.", nameof(enumType));

            EnumType = enumType;
            IsFlags = enumType.IsDefined(typeof(FlagsAttribute), false);
            SupportMultiSelect = IsFlags;
            BuildEntries();
            SelectedIndexChanged += _ => CacheValue();
        }

        public event Action<EnumComboBox>? ValueChanged;
        public Type EnumType { get; }
        public bool IsFlags { get; }
        public IReadOnlyList<Entry> Entries => _entries;

        public long Value
        {
            get => _value;
            set
            {
                if (_value == value && HasSelection)
                    return;
                ApplyValue(value);
            }
        }

        public object EnumTypeValue
        {
            get => Enum.ToObject(EnumType, Value);
            set => Value = Convert.ToInt64(value);
        }

        private void BuildEntries()
        {
            foreach (string name in Enum.GetNames(EnumType))
            {
                object raw = Enum.Parse(EnumType, name);
                long value = Convert.ToInt64(raw);
                _entries.Add(new Entry(name, value));
                AddItem(name);
            }
            if (_entries.Count > 0)
                ApplyValue(_entries[0].Value);
        }

        private void ApplyValue(long value)
        {
            _value = value;
            if (!IsFlags)
            {
                int selected = _entries.FindIndex(entry => entry.Value == value);
                SelectedIndex = selected;
                if (selected < 0)
                    ValueChanged?.Invoke(this);
                return;
            }

            var selection = new List<int>();
            for (int index = 0; index < _entries.Count; index++)
            {
                long entry = _entries[index].Value;
                if (entry != 0 && (entry & value) == entry)
                    selection.Add(index);
                else if (entry == 0 && value == 0)
                    selection.Add(index);
            }
            SetSelection(selection);
            ValueChanged?.Invoke(this);
        }

        private void CacheValue()
        {
            long value = 0;
            foreach (int index in Selection)
                value |= _entries[index].Value;
            if (_value == value)
                return;
            _value = value;
            ValueChanged?.Invoke(this);
        }
    }
}

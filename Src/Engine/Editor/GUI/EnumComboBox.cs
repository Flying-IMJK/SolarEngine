using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Reflection;

namespace SE.Editor.GUI
{
    /// <summary>Combo box for enum values, including combined flags.</summary>
    public class EnumComboBox : ComboBox
    {
        private readonly Type _enumType;
        private readonly List<Entry> _entries = new();
        private long _cachedValue;
        private bool _hasValueCached;

        private readonly struct Entry
        {
            public readonly string Name;
            public readonly string? Tooltip;
            public readonly long Value;

            public Entry(string name, long value, string? tooltip)
            {
                Name = name;
                Value = value;
                Tooltip = tooltip;
            }
        }

        public bool IsFlags { get; }
        public event Action? ValueChanged;

        public object EnumTypeValue
        {
            get => Enum.ToObject(_enumType, Value);
            set => Value = Convert.ToInt64(value);
        }

        public long Value
        {
            get => _cachedValue;
            set
            {
                if (_cachedValue == value && _hasValueCached)
                    return;
                for (int i = 0; i < _entries.Count; i++)
                {
                    if (_entries[i].Value == value)
                    {
                        SelectedIndex = i;
                        return;
                    }
                }
                if (IsFlags)
                {
                    var selection = new List<int>();
                    for (int i = 0; i < _entries.Count; i++)
                    {
                        long entryValue = _entries[i].Value;
                        if (entryValue != 0 && (entryValue & value) == entryValue)
                            selection.Add(i);
                    }
                    Selection = selection;
                }
                else
                    SelectedIndex = -1;
            }
        }

        public EnumComboBox(Type type, EnumDisplayAttribute.FormatMode formatMode = EnumDisplayAttribute.FormatMode.Default)
        {
            ArgumentNullException.ThrowIfNull(type);
            if (!type.IsEnum)
                throw new ArgumentException($"Invalid enum type {type.FullName}.", nameof(type));
            _enumType = type;
            IsFlags = type.GetCustomAttribute<FlagsAttribute>() != null;
            SupportMultiSelect = IsFlags;
            MaximumItemsInViewCount = 15;
            SelectedIndexChanged += _ =>
            {
                CacheValue();
                ValueChanged?.Invoke();
            };

            BuildEntries(type, formatMode);
            bool hasTooltips = false;
            foreach (Entry entry in _entries)
            {
                Items.Add(entry.Name);
                Tooltips.Add(entry.Tooltip ?? string.Empty);
                hasTooltips |= entry.Tooltip != null;
            }
            if (!hasTooltips)
                Tooltips.Clear();
        }

        private void BuildEntries(Type type, EnumDisplayAttribute.FormatMode formatMode)
        {
            if (formatMode == EnumDisplayAttribute.FormatMode.Default &&
                type.GetCustomAttribute<EnumDisplayAttribute>() is { } typeDisplay)
                formatMode = typeDisplay.Mode;
            foreach (FieldInfo field in type.GetFields())
            {
                if (field.Name == "value__" || field.IsDefined(typeof(HideInEditorAttribute), false))
                    continue;
                string name = field.GetCustomAttribute<EditorDisplayAttribute>()?.Name ??
                    (formatMode == EnumDisplayAttribute.FormatMode.None
                        ? field.Name
                        : GenericEditor.ItemInfo.GetPropertyNameUI(field.Name));
                string? tooltip = field.GetCustomAttribute<TooltipAttribute>()?.Text ??
                    field.GetCustomAttribute<DescriptionAttribute>()?.Description;
                _entries.Add(new Entry(name, Convert.ToInt64(field.GetRawConstantValue()), tooltip));
            }
        }

        private void CacheValue()
        {
            long value = 0;
            if (IsFlags)
            {
                foreach (int index in Selection)
                    value |= _entries[index].Value;
            }
            else if (SelectedIndex >= 0)
                value = _entries[SelectedIndex].Value;
            _cachedValue = value;
            _hasValueCached = true;
        }

        protected override void OnLayoutMenuButton(ContextMenuButton button, int index, bool construct = false)
        {
            base.OnLayoutMenuButton(button, index, construct);
            if (IsFlags)
                button.CloseMenuOnClick = false;
        }

        protected override void OnItemClicked(int index)
        {
            if (!IsFlags)
            {
                base.OnItemClicked(index);
                return;
            }
            if (_entries[index].Value == 0)
            {
                SelectedIndex = index;
                return;
            }
            long selectedValue = _entries[index].Value;
            bool isSelected = _selectedIndices.Contains(index);
            long valueAfter = 0;
            foreach (int selectedIndex in _selectedIndices)
            {
                if (selectedIndex != index &&
                    (isSelected || (_entries[selectedIndex].Value & selectedValue) == 0))
                    valueAfter |= _entries[selectedIndex].Value;
            }
            if (!isSelected)
                valueAfter |= selectedValue;
            if (Value == valueAfter)
                return;
            for (int i = 0; i < _entries.Count; i++)
            {
                if (_entries[i].Value == valueAfter)
                {
                    SelectedIndex = i;
                    return;
                }
            }
            _selectedIndices.Clear();
            for (int i = 0; i < _entries.Count; i++)
            {
                long entryValue = _entries[i].Value;
                if (entryValue != 0 && (entryValue & valueAfter) == entryValue)
                    _selectedIndices.Add(i);
            }
            OnSelectedIndexChanged();
        }
    }
}

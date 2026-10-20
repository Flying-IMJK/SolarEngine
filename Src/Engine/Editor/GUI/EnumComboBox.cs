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
        private readonly Type m_EnumType;
        private readonly List<Entry> m_Entries = new();
        private long m_CachedValue;
        private bool m_HasValueCached;

        private readonly struct Entry
        {
            public readonly string Name;
            public readonly string Tooltip;
            public readonly long Value;

            public Entry(string name, long value, string tooltip)
            {
                Name = name;
                Value = value;
                Tooltip = tooltip;
            }
        }

        public bool IsFlags { get; }
        public event Action ValueChanged;

        public object EnumTypeValue
        {
            get => Enum.ToObject(m_EnumType, Value);
            set => Value = Convert.ToInt64(value);
        }

        public long Value
        {
            get => m_CachedValue;
            set
            {
                if (m_CachedValue == value && m_HasValueCached)
                {
                    return;
                }
                for (int i = 0; i < m_Entries.Count; i++)
                {
                    if (m_Entries[i].Value == value)
                    {
                        SelectedIndex = i;
                        return;
                    }
                }
                if (IsFlags)
                {
                    List<int> selection = new List<int>();
                    for (int i = 0; i < m_Entries.Count; i++)
                    {
                        long entryValue = m_Entries[i].Value;
                        if (entryValue != 0 && (entryValue & value) == entryValue)
                        {
                            selection.Add(i);
                        }
                    }
                    Selection = selection;
                }
                else
                {
                    SelectedIndex = -1;
                }
            }
        }

        public EnumComboBox(Type type, EnumDisplayAttribute.FormatMode formatMode = EnumDisplayAttribute.FormatMode
            .Default)
        {
            ArgumentNullException.ThrowIfNull(type);
            if (!type.IsEnum)
            {
                throw new ArgumentException($"Invalid enum type {type.FullName}.", nameof(type));
            }
            m_EnumType = type;
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
            foreach (Entry entry in m_Entries)
            {
                Items.Add(entry.Name);
                Tooltips.Add(entry.Tooltip ?? string.Empty);
                hasTooltips |= entry.Tooltip != null;
            }
            if (!hasTooltips)
            {
                Tooltips.Clear();
            }
        }

        private void BuildEntries(Type type, EnumDisplayAttribute.FormatMode formatMode)
        {
            if (formatMode == EnumDisplayAttribute.FormatMode.Default &&
                type.GetCustomAttribute<EnumDisplayAttribute>() is { } typeDisplay)
            {
                formatMode = typeDisplay.Mode;
            }
            foreach (FieldInfo field in type.GetFields())
            {
                // A flags enum's MAX sentinel is not a selectable bit.
                if (field.Name == "value__" || IsFlags && field.Name == "MAX" ||
                    field.IsDefined(typeof(HideInEditorAttribute), false))
                {
                    continue;
                }
                string name = field.GetCustomAttribute<EditorDisplayAttribute>()?.Name ??
                    (formatMode == EnumDisplayAttribute.FormatMode.None
                        ? field.Name
                        : GenericEditor.ItemInfo.GetPropertyNameUI(field.Name));
                string tooltip = field.GetCustomAttribute<TooltipAttribute>()?.Text ??
                    field.GetCustomAttribute<DescriptionAttribute>()?.Description;
                m_Entries.Add(new Entry(name, Convert.ToInt64(field.GetRawConstantValue()), tooltip));
            }
        }

        private void CacheValue()
        {
            long value = 0;
            if (IsFlags)
            {
                foreach (int index in Selection)
                {
                    value |= m_Entries[index].Value;
                }
            }
            else if (SelectedIndex >= 0)
            {
                value = m_Entries[SelectedIndex].Value;
            }
            m_CachedValue = value;
            m_HasValueCached = true;
        }

        protected override void OnLayoutMenuButton(ContextMenuButton button, int index, bool construct = false)
        {
            base.OnLayoutMenuButton(button, index, construct);
            if (IsFlags)
            {
                button.CloseMenuOnClick = false;
            }
        }

        protected override void OnItemClicked(int index)
        {
            if (!IsFlags)
            {
                base.OnItemClicked(index);
                return;
            }
            if (m_Entries[index].Value == 0)
            {
                SelectedIndex = index;
                return;
            }
            long selectedValue = m_Entries[index].Value;
            // Toggle the bits in the current value, including when it is shown as a combined entry.
            long currentValue = Value;
            long valueAfter = (currentValue & selectedValue) == selectedValue
                ? currentValue & ~selectedValue
                : currentValue | selectedValue;
            if (Value == valueAfter)
            {
                return;
            }
            for (int i = 0; i < m_Entries.Count; i++)
            {
                if (m_Entries[i].Value == valueAfter)
                {
                    SelectedIndex = i;
                    return;
                }
            }
            _selectedIndices.Clear();
            for (int i = 0; i < m_Entries.Count; i++)
            {
                long entryValue = m_Entries[i].Value;
                if (entryValue != 0 && (entryValue & valueAfter) == entryValue)
                {
                    _selectedIndices.Add(i);
                }
            }
            OnSelectedIndexChanged();
        }
    }
}

using System;
using System.Collections;
using System.Linq;
using System.Text.Json;
using SE.Editor.GUI;
using SE.GUI;

namespace SE.Editor
{
    [CustomEditor(typeof(System.Collections.Generic.Dictionary<,>)), DefaultEditor]
    public class DictionaryEditor : CustomEditor
    {
        private IntValueBox m_SizeBox;
        private int m_ElementsCount;
        private bool m_ReadOnly;
        private bool m_NotNullItems;
        private bool m_CanEditKeys;
        private bool m_KeyEdited;

        public int Count => (Values[0] as IDictionary)?.Count ?? 0;

        public override void Initialize(LayoutElementsContainer layout)
        {
            if (HasDifferentValues || HasDifferentTypes)
            {
                return;
            }
            Type type = Values.Type;
            Type[] arguments = type.GetGenericArguments();
            Type keyType = arguments[0], valueType = arguments[1];
            object[] attributes = Values.GetAttributes();
            CollectionAttribute meta = attributes.OfType<CollectionAttribute>().FirstOrDefault();
            m_ReadOnly = meta?.ReadOnly ?? false;
            m_NotNullItems = meta?.NotNullItems ?? false;
            m_CanEditKeys = keyType == typeof(string) || keyType.IsPrimitive || keyType.IsEnum || keyType.IsValueType;
            if (meta != null)
            {
                m_CanEditKeys &= meta.CanReorderItems;
            }
            if (attributes.Any(x => x is SE.ReadOnlyAttribute))
            {
                m_ReadOnly = true;
                m_CanEditKeys = false;
            }

            int size = Count;
            UniformGridPanel sizeRow = new UniformGridPanel
            {
                Parent = layout.ContainerControl,
                Height = TextBox.DefaultHeight,
                SlotsHorizontally = 4,
                SlotsVertically = 1,
                ClipChildren = false,
            };
            new Label { Text = "Size", Parent = sizeRow, Height = TextBox.DefaultHeight };
            m_SizeBox = new IntValueBox(size) { Parent = sizeRow };
            m_SizeBox.SetLimits(0, m_NotNullItems ? size : ushort.MaxValue);
            m_SizeBox.IsReadOnly = m_ReadOnly || !m_CanEditKeys;
            m_SizeBox.EditEnd += OnSizeChanged;
            Button add = new Button
            {
                Text = "+",
                Parent = sizeRow,
                Enabled = !m_ReadOnly && m_CanEditKeys && !m_NotNullItems
            };
            add.Clicked += () => Resize(Count + 1);
            Button remove = new Button
            {
                Text = "-",
                Parent = sizeRow,
                Enabled = !m_ReadOnly && m_CanEditKeys && size > 0
            };
            remove.Clicked += () => Resize(Count - 1);

            if (Values[0] is IDictionary dictionary)
            {
                VerticalPanelElement itemsLayout = layout.VerticalPanel();
                itemsLayout.ContainerControl.BackgroundColor = meta?.BackgroundColor ??
                    SE.GUI.Style.Current.CollectionBackgroundColor;
                Type overrideType = meta?.OverrideEditorTypeName == null
                    ? null : CustomEditorsUtil.ResolveEditorAlias(meta.OverrideEditorTypeName);
                object[] keys = dictionary.Keys.Cast<object>().ToArray();
                for (int i = 0; i < keys.Length; i++)
                {
                    if (i > 0 && meta?.Spacing > 0)
                    {
                        itemsLayout.Space(meta.Spacing);
                    }
                    object key = keys[i];
                    CustomEditor overrideEditor = overrideType != null
                        ? Activator.CreateInstance(overrideType) as CustomEditor : null;
                    DictionaryValueContainer itemValues = new DictionaryValueContainer(valueType, key, Values,
                        attributes);
                    CustomEditor itemEditor = itemsLayout.Property(key?.ToString() ?? "<null>", itemValues,
                        overrideEditor);
                    if (m_ReadOnly)
                    {
                        itemEditor.SetReadOnly(true);
                    }
                    if (itemEditor.LinkedLabel != null)
                    {
                        PropertyNameLabel label = itemEditor.LinkedLabel;
                        label.SetupContextMenu += (_, menu, _) => AddKeyActions(menu, label, key, keyType);
                    }
                    else if (itemEditor.Layout is GroupElement group)
                    {
                        group.SetupContextMenu += (menu, _) => AddKeyActions(menu, group.Control, key, keyType);
                    }
                }
            }
            m_ElementsCount = size;
        }

        private void AddKeyActions(ContextMenu menu, Control target, object key, Type keyType)
        {
            menu.AddSeparator();
            menu.AddButton("Remove", () => Remove(key)).Enabled = !m_ReadOnly;
            menu.AddButton("Edit", () => EditKey(target, key, keyType)).Enabled = !m_ReadOnly && m_CanEditKeys;
        }

        private void EditKey(Control target, object oldKey, Type keyType)
        {
            if (m_ReadOnly || !m_CanEditKeys)
            {
                return;
            }
            Rectangle area = new Rectangle(0, 0, target.Width, Math.Max(target.Height, TextBox.DefaultHeight));
            if (keyType == typeof(string) || keyType.IsPrimitive)
            {
                RenamePopup popup = RenamePopup.ShowPopup(target, area, oldKey?.ToString() ?? string.Empty, false);
                popup.Validate += (RenamePopup _, ref string input) =>
                    TryParseKey(input, keyType, out object parsed) &&
                    parsed != null && !(Values[0] as IDictionary)!.Contains(parsed);
                popup.Renamed += p =>
                {
                    if (TryParseKey(p.Text, keyType, out object newKey) && newKey != null)
                    {
                        ChangeKey(oldKey, newKey);
                    }
                };
            }
            else if (keyType.IsEnum)
            {
                ContextMenuBase popup = ContextMenuBase.ShowEmptyMenu(target, area);
                EnumComboBox picker = new EnumComboBox(keyType)
                {
                    AnchorPreset = AnchorPresets.StretchAll,
                    Offsets = Margin.Zero,
                    Parent = popup,
                    EnumTypeValue = oldKey,
                };
                picker.ValueChanged += () =>
                {
                    popup.Hide();
                    ChangeKey(oldKey, picker.EnumTypeValue);
                };
            }
            else
            {
                DictionaryKeyPopup popup = new DictionaryKeyPopup(oldKey, newKey => ChangeKey(oldKey, newKey))
                {
                    Size = new Float2(Math.Max(target.Width, 180), 80),
                };
                popup.Show(target, Float2.Zero);
            }
        }

        private static bool TryParseKey(string text, Type type, out object key)
        {
            try
            {
                key = type == typeof(string) ? text : JsonSerializer.Deserialize(text, type);
                return key != null;
            }
            catch
            {
                key = null;
                return false;
            }
        }

        private sealed class DictionaryKeyPopup : ContextMenuBase
        {
            private readonly CustomEditorPresenter m_Presenter = new(null);
            private readonly Action<object> m_OnChanged;
            public DictionaryKeyPopup(object key, Action<object> onChanged)
            {
                m_OnChanged = onChanged;
                m_Presenter.Panel.AnchorPreset = AnchorPresets.StretchAll;
                m_Presenter.Panel.IsScrollable = false;
                m_Presenter.Panel.Parent = this;
                m_Presenter.Select(key);
                m_Presenter.Modified += () =>
                {
                    if (m_Presenter.Selection[0] is { } updated)
                    {
                        m_OnChanged(updated);
                    }
                    Hide();
                };
            }
        }

        private void OnSizeChanged()
        {
            if (IsSetBlocked || m_SizeBox == null || m_ReadOnly || !m_CanEditKeys)
            {
                return;
            }
            Resize(m_SizeBox.GetValue());
        }

        private void Remove(object key)
        {
            if (IsSetBlocked || m_ReadOnly)
            {
                return;
            }
            IDictionary original = Values[0] as IDictionary;
            if (original == null || !original.Contains(key))
            {
                return;
            }
            IDictionary result = NewDictionary();
            foreach (object existing in original.Keys)
            {
                if (!Equals(existing, key))
                {
                    result[existing] = original[existing];
                }
            }
            SetValue(result);
        }

        protected void ChangeKey(object oldKey, object newKey)
        {
            if (IsSetBlocked || m_ReadOnly || newKey == null)
            {
                return;
            }
            IDictionary original = Values[0] as IDictionary;
            if (original == null || !original.Contains(oldKey) || original.Contains(newKey))
            {
                return;
            }
            IDictionary result = NewDictionary();
            foreach (object existing in original.Keys)
            {
                result[Equals(existing, oldKey) ? newKey : existing] = original[existing];
            }
            SetValue(result);
            m_KeyEdited = true;
        }

        protected void Resize(int newSize)
        {
            if (IsSetBlocked || m_ReadOnly || !m_CanEditKeys || newSize < 0 || newSize > ushort.MaxValue ||
                m_NotNullItems && newSize > Count)
            {
                return;
            }
            IDictionary original = Values[0] as IDictionary;
            int oldSize = original?.Count ?? 0;
            if (oldSize == newSize)
            {
                return;
            }
            Type[] arguments = Values.Type.GetGenericArguments();
            Type keyType = arguments[0], valueType = arguments[1];
            IDictionary result = NewDictionary();
            int remaining = newSize;
            if (original != null)
            {
                foreach (object key in original.Keys)
                {
                    if (remaining-- == 0)
                    {
                        break;
                    }
                    result[key] = original[key];
                }
            }
            for (int i = oldSize; i < newSize; i++)
            {
                if (!TryCreateUniqueKey(result, keyType, out object key))
                {
                    return;
                }
                result[key!] = valueType.IsValueType ? Activator.CreateInstance(valueType) : null;
            }
            SetValue(result);
        }

        private static bool TryCreateUniqueKey(IDictionary dictionary, Type type, out object key)
        {
            if (type.IsPrimitive)
            {
                for (long candidate = 0; candidate < ushort.MaxValue; candidate++)
                {
                    try
                    {
                        object converted = Convert.ChangeType(candidate, type);
                        if (!dictionary.Contains(converted)) { key = converted; return true; }
                    }
                    catch { break; }
                }
            }
            else if (type.IsEnum)
            {
                foreach (object candidate in Enum.GetValues(type))
                {
                    if (!dictionary.Contains(candidate)) { key = candidate; return true; }
                }
            }
            else if (type == typeof(string))
            {
                string candidate = "Key";
                while (dictionary.Contains(candidate))
                {
                    candidate += "*";
                }
                key = candidate;
                return true;
            }
            else
            {
                object candidate = type.IsValueType ? Activator.CreateInstance(type) : null;
                if (candidate != null && !dictionary.Contains(candidate)) { key = candidate; return true; }
            }
            key = null;
            return false;
        }

        private IDictionary NewDictionary() => (IDictionary)Activator.CreateInstance(Values.Type)!;

        public override void Refresh()
        {
            base.Refresh();
            if (m_KeyEdited)
            {
                m_KeyEdited = false;
                RebuildLayoutOnRefresh();
            }
            if (HasDifferentValues || HasDifferentTypes)
            {
                return;
            }
            if (Count != m_ElementsCount)
            {
                RebuildLayoutOnRefresh();
            }
        }
    }
}

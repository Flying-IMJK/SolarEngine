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
        private IntValueBox? _sizeBox;
        private int _elementsCount;
        private bool _readOnly;
        private bool _notNullItems;
        private bool _canEditKeys;
        private bool _keyEdited;

        public int Count => (Values[0] as IDictionary)?.Count ?? 0;

        public override void Initialize(LayoutElementsContainer layout)
        {
            if (HasDifferentValues || HasDifferentTypes) return;
            var type = Values.Type;
            var arguments = type.GetGenericArguments();
            Type keyType = arguments[0], valueType = arguments[1];
            var attributes = Values.GetAttributes();
            var meta = attributes.OfType<CollectionAttribute>().FirstOrDefault();
            _readOnly = meta?.ReadOnly ?? false;
            _notNullItems = meta?.NotNullItems ?? false;
            _canEditKeys = keyType == typeof(string) || keyType.IsPrimitive || keyType.IsEnum || keyType.IsValueType;
            if (meta != null) _canEditKeys &= meta.CanReorderItems;
            if (attributes.Any(x => x is SE.ReadOnlyAttribute))
            {
                _readOnly = true;
                _canEditKeys = false;
            }

            int size = Count;
            var sizeRow = new UniformGridPanel
            {
                Parent = layout.ContainerControl,
                Height = TextBox.DefaultHeight,
                SlotsHorizontally = 4,
                SlotsVertically = 1,
                ClipChildren = false,
            };
            new Label { Text = "Size", Parent = sizeRow, Height = TextBox.DefaultHeight };
            _sizeBox = new IntValueBox(size) { Parent = sizeRow };
            _sizeBox.SetLimits(0, _notNullItems ? size : ushort.MaxValue);
            _sizeBox.IsReadOnly = _readOnly || !_canEditKeys;
            _sizeBox.EditEnd += OnSizeChanged;
            var add = new Button { Text = "+", Parent = sizeRow,
                Enabled = !_readOnly && _canEditKeys && !_notNullItems };
            add.Clicked += () => Resize(Count + 1);
            var remove = new Button { Text = "-", Parent = sizeRow,
                Enabled = !_readOnly && _canEditKeys && size > 0 };
            remove.Clicked += () => Resize(Count - 1);

            if (Values[0] is IDictionary dictionary)
            {
                var itemsLayout = layout.VerticalPanel();
                itemsLayout.ContainerControl.BackgroundColor = meta?.BackgroundColor ??
                    SE.GUI.Style.Current.CollectionBackgroundColor;
                Type? overrideType = meta?.OverrideEditorTypeName == null
                    ? null : CustomEditorsUtil.ResolveEditorAlias(meta.OverrideEditorTypeName);
                object[] keys = dictionary.Keys.Cast<object>().ToArray();
                for (int i = 0; i < keys.Length; i++)
                {
                    if (i > 0 && meta?.Spacing > 0) itemsLayout.Space(meta.Spacing);
                    object key = keys[i];
                    CustomEditor? overrideEditor = overrideType != null
                        ? Activator.CreateInstance(overrideType) as CustomEditor : null;
                    var itemValues = new DictionaryValueContainer(valueType, key, Values, attributes);
                    var itemEditor = itemsLayout.Property(key?.ToString() ?? "<null>", itemValues, overrideEditor);
                    if (_readOnly) itemEditor.SetReadOnly(true);
                    if (itemEditor.LinkedLabel != null)
                    {
                        var label = itemEditor.LinkedLabel;
                        label.SetupContextMenu += (_, menu, _) => AddKeyActions(menu, label, key, keyType);
                    }
                    else if (itemEditor.Layout is LayoutElementsContainer.GroupElement group)
                        group.SetupContextMenu += menu => AddKeyActions(menu, group.Control, key, keyType);
                }
            }
            _elementsCount = size;
        }

        private void AddKeyActions(ContextMenu menu, Control target, object key, Type keyType)
        {
            menu.AddSeparator();
            menu.AddButton("Remove", () => Remove(key)).Enabled = !_readOnly;
            menu.AddButton("Edit", () => EditKey(target, key, keyType)).Enabled = !_readOnly && _canEditKeys;
        }

        private void EditKey(Control target, object oldKey, Type keyType)
        {
            if (_readOnly || !_canEditKeys) return;
            var area = new Rectangle(0, 0, target.Width, Math.Max(target.Height, TextBox.DefaultHeight));
            if (keyType == typeof(string) || keyType.IsPrimitive)
            {
                var popup = RenamePopup.ShowPopup(target, area, oldKey?.ToString() ?? string.Empty, false);
                popup.Validate += (RenamePopup _, ref string input) =>
                    TryParseKey(input, keyType, out object? parsed) &&
                    parsed != null && !(Values[0] as IDictionary)!.Contains(parsed);
                popup.Renamed += p =>
                {
                    if (TryParseKey(p.Text, keyType, out object? newKey) && newKey != null)
                        ChangeKey(oldKey, newKey);
                };
            }
            else if (keyType.IsEnum)
            {
                var popup = ContextMenuBase.ShowEmptyMenu(target, area);
                var picker = new EnumComboBox(keyType)
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
                var popup = new DictionaryKeyPopup(oldKey, newKey => ChangeKey(oldKey, newKey))
                {
                    Size = new Float2(Math.Max(target.Width, 180), 80),
                };
                popup.Show(target, Float2.Zero);
            }
        }

        private static bool TryParseKey(string text, Type type, out object? key)
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
            private readonly CustomEditorPresenter _presenter = new(null);
            private readonly Action<object> _onChanged;
            public DictionaryKeyPopup(object key, Action<object> onChanged)
            {
                _onChanged = onChanged;
                _presenter.Panel.AnchorPreset = AnchorPresets.StretchAll;
                _presenter.Panel.IsScrollable = false;
                _presenter.Panel.Parent = this;
                _presenter.Select(key);
                _presenter.Modified += () =>
                {
                    if (_presenter.Selection[0] is { } updated)
                        _onChanged(updated);
                    Hide();
                };
            }
            public override void Update(float deltaTime)
            {
                _presenter.Update();
                base.Update(deltaTime);
            }
        }

        private void OnSizeChanged()
        {
            if (IsSetBlocked || _sizeBox == null || _readOnly || !_canEditKeys) return;
            Resize(_sizeBox.GetValue());
        }

        private void Remove(object key)
        {
            if (IsSetBlocked || _readOnly) return;
            var original = Values[0] as IDictionary;
            if (original == null || !original.Contains(key)) return;
            var result = NewDictionary();
            foreach (var existing in original.Keys)
                if (!Equals(existing, key)) result[existing] = original[existing];
            SetValue(result);
        }

        protected void ChangeKey(object oldKey, object newKey)
        {
            if (IsSetBlocked || _readOnly || newKey == null) return;
            var original = Values[0] as IDictionary;
            if (original == null || !original.Contains(oldKey) || original.Contains(newKey)) return;
            var result = NewDictionary();
            foreach (var existing in original.Keys)
                result[Equals(existing, oldKey) ? newKey : existing] = original[existing];
            SetValue(result);
            _keyEdited = true;
        }

        protected void Resize(int newSize)
        {
            if (IsSetBlocked || _readOnly || !_canEditKeys || newSize < 0 || newSize > ushort.MaxValue ||
                _notNullItems && newSize > Count) return;
            var original = Values[0] as IDictionary;
            int oldSize = original?.Count ?? 0;
            if (oldSize == newSize) return;
            Type[] arguments = Values.Type.GetGenericArguments();
            Type keyType = arguments[0], valueType = arguments[1];
            var result = NewDictionary();
            int remaining = newSize;
            if (original != null)
                foreach (var key in original.Keys)
                {
                    if (remaining-- == 0) break;
                    result[key] = original[key];
                }
            for (int i = oldSize; i < newSize; i++)
            {
                if (!TryCreateUniqueKey(result, keyType, out object? key)) return;
                result[key!] = valueType.IsValueType ? Activator.CreateInstance(valueType) : null;
            }
            SetValue(result);
        }

        private static bool TryCreateUniqueKey(IDictionary dictionary, Type type, out object? key)
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
                foreach (var candidate in Enum.GetValues(type))
                    if (!dictionary.Contains(candidate)) { key = candidate; return true; }
            }
            else if (type == typeof(string))
            {
                string candidate = "Key";
                while (dictionary.Contains(candidate)) candidate += "*";
                key = candidate;
                return true;
            }
            else
            {
                object? candidate = type.IsValueType ? Activator.CreateInstance(type) : null;
                if (candidate != null && !dictionary.Contains(candidate)) { key = candidate; return true; }
            }
            key = null;
            return false;
        }

        private IDictionary NewDictionary() => (IDictionary)Activator.CreateInstance(Values.Type)!;

        public override void Refresh()
        {
            base.Refresh();
            if (_keyEdited)
            {
                _keyEdited = false;
                RebuildLayoutOnRefresh();
            }
            if (HasDifferentValues || HasDifferentTypes) return;
            if (Count != _elementsCount) RebuildLayoutOnRefresh();
        }
    }
}

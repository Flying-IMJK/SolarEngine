using System;
using System.Collections;
using System.Linq;
using System.Reflection;
using SE.Editor.GUI;
using SE.GUI;

namespace SE.Editor
{
    [HideInEditor]
    public abstract class CollectionEditor : CustomEditor
    {
        protected bool NotNullItems;
        private bool _readOnly;
        private bool _canResize;
        private bool _canReorder;
        private int _minCount;
        private int _maxCount;
        private int _elementsCount;
        private IntValueBox? _sizeBox;

        public abstract int Count { get; }
        public Type ElementType => Values.Type.IsArray
            ? Values.Type.GetElementType()!
            : Values.Type.GetGenericArguments()[0];
        public override bool RevertValueWithChildren => false;

        public override void Initialize(LayoutElementsContainer layout)
        {
            if (HasDifferentTypes) return;
            int size = Count;
            var attributes = Values.GetAttributes();
            var meta = attributes.OfType<CollectionAttribute>().FirstOrDefault();
            _readOnly = meta?.ReadOnly ?? false;
            _canResize = meta?.CanResize ?? true;
            _canReorder = meta?.CanReorderItems ?? true;
            NotNullItems = meta?.NotNullItems ?? false;
            _minCount = meta?.MinCount ?? 0;
            _maxCount = meta?.MaxCount is > 0 ? meta.MaxCount : ushort.MaxValue;
            if (attributes.Any(x => x is SE.ReadOnlyAttribute))
            {
                _readOnly = true;
                _canResize = false;
                _canReorder = false;
            }
            _canResize &= _minCount < _maxCount;
            if (_canResize)
            {
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
                _sizeBox.SetLimits(_minCount, _maxCount);
                _sizeBox.IsReadOnly = _readOnly || NotNullItems && size == 0;
                _sizeBox.EditEnd += OnSizeChanged;
                var remove = new Button { Text = "-", Parent = sizeRow, Enabled = !_readOnly && size > _minCount };
                remove.Clicked += () => Resize(Count - 1);
                var add = new Button { Text = "+", Parent = sizeRow,
                    Enabled = !_readOnly && (!NotNullItems || size > 0) && size < _maxCount };
                add.Clicked += () => Resize(Count + 1);
            }
            else
            {
                layout.Label("Size " + size);
            }

            if (size > 0)
            {
                var itemsLayout = layout.VerticalPanel();
                var itemsPanel = (VerticalPanel)itemsLayout.ContainerControl;
                itemsPanel.Margin = new Margin(7, 7, 0, 0);
                itemsPanel.BackgroundColor = meta?.BackgroundColor ?? SE.GUI.Style.Current.CollectionBackgroundColor;
                Type? overrideType = meta?.OverrideEditorTypeName == null
                    ? null : CustomEditorsUtil.ResolveEditorAlias(meta.OverrideEditorTypeName);
                bool single = IsSingleEditorType(ElementType);
                DisplayStyle itemStyle = meta?.Display switch
                {
                    CollectionAttribute.DisplayType.Inline => DisplayStyle.Inline,
                    CollectionAttribute.DisplayType.Header => DisplayStyle.Group,
                    _ => single ? DisplayStyle.Inline : DisplayStyle.Group,
                };
                for (int i = 0; i < size; i++)
                {
                    if (i > 0 && meta?.Spacing > 0 && !single) itemsLayout.Space(meta.Spacing);
                    CustomEditor? overrideEditor = overrideType != null
                        ? Activator.CreateInstance(overrideType) as CustomEditor : null;
                    var itemValues = new ListValueContainer(ElementType, i, Values, attributes);
                    CustomEditor itemEditor = itemsLayout.Property("Element " + i, itemValues,
                        overrideEditor, displayStyle: itemStyle);
                    if (_readOnly) itemEditor.SetReadOnly(true);
                    int index = i;
                    if (itemEditor.LinkedLabel != null)
                        itemEditor.LinkedLabel.SetupContextMenu += (_, menu, _) => AddItemActions(menu, itemEditor, index);
                    else if (itemEditor.Layout is LayoutElementsContainer.GroupElement group)
                        group.SetupContextMenu += menu => AddItemActions(menu, itemEditor, index);
                }
            }
            _elementsCount = size;
        }

        private static bool IsSingleEditorType(Type type)
        {
            if (type.IsPrimitive || type == typeof(string) || type.IsEnum ||
                typeof(SceneObject).IsAssignableFrom(type))
                return true;
            const BindingFlags flags = BindingFlags.Instance | BindingFlags.Public;
            return type.GetFields(flags).Length + type.GetProperties(flags).Length == 1;
        }

        private void AddItemActions(ContextMenu menu, CustomEditor itemEditor, int index)
        {
            menu.AddSeparator();
            var duplicate = menu.AddButton("Duplicate", () => Duplicate(index));
            duplicate.Enabled = CanResizeTo(Count + 1);
            if (_canReorder && !_readOnly)
            {
                var up = menu.AddButton("Move up", () => Move(index, index - 1));
                up.Enabled = index > 0;
                var down = menu.AddButton("Move down", () => Move(index, index + 1));
                down.Enabled = index + 1 < Count;
            }
            var remove = menu.AddButton("Remove", () => Remove(index));
            remove.Enabled = CanResizeTo(Count - 1);
        }

        private bool CanResizeTo(int size) => !_readOnly && _canResize && size >= _minCount && size <= _maxCount &&
            (!NotNullItems || Count > 0);

        private void OnSizeChanged()
        {
            if (IsSetBlocked || _sizeBox == null) return;
            int size = _sizeBox.GetValue();
            if (CanResizeTo(size)) Resize(size);
        }

        public void Duplicate(int index)
        {
            if (IsSetBlocked || !CanResizeTo(Count + 1) || index < 0 || index >= Count) return;
            var oldValues = (IList)Values[0]!;
            var newValues = Allocate(Count + 1);
            for (int i = 0; i <= index; i++) newValues[i] = oldValues[i];
            newValues[index + 1] = CloneValue(oldValues[index]);
            for (int i = index + 1; i < Count; i++) newValues[i + 1] = oldValues[i];
            SetValue(newValues);
        }

        private void Move(int src, int dst)
        {
            if (IsSetBlocked || _readOnly || !_canReorder || src < 0 || dst < 0 || src >= Count || dst >= Count)
                return;
            var cloned = CloneValues();
            (cloned[src], cloned[dst]) = (cloned[dst], cloned[src]);
            SetValue(cloned);
        }

        private void Remove(int index)
        {
            if (IsSetBlocked || !CanResizeTo(Count - 1) || index < 0 || index >= Count) return;
            var oldValues = (IList)Values[0]!;
            var newValues = Allocate(Count - 1);
            for (int i = 0; i < index; i++) newValues[i] = oldValues[i];
            for (int i = index; i < newValues.Count; i++) newValues[i] = oldValues[i + 1];
            SetValue(newValues);
        }

        protected abstract IList Allocate(int size);
        protected abstract void Resize(int newSize);
        protected abstract IList CloneValues();

        public override void Refresh()
        {
            base.Refresh();
            if (HasDifferentValues || HasDifferentTypes) return;
            if (Count != _elementsCount)
            {
                RebuildLayoutOnRefresh();
                if (ParentEditor is CollectionEditor parent) parent.RebuildLayoutOnRefresh();
            }
        }

        protected override bool OnDirty(CustomEditor editor, object? value, object? token = null)
        {
            if (NotNullItems)
            {
                if (value == null && editor.ParentEditor == this) return false;
                if (editor == this && value is IList list)
                    foreach (var item in list)
                        if (item == null) return false;
            }
            return base.OnDirty(editor, value, token);
        }
    }
}

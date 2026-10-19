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
        private bool m_ReadOnly;
        private bool m_CanResize;
        private bool m_CanReorder;
        private int m_MinCount;
        private int m_MaxCount;
        private int m_ElementsCount;
        private IntValueBox m_SizeBox;

        public abstract int Count { get; }
        public Type ElementType
        {
            get
            {
                return Values.Type.IsArray
                            ? Values.Type.GetElementType()!
                            : Values.Type.GetGenericArguments()[0];
            }
        }
        public override bool RevertValueWithChildren => false;

        public override void Initialize(LayoutElementsContainer layout)
        {
            if (HasDifferentTypes)
            {
                return;
            }
            int size = Count;
            object[] attributes = Values.GetAttributes();
            CollectionAttribute meta = attributes.OfType<CollectionAttribute>().FirstOrDefault();
            m_ReadOnly = meta?.ReadOnly ?? false;
            m_CanResize = meta?.CanResize ?? true;
            m_CanReorder = meta?.CanReorderItems ?? true;
            NotNullItems = meta?.NotNullItems ?? false;
            m_MinCount = meta?.MinCount ?? 0;
            m_MaxCount = meta?.MaxCount is > 0 ? meta.MaxCount : ushort.MaxValue;
            if (attributes.Any(x => x is SE.ReadOnlyAttribute))
            {
                m_ReadOnly = true;
                m_CanResize = false;
                m_CanReorder = false;
            }
            m_CanResize &= m_MinCount < m_MaxCount;
            if (m_CanResize)
            {
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
                m_SizeBox.SetLimits(m_MinCount, m_MaxCount);
                m_SizeBox.IsReadOnly = m_ReadOnly || NotNullItems && size == 0;
                m_SizeBox.EditEnd += OnSizeChanged;
                Button remove = new Button { Text = "-", Parent = sizeRow, Enabled = !m_ReadOnly && size > m_MinCount };
                remove.Clicked += () => Resize(Count - 1);
                Button add = new Button
                {
                    Text = "+",
                    Parent = sizeRow,
                    Enabled = !m_ReadOnly && (!NotNullItems || size > 0) && size < m_MaxCount
                };
                add.Clicked += () => Resize(Count + 1);
            }
            else
            {
                layout.Label("Size " + size);
            }

            if (size > 0)
            {
                VerticalPanelElement itemsLayout = layout.VerticalPanel();
                VerticalPanel itemsPanel = (VerticalPanel)itemsLayout.ContainerControl;
                itemsPanel.Margin = new Margin(7, 7, 0, 0);
                itemsPanel.BackgroundColor = meta?.BackgroundColor ?? SE.GUI.Style.Current.CollectionBackgroundColor;
                Type overrideType = meta?.OverrideEditorTypeName == null
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
                    if (i > 0 && meta?.Spacing > 0 && !single)
                    {
                        itemsLayout.Space(meta.Spacing);
                    }
                    CustomEditor overrideEditor = overrideType != null
                        ? Activator.CreateInstance(overrideType) as CustomEditor : null;
                    ListValueContainer itemValues = new ListValueContainer(ElementType, i, Values, attributes);
                    CustomEditor itemEditor = itemsLayout.Property("Element " + i, itemValues,
                        overrideEditor, displayStyle: itemStyle);
                    if (m_ReadOnly)
                    {
                        itemEditor.SetReadOnly(true);
                    }
                    int index = i;
                    if (itemEditor.LinkedLabel != null)
                    {
                        itemEditor.LinkedLabel.SetupContextMenu += (_, menu, _) => AddItemActions(menu, itemEditor,
                            index);
                    }
                    else if (itemEditor.Layout is GroupElement group)
                    {
                        group.SetupContextMenu += (menu, _) => AddItemActions(menu, itemEditor, index);
                    }
                }
            }
            m_ElementsCount = size;
        }

        private static bool IsSingleEditorType(Type type)
        {
            if (type.IsPrimitive || type == typeof(string) || type.IsEnum ||
                typeof(SceneObject).IsAssignableFrom(type))
            {
                return true;
            }
            const BindingFlags flags = BindingFlags.Instance | BindingFlags.Public;
            return type.GetFields(flags).Length + type.GetProperties(flags).Length == 1;
        }

        private void AddItemActions(ContextMenu menu, CustomEditor itemEditor, int index)
        {
            menu.AddSeparator();
            ContextMenuButton duplicate = menu.AddButton("Duplicate", () => Duplicate(index));
            duplicate.Enabled = CanResizeTo(Count + 1);
            if (m_CanReorder && !m_ReadOnly)
            {
                ContextMenuButton up = menu.AddButton("Move up", () => Move(index, index - 1));
                up.Enabled = index > 0;
                ContextMenuButton down = menu.AddButton("Move down", () => Move(index, index + 1));
                down.Enabled = index + 1 < Count;
            }
            ContextMenuButton remove = menu.AddButton("Remove", () => Remove(index));
            remove.Enabled = CanResizeTo(Count - 1);
        }

        private bool CanResizeTo(int size)
        {
            return !m_ReadOnly && m_CanResize && size >= m_MinCount && size <= m_MaxCount &&
                        (!NotNullItems || Count > 0);
        }

        private void OnSizeChanged()
        {
            if (IsSetBlocked || m_SizeBox == null)
            {
                return;
            }
            int size = m_SizeBox.GetValue();
            if (CanResizeTo(size))
            {
                Resize(size);
            }
        }

        public void Duplicate(int index)
        {
            if (IsSetBlocked || !CanResizeTo(Count + 1) || index < 0 || index >= Count)
            {
                return;
            }
            IList oldValues = (IList)Values[0]!;
            IList newValues = Allocate(Count + 1);
            for (int i = 0; i <= index; i++)
            {
                newValues[i] = oldValues[i];
            }
            newValues[index + 1] = CloneValue(oldValues[index]);
            for (int i = index + 1; i < Count; i++)
            {
                newValues[i + 1] = oldValues[i];
            }
            SetValue(newValues);
        }

        private void Move(int src, int dst)
        {
            if (IsSetBlocked || m_ReadOnly || !m_CanReorder || src < 0 || dst < 0 || src >= Count || dst >= Count)
            {
                return;
            }
            IList cloned = CloneValues();
            (cloned[src], cloned[dst]) = (cloned[dst], cloned[src]);
            SetValue(cloned);
        }

        private void Remove(int index)
        {
            if (IsSetBlocked || !CanResizeTo(Count - 1) || index < 0 || index >= Count)
            {
                return;
            }
            IList oldValues = (IList)Values[0]!;
            IList newValues = Allocate(Count - 1);
            for (int i = 0; i < index; i++)
            {
                newValues[i] = oldValues[i];
            }
            for (int i = index; i < newValues.Count; i++)
            {
                newValues[i] = oldValues[i + 1];
            }
            SetValue(newValues);
        }

        protected abstract IList Allocate(int size);
        protected abstract void Resize(int newSize);
        protected abstract IList CloneValues();

        public override void Refresh()
        {
            base.Refresh();
            if (HasDifferentValues || HasDifferentTypes)
            {
                return;
            }
            if (Count != m_ElementsCount)
            {
                RebuildLayoutOnRefresh();
                if (ParentEditor is CollectionEditor parent)
                {
                    parent.RebuildLayoutOnRefresh();
                }
            }
        }

        protected override bool OnDirty(CustomEditor editor, object value, object token = null)
        {
            if (NotNullItems)
            {
                if (value == null && editor.ParentEditor == this)
                {
                    return false;
                }
                if (editor == this && value is IList list)
                {
                    foreach (object item in list)
                    {
                        if (item == null)
                        {
                            return false;
                        }
                    }
                }
            }
            return base.OnDirty(editor, value, token);
        }
    }
}

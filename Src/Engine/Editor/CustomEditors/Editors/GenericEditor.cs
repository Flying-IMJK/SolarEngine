// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Text.Json;
using SE.GUI;
using SE.Editor.GUI;

namespace SE.Editor
{
    [HideInEditor]
    public class GenericEditor : CustomEditor
    {
        public class ItemInfo : IComparable
        {
            public MemberInfo Info;
            public EditorOrderAttribute Order;
            public EditorDisplayAttribute Display;
            public VisibleIfAttribute[] VisibleIfs;
            public SpaceAttribute Space;
            public HeaderAttribute Header;
            public bool ExpandGroups;
            public CustomEditorAttribute CustomEditor;
            public CustomEditorAliasAttribute CustomEditorAlias;
            public bool IsReadOnly;
            public string DisplayName { get; }
            public string TooltipText;

            public ItemInfo(MemberInfo info)
                : this(info, info.GetCustomAttributes(true))
            {
            }

            public ItemInfo(MemberInfo info, object[] attributes)
            {
                Info = info;
                Order = attributes.OfType<EditorOrderAttribute>().FirstOrDefault();
                Display = attributes.OfType<EditorDisplayAttribute>().FirstOrDefault();
                VisibleIfs = attributes.OfType<VisibleIfAttribute>().ToArray();
                Space = attributes.OfType<SpaceAttribute>().FirstOrDefault();
                Header = attributes.OfType<HeaderAttribute>().FirstOrDefault();
                ExpandGroups = attributes.OfType<ExpandGroupsAttribute>().Any();
                CustomEditor = attributes.OfType<CustomEditorAttribute>().FirstOrDefault();
                CustomEditorAlias = attributes.OfType<CustomEditorAliasAttribute>().FirstOrDefault();
                IsReadOnly = attributes.Any(x => x is SE.ReadOnlyAttribute) ||
                    info is PropertyInfo property && property.SetMethod == null ||
                    info is FieldInfo field && field.IsInitOnly;
                DisplayName = Display?.Name ?? GetPropertyNameUI(info.Name);
                TooltipText = attributes.OfType<TooltipAttribute>().FirstOrDefault()?.Text ??
                    attributes.OfType<DescriptionAttribute>().FirstOrDefault()?.Description;
            }

            public ValueContainer GetValues(ValueContainer instanceValues) => new(Info, instanceValues);

            public CustomEditor OverrideEditor
            {
                get
                {
                    if (CustomEditor != null)
                    {
                        return (CustomEditor)Activator.CreateInstance(CustomEditor.Type)!;
                    }
                    if (CustomEditorAlias != null &&
                        CustomEditorsUtil.ResolveEditorAlias(CustomEditorAlias.TypeName) is { } type)
                    {
                        return (CustomEditor)Activator.CreateInstance(type)!;
                    }
                    return null;
                }
            }

            public int CompareTo(object obj)
            {
                if (obj is not ItemInfo other)
                {
                    return 0;
                }
                if (Order != null)
                {
                    return other.Order != null ? Order.Order - other.Order.Order : -1;
                }
                if (other.Order != null)
                {
                    return 1;
                }
                if (Display?.Group != null && other.Display?.Group != null)
                {
                    return string.Compare(Display.Group, other.Display.Group, StringComparison.InvariantCulture);
                }
                // Solar has no global member-order option; retain declaration ordering.
                return Info.MetadataToken.CompareTo(other.Info.MetadataToken);
            }

            public override string ToString() => Info.Name;

            public static bool CanMerge(ItemInfo a, ItemInfo b)
            {
                return a.Info.DeclaringType == b.Info.DeclaringType && a.Info.Name == b.Info.Name;
            }

            internal static string GetPropertyNameUI(string name)
            {
                int length = name.Length;
                StringBuilder builder = new StringBuilder(length + 8);
                int startIndex = 0;
                if (name.StartsWith("g_") || name.StartsWith("m_"))
                {
                    startIndex = 2;
                }
                if (name.StartsWith("_"))
                {
                    startIndex = 1;
                }
                char lastChar = '\0';
                for (int i = startIndex; i < length; i++)
                {
                    char c = name[i];
                    if (i == startIndex)
                    {
                        builder.Append(char.ToUpper(c));
                        continue;
                    }
                    if (char.IsUpper(c) && i > 0)
                    {
                        if (lastChar != ' ' && (!char.IsUpper(name[i - 1]) ||
                            (i + 1 != length && !char.IsUpper(name[i + 1]))))
                        {
                            lastChar = ' ';
                            builder.Append(lastChar);
                        }
                    }
                    else if (c == '_')
                    {
                        if (builder.Length > 0 && lastChar != ' ')
                        {
                            lastChar = ' ';
                            builder.Append(lastChar);
                        }
                        continue;
                    }
                    else if (i > 1 && char.IsDigit(c) && !char.IsDigit(name[i - 1]) && lastChar != ' ')
                    {
                        lastChar = ' ';
                        builder.Append(lastChar);
                    }
                    lastChar = c;
                    builder.Append(lastChar);
                }
                return builder.ToString();
            }
        }


        private struct VisibleIfCache
        {
            public MemberInfo Target;
            public MemberInfo[] Sources;
            public PropertiesListElement PropertiesList;
            public GroupElement Group;
            public bool[] InversionList;
            public int LabelIndex;

            public bool GetValue(object instance)
            {
                bool value = true;

                for (int i = 0; i < Sources.Length; i++)
                {
                    bool currentValue = (bool)(Sources[i] is PropertyInfo property
                        ? property.GetValue(instance) : ((FieldInfo)Sources[i]).GetValue(instance));
                    if (InversionList[i])
                    {
                        currentValue = !currentValue;
                    }

                    value = value && currentValue;
                }
                return value;
            }
        }

        private static HashSet<PropertiesList> s_VisibleIfPropertiesListsCache;
        private static Stack<Dictionary<string, GroupElement>> s_Groups;
        private static List<Dictionary<string, GroupElement>> s_GroupsPool;
        private VisibleIfCache[] m_VisibleIfCaches;
        private bool m_IsNull;

        public static List<ItemInfo> GetItemsForType(Type type, bool useProperties, bool useFields)
        {
            List<ItemInfo> items = new List<ItemInfo>();
            const BindingFlags flags = BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance;
            if (useProperties)
            {
                foreach (PropertyInfo property in type.GetProperties(flags))
                {
                    if (property.GetIndexParameters().Length != 0)
                    {
                        continue;
                    }

                    object[] attributes = property.GetCustomAttributes(true);

                    // meiyou Get方法的不显示
                    if (property.GetMethod == null)
                    {
                        continue;
                    }

                    // Get方法私有但是没有添加 ShowInEditorAttribute 属性
                    if (!property.GetMethod.IsPublic && !attributes.Any(x => x is ShowInEditorAttribute))
                    {
                        continue;
                    }

                    // 没有Set方法的默认不显示，除非添加了 ShowInEditorAttribute 属性
                    if (property.SetMethod == null && !attributes.Any(x => x is ShowInEditorAttribute))
                    {
                        continue;
                    }

                    // 主动不显示
                    if (attributes.Any(x => x is HideInEditorAttribute))
                    {
                        continue;
                    }

                    items.Add(new ItemInfo(property));
                }
            }

            if (useFields)
            {
                foreach (FieldInfo field in type.GetFields(flags))
                {
                    object[] attributes = field.GetCustomAttributes(true);

                    // 主动不显示
                    if (attributes.Any(x => x is HideInEditorAttribute))
                    {
                        continue;
                    }

                    // 私有但是没有添加 ShowInEditorAttribute 属性
                    if (!field.IsPublic && !attributes.Any(x => x is ShowInEditorAttribute))
                    {
                        continue;
                    }

                    items.Add(new ItemInfo(field));
                }
            }
            return items;
        }

        protected virtual List<ItemInfo> GetItemsForType(Type type) => GetItemsForType(type, type.IsClass, true);

        private static MemberInfo[] GetVisibleIfSources(Type type, VisibleIfAttribute[] rules)
        {
            List<MemberInfo> sources = new List<MemberInfo>();
            const BindingFlags flags = BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance
                | BindingFlags.Static;
            foreach (VisibleIfAttribute rule in rules)
            {
                PropertyInfo property = type.GetProperty(rule.MemberName, flags);
                if (property != null)
                {
                    if (property.GetMethod == null)
                    {
                        SE.Debug.LogError("Invalid VisibleIf rule. Property has missing getter " + rule.MemberName);
                    }
                    else if (property.PropertyType != typeof(bool))
                    {
                        SE.Debug.LogError("Invalid VisibleIf rule. Property has to return bool type " + rule
                            .MemberName);
                    }
                    else
                    {
                        sources.Add(property);
                    }
                    continue;
                }

                FieldInfo field = type.GetField(rule.MemberName, flags);
                if (field != null)
                {
                    if (field.FieldType != typeof(bool))
                    {
                        SE.Debug.LogError("Invalid VisibleIf rule. Field has to be bool type " + rule.MemberName);
                    }
                    else
                    {
                        sources.Add(field);
                    }
                    continue;
                }

                SE.Debug.LogError("Invalid VisibleIf rule. Cannot find member " + rule.MemberName);
            }
            return sources.ToArray();
        }

        // CLR equivalents of ScriptType's instance-creation contract.
        private static bool CanCreateInstance(Type type)
        {
            return type.IsValueType || type.GetConstructor(BindingFlags.Public | BindingFlags.NonPublic |
                BindingFlags.Instance, null, Type.EmptyTypes, null) != null;
        }

        private static object CreateInstance(Type type)
        {
            object value = System.Runtime.CompilerServices.RuntimeHelpers.GetUninitializedObject(type);
            if (!type.IsValueType)
            {
                ConstructorInfo constructor = type.GetConstructor(BindingFlags.Public | BindingFlags.NonPublic |
                    BindingFlags.Instance, null, Type.EmptyTypes, null);
                constructor.Invoke(value, null);
            }
            return value;
        }

        private static void GroupPanelCheckIfCanRevert(LayoutElementsContainer layout, ref bool canRevertReference,
            ref bool canRevertDefault)
        {
            if (layout == null || canRevertReference && canRevertDefault)
            {
                return;
            }

            foreach (CustomEditor editor in layout.Editors)
            {
                canRevertReference |= editor.CanRevertReferenceValue;
                canRevertDefault |= editor.CanRevertDefaultValue;
            }

            foreach (LayoutElement child in layout.Children)
            {
                GroupPanelCheckIfCanRevert(child as LayoutElementsContainer, ref canRevertReference,
                    ref canRevertDefault);
            }
        }

        private static void OnGroupPanelRevert(LayoutElementsContainer layout, bool toDefault)
        {
            if (layout == null)
            {
                return;
            }

            foreach (CustomEditor editor in layout.Editors)
            {
                if (toDefault && editor.CanRevertDefaultValue)
                {
                    editor.RevertToDefaultValue();
                }
                else if (!toDefault && editor.CanRevertReferenceValue)
                {
                    editor.RevertToReferenceValue();
                }
            }

            foreach (LayoutElement child in layout.Children)
            {
                OnGroupPanelRevert(child as LayoutElementsContainer, toDefault);
            }
        }

        private static void OnGroupPanelCopy(LayoutElementsContainer layout)
        {
            if (layout.Editors.Count == 1)
            {
                layout.Editors[0].Copy();
            }
            else if (layout.Editors.Count != 0)
            {
                string[] data = new string[layout.Editors.Count];
                StringBuilder sb = new StringBuilder();
                sb.Append("[\n");
                for (int i = 0; i < layout.Editors.Count; i++)
                {
                    layout.Editors[i].Copy();
                    if (i != 0)
                    {
                        sb.Append(",\n");
                    }
                    sb.Append(Clipboard.Text);
                    data[i] = Clipboard.Text;
                }
                sb.Append("\n]");
                Clipboard.Text = sb.ToString();
                Clipboard.Text = JsonSerializer.Serialize(data);
            }
            else if (layout.Children.Any(x => x is LayoutElementsContainer))
            {
                foreach (LayoutElement child in layout.Children)
                {
                    if (child is LayoutElementsContainer childContainer)
                    {
                        OnGroupPanelCopy(childContainer);
                        break;
                    }
                }
            }
        }

        private static bool OnGroupPanelCanCopy(LayoutElementsContainer layout)
        {
            return layout.Editors.Count != 0 || layout.Children.Any(x => x is LayoutElementsContainer);
        }

        private static void OnGroupPanelPaste(LayoutElementsContainer layout)
        {
            if (layout.Editors.Count == 1)
            {
                layout.Editors[0].Paste();
            }
            else if (layout.Editors.Count != 0)
            {
                string sb = Clipboard.Text;
                if (!string.IsNullOrEmpty(sb))
                {
                    try
                    {
                        string[] data = JsonSerializer.Deserialize<string[]>(sb);
                        if (data == null || data.Length != layout.Editors.Count)
                        {
                            return;
                        }
                        for (int i = 0; i < layout.Editors.Count; i++)
                        {
                            Clipboard.Text = data[i];
                            layout.Editors[i].Paste();
                        }
                    }
                    catch
                    {
                    }
                    finally
                    {
                        Clipboard.Text = sb;
                    }
                }
            }
            else if (layout.Children.Any(x => x is LayoutElementsContainer))
            {
                foreach (LayoutElement child in layout.Children)
                {
                    if (child is LayoutElementsContainer childContainer)
                    {
                        OnGroupPanelPaste(childContainer);
                        break;
                    }
                }
            }
        }

        private static bool OnGroupPanelCanPaste(LayoutElementsContainer layout)
        {
            if (layout.Editors.Count == 1)
            {
                return layout.Editors[0].CanPaste;
            }
            if (layout.Editors.Count != 0)
            {
                return !string.IsNullOrEmpty(Clipboard.Text);
            }
            if (layout.Children.Any(x => x is LayoutElementsContainer))
            {
                foreach (LayoutElement child in layout.Children)
                {
                    if (child is LayoutElementsContainer childContainer)
                    {
                        return OnGroupPanelCanPaste(childContainer);
                    }
                }
            }
            return false;
        }

        private static void OnGroupPanelMouseButtonRightClicked(DropPanel groupPanel, Float2 location)
        {
            GroupElement group = (GroupElement)groupPanel.Tag;
            bool canRevertReference = false, canRevertDefault = false;
            GroupPanelCheckIfCanRevert(group, ref canRevertReference, ref canRevertDefault);

            ContextMenu menu = new ContextMenu();
            ContextMenuButton revertToPrefab = menu.AddButton("Revert to Prefab",
                () => OnGroupPanelRevert(group, false));
            revertToPrefab.Enabled = canRevertReference;
            ContextMenuButton resetToDefault = menu.AddButton("Reset to default",
                () => OnGroupPanelRevert(group, true));
            resetToDefault.Enabled = canRevertDefault;
            menu.AddSeparator();
            ContextMenuButton copy = menu.AddButton("Copy", () => OnGroupPanelCopy(group));
            copy.Enabled = OnGroupPanelCanCopy(group);
            ContextMenuButton paste = menu.AddButton("Paste", () => OnGroupPanelPaste(group));
            paste.Enabled = OnGroupPanelCanPaste(group);

            menu.Show(groupPanel, location);
        }

        internal static void OnGroupsBegin()
        {
            if (s_Groups == null)
            {
                s_Groups = new Stack<Dictionary<string, GroupElement>>();
            }
            if (s_GroupsPool == null)
            {
                s_GroupsPool = new List<Dictionary<string, GroupElement>>();
            }
            Dictionary<string, GroupElement> group;
            if (s_GroupsPool.Count != 0)
            {
                group = s_GroupsPool[0];
                s_GroupsPool.RemoveAt(0);
            }
            else
            {
                group = new Dictionary<string, GroupElement>();
            }
            s_Groups.Push(group);
        }

        internal static void OnGroupsEnd()
        {
            Dictionary<string, GroupElement> groups = s_Groups.Pop();
            groups.Clear();
            s_GroupsPool.Add(groups);
        }

        internal static GroupElement OnGroup(LayoutElementsContainer layout, string name)
        {
            // Add new group
            GroupElement group = layout.Group(name);
            group.Panel.Tag = group;
            group.Panel.MouseButtonRightClicked += OnGroupPanelMouseButtonRightClicked;
            return group;
        }

        internal static LayoutElementsContainer OnGroup(LayoutElementsContainer layout, EditorDisplayAttribute display)
        {
            if (display?.Group != null)
            {
                Dictionary<string, GroupElement> groups = s_Groups.Peek();
                if (groups.TryGetValue(display.Group, out GroupElement group))
                {
                    // Reuse group
                    layout = group;
                }
                else
                {
                    // Add new group
                    group = layout.Group(display.Group);
                    group.Panel.Tag = group;
                    group.Panel.MouseButtonRightClicked += OnGroupPanelMouseButtonRightClicked;
                    groups.Add(display.Group, group);
                    layout = group;
                }
            }
            return layout;
        }

        internal static void OnReadOnlyProperty(LayoutElementsContainer itemLayout, int labelIndex = -1)
        {
            PropertiesList list = null;
            int firstChildControlIndex = 0;
            bool disableSingle = true;
            LayoutElement control = itemLayout.Children[itemLayout.Children.Count - 1];
            if (control is GroupElement group && group.Children.Count > 0)
            {
                list = (group.Children[0] as PropertiesListElement)?.Properties;
                disableSingle = false; // Disable all nested editors
            }
            else if (control is PropertiesListElement list1 && labelIndex != -1)
            {
                list = list1.Labels[labelIndex].FirstChildControlContainer ?? list1.Properties;
                firstChildControlIndex = list1.Labels[labelIndex].FirstChildControlIndex;
            }
            else if (control?.Control != null)
            {
                control.Control.Enabled = false;
            }

            if (list != null)
            {
                // Disable controls added to the editor
                int count = list.Children.Count;
                for (int j = firstChildControlIndex; j < count; j++)
                {
                    Control child = list.Children[j];
                    if (disableSingle && child is PropertyNameLabel)
                    {
                        break;
                    }

                    if (child != null)
                    {
                        child.Enabled = false;
                    }
                }
            }
        }

        /// <summary>
        /// Evaluate the <see cref="VisibleIfAttribute"/> cache for a given property item.
        /// </summary>
        /// <param name="itemLayout">The item layout.</param>
        /// <param name="item">The item.</param>
        /// <param name="labelIndex">The label index.</param>
        protected virtual void EvaluateVisibleIf(LayoutElementsContainer itemLayout, ItemInfo item, int labelIndex)
        {
            if (item.VisibleIfs.Length > 0 && itemLayout.Children.Count > 0)
            {
                PropertiesListElement list = null;
                GroupElement group = null;
                if (itemLayout.Children[itemLayout.Children.Count - 1] is PropertiesListElement list1)
                {
                    list = list1;
                }
                else if (itemLayout.Children[itemLayout.Children.Count - 1] is GroupElement group1)
                {
                    group = group1;
                }
                else
                {
                    return;
                }

                // Get source member used to check rule
                MemberInfo[] sourceMembers = GetVisibleIfSources(item.Info.DeclaringType, item.VisibleIfs);
                if (sourceMembers.Length == 0)
                {
                    return;
                }

                // Resize cache
                if (m_VisibleIfCaches == null)
                {
                    m_VisibleIfCaches = new VisibleIfCache[8];
                }
                int count = 0;
                while (count < m_VisibleIfCaches.Length && m_VisibleIfCaches[count].Target != null)
                {
                    count++;
                }
                if (count >= m_VisibleIfCaches.Length)
                {
                    Array.Resize(ref m_VisibleIfCaches, count * 2);
                }

                // Add item
                m_VisibleIfCaches[count] = new VisibleIfCache
                {
                    Target = item.Info,
                    Sources = sourceMembers,
                    PropertiesList = list,
                    Group = group,
                    LabelIndex = labelIndex,
                    InversionList = item.VisibleIfs.Select((x, i) => x.Invert).ToArray(),
                };
            }
        }

        /// <summary>
        /// Get the label index.
        /// </summary>
        /// <param name="itemLayout">The item layout.</param>
        /// <param name="item">The item.</param>
        /// <returns>The label index.</returns>
        protected virtual int GetLabelIndex(LayoutElementsContainer itemLayout, ItemInfo item)
        {
            int labelIndex = 0;
            if ((item.IsReadOnly || item.VisibleIfs.Length > 0) &&
                itemLayout.Children.Count > 0 &&
                itemLayout.Children[itemLayout.Children.Count - 1] is PropertiesListElement propertiesListElement)
            {
                labelIndex = propertiesListElement.Labels.Count;
            }

            return labelIndex;
        }

        /// <summary>
        /// Spawns the property for the given item.
        /// </summary>
        /// <param name="itemLayout">The item layout.</param>
        /// <param name="itemValues">The item values.</param>
        /// <param name="item">The item.</param>
        protected virtual void SpawnProperty(LayoutElementsContainer itemLayout, ValueContainer itemValues,
            ItemInfo item)
        {
            int labelIndex = GetLabelIndex(itemLayout, item);

            CustomEditor editor = itemLayout.Property(item.DisplayName, itemValues, item.OverrideEditor,
                item.TooltipText);
            // Solar carries the member's read-only state on the editor for paste/revert and nested writes.
            editor.SetReadOnly(item.IsReadOnly);

            if (item.IsReadOnly && itemLayout.Children.Count > 0)
            {
                OnReadOnlyProperty(itemLayout, labelIndex);
            }

            EvaluateVisibleIf(itemLayout, item, labelIndex);
        }

        /// <inheritdoc />
        internal override void Initialize(CustomEditorPresenter presenter, LayoutElementsContainer layout,
            ValueContainer values)
        {
            m_IsNull = values != null && values.IsNull;

            base.Initialize(presenter, layout, values);
        }

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            m_VisibleIfCaches = null;
            if (Values.Count == 0)
            {
                return;
            }

            // Collect items to edit
            List<ItemInfo> items;
            if (!HasDifferentTypes)
            {
                object value = Values[0];
                Type type = Values.Type;
                if (value == null)
                {
                    // Check if it's an object type that can be created in editor
                    if (type != null && CanEditValue)
                    {
                        Type[] types = null;
                        if (type.IsAbstract || type.IsInterface)
                        {
                            // Show picker with all types that implement specific class/interface but are not abstract
                            types = TypeSearchPopup.GetTypes().Where(x => !x.IsAbstract &&
                                !typeof(SE.Object).IsAssignableFrom(x) && CanCreateInstance(x) &&
                                type.IsAssignableFrom(x)).ToArray();
                        }
                        else if (CanCreateInstance(type))
                        {
                            types = new[] { type };
                        }
                        if (types != null && types.Length != 0)
                        {
                            layout = layout.Space(20);

                            const float ButtonSize = 14.0f;
                            Button button = new Button
                            {
                                Text = "+",
                                TooltipText = "Create a new instance of the object",
                                Size = new Float2(ButtonSize, ButtonSize),
                                AnchorPreset = AnchorPresets.MiddleRight,
                                Parent = layout.ContainerControl,
                                Location = new Float2(layout.ContainerControl.Width - ButtonSize - 4,
                                    (layout.ContainerControl.Height - ButtonSize) * 0.5f),
                            };
                            if (types.Length == 1)
                            {
                                // Single type
                                button.Clicked += () => SetValue(CreateInstance(types[0]));
                            }
                            else
                            {
                                // Picker
                                button.Clicked += () => TypeSearchPopup.Show(button, new Float2(0, button.Height),
                                    types, scriptType => SetValue(CreateInstance(scriptType)));
                            }
                        }
                    }

                    layout.Label("<null>");
                    return;
                }
                if (!type.IsArray && !type.IsValueType && !typeof(SE.Object).IsAssignableFrom(type) &&
                    (type.IsAbstract || type.IsInterface) && value.GetType() != type &&
                    layout is GroupElement group && CanEditValue)
                {
                    // Add button to unset the value to null (eg. to edit it to different type)
                    Image button = group.AddHeaderButton("Reset value to null", 0, SE.GUI.Style.Current.Cross);
                    button.Clicked += (_, _) => SetValue(null);
                }

                items = GetItemsForType(value.GetType());
            }
            else
            {
                Type[] types = ValuesTypes;
                items = new List<ItemInfo>(GetItemsForType(types[0]));
                for (int i = 1; i < types.Length && items.Count > 0; i++)
                {
                    List<ItemInfo> otherItems = GetItemsForType(types[i]);

                    // Merge items
                    for (int j = 0; j < items.Count && items.Count > 0; j++)
                    {
                        bool isInvalid = true;
                        for (int k = 0; k < otherItems.Count; k++)
                        {
                            ItemInfo a = items[j];
                            ItemInfo b = otherItems[k];

                            if (ItemInfo.CanMerge(a, b))
                            {
                                isInvalid = false;
                                break;
                            }
                        }

                        if (isInvalid)
                        {
                            items.RemoveAt(j--);
                        }
                    }
                }
            }

            // Sort items
            items.Sort();

            // Add items
            OnGroupsBegin();
            try
            {
                for (int i = 0; i < items.Count; i++)
                {
                    ItemInfo item = items[i];

                    // Group
                    LayoutElementsContainer itemLayout = OnGroup(layout, item.Display);

                    // Space
                    if (item.Space != null)
                    {
                        itemLayout.Space(item.Space.Height);
                    }

                    // Header
                    if (item.Header != null)
                    {
                        itemLayout.Header(item.Header);
                    }

                    try
                    {
                        // Peek values
                        ValueContainer itemValues = item.GetValues(Values);

                        // Spawn property editor
                        SpawnProperty(itemLayout, itemValues, item);
                    }
                    catch (Exception ex)
                    {
                        SE.Debug.Log("Failed to setup values and UI for item " + item);
                        SE.Debug.Log(ex.Message);
                        SE.Debug.Log(ex.StackTrace);
                        return;
                    }

                    // Expand all parent groups if need to
                    if (item.ExpandGroups)
                    {
                        Control c = itemLayout.ContainerControl;
                        do
                        {
                            if (c is DropPanel dropPanel)
                            {
                                dropPanel.Open(false);
                            }
                            else if (c is CustomEditorPresenter.PresenterPanel)
                            {
                                break;
                            }
                            c = c.Parent;
                        } while (c != null);
                    }
                }
            }
            finally
            {
                OnGroupsEnd();
            }
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            m_VisibleIfCaches = null;
            s_VisibleIfPropertiesListsCache = null;

            base.Deinitialize();
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            // Automatic refresh when value nullability changed
            if (m_IsNull != Values.IsNull)
            {
                RebuildLayout();
                return;
            }

            if (m_VisibleIfCaches != null)
            {
                if (s_VisibleIfPropertiesListsCache == null)
                {
                    s_VisibleIfPropertiesListsCache = new HashSet<PropertiesList>();
                }
                else
                {
                    s_VisibleIfPropertiesListsCache.Clear();
                }
                try
                {
                    // Update VisibleIf rules
                    for (int i = 0; i < m_VisibleIfCaches.Length; i++)
                    {
                        ref VisibleIfCache c = ref m_VisibleIfCaches[i];
                        if (c.Target == null)
                        {
                            break;
                        }

                        // Check rule (all objects must allow to show this property)
                        bool visible = true;
                        for (int j = 0; j < Values.Count; j++)
                        {
                            if (Values[j] != null && !c.GetValue(Values[j]))
                            {
                                visible = false;
                                break;
                            }
                        }

                        // Apply the visibility (note: there may be no label)
                        if (c.LabelIndex != -1 && c.PropertiesList != null &&
                            c.PropertiesList.Labels.Count > c.LabelIndex)
                        {
                            PropertyNameLabel label = c.PropertiesList.Labels[c.LabelIndex];
                            if (visible && Presenter != null && !string.IsNullOrEmpty(Presenter.SearchText))
                            {
                                bool match = label.Text.ToString().IndexOf(Presenter.SearchText,
                                    StringComparison.OrdinalIgnoreCase) >= 0;
                                if (!match)
                                {
                                    ContainerControl p = label.Parent;
                                    while (p != null)
                                    {
                                        if (p is DropPanel dropPanel)
                                        {
                                            string headerText = dropPanel.HeaderText;
                                            if (headerText != null && headerText.IndexOf(Presenter.SearchText,
                                                StringComparison.OrdinalIgnoreCase) >= 0)
                                            {
                                                match = true;
                                                break;
                                            }
                                        }
                                        p = p.Parent;
                                    }
                                }
                                visible = match;
                            }
                            label.Visible = visible;
                            for (int j = label.FirstChildControlIndex;
                                j < c.PropertiesList.Properties.Children.Count; j++)
                            {
                                Control child = c.PropertiesList.Properties.Children[j];
                                if (child is PropertyNameLabel)
                                {
                                    break;
                                }

                                child.Visible = visible;
                            }
                        }
                        if (c.Group != null)
                        {
                            c.Group.Panel.Visible = visible;
                        }
                        if (c.PropertiesList != null)
                        {
                            s_VisibleIfPropertiesListsCache.Add(c.PropertiesList.Properties);
                        }
                    }

                    // Hide properties lists with all labels being hidden
                    foreach (PropertiesList propertiesList in s_VisibleIfPropertiesListsCache)
                    {
                        propertiesList.Visible = propertiesList.Children.Any(c => c.Visible);
                    }

                    // Hide group panels with all properties lists hidden
                    foreach (PropertiesList propertiesList in s_VisibleIfPropertiesListsCache)
                    {
                        if (propertiesList.Parent is DropPanel dropPanel)
                        {
                            dropPanel.Visible = propertiesList.Visible ||
                                !dropPanel.Children.All(c => c is PropertiesList && !c.Visible);
                        }
                    }
                }
                catch (Exception ex)
                {
                    SE.Debug.Log(ex);
                    SE.Debug.LogError("Failed to update VisibleIf rules. " + ex.Message);

                    // Remove rules to prevent error in loop
                    m_VisibleIfCaches = null;
                }
            }
            if (Presenter != null && !string.IsNullOrEmpty(Presenter.SearchText))
            {
                Presenter.UpdateGroupsAndListsVisibility();
            }

            base.Refresh();
        }
    }
}

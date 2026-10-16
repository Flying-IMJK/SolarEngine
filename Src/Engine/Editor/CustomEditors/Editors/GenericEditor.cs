using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Reflection;
using System.Text;
using SE.GUI;
using SE.Editor.GUI;

namespace SE.Editor
{
    [HideInEditor]
    public class GenericEditor : CustomEditor
    {
        public sealed class ItemInfo : IComparable<ItemInfo>
        {
            public readonly MemberInfo Info;
            public readonly EditorOrderAttribute? Order;
            public readonly EditorDisplayAttribute? Display;
            public readonly VisibleIfAttribute[] VisibleIfs;
            public readonly SpaceAttribute? Space;
            public readonly HeaderAttribute? Header;
            public readonly bool ExpandGroups;
            public readonly CustomEditorAttribute? CustomEditor;
            public readonly CustomEditorAliasAttribute? CustomEditorAlias;
            public readonly bool IsReadOnly;
            public readonly string DisplayName;
            public readonly string? TooltipText;

            public ItemInfo(MemberInfo info)
            {
                Info = info;
                var attributes = info.GetCustomAttributes(true);
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

            public CustomEditor? OverrideEditor
            {
                get
                {
                    if (CustomEditor != null)
                        return (CustomEditor)Activator.CreateInstance(CustomEditor.Type)!;
                    if (CustomEditorAlias != null &&
                        CustomEditorsUtil.ResolveEditorAlias(CustomEditorAlias.TypeName) is { } type)
                        return (CustomEditor)Activator.CreateInstance(type)!;
                    return null;
                }
            }

            public int CompareTo(ItemInfo? other)
            {
                if (other == null)
                    return 1;
                if (Order != null)
                    return other.Order != null ? Order.Order - other.Order.Order : -1;
                if (other.Order != null)
                    return 1;
                if (Display?.Group != null && other.Display?.Group != null)
                {
                    int group = string.Compare(Display.Group, other.Display.Group, StringComparison.InvariantCulture);
                    if (group != 0)
                        return group;
                }
                return Info.MetadataToken.CompareTo(other.Info.MetadataToken);
            }

            public static bool CanMerge(ItemInfo a, ItemInfo b) =>
                a.Info.DeclaringType == b.Info.DeclaringType && a.Info.Name == b.Info.Name;

            internal static string GetPropertyNameUI(string name)
            {
                int length = name.Length;
                var builder = new StringBuilder(length + 8);
                int startIndex = 0;
                if (name.StartsWith("g_") || name.StartsWith("m_"))
                    startIndex = 2;
                if (name.StartsWith("_"))
                    startIndex = 1;
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

        public static List<ItemInfo> GetItemsForType(Type type, bool useProperties, bool useFields)
        {
            var items = new List<ItemInfo>();
            const BindingFlags flags = BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance;
            if (useProperties)
            {
                foreach (var property in type.GetProperties(flags))
                {
                    if (property.GetIndexParameters().Length != 0)
                    {
                        continue;
                    }

                    var attributes = property.GetCustomAttributes(true);

                    // 没有Get方法
                    if (property.GetMethod == null)
                    {
                        continue;
                    }

                    // 私有但是没有添加 ShowInEditorAttribute 属性
                    if (!property.GetMethod.IsPublic && !attributes.Any(x => x is ShowInEditorAttribute))
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
                foreach (var field in type.GetFields(flags))
                {
                    var attributes = field.GetCustomAttributes(true);

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
            var sources = new List<MemberInfo>();
            const BindingFlags flags = BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static;
            foreach (var rule in rules)
            {
                PropertyInfo? property = type.GetProperty(rule.MemberName, flags);
                if (property != null)
                {
                    if (property.GetMethod == null)
                        SE.Debug.LogError("Invalid VisibleIf rule. Property has missing getter " + rule.MemberName);
                    else if (property.PropertyType != typeof(bool))
                        SE.Debug.LogError("Invalid VisibleIf rule. Property has to return bool type " + rule.MemberName);
                    else
                        sources.Add(property);
                    continue;
                }

                FieldInfo? field = type.GetField(rule.MemberName, flags);
                if (field != null)
                {
                    if (field.FieldType != typeof(bool))
                        SE.Debug.LogError("Invalid VisibleIf rule. Field has to be bool type " + rule.MemberName);
                    else
                        sources.Add(field);
                    continue;
                }

                SE.Debug.LogError("Invalid VisibleIf rule. Cannot find member " + rule.MemberName);
            }
            return sources.ToArray();
        }

        private readonly List<(Control Control, MemberInfo[] Sources, bool[] Inversions)> _visibleIfItems = new();
        private bool _isNull;

        public override void Initialize(LayoutElementsContainer layout)
        {
            _visibleIfItems.Clear();
            _isNull = Values.IsNull;
            if (Values.Count == 0)
                return;
            if (!HasDifferentTypes && Values[0] == null)
            {
                Type type = Values.Type;
                if (Values.Info != null && CanEditValue)
                {
                    Type[] types;
                    if (type.IsAbstract || type.IsInterface)
                    {
                        types = AppDomain.CurrentDomain.GetAssemblies()
                            .SelectMany(assembly =>
                            {
                                try { return assembly.GetTypes(); }
                                catch (ReflectionTypeLoadException ex) { return ex.Types.OfType<Type>(); }
                            })
                            .Where(candidate => candidate != type && type.IsAssignableFrom(candidate) &&
                                !candidate.IsAbstract && !typeof(SE.Object).IsAssignableFrom(candidate) &&
                                candidate.GetConstructor(Type.EmptyTypes) != null)
                            .ToArray();
                    }
                    else
                    {
                        types = type.GetConstructor(Type.EmptyTypes) != null ? new[] { type } : Array.Empty<Type>();
                    }
                    if (types.Length != 0)
                    {
                        var button = new Button { Text = "+", Height = 20, Parent = layout.Space(20).ContainerControl };
                        if (types.Length == 1)
                            button.Clicked += () => SetValue(Activator.CreateInstance(types[0]));
                        else
                            button.Clicked += () =>
                            {
                                var menu = new ContextMenu();
                                foreach (Type candidate in types)
                                {
                                    Type selectedType = candidate;
                                    menu.AddButton(candidate.FullName ?? candidate.Name,
                                        () => SetValue(Activator.CreateInstance(selectedType)));
                                }
                                menu.Show(button, new Float2(0, button.Height));
                            };
                    }
                }
                layout.Label("<null>");
                return;
            }

            if (!Values.Type.IsArray && !Values.Type.IsValueType &&
                !typeof(SE.Object).IsAssignableFrom(Values.Type) &&
                (Values.Type.IsAbstract || Values.Type.IsInterface) &&
                Values[0]!.GetType() != Values.Type &&
                layout is LayoutElementsContainer.GroupElement valueGroup && CanEditValue)
            {
                valueGroup.AddHeaderButton("Reset value to null").Clicked += () => SetValue(null);
            }

            List<ItemInfo> items;
            if (!HasDifferentTypes)
            {
                items = GetItemsForType(Values[0]!.GetType());
            }
            else
            {
                Type[] types = ValuesTypes;
                items = new List<ItemInfo>(GetItemsForType(types[0]));
                for (int i = 1; i < types.Length && items.Count > 0; i++)
                {
                    var other = GetItemsForType(types[i]);
                    items.RemoveAll(item => !other.Any(candidate => ItemInfo.CanMerge(item, candidate)));
                }
            }
            items.Sort();

            var groups = new Dictionary<string, LayoutElementsContainer>();
            foreach (var item in items)
            {
                try
                {
                    Type memberType = item.Info switch
                    {
                        PropertyInfo property => property.PropertyType,
                        FieldInfo field => field.FieldType,
                        _ => typeof(object),
                    };
                    if (typeof(SE.Object).IsAssignableFrom(memberType))
                        continue;
                    LayoutElementsContainer itemLayout = layout;
                    if (!string.IsNullOrEmpty(item.Display?.Group) && item.Display.Group != EditorDisplayAttribute.InlineStyle)
                    {
                        string groupName = item.Display.Group;
                        if (!groups.TryGetValue(groupName, out itemLayout!))
                        {
                            itemLayout = layout.Group(groupName);
                            groups.Add(groupName, itemLayout);
                        }
                    }
                    ValueContainer itemValues = item.GetValues(Values);
                    if (item.Space != null)
                        itemLayout.Space(item.Space.Height);
                    if (item.Header != null)
                        itemLayout.Header(item.Header);
                    CustomEditor editor = itemLayout.Property(item.DisplayName, itemValues,
                        item.OverrideEditor, item.TooltipText);
                    editor.SetReadOnly(item.IsReadOnly);
                    if (item.VisibleIfs.Length != 0)
                    {
                        MemberInfo[] sources = GetVisibleIfSources(item.Info.DeclaringType!, item.VisibleIfs);
                        if (sources.Length != 0)
                            _visibleIfItems.Add((editor.Layout.Control, sources,
                                item.VisibleIfs.Select(rule => rule.Invert).ToArray()));
                    }
                    if (item.ExpandGroups)
                    {
                        Control? current = itemLayout.ContainerControl;
                        while (current != null)
                        {
                            if (current.Tag is LayoutElementsContainer.GroupElement group)
                                group.Open();
                            current = current.Parent;
                        }
                    }
                }
                catch (Exception ex)
                {
                    SE.Debug.Log($"Failed to setup values and UI for item {item.Info.Name}: {ex}");
                    return;
                }
            }
        }

        public override void Refresh()
        {
            base.Refresh();
            if (_isNull != Values.IsNull)
            {
                _isNull = Values.IsNull;
                RebuildLayoutOnRefresh();
                return;
            }
            foreach (var (control, sources, inversions) in _visibleIfItems)
            {
                bool visible = true;
                foreach (object? instance in Values)
                {
                    if (instance == null)
                        continue;
                    for (int i = 0; i < sources.Length; i++)
                    {
                        object? source = sources[i] switch
                        {
                            PropertyInfo property => property.GetValue(instance),
                            FieldInfo field => field.GetValue(instance),
                            _ => null,
                        };
                        bool state = (bool)source!;
                        if (inversions[i])
                            state = !state;
                        if (!state)
                            visible = false;
                    }
                }
                Presenter.SetConditionalVisible(control, visible);
            }
        }
    }
}

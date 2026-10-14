using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.Json;
using SE.GUI;
using SE.Editor.GUI;

namespace SE.Editor
{
    [HideInEditor]
    public abstract class LayoutElementsContainer : LayoutElement
    {
        internal bool isRootGroup = true;
        internal LayoutElementsContainer? _parent;
        public readonly List<LayoutElement> Children = new();
        public readonly List<CustomEditor> Editors = new();
        public abstract ContainerControl ContainerControl { get; }
        public override Control Control => ContainerControl;

        internal CustomEditorPresenter Presenter
        {
            get
            {
                LayoutElementsContainer current = this;
                while (current._parent != null)
                    current = current._parent;
                return (CustomEditorPresenter)current;
            }
        }

        public LayoutElementsContainer VerticalPanel()
        {
            var result = new PanelElement(new VerticalPanel());
            Add(result);
            return result;
        }

        public LayoutElementsContainer Group(string title)
        {
            string path = GetLayoutCachePath(title);
            bool cache = (Presenter.Features & FeatureFlags.CacheExpandedGroups) != 0;
            var result = new GroupElement(title, cache ? Presenter.GetGroupClosed(path, !isRootGroup) : !isRootGroup);
            if (cache)
                result.ClosedChanged += closed => Presenter.SetGroupClosed(path, closed);
            result.isRootGroup = false;
            Add(result);
            return result;
        }

        private string GetLayoutCachePath(string name)
        {
            LayoutElementsContainer? container = this;
            while (container != null && container is not CustomEditorPresenter)
            {
                if (container is GroupElement group)
                    name = group.Title + "/" + name;
                container = container._parent;
            }
            return name;
        }

        public LayoutElement Label(string text)
        {
            var label = new Label { Text = text, Height = 18, HorizontalAlignment = TextAlignment.Near };
            return AddControl(label);
        }

        public LayoutElement Header(string text)
        {
            var result = Label(text);
            ((Label)result.Control).Font = new FontReference(SE.GUI.Style.Current.FontLarge);
            return result;
        }

        public LayoutElement Header(HeaderAttribute header)
        {
            var element = Header(header.Text);
            var label = (Label)element.Control;
            if (header.FontSize > 0)
            {
                var font = label.Font;
                font.Size = header.FontSize;
                label.Font = font;
            }
            if (header.Color > 0)
                label.TextColor = Color.FromRGB(header.Color);
            if (label.Font.GetFont() is { } resolvedFont)
                label.Height = resolvedFont.MeasureText(header.Text, TextLayoutOptions.Default).Y;
            return element;
        }

        public LayoutElementsContainer Space(float height)
        {
            var result = new PanelElement(new VerticalPanel { Height = height, AutoSize = false });
            Add(result);
            return result;
        }

        public CustomEditor Property(string name, ValueContainer values, CustomEditor? overrideEditor = null,
            string? tooltip = null, DisplayStyle? displayStyle = null)
        {
            CustomEditor editor = overrideEditor ?? CustomEditorsUtil.CreateEditor(values);
            DisplayStyle style = displayStyle ?? editor.Style;
            if (style == DisplayStyle.Group)
            {
                var group = (GroupElement)Group(name);
                group.Control.TooltipText = tooltip ?? string.Empty;
                group.LinkEditor(editor);
                Editors.Add(editor);
                editor.Initialize(Presenter, group, values);
                return editor;
            }
            if (style == DisplayStyle.InlineIntoParent || name == EditorDisplayAttribute.InlineStyle)
            {
                Editors.Add(editor);
                editor.Initialize(Presenter, this, values);
                return editor;
            }
            var propertyLayout = new RowElement(new PropertyRowControl());
            Add(propertyLayout);
            var label = new PropertyNameLabel(name)
            {
                TooltipText = tooltip ?? string.Empty,
                Parent = propertyLayout.ContainerControl,
            };
            propertyLayout.Children.Add(new ControlElement(label));
            Editors.Add(editor);
            label.LinkEditor(editor);
            editor.Initialize(Presenter, propertyLayout, values);
            return editor;
        }

        public void ClearLayout()
        {
            Children.Clear();
            Editors.Clear();
        }

        private void Add(LayoutElementsContainer element)
        {
            element._parent = this;
            Children.Add(element);
            element.Control.Parent = ContainerControl;
        }

        private LayoutElement AddControl(Control control)
        {
            var element = new ControlElement(control);
            Children.Add(element);
            control.Parent = ContainerControl;
            return element;
        }

        private sealed class ControlElement : LayoutElement
        {
            public ControlElement(Control control) => Control = control;
            public override Control Control { get; }
        }

        private sealed class PanelElement : LayoutElementsContainer
        {
            public PanelElement(VerticalPanel panel) => ContainerControl = panel;
            public override ContainerControl ContainerControl { get; }
        }

        private sealed class RowElement : LayoutElementsContainer
        {
            public RowElement(PropertyRowControl row) => ContainerControl = row;
            public override ContainerControl ContainerControl { get; }
        }

        private sealed class PropertyRowControl : ContainerControl
        {
            public PropertyRowControl() => Height = 20;

            protected override void PerformLayoutBeforeChildren()
            {
                base.PerformLayoutBeforeChildren();
                float valuesHeight = 0;
                for (int i = 1; i < ChildrenCount; i++)
                    valuesHeight += Children[i].Height + (i > 1 ? 2 : 0);
                Height = Mathf.Max(20, valuesHeight);
                float labelWidth = Width * 0.42f;
                if (ChildrenCount > 0)
                    Children[0].Bounds = new Rectangle(0, 0, labelWidth, Height);
                float valueY = 0;
                for (int i = 1; i < ChildrenCount; i++)
                {
                    Control child = Children[i];
                    child.Bounds = new Rectangle(labelWidth, valueY, Width - labelWidth, child.Height);
                    valueY += child.Height + 2;
                }
            }
        }

        internal sealed class GroupElement : LayoutElementsContainer
        {
            private readonly VerticalPanel _outer;
            private readonly VerticalPanel _body;
            private readonly GroupHeaderButton _header;
            private CustomEditor? _linkedEditor;
            public readonly string Title;
            public event Action<bool>? ClosedChanged;
            public event Action<ContextMenu>? SetupContextMenu;
            public bool IsClosed { get; private set; }

            public GroupElement(string title, bool closed)
            {
                Title = title;
                _outer = new VerticalPanel { Margin = new Margin(0, 0, 2, 2) };
                _outer.Tag = this;
                _header = new GroupHeaderButton(this) { Text = title, Height = 22, Parent = _outer };
                _body = new VerticalPanel { Margin = new Margin(7, 7, 2, 2), Parent = _outer };
                _body.Tag = this;
                _header.Clicked += () => SetClosed(!IsClosed);
                _header.RightClicked += ShowContextMenu;
                SetClosed(closed);
            }

            public override Control Control => _outer;
            public override ContainerControl ContainerControl => _body;

            public void Open() => SetClosed(false);
            public void Close() => SetClosed(true);

            public void LinkEditor(CustomEditor editor)
            {
                _linkedEditor = editor;
            }

            public Button AddHeaderButton(string tooltip)
            {
                return new Button
                {
                    Text = "×",
                    TooltipText = tooltip,
                    Size = new Float2(18, 18),
                    AnchorPreset = AnchorPresets.MiddleRight,
                    Parent = _header,
                };
            }

            private void ShowContextMenu(Float2 location)
            {
                var menu = new ContextMenu();
                if (_linkedEditor != null)
                {
                    PropertyNameLabel.PopulateEditorActions(menu, _linkedEditor);
                }
                else
                {
                    var editors = GetAllEditors(this).ToArray();
                    var revertReference = menu.AddButton("Revert to Prefab", () => Revert(editors, false));
                    revertReference.Enabled = editors.Any(editor => editor.CanRevertReferenceValue);
                    var revertDefault = menu.AddButton("Reset to default", () => Revert(editors, true));
                    revertDefault.Enabled = editors.Any(editor => editor.CanRevertDefaultValue);
                    menu.AddSeparator();
                    var copy = menu.AddButton("Copy", () => CopyGroup(this));
                    copy.Enabled = CanCopyGroup(this);
                    var paste = menu.AddButton("Paste", () => PasteGroup(this));
                    paste.Enabled = CanPasteGroup(this);
                }
                SetupContextMenu?.Invoke(menu);
                menu.Show(_header, location);
            }

            private static IEnumerable<CustomEditor> GetAllEditors(LayoutElementsContainer layout)
            {
                foreach (var editor in layout.Editors)
                {
                    yield return editor;
                }

                foreach (var child in layout.Children)
                {
                    if (child is LayoutElementsContainer nested)
                    {
                        foreach (var editor in GetAllEditors(nested))
                        {
                            yield return editor;
                        }
                    }
                }
            }

            private static void Revert(IEnumerable<CustomEditor> editors, bool toDefault)
            {
                foreach (var editor in editors)
                {
                    if (toDefault && editor.CanRevertDefaultValue)
                        editor.RevertToDefaultValue();
                    else if (!toDefault && editor.CanRevertReferenceValue)
                        editor.RevertToReferenceValue();
                }
            }

            private static LayoutElementsContainer? FirstNested(LayoutElementsContainer layout) =>
                layout.Children.OfType<LayoutElementsContainer>().FirstOrDefault();

            private static bool CanCopyGroup(LayoutElementsContainer layout) =>
                layout.Editors.Count != 0 || FirstNested(layout) != null;

            private static bool CanPasteGroup(LayoutElementsContainer layout)
            {
                if (layout.Editors.Count == 1)
                    return layout.Editors[0].CanPaste;
                if (layout.Editors.Count != 0)
                    return !string.IsNullOrEmpty(Clipboard.Text);
                return FirstNested(layout) is { } child && CanPasteGroup(child);
            }

            private static void CopyGroup(LayoutElementsContainer layout)
            {
                if (layout.Editors.Count == 1)
                {
                    layout.Editors[0].Copy();
                }
                else if (layout.Editors.Count != 0)
                {
                    var values = new string[layout.Editors.Count];
                    for (int i = 0; i < values.Length; i++)
                    {
                        layout.Editors[i].Copy();
                        values[i] = Clipboard.Text;
                    }
                    Clipboard.Text = JsonSerializer.Serialize(values);
                }
                else if (FirstNested(layout) is { } child)
                {
                    CopyGroup(child);
                }
            }

            private static void PasteGroup(LayoutElementsContainer layout)
            {
                if (layout.Editors.Count == 1)
                {
                    layout.Editors[0].Paste();
                }
                else if (layout.Editors.Count != 0)
                {
                    string text = Clipboard.Text;
                    if (string.IsNullOrEmpty(text))
                        return;
                    try
                    {
                        string[]? values = JsonSerializer.Deserialize<string[]>(text);
                        if (values == null || values.Length != layout.Editors.Count)
                            return;
                        for (int i = 0; i < values.Length; i++)
                        {
                            Clipboard.Text = values[i];
                            layout.Editors[i].Paste();
                        }
                    }
                    catch (JsonException)
                    {
                    }
                    finally
                    {
                        Clipboard.Text = text;
                    }
                }
                else if (FirstNested(layout) is { } child)
                {
                    PasteGroup(child);
                }
            }

            private void SetClosed(bool closed)
            {
                if (IsClosed == closed)
                {
                    return;
                }
                IsClosed = closed;
                _body.Visible = !closed;
                ClosedChanged?.Invoke(closed);
            }

            private sealed class GroupHeaderButton : Button
            {
                private readonly GroupElement _group;
                public event Action<Float2>? RightClicked;

                public GroupHeaderButton(GroupElement group)
                {
                    _group = group;
                    HasBorder = false;
                }

                public override void DrawSelf()
                {
                    var style = SE.GUI.Style.Current;
                    var background = isPressed ? BackgroundColorSelected :
                        IsMouseOver || IsNavFocused ? BackgroundColorHighlighted : BackgroundColor;
                    var foreground = EnabledInHierarchy ? TextColor : style.ForegroundDisabled;
                    var bounds = new Rectangle(Float2.Zero, Size);
                    Render2D.FillRectangle(bounds, background);
                    Render2D.FillRectangle(new Rectangle(0, Height - 1, Width, 1), BorderColor);

                    const float arrowSize = 12;
                    Render2D.DrawSprite(_group.IsClosed ? style.ArrowRight : style.ArrowDown,
                        new Rectangle(5, (Height - arrowSize) * 0.5f, arrowSize, arrowSize),
                        EnabledInHierarchy ? style.ForegroundGrey : style.ForegroundDisabled);

                    float textWidth = Mathf.Max(0, Width - 26 - (ChildrenCount > 0 ? 22 : 0));
                    var textBounds = new Rectangle(24, 0, textWidth, Height);
                    Render2D.PushClip(textBounds);
                    Render2D.RenderText(Font, TextMaterial, Text, textBounds, foreground,
                        TextAlignment.Near, TextAlignment.Center);
                    Render2D.PopClip();
                }

                public override bool OnMouseUp(Float2 location, MouseButton button)
                {
                    if (button == MouseButton.Right)
                    {
                        RightClicked?.Invoke(location);
                        return true;
                    }
                    return base.OnMouseUp(location, button);
                }
            }
        }
    }
}

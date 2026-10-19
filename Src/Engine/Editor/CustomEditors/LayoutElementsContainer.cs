using System;
using System.Collections.Generic;
using SE.GUI;
using SE.Editor.GUI;

namespace SE.Editor
{
    [HideInEditor]
    public abstract class LayoutElementsContainer : LayoutElement
    {
        internal bool isRootGroup = true;
        internal LayoutElementsContainer _parent;
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
                {
                    current = current._parent;
                }
                return (CustomEditorPresenter)current;
            }
        }

        public VerticalPanelElement VerticalPanel()
        {
            VerticalPanelElement result = new VerticalPanelElement();
            Add(result);
            return result;
        }

        public GroupElement Group(string title, CustomEditor linkedEditor, bool useTransparentHeader = false)
        {
            GroupElement element = Group(title, useTransparentHeader);
            element.Panel.Tag = linkedEditor;
            element.Panel.MouseButtonRightClicked += (panel, location) =>
                OnGroupPanelMouseButtonRightClicked(element, panel, location);
            return element;
        }

        public GroupElement Group(string title, bool useTransparentHeader = false)
        {
            string path = GetLayoutCachePath(title);
            bool cache = (Presenter.Features & FeatureFlags.CacheExpandedGroups) != 0;
            GroupElement result = new GroupElement();
            result.Panel.HeaderText = title;
            result.Panel.IsClosed = cache ? Presenter.GetGroupClosed(path, !isRootGroup) : !isRootGroup;
            if (cache)
            {
                result.Panel.IsClosedChanged += panel => Presenter.SetGroupClosed(path, panel.IsClosed);
            }
            if (useTransparentHeader)
            {
                result.Panel.EnableDropDownIcon = true;
                result.Panel.EnableContainmentLines = false;
                result.Panel.HeaderColor = result.Panel.HeaderColorMouseOver = Color.Transparent;
            }
            result.isRootGroup = false;
            AddElement(result);
            return result;
        }

        private void OnGroupPanelMouseButtonRightClicked(GroupElement element, DropPanel groupPanel, Float2 location)
        {
            CustomEditor linkedEditor = (CustomEditor)groupPanel.Tag;
            ContextMenu menu = new ContextMenu();

            ContextMenuButton revertToPrefab = menu.AddButton("Revert to Prefab", linkedEditor.RevertToReferenceValue);
            revertToPrefab.Enabled = linkedEditor.CanRevertReferenceValue;
            ContextMenuButton resetToDefault = menu.AddButton("Reset to default", linkedEditor.RevertToDefaultValue);
            resetToDefault.Enabled = linkedEditor.CanRevertDefaultValue;
            menu.AddSeparator();
            menu.AddButton("Copy", linkedEditor.Copy);
            ContextMenuButton paste = menu.AddButton("Paste", linkedEditor.Paste);
            paste.Enabled = linkedEditor.CanPaste;
            element.SetupContextMenu?.Invoke(menu, groupPanel);

            menu.Show(groupPanel, location);
        }

        private string GetLayoutCachePath(string name)
        {
            LayoutElementsContainer container = this;
            while (container != null && container is not CustomEditorPresenter)
            {
                if (container is GroupElement group)
                {
                    name = group.Panel.HeaderText + "/" + name;
                }
                container = container._parent;
            }
            return name;
        }

        public LabelElement Label(string text, TextAlignment horizontalAlignment = TextAlignment.Near)
        {
            LabelElement element = new LabelElement();
            element.Label.Text = text;
            element.Label.HorizontalAlignment = horizontalAlignment;
            AddElement(element);
            return element;
        }

        public LabelElement Header(string text)
        {
            LabelElement result = Label(text);
            ((Label)result.Control).Font = new FontReference(SE.GUI.Style.Current.FontLarge);
            return result;
        }

        public LabelElement Header(HeaderAttribute header)
        {
            LabelElement element = Header(header.Text);
            Label label = (Label)element.Control;
            if (header.FontSize > 0)
            {
                FontReference font = label.Font;
                font.Size = header.FontSize;
                label.Font = font;
            }
            if (header.Color > 0)
            {
                label.TextColor = Color.FromRGB(header.Color);
            }
            if (label.Font.GetFont() is { } resolvedFont)
            {
                label.Height = resolvedFont.MeasureText(header.Text, TextLayoutOptions.Default).Y;
            }
            return element;
        }

        public SpaceElement Space(float height)
        {
            SpaceElement element = new SpaceElement();
            element.Init(height);
            AddElement(element);
            return element;
        }

        /// <summary>Adds an editor for values directly into this container.</summary>
        public CustomEditor Object(ValueContainer values, CustomEditor overrideEditor = null)
        {
            CustomEditor editor = CustomEditorsUtil.CreateEditor(values, overrideEditor);
            OnAddEditor(editor);
            editor.Initialize(Presenter, this, values);
            return editor;
        }

        /// <summary>Adds a named property using its editor's display style.</summary>
        public CustomEditor Property(string name, ValueContainer values, CustomEditor overrideEditor = null,
            string tooltip = null, DisplayStyle? displayStyle = null)
        {
            CustomEditor editor = CustomEditorsUtil.CreateEditor(values, overrideEditor);
            DisplayStyle style = displayStyle ?? editor.Style;
            if (style == DisplayStyle.InlineIntoParent || name == EditorDisplayAttribute.InlineStyle)
            {
                return Object(values, editor);
            }
            if (style == DisplayStyle.Group)
            {
                GroupElement group = Group(name, editor, true);
                group.Control.TooltipText = tooltip ?? string.Empty;
                return group.Object(values, editor);
            }
            PropertiesListElement property = AddPropertyItem(name, tooltip);
            int start = property.Properties.Children.Count;
            CustomEditor result = property.Object(values, editor);
            if (this is PropertiesListElement && editor.ParentEditor?.LinkedLabel is { } parentLabel &&
                parentLabel.FirstChildControlContainer == null)
            {
                parentLabel.FirstChildControlIndex = start;
                parentLabel.FirstChildControlContainer = property.Properties;
            }
            return result;
        }

        private PropertiesListElement AddPropertyItem()
        {
            if (Children.Count > 0 && Children[^1] is PropertiesListElement previous)
            {
                return previous;
            }
            PropertiesListElement element = new PropertiesListElement();
            AddElement(element);
            return element;
        }

        public PropertiesListElement AddPropertyItem(string name, string tooltip = null)
        {
            PropertiesListElement element = AddPropertyItem();
            element.OnAddProperty(name, tooltip);
            return element;
        }

        public PropertiesListElement AddPropertyItem(PropertyNameLabel label, string tooltip = null)
        {
            ArgumentNullException.ThrowIfNull(label);
            PropertiesListElement element = AddPropertyItem();
            element.OnAddProperty(label, tooltip);
            return element;
        }

        protected virtual void OnAddEditor(CustomEditor editor)
        {
            CustomEditor.CurrentCustomEditor?.OnChildCreated(editor);
            Editors.Add(editor);
        }

        /// <summary>Adds a caller-defined control.</summary>
        public CustomElement<T> Custom<T>() where T : Control, new()
        {
            CustomElement<T> element = new CustomElement<T>();
            AddElement(element);
            return element;
        }

        /// <summary>Adds a caller-defined container with layout helpers.</summary>
        public CustomElementsContainer<T> CustomContainer<T>() where T : ContainerControl, new()
        {
            CustomElementsContainer<T> element = new CustomElementsContainer<T>();
            AddElement(element);
            return element;
        }

        /// <summary>Adds a button.</summary>
        public ButtonElement Button(string text, string tooltip = null)
        {
            ButtonElement element = new ButtonElement();
            element.Button.Text = text;
            element.Button.TooltipText = tooltip ?? string.Empty;
            AddElement(element);
            return element;
        }

        /// <summary>Adds a single-line or multiline text box.</summary>
        public TextBoxElement TextBox(bool multiline = false)
        {
            TextBoxElement element = new TextBoxElement(multiline);
            AddElement(element);
            return element;
        }

        /// <summary>Adds a checkbox.</summary>
        public CheckBoxElement Checkbox()
        {
            CheckBoxElement element = new CheckBoxElement();
            AddElement(element);
            return element;
        }

        public virtual void ClearLayout()
        {
            Children.Clear();
            Editors.Clear();
        }

        /// <summary>Registers and parents a layout element.</summary>
        public void AddElement(LayoutElement element)
        {
            ArgumentNullException.ThrowIfNull(element);
            OnAddElement(element);
        }

        protected virtual void OnAddElement(LayoutElement element)
        {
            if (element is LayoutElementsContainer container)
            {
                container._parent = this;
            }
            Children.Add(element);
            element.Control.Parent = ContainerControl;
        }

        private void Add(LayoutElementsContainer element) => AddElement(element);

    }
}

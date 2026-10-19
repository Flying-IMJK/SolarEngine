
using System;
using SE.Editor.GUI;
using SE.Editor.SceneGraph;
using SE.GUI;

namespace SE.Editor
{
    /// <summary>
    /// A custom control type used to pick reference to <see cref="SE.Object"/>.
    /// </summary>
    /// <seealso cref="SE.GUI.Control" />
    [HideInEditor]
    public class ObjectRefPickerControl : Control
    {
        private Type _type;
        private ActorTreeNode m_LinkedTreeNode;
        private Object m_Value;
        private string m_ValueName;
        private bool m_SupportsPickDropDown;

        private bool m_IsMouseDown;
        private Float2 m_MouseDownPos;
        private Float2 m_MousePos;

        private bool m_HasValidDragOver;
        private DragActors m_DragActors;
        private DragAssets m_DragAssets;
        private DragHandlers m_DragHandlers;

        /// <summary>
        /// Gets or sets the allowed objects type (given type and all subclasses). Must be <see cref="Object"/> type of
        /// any subclass or a scripting interface.
        /// </summary>
        public Type Type
        {
            get => _type;
            set
            {
                if (_type == value)
                {
                    return;
                }
                if (value == null || (!value.IsInterface && value != typeof(Object) && !typeof(Object)
                    .IsAssignableFrom(value)))
                {
                    throw new ArgumentException(string.Format("Invalid type for ObjectRefEditor. Input type: {0}",
                        value != null ? value.FullName : "null"));
                }

                _type = value;
                m_SupportsPickDropDown = value.IsInterface ||
                                        typeof(Actor).IsAssignableFrom(value);

                // Deselect value if it's not valid now
                if (!IsValid(m_Value))
                {
                    Value = null;
                }
            }
        }

        /// <summary>
        /// Gets or sets the selected object value.
        /// </summary>
        public Object Value
        {
            get => m_Value;
            set
            {
                if (m_Value == value)
                {
                    return;
                }
                if (!IsValid(value))
                {
                    value = null;
                }

                // Special case for missing objects (eg. referenced actor in script that is deleted in editor)
                if (value != null && (Object.GetUnmanagedPtr(value) == IntPtr.Zero || value.ID == Guid.Empty))
                {
                    value = null;
                }

                m_Value = value;

                // Get name to display
                if (m_Value != null)
                {
                    m_ValueName = m_Value.ToString();
                }
                else
                {
                    m_ValueName = string.Empty;
                }

                // Update tooltip
                if (m_Value is SceneObject sceneObject)
                {
                    TooltipText = sceneObject.ToString();
                }
                else
                {
                    TooltipText = string.Empty;
                }

                OnValueChanged();
            }
        }

        /// <summary>
        /// Gets or sets the selected object value by identifier.
        /// </summary>
        public Guid ValueID
        {
            get => m_Value ? m_Value.ID : Guid.Empty;
            set => Value = Object.Find<Object>(ref value);
        }

        /// <summary>
        /// Occurs when value gets changed.
        /// </summary>
        public event Action ValueChanged;

        /// <summary>
        /// The custom callback for objects validation. Cane be used to implement a rule for objects to pick.
        /// </summary>
        public Func<Object, Type, bool> CheckValid;

        /// <summary>
        /// Utility flag used to indicate that there are different values assigned to this reference editor and user
        /// should be informed about it.
        /// </summary>
        public bool DifferentValues;

        /// <summary>
        /// Initializes a new instance of the <see cref="ObjectRefPickerControl"/> class.
        /// </summary>
        public ObjectRefPickerControl()
        : base(0, 0, 50, 16)
        {
            _type = typeof(Object);
        }

        /// <summary>
        /// Object validation check routine.
        /// </summary>
        /// <param name="obj">Input object to check.</param>
        /// <returns>True if it can be assigned, otherwise false.</returns>
        protected virtual bool IsValid(Object obj)
        {
            Type type = obj?.GetType();
            return obj == null || (!_type.IsInterface || obj is SceneObject) && _type.IsAssignableFrom(type)
                && (CheckValid == null || CheckValid(obj, type));
        }

        private void ShowDropDownMenu()
        {
            if (!EnabledInHierarchy)
            {
                return;
            }
            Focus();
            ActorSearchPopup.Show(this, new Float2(0, Height), actor => IsValid(actor),
                actor => SetDropDownResult(actor));
        }

        private void SetDropDownResult(Object value)
        {
            Value = value;
            RootWindow().Focus();
            Focus();
        }

        /// <summary>
        /// Called when value gets changed.
        /// </summary>
        protected virtual void OnValueChanged()
        {
            ValueChanged?.Invoke();
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // Cache data
            Style style = Style.Current;
            bool isSelected = m_Value != null;
            bool isEnabled = EnabledInHierarchy;
            Rectangle frameRect = new Rectangle(0, 0, Width, 16);
            if (isSelected)
            {
                frameRect.Width -= 16;
            }
            if (m_SupportsPickDropDown && isEnabled)
            {
                frameRect.Width -= 16;
            }
            Rectangle nameRect = new Rectangle(2, 1, frameRect.Width - 4, 14);
            Rectangle button1Rect = new Rectangle(nameRect.Right + 2, 1, 14, 14);
            Rectangle button2Rect = new Rectangle(button1Rect.Right + 2, 1, 14, 14);

            // Draw frame
            Render2D.DrawRectangle(frameRect, isEnabled && (IsMouseOver || IsNavFocused) ? style.BorderHighlighted
                : style.BorderNormal);

            // Check if has item selected
            if (DifferentValues)
            {
                // Draw info
                Render2D.PushClip(nameRect);
                Render2D.DrawText(style.FontMedium, Type != null ? $"Multiple Values ({_type.Name})" : "-",
                    nameRect, isEnabled ? style.ForegroundGrey : style.ForegroundGrey.AlphaMultiplied(0.75f),
                        TextAlignment.Near, TextAlignment.Center);
                Render2D.PopClip();
            }
            else if (isSelected)
            {
                // Draw name
                Render2D.PushClip(nameRect);
                Render2D.DrawText(style.FontMedium, m_ValueName, nameRect, isEnabled ? style.Foreground : style
                    .ForegroundDisabled, TextAlignment.Near, TextAlignment.Center);
                Render2D.PopClip();

                // Draw deselect button
                Render2D.DrawSprite(EditorIcons.Cross16, button1Rect, isEnabled && button1Rect.Contains(m_MousePos)
                    ? style.Foreground : style.ForegroundGrey);
            }
            else
            {
                // Draw info
                Render2D.PushClip(nameRect);
                Render2D.DrawText(style.FontMedium, Type != null ? $"None ({_type.Name})" : "-", nameRect,
                    isEnabled ? style.ForegroundGrey : style.ForegroundGrey.AlphaMultiplied(0.75f), TextAlignment
                        .Near, TextAlignment.Center);
                Render2D.PopClip();
            }

            // Draw picker button
            if (m_SupportsPickDropDown && isEnabled)
            {
                Rectangle pickerRect = isSelected ? button2Rect : button1Rect;
                Render2D.DrawSprite(EditorIcons.ArrowDown12, pickerRect, isEnabled && pickerRect
                    .Contains(m_MousePos) ? style.Foreground : style.ForegroundGrey);
            }

            // Check if drag is over
            if (IsDragOver() && m_HasValidDragOver)
            {
                Rectangle bounds = new Rectangle(Float2.Zero, Size);
                Render2D.FillRectangle(bounds, style.Selection);
                Render2D.DrawRectangle(bounds, style.SelectionBorder);
            }
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            m_MousePos = location;
            m_MouseDownPos = Float2.Minimum;

            base.OnMouseEnter(location);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            m_MousePos = Float2.Minimum;

            // Check if start drag drop
            if (m_IsMouseDown)
            {
                // Do the drag
                DoDrag();

                // Clear flag
                m_IsMouseDown = false;
            }

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            m_MousePos = location;

            // Check if start drag drop
            if (m_IsMouseDown && Float2.Distance(location, m_MouseDownPos) > 10.0f)
            {
                // Do the drag
                DoDrag();

                // Clear flag
                m_IsMouseDown = false;
            }

            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (!EnabledInHierarchy)
            {
                return false;
            }
            if (button == MouseButton.Left)
            {
                // Clear flag
                m_IsMouseDown = false;
            }

            // Cache data
            bool isSelected = m_Value != null;
            Rectangle frameRect = new Rectangle(0, 0, Width, 16);
            if (isSelected)
            {
                frameRect.Width -= 16;
            }
            if (m_SupportsPickDropDown)
            {
                frameRect.Width -= 16;
            }
            Rectangle nameRect = new Rectangle(2, 1, frameRect.Width - 4, 14);
            Rectangle button1Rect = new Rectangle(nameRect.Right + 2, 1, 14, 14);
            Rectangle button2Rect = new Rectangle(button1Rect.Right + 2, 1, 14, 14);

            // Deselect
            if (m_Value != null && button1Rect.Contains(ref location))
            {
                Focus();
                Value = null;
                return true;
            }

            // Picker dropdown menu
            if (m_SupportsPickDropDown && (isSelected ? button2Rect : button1Rect).Contains(ref location))
            {
                ShowDropDownMenu();
                return true;
            }

            if (button == MouseButton.Left)
            {
                m_IsMouseDown = false;

                // Highlight actor or script reference
                if (!m_HasValidDragOver && !IsDragOver())
                {
                    Actor actor = m_Value as Actor;
                    if (actor != null)
                    {
                        m_LinkedTreeNode = Editor.Instance.Scene.GetActorNode(actor)?.TreeNode;
                        if (m_LinkedTreeNode != null)
                        {
                            m_LinkedTreeNode.ExpandAllParents();
                            Editor.Instance.Windows.SceneWin.SceneTreePanel.ScrollViewTo(m_LinkedTreeNode, true);
                        }
                        return true;
                    }
                }

                // Reset valid drag over if still true at this point
                if (m_HasValidDragOver)
                {
                    m_HasValidDragOver = false;
                }
            }
            if (button == MouseButton.Middle)
            {
                // Clear value
                Focus();
                Value = null;
                return true;
            }
            if (button == MouseButton.Right && m_SupportsPickDropDown)
            {
                // Show picker
                ShowDropDownMenu();
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                // Set flag
                m_IsMouseDown = true;
                m_MouseDownPos = location;
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            Focus();

            // Check if has object selected
            if (m_Value != null)
            {
                if (m_LinkedTreeNode != null)
                {
                    m_LinkedTreeNode = null;
                }

                // Select object
                if (m_Value is Actor actor)
                {
                    Select(actor);
                }
                else if (m_Value is Asset asset)
                {
                    Editor.Instance.Windows.ContentWin.Select(asset);
                }
            }

            return base.OnMouseDoubleClick(location, button);
        }

        /// <inheritdoc />
        public override void OnSubmit()
        {
            base.OnSubmit();

            // Picker dropdown menu
            if (m_SupportsPickDropDown)
            {
                ShowDropDownMenu();
            }
        }

        private void Select(Actor actor)
        {
            Editor.Instance.Scene.Select(actor);
        }

        private void DoDrag()
        {
            if (m_Value is Actor actor && Editor.Instance.Scene.GetActorNode(actor) is { } node)
            {
                DoDragDrop(DragActors.GetDragData(node));
            }
            else if (m_Value is Asset asset)
            {
                DoDragDrop(DragItems.GetDragData(asset.GetPath()));
            }
        }

        private DragDropEffect DragEffect => m_HasValidDragOver ? DragDropEffect.Move : DragDropEffect.None;

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            base.OnDragEnter(ref location, data);
            if (!EnabledInHierarchy)
            {
                return DragDropEffect.None;
            }
            m_DragActors ??= new DragActors(ValidateDragActor, id => Editor.Instance.Scene.GetActorNode(id));
            m_DragAssets ??= new DragAssets(item => item is AssetItem asset && ValidateDragAsset(asset),
                path => Editor.Instance.AssetDatabase.Find(path));
            m_DragHandlers ??= new DragHandlers { m_DragActors, m_DragAssets };
            m_HasValidDragOver = m_DragHandlers.OnDragEnter(data) != DragDropEffect.None;
            return DragEffect;
        }

        private bool ValidateDragActor(ActorGraphNode node) => node.ParentScene != null && IsValid(node.Actor);

        private bool ValidateDragAsset(AssetItem item)
        {
            if (!typeof(Asset).IsAssignableFrom(_type))
            {
                return false;
            }
            Guid id = item.Id;
            Object asset = Object.Find(ref id, typeof(Asset));
            return asset != null && IsValid(asset);
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            base.OnDragMove(ref location, data);

            return DragEffect;
        }

        /// <inheritdoc />
        public override void OnDragLeave()
        {
            m_HasValidDragOver = false;
            m_DragHandlers.OnDragLeave();

            base.OnDragLeave();
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
        {
            DragDropEffect result = DragEffect;

            base.OnDragDrop(ref location, data);

            if (m_DragActors.HasValidDrag)
            {
                Value = m_DragActors.Objects[0].Actor;
            }
            else if (m_DragAssets.HasValidDrag)
            {
                ValueID = ((AssetItem)m_DragAssets.Objects[0]).Id;
            }

            return result;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            m_Value = null;
            _type = null;
            m_ValueName = null;
            m_LinkedTreeNode = null;
            ValueChanged = null;
            CheckValid = null;
            m_DragHandlers = null;
            m_DragActors = null;
            m_DragAssets = null;

            base.OnDestroy();
        }
    }

    /// <summary>
    /// Default implementation of the inspector used to edit reference to the <see cref="SE.Object"/>.
    /// </summary>
    [CustomEditor(typeof(Object)), DefaultEditor]
    public sealed class ObjectRefEditor : CustomEditor
    {
        private CustomElement<ObjectRefPickerControl> m_Element;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            m_Element = layout.Custom<ObjectRefPickerControl>();
            m_Element.CustomControl.Type = Values.Type != typeof(object) || Values[0] == null ? Values.Type
                : Values[0]!.GetType();
            m_Element.CustomControl.ValueChanged += () => SetValue(m_Element.CustomControl.Value);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            bool differentValues = HasDifferentValues;
            m_Element.CustomControl.DifferentValues = differentValues;
            if (!differentValues)
            {
                m_Element.CustomControl.Value = Values[0] as Object;
            }
        }
    }

}

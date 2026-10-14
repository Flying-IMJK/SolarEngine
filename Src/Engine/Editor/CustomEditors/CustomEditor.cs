using SE.GUI;

using System;
using System.Collections.Generic;
using System.Reflection;
using System.Text.Json;


namespace SE.Editor
{
    /// <summary>
    /// Custom editor layout style modes.
    /// </summary>
    [HideInEditor]
    public enum DisplayStyle
    {
        /// <summary>
        /// Creates a separate group for the editor (drop down element). This is a default value.
        /// </summary>
        Group,

        /// <summary>
        /// Inlines editor contents into the property area without creating a drop-down group.
        /// </summary>
        Inline,

        /// <summary>
        /// Inlines editor contents into the parent editor layout. Won;t use property with label name.
        /// </summary>
        InlineIntoParent,
    }

    /// <summary>
    /// Base class for all custom editors used to present object(s) properties. Allows to extend game objects editing with more logic and customization.
    /// </summary>
    [HideInEditor]
    public abstract class CustomEditor
    {
        private static readonly JsonSerializerOptions JsonOptions = new() { IncludeFields = true };
        private CustomEditorPresenter? m_Presenter;
        private LayoutElementsContainer? m_Layout;
        private ValueContainer? m_Values;
        private CustomEditor? m_Parent;
        private readonly List<CustomEditor> m_Children = new();
        private bool m_IsSetBlocked;
        private bool m_IsReadOnly;
        private bool m_HasValueDirty;
        private object? m_ValueToSet;
        private bool m_SkipChildrenRefresh;
        private bool m_RebuildOnRefresh;

        public static bool IsSettingValue;
        internal static CustomEditor? CurrentCustomEditor;

        public virtual DisplayStyle Style => DisplayStyle.Group;
        public bool IsSingleObject => Values.IsSingleObject;
        public bool HasDifferentValues => Values.HasDifferentValues;
        public bool HasDifferentTypes => Values.HasDifferentTypes;
        public Type[] ValuesTypes => Values.ValuesTypes;
        public ValueContainer Values => m_Values ?? throw new InvalidOperationException("Editor is not initialized.");
        public CustomEditor? ParentEditor => m_Parent;
        public List<CustomEditor> ChildrenEditors => m_Children;
        public CustomEditorPresenter Presenter => m_Presenter ?? throw new InvalidOperationException("Editor is not initialized.");
        public LayoutElementsContainer Layout => m_Layout ?? throw new InvalidOperationException("Editor is not initialized.");
        public PropertyNameLabel? LinkedLabel;
        protected bool IsSetBlocked => m_IsSetBlocked;
        internal bool CanEditValue => !m_IsReadOnly && (m_Parent == null || m_Parent.CanEditValue);

        internal virtual void Initialize(CustomEditorPresenter presenter, LayoutElementsContainer layout, ValueContainer values)
        {
            m_Presenter = presenter;
            m_Layout = layout;
            m_Values = values;
            m_Parent = CurrentCustomEditor;
            m_Parent?.m_Children.Add(this);
            var previous = CurrentCustomEditor;
            CurrentCustomEditor = this;
            try
            {
                m_IsSetBlocked = true;
                Initialize(layout);
                ShowButtons();
                Refresh();
            }
            finally
            {
                m_IsSetBlocked = false;
                CurrentCustomEditor = previous;
            }
        }

        public abstract void Initialize(LayoutElementsContainer layout);

        private void ShowButtons()
        {
            if (m_Values == null || m_Values.Count == 0 || m_Values.HasDifferentTypes)
                return;

            Type type = m_Values[0]?.GetType() ?? m_Values.Type;
            const BindingFlags flags = BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic;
            foreach (MethodInfo method in type.GetMethods(flags))
            {
                var attribute = method.GetCustomAttribute<ButtonAttribute>();
                if (attribute == null || method.GetParameters().Length != 0)
                    continue;
                string name = string.IsNullOrEmpty(attribute.Text)
                    ? GenericEditor.ItemInfo.GetPropertyNameUI(method.Name)
                    : attribute.Text;
                var button = new Button
                {
                    Text = name,
                    TooltipText = attribute.Tooltip ?? string.Empty,
                    Height = 20,
                    Parent = Layout.ContainerControl,
                };
                button.Clicked += () => method.Invoke(method.IsStatic ? null : m_Values[0], null);
            }
        }

        public void RebuildLayout() => Presenter.BuildLayout();
        public void RebuildLayoutOnRefresh() => m_RebuildOnRefresh = true;
        public void SkipChildrenRefresh() => m_SkipChildrenRefresh = true;

        internal void Cleanup()
        {
            Deinitialize();
            foreach (var child in m_Children)
            {
                child.Cleanup();
            }
            m_Children.Clear();
            m_Presenter = null;
            m_Parent = null;
            m_Layout = null;
            m_Values = null;
            m_HasValueDirty = false;
            m_ValueToSet = null;
            m_IsSetBlocked = false;
            m_IsReadOnly = false;
            m_RebuildOnRefresh = false;
            LinkedLabel = null;
        }

        protected virtual void Deinitialize() { }

        internal virtual void RefreshInternal()
        {
            if (m_Values == null)
            {
                return;
            }

            if (m_HasValueDirty)
            {
                IsSettingValue = true;
                try
                {
                    object? value = m_ValueToSet;
                    m_HasValueDirty = false;
                    m_ValueToSet = null;
                    if (m_Parent != null)
                    {
                        Values.Set(m_Parent.Values, value);
                        var obj = m_Parent;
                        while (obj.m_Parent != null && obj.m_Parent is not SyncPointEditor)
                        {
                            obj.Values.Set(obj.m_Parent.Values, obj.Values);
                            obj = obj.m_Parent;
                        }
                    }
                }
                finally
                {
                    OnUnDirty();
                    IsSettingValue = false;
                }
            }
            else if (m_Parent != null && m_Parent is not SyncPointEditor)
            {
                Values.Refresh(m_Parent.Values);
            }
            m_IsSetBlocked = true;
            try { Refresh(); }
            finally { m_IsSetBlocked = false; }
            if (m_RebuildOnRefresh)
            {
                m_RebuildOnRefresh = false;
                Presenter.BuildLayoutOnUpdate();
                return;
            }
            if (m_SkipChildrenRefresh)
            {
                m_SkipChildrenRefresh = false;
                return;
            }
            foreach (var child in m_Children)
                child.RefreshInternal();
            if (m_RebuildOnRefresh)
            {
                m_RebuildOnRefresh = false;
                Presenter.BuildLayoutOnUpdate();
            }
        }

        public virtual void Refresh()
        {
            if (LinkedLabel == null)
                return;
            Color color = Color.Transparent;
            if (Values.HasReferenceValue && CanRevertReferenceValue)
                color = SE.GUI.Style.Current.BackgroundSelected;
            else if (Values.HasDefaultValue && CanRevertDefaultValue)
                color = Color.Yellow * 0.8f;
            LinkedLabel.HighlightStripColor = color;
            if (Values.IsObsolete)
                LinkedLabel.TextColor = LinkedLabel.TextColorHighlighted = SE.GUI.Style.Current.ForegroundGrey;
        }

        public virtual bool RevertValueWithChildren => ChildrenEditors.Count != 0;
        public bool CanRevertDefaultValue => Values.IsDefaultValueModified && CanEditValue;
        public bool CanRevertReferenceValue => Values.IsReferenceValueModified && CanEditValue;

        public void RevertToDefaultValue()
        {
            if (Values.HasDefaultValue && CanEditValue)
                RevertDiffToDefault();
        }

        private void RevertDiffToDefault()
        {
            if (RevertValueWithChildren)
                foreach (var child in ChildrenEditors)
                    child.RevertDiffToDefault();
            else if (Values.IsDefaultValueModified)
                SetValueToDefault();
        }

        public void RevertToReferenceValue()
        {
            if (Values.HasReferenceValue && CanEditValue)
                RevertDiffToReference();
        }

        private void RevertDiffToReference()
        {
            if (RevertValueWithChildren)
                foreach (var child in ChildrenEditors)
                    child.RevertDiffToReference();
            else if (CanRevertReferenceValue)
                SetValueToReference();
        }

        public void RefreshDefaultValue()
        {
            if (!Values.HasDefaultValue)
                return;
            if (ParentEditor?.Values.HasDefaultValue ?? false)
                Values.RefreshDefaultValue(ParentEditor.Values.DefaultValue);
            foreach (var child in ChildrenEditors)
                child.RefreshDefaultValue();
        }

        public void RefreshReferenceValue()
        {
            if (!Values.HasReferenceValue)
                return;
            if (ParentEditor?.Values.HasReferenceValue ?? false)
                Values.RefreshReferenceValue(ParentEditor.Values.ReferenceValue);
            foreach (var child in ChildrenEditors)
                child.RefreshReferenceValue();
        }

        public void ClearDefaultValueAll()
        {
            Values.ClearDefaultValue();
            foreach (var child in ChildrenEditors)
                child.ClearDefaultValueAll();
        }

        public void ClearReferenceValueAll()
        {
            Values.ClearReferenceValue();
            foreach (var child in ChildrenEditors)
                child.ClearReferenceValueAll();
        }

        public void SetValueToDefault() => SetValue(CloneValue(Values.DefaultValue));
        public void SetValueToReference() => SetValue(CloneValue(Values.ReferenceValue));

        protected static object? CloneValue(object? value)
        {
            if (value is SE.Object)
                return value;
            if (value is ICloneable clonable)
                return clonable.Clone();
            if (value != null && (!value.GetType().IsValueType || !value.GetType().IsClass))
            {
                string json = JsonSerializer.Serialize(value, value.GetType(), JsonOptions);
                return JsonSerializer.Deserialize(json, value.GetType(), JsonOptions);
            }
            return value;
        }

        public void Copy()
        {
            if (Values.Count == 0)
                return;
            try
            {
                Clipboard.Text = JsonSerializer.Serialize(Values[0], Values.Type, JsonOptions);
            }
            catch (Exception ex)
            {
                SE.Debug.Log(ex.ToString());
            }
        }

        public bool CanPaste => !string.IsNullOrEmpty(Clipboard.Text);

        public void Paste()
        {
            string text = Clipboard.Text;
            if (string.IsNullOrEmpty(text))
                return;
            try
            {
                object? value = JsonSerializer.Deserialize(text, Values.Type, JsonOptions);
                if (value == null && Values.Type.IsValueType)
                    return;
                SetValue(value);
            }
            catch (JsonException)
            {
            }
        }

        protected void SetValue(object? value, object? token = null)
        {
            if (m_IsSetBlocked || !CanEditValue)
                return;
            if (OnDirty(this, value, token))
            {
                m_HasValueDirty = true;
                m_ValueToSet = value;
            }
        }

        protected virtual bool OnDirty(CustomEditor editor, object? value, object? token = null) =>
            m_Parent?.OnDirty(editor, value, token) ?? false;
        protected virtual void OnUnDirty() { }
        protected virtual void ClearToken() => m_Parent?.ClearToken();

        internal void SetReadOnly(bool value)
        {
            m_IsReadOnly = value;
            if (m_Layout != null)
                foreach (var control in m_Layout.ContainerControl.Children)
                    if (control is not PropertyNameLabel)
                        control.Enabled = !value;
        }
    }
}

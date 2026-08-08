using System;

namespace SE.GUI
{
    /// <summary>
    /// Base class for controls that collect editable text input.
    /// </summary>
    public abstract class TextBoxBase : ContainerControl
    {
        private string m_Text = string.Empty;

        protected TextBoxBase()
        {
        }

        protected TextBoxBase(Rectangle bounds, bool isMultiline = false)
            : base(bounds)
        {
            IsMultiline = isMultiline;
        }

        /// <summary>
        /// Gets or sets the editable text.
        /// </summary>
        public string Text
        {
            get => m_Text;
            set
            {
                value ??= string.Empty;
                if (string.Equals(m_Text, value, StringComparison.Ordinal))
                    return;

                m_Text = value;
                OnTextChanged();
            }
        }

        public bool IsMultiline { get; }
        public bool IsEditing { get; protected set; }

        public virtual void BeginEdit()
        {
            IsEditing = true;
            Root?.Focus(this);
        }

        public virtual void EndEdit()
        {
            IsEditing = false;
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button != MouseButton.Left)
                return false;

            BeginEdit();
            return true;
        }

        public override bool OnCharInput(char character)
        {
            if (char.IsControl(character))
                return false;

            BeginEdit();
            Text += character;
            return true;
        }

        public override bool OnKeyDown(KeyboardKeys key)
        {
            switch (key)
            {
            case KeyboardKeys.Backspace: // Backspace
                if (Text.Length > 0)
                {
                    Text = Text[..^1];
                }
                return true;
            case KeyboardKeys.Return: // Enter
                if (IsMultiline)
                {
                    Text += '\n';
                    return true;
                }

                EndEdit();
                return true;
            case KeyboardKeys.Escape: // Escape
                EndEdit();
                return true;
            default:
                return base.OnKeyDown(key);
            }
        }

        public override void ClearState()
        {
            EndEdit();
            base.ClearState();
        }

        protected virtual void OnTextChanged()
        {
        }
    }
}

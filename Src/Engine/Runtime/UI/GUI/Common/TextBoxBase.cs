using System;

namespace SE.GUI
{
    /// <summary>
    /// Base class for controls that collect editable text input.
    /// </summary>
    public abstract class TextBoxBase : ContainerControl
    {
        private string _text = string.Empty;

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
            get => _text;
            set
            {
                value ??= string.Empty;
                if (string.Equals(_text, value, StringComparison.Ordinal))
                    return;

                _text = value;
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

        public override bool OnMouseDown(Float2 location, int button)
        {
            if (button != 1)
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

        public override bool OnKeyDown(int key)
        {
            switch (key)
            {
            case 8: // Backspace
                if (Text.Length > 0)
                    Text = Text[..^1];
                return true;
            case 13: // Enter
                if (IsMultiline)
                {
                    Text += '\n';
                    return true;
                }

                EndEdit();
                return true;
            case 27: // Escape
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

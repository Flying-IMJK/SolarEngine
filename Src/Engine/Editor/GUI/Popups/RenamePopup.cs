#nullable disable annotations

using System;
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Popup menu useful for renaming objects via UI. Displays text box for renaming.
    /// </summary>
    /// <seealso cref="ContextMenuBase" />
    public class RenamePopup : ContextMenuBase
    {
        /// <summary>
        /// Input value validation delegate.
        /// </summary>
        /// <param name="popup">The popup reference.</param>
        /// <param name="value">The input text value.</param>
        /// <returns>True if text is valid, otherwise false.</returns>
        public delegate bool ValidateFunc(RenamePopup popup, ref string value);

        private TextBox m_InputField;

        /// <summary>
        /// Occurs when renaming is done.
        /// </summary>
        public event Action<RenamePopup> Renamed;

        /// <summary>
        /// Occurs when popup is closing (after renaming done or not).
        /// </summary>
        public event Action<RenamePopup> Closed;

        /// <summary>
        /// Occurs when input text validation should be performed.
        /// </summary>
        public ValidateFunc Validate;

        /// <summary>
        /// Gets or sets the initial value.
        /// </summary>
        public string InitialValue;

        /// <summary>
        /// Gets or sets the input field text.
        /// </summary>
        public string Text
        {
            get => m_InputField.Text;
            set => m_InputField.Text = value;
        }

        /// <summary>
        /// Gets the text input field control.
        /// </summary>
        public TextBox InputField => m_InputField;

        /// <summary>
        /// Initializes a new instance of the <see cref="RenamePopup"/> class.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="size">The size.</param>
        /// <param name="isMultiline">Enable/disable multiline text input support</param>
        public RenamePopup(string value, Float2 size, bool isMultiline)
        {
            if (!isMultiline)
            {
                size.Y = TextBox.DefaultHeight;
            }
            Size = size;

            InitialValue = value;

            m_InputField = new TextBox(isMultiline, 0, 0, size.Y);
            m_InputField.TextChanged += OnTextChanged;
            m_InputField.AnchorPreset = AnchorPresets.StretchAll;
            m_InputField.Offsets = Margin.Zero;
            m_InputField.Text = InitialValue;
            m_InputField.Parent = this;
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            Float2 mouseLocation = Root.GetMousePosition();
            if (!ContainsPoint(mouseLocation) && RootWindow().ContainsFocus && Text != InitialValue)
            {
                // rename item before closing if left mouse button in clicked
                if (Input.GetMouseButtonDown(MouseButton.Left))
                {
                    OnEnd();
                }
            }

            base.Update(deltaTime);
        }

        /// <summary>
        /// Shows the rename popup.
        /// </summary>
        /// <param name="control">The target control.</param>
        /// <param name="area">The target control area to cover.</param>
        /// <param name="value">The initial value.</param>
        /// <param name="isMultiline">Enable/disable multiline text input support</param>
        /// <returns>Created popup.</returns>
        public static RenamePopup ShowPopup(Control control, Rectangle area, string value, bool isMultiline)
        {
            // Calculate the control size in the window space to handle scaled controls
            Float2 upperLeft = control.PointToWindow(area.UpperLeft);
            Float2 bottomRight = control.PointToWindow(area.BottomRight);
            Float2 size = bottomRight - upperLeft;

            RenamePopup rename = new RenamePopup(value, size, isMultiline);
            Float2 pos = area.Location + new Float2(0, (size.Y - rename.Height) * 0.5f);
            rename.Show(control, pos);
            return rename;
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            // Enter
            if (key == KeyboardKeys.Return)
            {
                OnEnd();
                return true;
            }
            // Esc
            if (key == KeyboardKeys.Escape)
            {
                Hide();
                return true;
            }

            // Base
            return base.OnKeyDown(key);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            Renamed = null;
            Closed = null;
            Validate = null;
            m_InputField = null;

            base.OnDestroy();
        }

        protected override bool UseAutomaticDirectionFix()
        {
            return false;
        }

        /// <inheritdoc />
        protected override void OnShow()
        {
            m_InputField.EndEditOnClick = false; // Ending edit is handled through popup
            m_InputField.Focus();
            m_InputField.SelectAll();

            base.OnShow();
        }

        /// <inheritdoc />
        protected override void OnHide()
        {
            Closed?.Invoke(this);
            Closed = null;

            base.OnHide();

            // Remove itself
            Dispose();
        }

        private void OnEnd()
        {
            string text = Text;
            if (text != InitialValue && GetIsInputValid())
            {
                Renamed?.Invoke(this);
            }

            Hide();
        }

        private void OnTextChanged()
        {
            if (Validate == null)
            {
                return;
            }

            bool valid = GetIsInputValid();
            Style style = Style.Current;
            if (valid)
            {
                m_InputField.BorderColor = Color.Transparent;
                m_InputField.BorderSelectedColor = style.BackgroundSelected;
            }
            else
            {
                Color color = new Color(1.0f, 0.0f, 0.02745f, 1.0f);
                m_InputField.BorderColor = Color.Lerp(color, style.TextBoxBackground, 0.6f);
                m_InputField.BorderSelectedColor = color;
            }
        }

        private bool GetIsInputValid()
        {
            if (string.IsNullOrEmpty(m_InputField.Text))
            {
                return false;
            }

            if (m_InputField.Text == InitialValue || Validate == null)
            {
                return true;
            }

            string text = m_InputField.Text;
            bool valid = Validate(this, ref text);
            if (text != m_InputField.Text)
            {
                m_InputField.SetText(text);
            }

            return valid;
        }
    }
}
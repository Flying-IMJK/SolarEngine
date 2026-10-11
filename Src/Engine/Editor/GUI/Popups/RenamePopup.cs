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

        private TextBox _inputField;

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
            get => _inputField.Text;
            set => _inputField.Text = value;
        }

        /// <summary>
        /// Gets the text input field control.
        /// </summary>
        public TextBox InputField => _inputField;

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

            _inputField = new TextBox(isMultiline, 0, 0, size.Y);
            _inputField.TextChanged += OnTextChanged;
            _inputField.AnchorPreset = AnchorPresets.StretchAll;
            _inputField.Offsets = Margin.Zero;
            _inputField.Text = InitialValue;
            _inputField.Parent = this;
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
            _inputField = null;

            base.OnDestroy();
        }

        protected override bool UseAutomaticDirectionFix()
        {
            return false;
        }

        /// <inheritdoc />
        protected override void OnShow()
        {
            _inputField.EndEditOnClick = false; // Ending edit is handled through popup
            _inputField.Focus();
            _inputField.SelectAll();

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
                _inputField.BorderColor = Color.Transparent;
                _inputField.BorderSelectedColor = style.BackgroundSelected;
            }
            else
            {
                Color color = new Color(1.0f, 0.0f, 0.02745f, 1.0f);
                _inputField.BorderColor = Color.Lerp(color, style.TextBoxBackground, 0.6f);
                _inputField.BorderSelectedColor = color;
            }
        }

        private bool GetIsInputValid()
        {
            if (string.IsNullOrEmpty(_inputField.Text))
            {
                return false;
            }

            if (_inputField.Text == InitialValue || Validate == null)
            {
                return true;
            }

            string text = _inputField.Text;
            bool valid = Validate(this, ref text);
            if (text != _inputField.Text)
            {
                _inputField.SetText(text);
            }

            return valid;
        }
    }
}
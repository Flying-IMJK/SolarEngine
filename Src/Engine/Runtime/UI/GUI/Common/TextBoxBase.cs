#nullable disable annotations

using System;

namespace SE.GUI
{
    /// <summary>
    /// Base class for all text box controls which can gather text input from the user.
    /// </summary>
    public abstract partial class TextBoxBase : ContainerControl
    {
        /// <summary>
        /// The delete control character (used for text filtering).
        /// </summary>
        public static char DelChar = (char)0x7F;

        /// <summary>
        /// The text separators (used for words skipping).
        /// </summary>
        public static char[] Separators =
        {
            ' ',
            '.',
            ',',
            '\t',
            '\r',
            '\n',
            ':',
            ';',
            '\'',
            '"',
            ')',
            '(',
            '/',
            '\\',
            '>',
            '<',
        };

        /// <summary>
        /// Default height of the text box
        /// </summary>
        public const float DefaultHeight = 18;

        /// <summary>
        /// Left and right margin for text inside the text box bounds rectangle
        /// </summary>
        public const float DefaultMargin = 4;

        /// <summary>
        /// The current text value.
        /// </summary>
        protected string _text = string.Empty;

        /// <summary>
        /// The text value captured when user started editing text. Used to detect content modification.
        /// </summary>
        protected string _onStartEditValue = string.Empty;

        /// <summary>
        /// Flag used to indicate whenever user is editing the text.
        /// </summary>
        protected bool _isEditing;

        /// <summary>
        /// The view offset
        /// </summary>
        protected Float2 _viewOffset;

        /// <summary>
        /// The target view offset.
        /// </summary>
        protected Float2 _targetViewOffset;

        /// <summary>
        /// The text size calculated from font.
        /// </summary>
        protected Float2 _textSize;

        /// <summary>
        /// Flag used to indicate whenever text can contain multiple lines.
        /// </summary>
        protected bool _isMultiline;

        /// <summary>
        /// Flag used to indicate whenever text is read-only and cannot be modified by the user.
        /// </summary>
        protected bool _isReadOnly;

        /// <summary>
        /// Flag used to indicate whenever text is selectable.
        /// </summary>
        protected bool _isSelectable = true;

        /// <summary>
        /// The maximum length of the text.
        /// </summary>
        protected int _maxLength;

        /// <summary>
        /// Flag used to indicate whenever user is selecting text.
        /// </summary>
        protected bool _isSelecting;

        /// <summary>
        /// The selection start position (character index).
        /// </summary>
        protected int _selectionStart;

        /// <summary>
        /// The selection end position (character index).
        /// </summary>
        protected int _selectionEnd;

        /// <summary>
        /// The animate time for selection.
        /// </summary>
        protected float _animateTime;

        /// <summary>
        /// If the cursor should change to an IBeam
        /// </summary>
        protected bool _changeCursor = true;

        /// <summary>
        /// Event fired when text gets changed
        /// </summary>
        public Action TextChanged;

        /// <summary>
        /// Event fired when text gets changed after editing (user accepted entered value).
        /// </summary>
        public Action EditEnd;

        /// <summary>
        /// Event fired when text gets changed after editing (user accepted entered value).
        /// </summary>
        public Action<TextBoxBase> TextBoxEditEnd;

        /// <summary>
        /// Event fired when a key is down.
        /// </summary>
        public Action<KeyboardKeys> KeyDown;

        /// <summary>
        /// Event fired when a key is up.
        /// </summary>
        public Action<KeyboardKeys> KeyUp;

        /// <summary>
        /// Gets or sets a value indicating whether the text box can end edit via left click outside of the control
        /// </summary>
        public bool EndEditOnClick = true;

        /// <summary>
        /// Gets or sets a value indicating whether this is a multiline text box control.
        /// </summary>
        public bool IsMultiline
        {
            get => _isMultiline;
            set
            {
                if (_isMultiline != value)
                {
                    _isMultiline = value;

                    OnIsMultilineChanged();
                    Deselect();

                    if (!_isMultiline)
                    {
                        string[] lines = _text.Split('\n');
                        _text = lines[0];
                    }
                }
            }
        }

        /// <summary>
        /// Gets or sets the maximum number of characters the user can type into the text box control.
        /// </summary>
        public int MaxLength
        {
            get => _maxLength;
            set
            {
                SE.Debug.Assert(_maxLength > 0, "Invalid maximum text length.");
                if (_maxLength != value)
                {
                    _maxLength = value;

                    // Cut too long text
                    if (_text.Length > _maxLength)
                    {
                        Text = _text.Substring(0, _maxLength);
                    }
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether text in the text box is read-only.
        /// </summary>
        public bool IsReadOnly
        {
            get => _isReadOnly;
            set
            {
                if (_isReadOnly != value)
                {
                    _isReadOnly = value;
                    OnIsReadOnlyChanged();
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether text can be selected in text box.
        /// </summary>
        public bool IsSelectable
        {
            get => _isSelectable;
            set
            {
                if (_isSelectable != value)
                {
                    _isSelectable = value;
                    OnIsSelectableChanged();
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether apply clipping mask on text during rendering.
        /// </summary>
        public bool ClipText = true;

        /// <summary>
        /// Gets or sets a value indicating whether you can scroll the text in the text box (eg. with a mouse wheel).
        /// </summary>
        public bool IsMultilineScrollable = true;

        /// <summary>
        /// Gets or sets textbox background color when the control is selected (has focus).
        /// </summary>
        public Color BackgroundSelectedColor;

        /// <summary>
        /// Gets or sets the color of the caret (Transparent if not used).
        /// </summary>
        public Color CaretColor;

        /// <summary>
        /// Gets or sets the speed of the caret flashing animation.
        /// </summary>
        public float CaretFlashSpeed = 6.0f;

        /// <summary>
        /// Gets or sets the speed of the selection background flashing animation.
        /// </summary>
        public float BackgroundSelectedFlashSpeed = 6.0f;

        /// <summary>
        /// Gets or sets whether to have a border.
        /// </summary>
        public bool HasBorder = true;

        /// <summary>
        /// Gets or sets the border thickness.
        /// </summary>
        public float BorderThickness = 1.0f;

        /// <summary>
        /// Gets or sets the color of the border (Transparent if not used).
        /// </summary>
        public Color BorderColor;

        /// <summary>
        /// Gets or sets the color of the border when control is focused (Transparent if not used).
        /// </summary>
        public Color BorderSelectedColor;

        /// <summary>
        /// Gets the size of the text (cached).
        /// </summary>
        public Float2 TextSize => _textSize;

        /// <summary>
        /// Occurs when target view offset gets changed.
        /// </summary>
        public Action TargetViewOffsetChanged;

        /// <summary>
        /// Gets the current view offset (text scrolling offset). Includes the smoothing.
        /// </summary>
        public Float2 ViewOffset => _viewOffset;

        /// <summary>
        /// Gets or sets the target view offset (text scrolling offset).
        /// </summary>
        public Float2 TargetViewOffset
        {
            get => _targetViewOffset;
            set
            {
                value = Float2.Round(value);
                if (Float2.NearEqual(value, _targetViewOffset))
                {
                    return;
                }

                _targetViewOffset = _viewOffset = value;
                OnTargetViewOffsetChanged();
            }
        }

        /// <summary>
        /// Gets a value indicating whether user is editing the text.
        /// </summary>
        public bool IsEditing => _isEditing;

        /// <summary>
        /// Gets or sets text property.
        /// </summary>
        public string Text
        {
            get => _text;
            set
            {
                // Skip set if user is editing value
                if (_isEditing)
                {
                    return;
                }

                SetText(value);
            }
        }

        /// <summary>
        /// Gets length of the text
        /// </summary>
        public int TextLength => _text.Length;

        /// <summary>
        /// Gets the currently selected text in the control.
        /// </summary>
        public string SelectedText
        {
            get
            {
                int length = SelectionLength;
                return length > 0 ? _text.Substring(SelectionLeft, length) : string.Empty;
            }
        }

        /// <summary>
        /// Gets the number of characters selected in the text box.
        /// </summary>
        public int SelectionLength => Math.Abs(_selectionEnd - _selectionStart);

        /// <summary>
        /// Gets or sets the selection range.
        /// </summary>
        public TextRange SelectionRange
        {
            get
            {
                return new TextRange(SelectionLeft, SelectionRight);
            }
            set
            {
                SetSelection(value.StartIndex, value.EndIndex, false);
            }
        }

        /// <summary>
        /// Returns true if any text is selected, otherwise false
        /// </summary>
        public bool HasSelection => SelectionLength > 0;

        /// <summary>
        /// Index of the character on left edge of the selection
        /// </summary>
        protected int SelectionLeft => Math.Min(_selectionStart, _selectionEnd);

        /// <summary>
        /// Index of the character on right edge of the selection
        /// </summary>
        protected int SelectionRight => Math.Max(_selectionStart, _selectionEnd);

        /// <summary>
        /// Gets current caret position (index of the character)
        /// </summary>
        protected int CaretPosition => _selectionEnd;

        /// <summary>
        /// Calculates the caret rectangle.
        /// </summary>
        protected Rectangle CaretBounds
        {
            get
            {
                const float caretWidth = 1.2f;
                float height = 0;
                Float2 caretPos = GetCharPosition(CaretPosition, ref height);
                return new Rectangle(caretPos.X - caretWidth * 0.5f, caretPos.Y, caretWidth, height * GetDpiScale());
            }
        }

        /// <summary>
        /// Gets rectangle with area for text
        /// </summary>
        protected virtual Rectangle TextRectangle
        {
            get
            {
                return new Rectangle(DefaultMargin, 1, Width - 2 * DefaultMargin, Height - 2);
            }
        }

        /// <summary>
        /// Gets rectangle used to clip text
        /// </summary>
        protected Rectangle textClipRectangle
        {
            get
            {
                return GetTextClipRectangle();
            }
        }

        protected virtual Rectangle GetTextClipRectangle()
        {
            return new Rectangle(1, 1, Width - 2, Height - 2);
        }

        /// <summary>
        /// Sets the text (forced, even if user is editing it).
        /// </summary>
        /// <param name="value">The value.</param>
        public void SetText(string value)
        {
            string temp = string.Empty;
            // Prevent from null problems
            if (temp.Length <= 0)
            {
                temp = string.Empty;
            }
            else
            {
                temp = value;
            }

            // Filter text
            if (temp.IndexOf('\r') != -1)
            {
                temp = temp.Replace("\r", "");
            }

            // Filter text (handle backspace control character)
            if (temp.IndexOf(DelChar) != -1)
            {
                temp = temp.Replace(DelChar.ToString(), "");
            }

            // Clamp length
            if (temp.Length > MaxLength)
            {
                temp = temp.Substring(0, MaxLength);
            }

            // Ensure to use only single line
            if (_isMultiline == false && temp.Length > 0)
            {
                // Extract only the first line
                string[] lines = temp.Split(new[] { "\r\n", "\r", "\n" }, StringSplitOptions.None);
                temp = lines[0];
            }

            if (_text != value)
            {
                Deselect();
                ResetViewOffset();

                _text = value;

                OnTextChanged();
            }
        }

        /// <summary>
        /// Sets the text as it was entered by user (focus, change value, defocus).
        /// </summary>
        /// <param name="value">The value.</param>
        public void SetTextAsUser(string value)
        {
            Focus();
            SetText(value);
            RemoveFocus();
        }

        /// <summary>
        /// Clears all text from the text box control.
        /// </summary>
        public virtual void Clear()
        {
            Text = string.Empty;
        }

        /// <summary>
        /// Clear selection range
        /// </summary>
        public virtual void ClearSelection()
        {
            OnSelectingEnd();
            SetSelection(-1);
        }

        /// <summary>
        /// Resets the view offset (text scroll view).
        /// </summary>
        public virtual void ResetViewOffset()
        {
            TargetViewOffset = Float2.Zero;
        }

        /// <summary>
        /// Copies the current selection in the text box to the Clipboard.
        /// </summary>
        public virtual void Copy()
        {
            string selectedText = SelectedText;
            if (selectedText.Length > 0)
            {
                // Copy selected text
                Clipboard.Text = selectedText;
            }
        }

        /// <summary>
        /// Moves the current selection in the text box to the Clipboard.
        /// </summary>
        public virtual void Cut()
        {
            string selectedText = SelectedText;
            if (selectedText.Length > 0)
            {
                // Copy selected text
                Clipboard.Text = selectedText;
                if (IsReadOnly)
                {
                    return;
                }

                // Remove selection
                int left = SelectionLeft;
                _text = _text.Remove(left, SelectionLength);
                SetSelection(left);
                OnTextChanged();
            }
        }

        /// <summary>
        /// Replaces the current selection in the text box with the contents of the Clipboard.
        /// </summary>
        public virtual void Paste()
        {
            if (IsReadOnly)
            {
                return;
            }

            // Get clipboard data
            string clipboardText = Clipboard.Text;
            if (clipboardText.Length <= 0)
            {
                return;
            }

            Insert(clipboardText);
        }

        /// <summary>
        /// Duplicates the current selection in the text box.
        /// </summary>
        public virtual void Duplicate()
        {
            if (IsReadOnly)
            {
                return;
            }

            string selectedText = SelectedText;
            if (selectedText.Length > 0)
            {
                float right = SelectionRight;
                SetSelection((int)right);
                Insert(selectedText);
                SetSelection((int)right, (int)(right + selectedText.Length), true);
            }
        }

        /// <summary>
        /// Ensures that the caret is visible in the TextBox window, by scrolling the TextBox control surface if
        /// necessary.
        /// </summary>
        public virtual void ScrollToCaret()
        {
            // If it's empty
            if (_text.Length == 0)
            {
                TargetViewOffset = Float2.Zero;
                return;
            }
            // If it's not selected
            if (_selectionStart == -1 && _selectionEnd == -1)
            {
                return;
            }

            Rectangle caretBounds = CaretBounds;
            Rectangle textArea = TextRectangle;

            // Update view offset (caret needs to be in a view)
            Float2 caretInView = caretBounds.Location - _targetViewOffset;
            Float2 clampedCaretInView = Float2.Clamp(caretInView, textArea.Location, textArea.Location + textArea.Size);
            TargetViewOffset = caretInView - clampedCaretInView + TargetViewOffset;
        }

        /// <summary>
        /// Selects all text in the text box.
        /// </summary>
        public virtual void SelectAll()
        {
            if (TextLength > 0)
            {
                SetSelection(0, TextLength);
            }
        }

        /// <summary>
        /// Sets the selection to empty value.
        /// </summary>
        public virtual void Deselect()
        {
            SetSelection(-1);
        }

        /// <summary>
        /// Gets the character the index at point (eg. mouse location in control-space).
        /// </summary>
        /// <param name="location">The location (in control-space).</param>
        /// <returns>The character index under the location</returns>
        public virtual int CharIndexAtPoint(ref Float2 location)
        {
            return HitTestText(location + _viewOffset);
        }

        /// <summary>
        /// Inserts the specified character (at the current selection).
        /// </summary>
        /// <param name="c">The character.</param>
        public virtual void Insert(char c)
        {
            Insert(c.ToString());
        }

        /// <summary>
        /// Inserts the specified text (at the current selection).
        /// </summary>
        /// <param name="str">The string.</param>
        public virtual void Insert(string str)
        {
            if (IsReadOnly)
            {
                return;
            }

            string temp = str;

            // Filter text
            if (temp.IndexOf('\r') != -1)
            {
                temp = temp.Replace("\r", "");
            }
            if (temp.IndexOf(DelChar) != -1)
            {
                temp = temp.Replace(DelChar.ToString(), "");
            }
            if (!IsMultiline && str.IndexOf('\n') != -1)
            {
                temp = temp.Replace("\n", "");
            }

            int selectionLength = SelectionLength;
            int charactersLeft = MaxLength - _text.Length + selectionLength;
            SE.Debug.Assert(charactersLeft >= 0, "Invalid remaining text length.");
            if (charactersLeft == 0)
            {
                return;
            }

            if (charactersLeft < str.Length)
            {
                str = str[..charactersLeft];
            }

            if (TextLength == 0)
            {
                _text = str;
                SetSelection(TextLength);
            }
            else
            {
                int left = SelectionLeft >= 0 ? SelectionLeft : 0;
                if (HasSelection)
                {
                    _text = _text.Remove(left, selectionLength);
                    SetSelection(left);
                }
                _text = _text.Insert(left, str);
                SetSelection(left + str.Length);
            }
            OnTextChanged();
        }

        /// <summary>
        /// Calculates total text size. Called by <see cref="OnTextChanged"/> to cache the text size.
        /// </summary>
        /// <returns>The total text size.</returns>
        public abstract Float2 GetTextSize();

        /// <summary>
        /// Calculates character position for given character index.
        /// </summary>
        /// <param name="index">The text position to get it's coordinates.</param>
        /// <param name="height">The character height (at the given character position).</param>
        /// <returns>The character position (upper left corner which can be used for a caret position).</returns>
        public abstract Float2 GetCharPosition(int index, ref float height);

        /// <summary>
        /// Calculates hit character index at given location.
        /// </summary>
        /// <param name="location">The location to test.</param>
        /// <returns> The selected character position index (can be equal to text length if location is outside of the
        /// layout rectangle).</returns>
        public abstract int HitTestText(Float2 location);

        /// <summary>
        /// Initializes a new instance of the <see cref="TextBoxBase"/> class.
        /// </summary>
        protected TextBoxBase()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="TextBoxBase"/> class.
        /// </summary>
        /// <param name="isMultiline">Enable/disable multiline text input support.</param>
        /// <param name="x">The control position X coordinate.</param>
        /// <param name="y">The control position Y coordinate.</param>
        /// <param name="width">The control width.</param>
        protected TextBoxBase(bool isMultiline, float x, float y, float width = 120)
            : base(x, y, width, DefaultHeight)
        {
            _isMultiline = isMultiline;
            _maxLength = 2147483646;
            _selectionStart = _selectionEnd = -1;

            Style style = Style.Current;
            CaretColor = style.Foreground;
            BorderColor = Color.Transparent;
            BorderSelectedColor = style.BackgroundSelected;
            BackgroundColor = style.TextBoxBackground;
            BackgroundSelectedColor = style.TextBoxBackgroundSelected;
        }

        /// <summary>
        /// Called when target view offset gets changed.
        /// </summary>
        protected virtual void OnTargetViewOffsetChanged()
        {
            TargetViewOffsetChanged?.Invoke();
        }

        /// <summary>
        /// Called when is multiline gets changed.
        /// </summary>
        protected virtual void OnIsMultilineChanged()
        {
        }

        /// <summary>
        /// Called when is read only gets changed.
        /// </summary>
        protected virtual void OnIsReadOnlyChanged()
        {
        }

        /// <summary>
        /// Called when is selectable flag gets changed.
        /// </summary>
        protected virtual void OnIsSelectableChanged()
        {
        }

        /// <summary>
        /// Action called when user starts text selecting
        /// </summary>
        protected virtual void OnSelectingBegin()
        {
            if (!_isSelecting)
            {
                // Set flag
                _isSelecting = true;

                // Start tracking mouse
                StartMouseCapture();
            }
        }

        /// <summary>
        /// Action called when user ends text selecting
        /// </summary>
        protected virtual void OnSelectingEnd()
        {
            if (_isSelecting)
            {
                // Clear flag
                _isSelecting = false;

                // Stop tracking mouse
                EndMouseCapture();
            }
        }

        /// <summary>
        /// Action called when user starts text editing
        /// </summary>
        protected virtual void OnEditBegin()
        {
            if (_isEditing)
            {
                return;
            }

            _isEditing = true;
            _onStartEditValue = _text;

            // Reset caret visibility
            _animateTime = 0;
        }

        /// <summary>
        /// Action called when user ends text editing.
        /// </summary>
        protected virtual void OnEditEnd()
        {
            if (!_isEditing)
            {
                return;
            }

            _isEditing = false;
            if (_onStartEditValue != _text)
            {
                _onStartEditValue = _text;
                EditEnd?.Invoke();
                TextBoxEditEnd?.Invoke(this);
            }
            _onStartEditValue = string.Empty;
            ClearSelection();
            ResetViewOffset();
        }

        /// <summary>
        /// Action called when text gets modified.
        /// </summary>
        protected virtual void OnTextChanged()
        {
            _textSize = TextSize;
            TextChanged?.Invoke();
        }

        /// <summary>
        /// Sets the caret position.
        /// </summary>
        /// <param name="caret">The caret position.</param>
        /// <param name="withScroll">If set to <c>true</c> with auto-scroll.</param>
        protected virtual void SetSelection(int caret, bool withScroll = true)
        {
            SetSelection(caret, caret);
        }

        /// <summary>
        /// Sets the selection.
        /// </summary>
        /// <param name="start">The selection start character.</param>
        /// <param name="end">The selection end character.</param>
        /// <param name="withScroll">If set to <c>true</c> with auto-scroll.</param>
        protected virtual void SetSelection(int start, int end, bool withScroll = true)
        {
            // Update parameters
            int textLength = _text.Length;
            _selectionStart = Math.Clamp(start, -1, textLength);
            _selectionEnd = Math.Clamp(end, -1, textLength);
            if (withScroll)
            {
                // Update view on caret modified
                ScrollToCaret();

                // Reset caret and selection animation
                _animateTime = 0;
            }
        }

        private void RemoveFocus()
        {
            if (Parent != null)
            {
                Parent.Focus();
            }
            else
            {
                Defocus();
            }
        }
    }
}

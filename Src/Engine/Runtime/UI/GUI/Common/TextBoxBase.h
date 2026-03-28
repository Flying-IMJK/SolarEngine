#pragma once
#include "Runtime/Render/2D/Font.h"
#include "Runtime/UI/GUI/ContainerControl.h"

namespace SE
{
    /// <summary>
    /// Base class for all text box controls which can gather text input from the user.
    /// </summary>
    class SE_API_RUNTIME TextBoxBase : public ContainerControl
    {
        SE_CLASS(TextBoxBase, ContainerControl)
    public:
        /// <summary>
        /// The delete control character (used for text filtering).
        /// </summary>
        static Char DelChar;

        /// <summary>
        /// The text separators (used for words skipping).
        /// </summary>
        static Char Separators[];

        /// <summary>
        /// Default height of the text box
        /// </summary>
        static constexpr float DefaultHeight = 18;

        /// <summary>
        /// Left and right margin for text inside the text box bounds rectangle
        /// </summary>
        static constexpr float DefaultMargin = 4;

    protected:
        /// <summary>
        /// The current text value.
        /// </summary>
        String _text = String::Empty;

        /// <summary>
        /// The text value captured when user started editing text. Used to detect content modification.
        /// </summary>
        String _onStartEditValue;

        /// <summary>
        /// Flag used to indicate whenever user is editing the text.
        /// </summary>
        bool _isEditing;

        /// <summary>
        /// The view offset
        /// </summary>
        Float2 _viewOffset;

        /// <summary>
        /// The target view offset.
        /// </summary>
        Float2 _targetViewOffset;

        /// <summary>
        /// The text size calculated from font.
        /// </summary>
        Float2 _textSize;

        /// <summary>
        /// Flag used to indicate whenever text can contain multiple lines.
        /// </summary>
        bool _isMultiline;

        /// <summary>
        /// Flag used to indicate whenever text is read-only and cannot be modified by the user.
        /// </summary>
        bool _isReadOnly;

        /// <summary>
        /// Flag used to indicate whenever text is selectable.
        /// </summary>
        bool _isSelectable = true;

        /// <summary>
        /// The maximum length of the text.
        /// </summary>
        int _maxLength;

        /// <summary>
        /// Flag used to indicate whenever user is selecting text.
        /// </summary>
        bool _isSelecting;

        /// <summary>
        /// The selection start position (character index).
        /// </summary>
        int _selectionStart;

        /// <summary>
        /// The selection end position (character index).
        /// </summary>
        int _selectionEnd;

        /// <summary>
        /// The animate time for selection.
        /// </summary>
        float _animateTime;

        /// <summary>
        /// If the cursor should change to an IBeam
        /// </summary>
        bool _changeCursor = true;

    public:
        /// <summary>
        /// Event fired when text gets changed
        /// </summary>
        Delegate<> TextChanged;

        /// <summary>
        /// Event fired when text gets changed after editing (user accepted entered value).
        /// </summary>
        Delegate<> EditEnd;

        /// <summary>
        /// Event fired when text gets changed after editing (user accepted entered value).
        /// </summary>
        Delegate<TextBoxBase*> TextBoxEditEnd;

        /// <summary>
        /// Event fired when a key is down.
        /// </summary>
        Delegate<KeyboardKeys> KeyDown;

        /// <summary>
        /// Event fired when a key is up.
        /// </summary>
        Delegate<KeyboardKeys> KeyUp;

        /// <summary>
        /// Gets or sets a value indicating whether the text box can end edit via left click outside of the control
        /// </summary>
        bool EndEditOnClick = true;

        /// <summary>
        /// Gets or sets a value indicating whether this is a multiline text box control.
        /// </summary>
        // [EditorOrder(40), Tooltip("If checked, the textbox will support multiline text input.")]
        PRO(IsMultiline, TextBoxBase, bool, __GetIsMultiline, __SetIsMultiline);

        /// <summary>
        /// Gets or sets the maximum number of characters the user can type into the text box control.
        /// </summary>
        // [EditorOrder(50), Tooltip("The maximum number of characters the user can type into the text box control.")]
        PRO(MaxLength, TextBoxBase, int, __GetMaxLength, __SetMaxLength);

        /// <summary>
        /// Gets or sets a value indicating whether text in the text box is read-only. 
        /// </summary>
        // [EditorOrder(60), Tooltip("If checked, text in the text box is read-only.")]
        PRO(IsReadOnly, TextBoxBase, bool, __GetIsReadOnly, __SetIsReadOnly);

        /// <summary>
        /// Gets or sets a value indicating whether text can be selected in text box. 
        /// </summary>
        // [EditorOrder(62), Tooltip("If checked, text can be selected in text box.")]
        PRO(IsSelectable, TextBoxBase, bool, __GetIsSelectable, __SetIsSelectable);


        /// <summary>
        /// Gets or sets a value indicating whether apply clipping mask on text during rendering.
        /// </summary>
        bool ClipText = true;

        /// <summary>
        /// Gets or sets a value indicating whether you can scroll the text in the text box (eg. with a mouse wheel).
        /// </summary>
        bool IsMultilineScrollable = true;

        /// <summary>
        /// Gets or sets textbox background color when the control is selected (has focus).
        /// </summary>
        // [EditorDisplay("Background Style"), EditorOrder(2001), Tooltip("The textbox background color when the control is selected (has focus)."), ExpandGroups]
        Color BackgroundSelectedColor;

        /// <summary>
        /// Gets or sets the color of the caret (Transparent if not used).
        /// </summary>
        // [EditorDisplay("Caret Style"), EditorOrder(2020), Tooltip("The color of the caret (Transparent if not used)."), ExpandGroups]
        Color CaretColor;

        /// <summary>
        /// Gets or sets the speed of the caret flashing animation.
        /// </summary>
        // [EditorDisplay("Caret Style"), EditorOrder(2021), Tooltip("The speed of the caret flashing animation.")]
        float CaretFlashSpeed = 6.0f;

        /// <summary>
        /// Gets or sets the speed of the selection background flashing animation.
        /// </summary>
        // [EditorDisplay("Background Style"), EditorOrder(2002), Tooltip("The speed of the selection background flashing animation.")]
        float BackgroundSelectedFlashSpeed = 6.0f;

        /// <summary>
        /// Gets or sets whether to have a border.
        /// </summary>
        // [EditorDisplay("Border Style"), EditorOrder(2010), Tooltip("Whether to have a border."), ExpandGroups]
        bool HasBorder = true;

        /// <summary>
        /// Gets or sets the border thickness.
        /// </summary>
        // [EditorDisplay("Border Style"), EditorOrder(2011), Tooltip("The thickness of the border."), Limit(0)]
        float BorderThickness = 1.0f;

        /// <summary>
        /// Gets or sets the color of the border (Transparent if not used).
        /// </summary>
        // [EditorDisplay("Border Style"), EditorOrder(2012), Tooltip("The color of the border (Transparent if not used).")]
        Color BorderColor;

        /// <summary>
        /// Gets or sets the color of the border when control is focused (Transparent if not used).
        /// </summary>
        // [EditorDisplay("Border Style"), EditorOrder(2013), Tooltip("The color of the border when control is focused (Transparent if not used)")]
        Color BorderSelectedColor;

        /// <summary>
        /// Gets the size of the text (cached).
        /// </summary>
        PRO_GET(TextSize, TextBoxBase, Float2, __GetTextSize);

        /// <summary>
        /// Occurs when target view offset gets changed.
        /// </summary>
        Delegate<> TargetViewOffsetChanged;

        /// <summary>
        /// Gets the current view offset (text scrolling offset). Includes the smoothing.
        /// </summary>
        PRO_GET(ViewOffset, TextBoxBase, Float2, __GetViewOffset);

        /// <summary>
        /// Gets or sets the target view offset (text scrolling offset).
        /// </summary>
        // [NoAnimate, NoSerialize, HideInEditor]
        PRO_REF(TargetViewOffset, TextBoxBase, Float2, __GetTargetViewOffset, __SetTargetViewOffset);

        /// <summary>
        /// Gets a value indicating whether user is editing the text.
        /// </summary>
        PRO_GET(IsEditing, TextBoxBase, bool, __GetIsEditing);

        /// <summary>
        /// Gets or sets text property.
        /// </summary>
        // [EditorOrder(0), MultilineText, Tooltip("The entered text.")]
        PRO_REF(Text, TextBoxBase, String, __GetText, __SetText);

        /// <summary>
        /// Sets the text (forced, even if user is editing it).
        /// </summary>
        /// <param name="value">The value.</param>
        void SetText(StringView value);

        void SetText(String& value);

        /// <summary>
        /// Sets the text as it was entered by user (focus, change value, defocus).
        /// </summary>
        /// <param name="value">The value.</param>
        void SetTextAsUser(StringView value);

        void SetTextAsUser(String& value);

        /// <summary>
        /// Gets length of the text
        /// </summary>
        PRO_GET(TextLength, TextBoxBase, int, __GetTextLength);

        /// <summary>
        /// Gets the currently selected text in the control.
        /// </summary>
        PRO_GET(SelectedText, TextBoxBase, String, __GetSelectedText);

        /// <summary>
        /// Gets the number of characters selected in the text box.
        /// </summary>
        PRO_GET(SelectionLength, TextBoxBase, int, __GetSelectionLength);

        /// <summary>
        /// Gets or sets the selection range.
        /// </summary>
        // [EditorOrder(50)]
        PRO(SelectionRange, TextBoxBase, TextRange, __GetSelectionRange, __SetSelectionRange);

        /// <summary>
        /// Returns true if any text is selected, otherwise false
        /// </summary>
        PRO_GET(HasSelection, TextBoxBase, bool, __GetHasSelection);

    protected:
        /// <summary>
        /// Index of the character on left edge of the selection
        /// </summary>
        PRO_GET(SelectionLeft, TextBoxBase, int, __GetSelectionLeft);

        /// <summary>
        /// Index of the character on right edge of the selection
        /// </summary>
        PRO_GET(SelectionRight, TextBoxBase, int, __GetSelectionRight);

        /// <summary>
        /// Gets current caret position (index of the character)
        /// </summary>
        PRO_GET(CaretPosition, TextBoxBase, int, __GetCaretPosition);

        /// <summary>
        /// Calculates the caret rectangle.
        /// </summary>
        PRO_GET(CaretBounds, TextBoxBase, Rectangle, __GetCaretBounds);

        /// <summary>
        /// Gets rectangle with area for text
        /// </summary>
        PRO_GET(TextRectangle, TextBoxBase, Rectangle, __GetTextRectangle);

        /// <summary>
        /// Gets rectangle used to clip text
        /// </summary>
        PRO_GET(textClipRectangle, TextBoxBase, Rectangle, GetTextClipRectangle);

        virtual Rectangle GetTextClipRectangle()
        {
            return Rectangle(1, 1, Width - 2, Height - 2);
        }

    public:
        /// <summary>
        /// Clears all text from the text box control. 
        /// </summary>
        virtual void Clear();

        /// <summary>
        /// Clear selection range
        /// </summary>
        virtual void ClearSelection();

        /// <summary>
        /// Resets the view offset (text scroll view).
        /// </summary>
        virtual void ResetViewOffset();

        /// <summary>
        /// Copies the current selection in the text box to the Clipboard. 
        /// </summary>
        virtual void Copy();

        /// <summary>
        /// Moves the current selection in the text box to the Clipboard. 
        /// </summary>
        virtual void Cut();

        /// <summary>
        /// Replaces the current selection in the text box with the contents of the Clipboard.
        /// </summary>
        virtual void Paste();

        /// <summary>
        /// Duplicates the current selection in the text box.
        /// </summary>
        virtual void Duplicate();

        /// <summary>
        /// Ensures that the caret is visible in the TextBox window, by scrolling the TextBox control surface if necessary.
        /// </summary>
        virtual void ScrollToCaret();

        /// <summary>
        /// Selects all text in the text box.
        /// </summary>
        virtual void SelectAll();

        /// <summary>
        /// Sets the selection to empty value.
        /// </summary>
        virtual void Deselect();

        /// <summary>
        /// Gets the character the index at point (eg. mouse location in control-space).
        /// </summary>
        /// <param name="location">The location (in control-space).</param>
        /// <returns>The character index under the location</returns>
        virtual int CharIndexAtPoint(Float2& location);

        /// <summary>
        /// Inserts the specified character (at the current selection).
        /// </summary>
        /// <param name="c">The character.</param>
        virtual void Insert(Char c);

        /// <summary>
        /// Inserts the specified text (at the current selection).
        /// </summary>
        /// <param name="str">The string.</param>
        virtual void Insert(StringView str);
        
    private:
        int FindNextWordBegin();

        int FindPrevWordBegin();

        int FindPrevLineBegin();

        int FindLineDownChar(int index);

        int FindLineUpChar(int index);

        void RemoveFocus();
        
    public:

        /// <summary>
        /// Calculates total text size. Called by <see cref="OnTextChanged"/> to cache the text size.
        /// </summary>
        /// <returns>The total text size.</returns>
        virtual Float2 GetTextSize() = 0;

        /// <summary>
        /// Calculates character position for given character index.
        /// </summary>
        /// <param name="index">The text position to get it's coordinates.</param>
        /// <param name="height">The character height (at the given character position).</param>
        /// <returns>The character position (upper left corner which can be used for a caret position).</returns>
        virtual Float2 GetCharPosition(int index, float& height) = 0;

        /// <summary>
        /// Calculates hit character index at given location.
        /// </summary>
        /// <param name="location">The location to test.</param>
        /// <returns>The selected character position index (can be equal to text length if location is outside of the layout rectangle).</returns>
        virtual int HitTestText(Float2 location) = 0;
        
        /// <inheritdoc />
        void Update(float deltaTime) override;

        /// <summary>
        /// Restores the Text from the start.
        /// </summary>
        void RestoreTextFromStart();

        /// <inheritdoc />
        void OnGetFocus() override;
        
        /// <inheritdoc />
        void OnLostFocus() override;

        /// <inheritdoc />
        void OnEndMouseCapture() override;

        /// <inheritdoc />
        void NavigationFocus() override;

        /// <inheritdoc />
        void OnSubmit() override;

        /// <inheritdoc />
        void OnMouseEnter(Float2 location) override;

        /// <inheritdoc />
        void OnMouseLeave() override;

        /// <inheritdoc />
        void OnMouseMove(Float2 location) override;

        /// <inheritdoc />
        bool OnMouseDown(Float2 location, MouseButton button) override;
        
        /// <inheritdoc />
        bool OnMouseUp(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        bool OnMouseWheel(Float2 location, float delta) override;

        /// <inheritdoc />
        bool OnCharInput(Char c) override;

        /// <inheritdoc />
        void OnKeyUp(KeyboardKeys key) override;
        
        /// <inheritdoc />
        bool OnKeyDown(KeyboardKeys key) override;

    protected:

        TextBoxBase();

        /// <summary>
        /// Initializes a new instance of the <see cref="TextBoxBase"/> class.
        /// </summary>
        /// <param name="isMultiline">Enable/disable multiline text input support.</param>
        /// <param name="x">The control position X coordinate.</param>
        /// <param name="y">The control position Y coordinate.</param>
        /// <param name="width">The control width.</param>
        TextBoxBase(bool isMultiline, float x, float y, float width = 120);

        /// <summary>
        /// Called when target view offset gets changed.
        /// </summary>
        virtual void OnTargetViewOffsetChanged();

        /// <summary>
        /// Moves the caret right.
        /// </summary>
        /// <param name="shift">Shift is held.</param>
        /// <param name="ctrl">Control is held.</param>
        virtual void MoveRight(bool shift, bool ctrl);

        /// <summary>
        /// Moves the caret left.
        /// </summary>
        /// <param name="shift">Shift is held.</param>
        /// <param name="ctrl">Control is held.</param>
        virtual void MoveLeft(bool shift, bool ctrl);

        /// <summary>
        /// Moves the caret down.
        /// </summary>
        /// <param name="shift">Shift is held.</param>
        /// <param name="ctrl">Control is held.</param>
        virtual void MoveDown(bool shift, bool ctrl);

        /// <summary>
        /// Moves the caret up.
        /// </summary>
        /// <param name="shift">Shift is held.</param>
        /// <param name="ctrl">Control is held.</param>
        virtual void MoveUp(bool shift, bool ctrl);

        /// <summary>
        /// Sets the caret position.
        /// </summary>
        /// <param name="caret">The caret position.</param>
        /// <param name="withScroll">If set to <c>true</c> with auto-scroll.</param>
        virtual void SetSelection(int caret, bool withScroll = true)
        {
            SetSelection(caret, caret);
        }

        /// <summary>
        /// Sets the selection.
        /// </summary>
        /// <param name="start">The selection start character.</param>
        /// <param name="end">The selection end character.</param>
        /// <param name="withScroll">If set to <c>true</c> with auto-scroll.</param>
        virtual void SetSelection(int start, int end, bool withScroll = true);
        
        /// <summary>
        /// Called when is multiline gets changed.
        /// </summary>
        virtual void OnIsMultilineChanged()
        {
        }

        /// <summary>
        /// Called when is read only gets changed.
        /// </summary>
        virtual void OnIsReadOnlyChanged()
        {
        }

        /// <summary>
        /// Called when is selectable flag gets changed.
        /// </summary>
        virtual void OnIsSelectableChanged()
        {
        }

        /// <summary>
        /// Action called when user starts text selecting
        /// </summary>
        virtual void OnSelectingBegin();

        /// <summary>
        /// Action called when user ends text selecting
        /// </summary>
        virtual void OnSelectingEnd();

        /// <summary>
        /// Action called when user starts text editing
        /// </summary>
        virtual void OnEditBegin();

        /// <summary>
        /// Action called when user ends text editing.
        /// </summary>
        virtual void OnEditEnd();

        /// <summary>
        /// Action called when text gets modified.
        /// </summary>
        virtual void OnTextChanged();

        virtual Rectangle __GetTextRectangle();

    private:
        bool __GetIsMultiline() { return _isMultiline; }
        void __SetIsMultiline(bool value);

        int __GetMaxLength() { return _maxLength; }
        void __SetMaxLength(int value);

        bool __GetIsReadOnly() { return _isReadOnly; }
        void __SetIsReadOnly(bool value);

        bool __GetIsSelectable() { return _isSelectable; }
        void __SetIsSelectable(bool value);

        Float2 __GetTextSize() { return _textSize; }
        Float2 __GetViewOffset() { return _viewOffset; }

        Float2& __GetTargetViewOffset() { return _targetViewOffset; }
        void __SetTargetViewOffset(Float2& value);

        bool __GetIsEditing() { return _isEditing; }

        String& __GetText() { return _text; }
        void __SetText(String& value);

        int __GetTextLength() { return _text.Length(); }

        String __GetSelectedText();

        int __GetSelectionLength() { return Math::Abs(_selectionEnd - _selectionStart); }

        TextRange __GetSelectionRange();

        void __SetSelectionRange(TextRange value)
        {
            SetSelection(value.StartIndex, value.EndIndex, false);
        }

        bool __GetHasSelection() { return SelectionLength > 0; }


        int __GetSelectionLeft() { return Math::Min(_selectionStart, _selectionEnd); }
        int __GetSelectionRight() { return Math::Max(_selectionStart, _selectionEnd); }
        int __GetCaretPosition() { return  _selectionEnd; }

        Rectangle __GetCaretBounds();
    };

} // SE


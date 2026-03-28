
#include "TextBoxBase.h"

#include "Core/Platform/Clipboard.h"
#include "Core/Input/Input.h"
#include "Runtime/UI/GUI/RootControl.h"
#include "Runtime/UI/GUI/Style.h"
#include "Runtime/UI/GUI/WindowRootControl.h"

namespace SE
{
    Char TextBoxBase::DelChar = (Char)0x7F;

    Char TextBoxBase::Separators[] =
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
        '\"',
        ')',
        '(',
        '/',
        '\\',
        '>',
        '<',
    };

    void TextBoxBase::MoveLeft(bool shift, bool ctrl)
    {
        if (HasSelection && !shift)
        {
            SetSelection(SelectionLeft);
        }
        else if (SelectionLeft >= 0)
        {
            int position;
            if (ctrl)
                position = FindPrevWordBegin();
            else
                position = Math::Max(_selectionEnd - 1, 0);

            if (shift)
            {
                SetSelection(_selectionStart, position);
            }
            else
            {
                SetSelection(position);
            }
        }
    }

    void TextBoxBase::MoveDown(bool shift, bool ctrl)
    {
        if (HasSelection && !shift)
        {
            SetSelection(SelectionRight);
        }
        else
        {
            int position = FindLineDownChar(CaretPosition);

            if (shift)
            {
                SetSelection(_selectionStart, position);
            }
            else
            {
                SetSelection(position);
            }
        }
    }

    void TextBoxBase::MoveUp(bool shift, bool ctrl)
    {
        if (HasSelection && !shift)
        {
            SetSelection(SelectionLeft);
        }
        else
        {
            int position = FindLineUpChar(CaretPosition);

            if (shift)
            {
                SetSelection(_selectionStart, position);
            }
            else
            {
                SetSelection(position);
            }
        }
    }

    void TextBoxBase::SetSelection(int start, int end, bool withScroll)
    {
        // Update parameters
        int textLength = _text.Length();
        _selectionStart = Math::Clamp(start, -1, textLength);
        _selectionEnd = Math::Clamp(end, -1, textLength);

        if (withScroll)
        {
            // Update view on caret modified
            ScrollToCaret();

            // Reset caret and selection animation
            _animateTime = 0.0f;
        }
    }

    void TextBoxBase::OnSelectingBegin()
    {
        if (!_isSelecting)
        {
            // Set flag
            _isSelecting = true;

            // Start tracking mouse
            StartMouseCapture();
        }
    }

    void TextBoxBase::OnSelectingEnd()
    {
        if (_isSelecting)
        {
            // Clear flag
            _isSelecting = false;

            // Stop tracking mouse
            EndMouseCapture();
        }
    }

    void TextBoxBase::OnEditBegin()
    {
        if (_isEditing)
            return;

        _isEditing = true;
        _onStartEditValue = _text;

        // Reset caret visibility
        _animateTime = 0;
    }

    void TextBoxBase::OnEditEnd()
    {
        if (!_isEditing)
            return;

        _isEditing = false;
        if (_onStartEditValue != _text)
        {
            _onStartEditValue = _text;
            EditEnd();
            TextBoxEditEnd(this);
        }
        _onStartEditValue = String::Empty;

        ClearSelection();
        ResetViewOffset();
    }

    void TextBoxBase::OnTextChanged()
    {
        _textSize = __GetTextSize();
        TextChanged();
    }

    void TextBoxBase::__SetIsMultiline(bool value)
    {
        if (_isMultiline != value)
        {
            _isMultiline = value;

            OnIsMultilineChanged();
            Deselect();

            if (!_isMultiline)
            {
                List<String> lines;
                _text.Split(SE_TEXT('\n'), lines);
                _text = lines[0];
            }
        }
    }

    void TextBoxBase::__SetMaxLength(int value)
    {
        ENGINE_ASSERT(_maxLength > 0);
        if (_maxLength != value)
        {
            _maxLength = value;

            // Cut too long text
            if (_text.Length() > _maxLength)
            {
                Text = _text.Substring(0, _maxLength);
            }
        }
    }

    void TextBoxBase::__SetIsReadOnly(bool value)
    {
        if (_isReadOnly != value)
        {
            _isReadOnly = value;
            OnIsReadOnlyChanged();
        }
    }

    void TextBoxBase::__SetIsSelectable(bool value)
    {
        if (_isSelectable != value)
        {
            _isSelectable = value;
            OnIsSelectableChanged();
        }
    }
    void TextBoxBase::__SetTargetViewOffset(Float2& value)
    {
        value = Float2::Round(value);
        if (Float2::NearEqual( value, _targetViewOffset))
            return;
        _targetViewOffset = _viewOffset = value;
        OnTargetViewOffsetChanged();
    }

    void TextBoxBase::__SetText(String& value)
    {
        // Skip set if user is editing value
        if (_isEditing)
            return;

        SetText(value);
    }

    String TextBoxBase::__GetSelectedText()
    {
        int length = SelectionLength;
        return length > 0 ? _text.Substring(SelectionLeft, length) : String::Empty;
    }

    TextRange TextBoxBase::__GetSelectionRange()
    {
        TextRange textRange;
        textRange.StartIndex = SelectionLeft;
        textRange.EndIndex = SelectionRight;
        return textRange;
    }

    Rectangle TextBoxBase::__GetCaretBounds()
    {
        const float caretWidth = 1.2f;
        float height;
        Float2 caretPos = GetCharPosition(CaretPosition, height);
        return  Rectangle(
                             caretPos.x - (caretWidth * 0.5f),
                             caretPos.y,
                             caretWidth,
                             height * GetDpiScale());
    }
    Rectangle TextBoxBase::__GetTextRectangle()
    {
        return Rectangle(DefaultMargin, 1, Width - 2 * DefaultMargin, Height - 2);
    }

    void TextBoxBase::SetText(StringView value)
    {
        String temp;
        // Prevent from null problems
        if (temp.Length() <= 0)
        {
            temp = String::Empty;
        }
        else
        {
            temp = value;
        }

        // Filter text
        if (temp.Find('\r') != -1)
        {
            temp = temp.Replace(SE_TEXT("\r"), SE_TEXT(""));
        }

        // Filter text (handle backspace control character)
        if (temp.Find(DelChar) != -1)
            temp = temp.Replace(&DelChar, SE_TEXT(""));

        // Clamp length
        if (temp.Length() > MaxLength)
            temp = temp.Substring(0, MaxLength);


        // Ensure to use only single line
        if (_isMultiline == false && temp.Length() > 0)
        {
            // Extract only the first line
            List<String> lines;
            temp.Split({SE_TEXT("\r\n"), SE_TEXT("\r"), SE_TEXT("\n") }, lines);
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

    void TextBoxBase::SetText(String& value)
    {
        SetText(StringView(value));
    }

    void TextBoxBase::SetTextAsUser(StringView value)
    {
        Focus();
        SetText(value);
        RemoveFocus();
    }

    void TextBoxBase::SetTextAsUser(String& value)
    {
        SetTextAsUser(StringView(value));
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="TextBoxBase"/> class.
    /// </summary>
    TextBoxBase::TextBoxBase()
    {
    }

    void TextBoxBase::Clear()
    {
        Text = String::Empty;
    }

    void TextBoxBase::ClearSelection()
    {
        OnSelectingEnd();
        SetSelection(-1);
    }

    void TextBoxBase::ResetViewOffset()
    {
        TargetViewOffset = Float2::Zero;
    }

    void TextBoxBase::Copy()
    {
        StringView selectedText(SelectedText);
        if (selectedText.Length() > 0)
        {
            // Copy selected text
            Clipboard::SetText(selectedText);
        }
    }

    void TextBoxBase::Cut()
    {
        StringView selectedText{ SelectedText };
        if (selectedText.Length() > 0)
        {
            // Copy selected text
            Clipboard::SetText(selectedText);

            if (IsReadOnly)
                return;

            // Remove selection
            int left = SelectionLeft;
            _text.Remove(left, SelectionLength);
            SetSelection(left);
            OnTextChanged();
        }
    }

    void TextBoxBase::Paste()
    {
        if (IsReadOnly)
            return;

        // Get clipboard data
        StringView clipboardText = Clipboard::GetText();
        if (clipboardText.Length() <= 0)
        {
            return;
        }

        Insert(clipboardText);
    }

    void TextBoxBase::Duplicate()
    {
        if (IsReadOnly)
            return;

        StringView selectedText{ SelectedText };
        if (selectedText.Length() > 0)
        {
            float right = SelectionRight;
            SetSelection(right);
            Insert(selectedText);
            SetSelection(right, right + selectedText.Length(), true);
        }
    }

    void TextBoxBase::ScrollToCaret()
    {
        // If it's empty
        if (_text.Length() == 0)
        {
            TargetViewOffset = Float2::Zero;
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
        Float2 clampedCaretInView = Float2::Clamp(caretInView, textArea.GetUpperLeft(), textArea.GetBottomRight());
        TargetViewOffset = caretInView - clampedCaretInView + TargetViewOffset;
    }

    void TextBoxBase::SelectAll()
    {
        if (TextLength > 0)
        {
            SetSelection(0, TextLength);
        }
    }

    void TextBoxBase::Deselect()
    {
        SetSelection(-1);
    }

    int TextBoxBase::CharIndexAtPoint(Float2& location)
    {
        return HitTestText(location + _viewOffset);
    }

    void TextBoxBase::Insert(Char c)
    {
        Insert(StringView(&c, 1));
    }

    void TextBoxBase::Insert(StringView str)
    {
        if (IsReadOnly)
            return;

        String temp = str;

        // Filter text
        if (temp.Find('\r') != INVALID_INDEX)
            temp = temp.Replace(SE_TEXT("\r"), SE_TEXT(""));
        if (temp.Find(DelChar) != INVALID_INDEX)
            temp = temp.Replace(&DelChar, SE_TEXT(""));
        if (!IsMultiline && str.Find('\n') != INVALID_INDEX)
            temp = temp.Replace(SE_TEXT("\n"), SE_TEXT(""));

        int selectionLength = SelectionLength;
        int charactersLeft = MaxLength - _text.Length() + selectionLength;
        ENGINE_ASSERT(charactersLeft >= 0);

        if (charactersLeft == 0)
            return;
        if (charactersLeft < str.Length())
            str = str.Substring(0, charactersLeft);

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
                _text.Remove(left, selectionLength);
                SetSelection(left);
            }
            _text.Insert(left, str);

            SetSelection(left + str.Length());
        }

        OnTextChanged();
    }

    int TextBoxBase::FindNextWordBegin()
    {
        int textLength = TextLength;
        int caretPos = CaretPosition;

        if (caretPos + 1 >= textLength)
            return textLength;

        int spaceLoc = _text.Find(Separators, caretPos + 1);

        if (spaceLoc == -1)
            spaceLoc = textLength;
        else
            spaceLoc++;

        return spaceLoc;
    }

    int TextBoxBase::FindPrevWordBegin()
    {
        int caretPos = CaretPosition;

        if (caretPos - 2 < 0)
            return 0;

        int spaceLoc = _text.FindLast(Separators, caretPos - 2);

        if (spaceLoc == -1)
            spaceLoc = 0;
        else
            spaceLoc++;

        return spaceLoc;
    }

    int TextBoxBase::FindPrevLineBegin()
    {
        int caretPos = CaretPosition;
        if (caretPos - 2 < 0)
            return 0;
        int newLineLoc = _text.FindLast(SE_TEXT('\n'), caretPos - 2);
        if (newLineLoc == -1)
            newLineLoc = 0;
        else
            newLineLoc++;
        return newLineLoc;
    }

    int TextBoxBase::FindLineDownChar(int index)
    {
        if (!IsMultiline)
            return 0;

        float height = Height;
        Float2 location = GetCharPosition(index, height);
        location.y += height;

        return HitTestText(location);
    }

    int TextBoxBase::FindLineUpChar(int index)
    {
        if (!IsMultiline)
            return _text.Length();

        float height = Height;
        Float2 location = GetCharPosition(index, height);
        location.y -= height;

        return HitTestText(location);
    }

    void TextBoxBase::RemoveFocus()
    {
        if (Parent != nullptr)
            Parent->Focus();
        else
            Defocus();
    }

    void TextBoxBase::Update(float deltaTime)
    {
        bool isDeltaSlow = deltaTime > (1 / 20.0f);

        _animateTime += deltaTime;

        // Animate view offset
        _viewOffset = isDeltaSlow ? _targetViewOffset : Float2::Lerp(_viewOffset, _targetViewOffset, deltaTime * 20.0f);

        // Clicking outside of the text box will end text editing. Left will keep the value, right will restore original value
        if (_isEditing && EndEditOnClick)
        {
            if (!IsMouseOver && RootWindow()->ContainsFocus())
            {
                if (Input::GetMouseButtonDown(MouseButton::Left))
                {
                    RemoveFocus();
                }
                else if (Input::GetMouseButtonDown(MouseButton::Right))
                {
                    RestoreTextFromStart();
                    RemoveFocus();
                }
            }
        }

        ContainerControl::Update(deltaTime);
    }
    
    void TextBoxBase::RestoreTextFromStart()
    {
        // Restore text from start
        SetSelection(-1);
        _text = _onStartEditValue;
        OnTextChanged();
    }
    
    void TextBoxBase::OnGetFocus()
    {
        ContainerControl::OnGetFocus();

        if (IsReadOnly)
            return;
        OnEditBegin();
    }
    
    void TextBoxBase::OnLostFocus()
    {
        ContainerControl::OnLostFocus();

        if (IsReadOnly)
            return;
        OnEditEnd();
    }
    
    void TextBoxBase::OnEndMouseCapture()
    {
        // Clear flag
        _isSelecting = false;
    }
    
    void TextBoxBase::NavigationFocus()
    {
        ContainerControl::NavigationFocus();

        if (IsNavFocused)
            SelectAll();
    }
    
    void TextBoxBase::OnSubmit()
    {
        OnEditEnd();
        if (IsNavFocused)
        {
            OnEditBegin();
            SelectAll();
        }

        ContainerControl::OnSubmit();
    }
    
    void TextBoxBase::OnMouseEnter(Float2 location)
    {
        if (_isEditing && _changeCursor)
            Cursor = CursorType::IBeam;

        ContainerControl::OnMouseEnter(location);
    }
    
    void TextBoxBase::OnMouseLeave()
    {
        if (Cursor == CursorType::IBeam)
            Cursor = CursorType::Default;

        ContainerControl::OnMouseLeave();
    }
    
    void TextBoxBase::OnMouseMove(Float2 location)
    {
        ContainerControl::OnMouseMove(location);

        if (_isSelecting)
        {
            // Find char index at current mouse location
            int currentIndex = CharIndexAtPoint(location);

            // Modify selection end
            SetSelection(_selectionStart, currentIndex);
        }

        if (Cursor == CursorType::Default && _isEditing && _changeCursor)
        {
            Cursor = CursorType::IBeam;
        }
    }
    
    bool TextBoxBase::OnMouseDown(Float2 location, MouseButton button)
    {
        if (ContainerControl::OnMouseDown(location, button))
            return true;

        if (button == MouseButton::Left && _isSelectable)
        {
            Focus();
            OnSelectingBegin();

            // Calculate char index under the mouse location
            int hitPos = CharIndexAtPoint(location);

            // Select range with shift
            if (_selectionStart != -1 && RootWindow()->GetKey(KeyboardKeys::Shift) && SelectionLength == 0)
            {
                if (hitPos < _selectionStart)
                    SetSelection(hitPos, _selectionStart);
                else
                    SetSelection(_selectionStart, hitPos);
            }
            else if (_text.Length() <= 0)
            {
                SetSelection(0);
            }
            else
            {
                SetSelection(hitPos);
            }

            if (Cursor == CursorType::Default && _changeCursor)
                Cursor = CursorType::IBeam;

            return true;
        }

        if (button == MouseButton::Left && !IsFocused)
        {
            Focus();
            if (_changeCursor)
                Cursor = CursorType::IBeam;
            return true;
        }

        return false;
    }
    
    bool TextBoxBase::OnMouseUp(Float2 location, MouseButton button)
    {
        if (ContainerControl::OnMouseUp(location, button))
            return true;

        if (button == MouseButton::Left && _isSelectable)
        {
            OnSelectingEnd();
            return true;
        }

        return false;
    }
    
    bool TextBoxBase::OnMouseWheel(Float2 location, float delta)
    {
        if (ContainerControl::OnMouseWheel(location, delta))
            return true;

        // Multiline scroll
        if (IsMultiline && _text.Length() != 0 && IsMultilineScrollable)
        {
            if (Input::GetKey(KeyboardKeys::Shift))
                TargetViewOffset = Float2::Clamp(_targetViewOffset - Float2(delta * 20.0f, 0), Float2::Zero,  Float2(_textSize.x, _targetViewOffset.y));
            else
                TargetViewOffset = Float2::Clamp(_targetViewOffset - Float2(0, delta * 10.0f), Float2::Zero,  Float2(_targetViewOffset.x, _textSize.y - Height));
                
            return true;
        }

        // No event handled
        return false;
    }
    
    bool TextBoxBase::OnCharInput(Char c)
    {
        if (ContainerControl::OnCharInput(c))
            return true;
        if (IsReadOnly)
            return false;
        Insert(c);
        return true;
    }
    
    void TextBoxBase::OnKeyUp(KeyboardKeys key)
    {
        ContainerControl::OnKeyUp(key);
        KeyUp(key);
    }
    
	bool TextBoxBase::OnKeyDown(KeyboardKeys key)
    {
        RootControl* window = Root;
        bool shiftDown = window->GetKey(KeyboardKeys::Shift);
        bool ctrDown = window->GetKey(KeyboardKeys::Control);
        KeyDown(key);

        // Handle controls that have bindings
/*#if SE_EDITOR
        InputOptions options = FlaxEditor.Editor.Instance.Options.Options.Input;
        if (options.Copy.Process(this))
        {
            Copy();
            return true;
        }
        else if (options.Paste.Process(this))
        {
            Paste();
            return true;
        }
        else if (options.Duplicate.Process(this))
        {
            Duplicate();
            return true;
        }
        else if (options.Cut.Process(this))
        {
            Cut();
            return true;
        }
        else if (options.SelectAll.Process(this))
        {
            SelectAll();
            return true;
        }
        else if (options.DeselectAll.Process(this))
        {
            Deselect();
            return true;
        }
#endif*/

        // Handle controls without bindings
        switch (key)
        {
        case KeyboardKeys::ArrowRight:
            MoveRight(shiftDown, ctrDown);
            return true;
        case KeyboardKeys::ArrowLeft:
            MoveLeft(shiftDown, ctrDown);
            return true;
        case KeyboardKeys::ArrowUp:
            MoveUp(shiftDown, ctrDown);
            return true;
        case KeyboardKeys::ArrowDown:
            MoveDown(shiftDown, ctrDown);
            return true;
        case KeyboardKeys::C:
            if (ctrDown)
            {
                Copy();
                return true;
            }
            break;
        case KeyboardKeys::V:
            if (ctrDown)
            {
                Paste();
                return true;
            }
            break;
        case KeyboardKeys::D:
            if (ctrDown)
            {
                Duplicate();
                return true;
            }
            break;
        case KeyboardKeys::X:
            if (ctrDown)
            {
                Cut();
                return true;
            }
            break;
        case KeyboardKeys::A:
            if (ctrDown)
            {
                SelectAll();
                return true;
            }
            break;
        case KeyboardKeys::Backspace:
        {
            if (IsReadOnly)
                return true;

            if (ctrDown)
            {
                int prevWordBegin = FindPrevWordBegin();
                _text.Remove(prevWordBegin, CaretPosition - prevWordBegin);
                SetSelection(prevWordBegin);
                OnTextChanged();
                return true;
            }

            int left = SelectionLeft;
            if (HasSelection)
            {
                _text.Remove(left, SelectionLength);
                SetSelection(left);
                OnTextChanged();
            }
            else if (CaretPosition > 0)
            {
                left -= 1;
                _text.Remove(left, 1);
                SetSelection(left);
                OnTextChanged();
            }

            return true;
        }
        case KeyboardKeys::Delete:
        {
            if (IsReadOnly)
                return true;

            int left = SelectionLeft;
            if (HasSelection)
            {
                _text.Remove(left, SelectionLength);
                SetSelection(left);
                OnTextChanged();
            }
            else if (TextLength > 0 && left < TextLength)
            {
                _text.Remove(left, 1);
                SetSelection(left);
                OnTextChanged();
            }

            return true;
        }
        case KeyboardKeys::Escape:
        {
            if (IsReadOnly)
            {
                SetSelection(_selectionEnd);
                return true;
            }

            RestoreTextFromStart();

            if (!IsNavFocused)
                RemoveFocus();

            return true;
        }
        case KeyboardKeys::Return:
            if (IsMultiline)
            {
                // Insert new line
                Insert('\n');
                ScrollToCaret();
            }
            else if (!IsNavFocused)
            {
                // End editing
                RemoveFocus();
            }
            else
                return false;
            return true;
        case KeyboardKeys::Home:
            if (shiftDown)
            {
                // Select text from the current cursor point back to the beginning of the line
                if (_selectionStart != -1)
                {
                    SetSelection(FindPrevLineBegin(), _selectionStart);
                }
            }
            else
            {
                // Move caret to the first character
                SetSelection(0);
            }
            return true;
        case KeyboardKeys::End:
        {
            // Move caret after last character
            SetSelection(TextLength);
            return true;
        }
        case KeyboardKeys::Tab:
            // Don't process
                return false;
        }

        return true;
    }

    TextBoxBase::TextBoxBase(bool isMultiline, float x, float y, float width) : ContainerControl(x, y, width, DefaultHeight)
    {
        _isMultiline = isMultiline;
        _maxLength = 2147483646;
        _selectionStart = _selectionEnd = -1;

        Style* style = Style::Current;
        CaretColor = style->Foreground;
        BorderColor = Colors::Transparent;
        BorderSelectedColor = style->BackgroundSelected;
        BackgroundColor = style->TextBoxBackground;
        BackgroundSelectedColor = style->TextBoxBackgroundSelected;
    }
    void TextBoxBase::OnTargetViewOffsetChanged()
    {
        TargetViewOffsetChanged();
    }

    void TextBoxBase::MoveRight(bool shift, bool ctrl)
    {
        if (HasSelection && !shift)
        {
            SetSelection(SelectionRight);
        }
        else if (SelectionRight < TextLength)
        {
            int position;
            if (ctrl)
                position = FindNextWordBegin();
            else
                position = _selectionEnd + 1;

            if (shift)
            {
                SetSelection(_selectionStart, position);
            }
            else
            {
                SetSelection(position);
            }
        }
    }
} // SE
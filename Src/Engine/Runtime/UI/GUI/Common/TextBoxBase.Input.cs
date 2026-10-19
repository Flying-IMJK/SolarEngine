#nullable disable annotations

using System;

namespace SE.GUI
{
    public abstract partial class TextBoxBase
    {
        /// <summary>
        /// Moves the caret left.
        /// </summary>
        /// <param name="shift">Shift is held.</param>
        /// <param name="ctrl">Control is held.</param>
        protected virtual void MoveLeft(bool shift, bool ctrl)
        {
            if (HasSelection && !shift)
            {
                SetSelection(SelectionLeft);
            }
            else if (SelectionLeft >= 0)
            {
                int position;
                if (ctrl)
                {
                    position = FindPrevWordBegin();
                }
                else
                {
                    position = Math.Max(_selectionEnd - 1, 0);
                }

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

        /// <summary>
        /// Moves the caret right.
        /// </summary>
        /// <param name="shift">Shift is held.</param>
        /// <param name="ctrl">Control is held.</param>
        protected virtual void MoveRight(bool shift, bool ctrl)
        {
            if (HasSelection && !shift)
            {
                SetSelection(SelectionRight);
            }
            else if (SelectionRight < TextLength)
            {
                int position;
                if (ctrl)
                {
                    position = FindNextWordBegin();
                }
                else
                {
                    position = _selectionEnd + 1;
                }

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

        /// <summary>
        /// Moves the caret down.
        /// </summary>
        /// <param name="shift">Shift is held.</param>
        /// <param name="ctrl">Control is held.</param>
        protected virtual void MoveDown(bool shift, bool ctrl)
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

        /// <summary>
        /// Moves the caret up.
        /// </summary>
        /// <param name="shift">Shift is held.</param>
        /// <param name="ctrl">Control is held.</param>
        protected virtual void MoveUp(bool shift, bool ctrl)
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
        private int FindNextWordBegin()
        {
            int textLength = TextLength;
            int caretPos = CaretPosition;
            if (caretPos + 1 >= textLength)
            {
                return textLength;
            }

            int spaceLoc = _text.IndexOfAny(Separators, caretPos + 1);
            if (spaceLoc == -1)
            {
                spaceLoc = textLength;
            }
            else
            {
                spaceLoc++;
            }

            return spaceLoc;
        }
        private int FindPrevWordBegin()
        {
            int caretPos = CaretPosition;
            if (caretPos - 2 < 0)
            {
                return 0;
            }

            int spaceLoc = _text.LastIndexOfAny(Separators, caretPos - 2);
            if (spaceLoc == -1)
            {
                spaceLoc = 0;
            }
            else
            {
                spaceLoc++;
            }

            return spaceLoc;
        }
        private int FindPrevLineBegin()
        {
            int caretPos = CaretPosition;
            if (caretPos - 2 < 0)
            {
                return 0;
            }

            int newLineLoc = _text.LastIndexOf('\n', caretPos - 2);
            if (newLineLoc == -1)
            {
                newLineLoc = 0;
            }
            else
            {
                newLineLoc++;
            }

            return newLineLoc;
        }
        private int FindLineDownChar(int index)
        {
            if (!IsMultiline)
            {
                return 0;
            }

            float height = Height;
            Float2 location = GetCharPosition(index, ref height);
            location.Y += height;
            return HitTestText(location);
        }
        private int FindLineUpChar(int index)
        {
            if (!IsMultiline)
            {
                return TextLength;
            }

            float height = Height;
            Float2 location = GetCharPosition(index, ref height);
            location.Y -= height;
            return HitTestText(location);
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            bool isDeltaSlow = deltaTime > (1 / 20.0f);
            _animateTime += deltaTime;

            // Animate view offset
            _viewOffset = isDeltaSlow ? _targetViewOffset : Float2.Lerp(_viewOffset, _targetViewOffset, deltaTime
                * 20.0f);

            // Clicking outside of the text box will end text editing. Left will keep the value, right will restore
            // original value
            if (_isEditing && EndEditOnClick)
            {
                if (!IsMouseOver && RootWindow().ContainsFocus)
                {
                    if (Input.GetMouseButtonDown(MouseButton.Left))
                    {
                        RemoveFocus();
                    }
                    else if (Input.GetMouseButtonDown(MouseButton.Right))
                    {
                        RestoreTextFromStart();
                        RemoveFocus();
                    }
                }
            }
            base.Update(deltaTime);
        }

        /// <summary>
        /// Restores the Text from the start.
        /// </summary>
        public void RestoreTextFromStart()
        {
            // Restore text from start
            SetSelection(-1);
            _text = _onStartEditValue;
            OnTextChanged();
        }

        /// <inheritdoc />
        public override void OnGetFocus()
        {
            base.OnGetFocus();
            if (IsReadOnly)
            {
                return;
            }
            OnEditBegin();
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            base.OnLostFocus();
            if (IsReadOnly)
            {
                return;
            }
            OnEditEnd();
        }

        /// <inheritdoc />
        public override void OnEndMouseCapture()
        {
            // Clear flag
            _isSelecting = false;
        }

        /// <inheritdoc />
        public override void NavigationFocus()
        {
            base.NavigationFocus();
            if (IsNavFocused)
            {
                SelectAll();
            }
        }

        /// <inheritdoc />
        public override void OnSubmit()
        {
            OnEditEnd();
            if (IsNavFocused)
            {
                OnEditBegin();
                SelectAll();
            }
            base.OnSubmit();
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            if (_isEditing && _changeCursor)
            {
                Cursor = CursorType.IBeam;
            }

            base.OnMouseEnter(location);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            if (Cursor == CursorType.IBeam)
            {
                Cursor = CursorType.Default;
            }

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            base.OnMouseMove(location);
            if (_isSelecting)
            {
                // Find char index at current mouse location
                int currentIndex = CharIndexAtPoint(ref location);

                // Modify selection end
                SetSelection(_selectionStart, currentIndex);
            }

            if (Cursor == CursorType.Default && _isEditing && _changeCursor)
            {
                Cursor = CursorType.IBeam;
            }
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
            {
                return true;
            }

            if (button == MouseButton.Left && _isSelectable)
            {
                Focus();
                OnSelectingBegin();
                // Calculate char index under the mouse location
                int hitPos = CharIndexAtPoint(ref location);

                // Select range with shift
                if (_selectionStart != -1 && RootWindow().GetKey(KeyboardKeys.Shift) && SelectionLength == 0)
                {
                    if (hitPos < _selectionStart)
                    {
                        SetSelection(hitPos, _selectionStart);
                    }
                    else
                    {
                        SetSelection(_selectionStart, hitPos);
                    }
                }
                else if (_text.Length <= 0)
                {
                    SetSelection(0);
                }
                else
                {
                    SetSelection(hitPos);
                }

                if (Cursor == CursorType.Default && _changeCursor)
                {
                    Cursor = CursorType.IBeam;
                }

                return true;
            }
            if (button == MouseButton.Left && !IsFocused)
            {
                Focus();
                if (_changeCursor)
                {
                    Cursor = CursorType.IBeam;
                }

                return true;
            }
            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (base.OnMouseUp(location, button))
            {
                return true;
            }

            if (button == MouseButton.Left && _isSelectable)
            {
                OnSelectingEnd();
                return true;
            }
            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseWheel(Float2 location, float delta)
        {
            if (base.OnMouseWheel(location, delta))
            {
                return true;
            }

            // Multiline scroll
            if (IsMultiline && _text.Length != 0 && IsMultilineScrollable)
            {
                if (Input.GetKey(KeyboardKeys.Shift))
                {
                    TargetViewOffset = Float2.Clamp(
                        _targetViewOffset - new Float2(delta * 20, 0),
                        Float2.Zero,
                        new Float2(_textSize.X, _targetViewOffset.Y));
                }
                else
                {
                    TargetViewOffset = Float2.Clamp(
                        _targetViewOffset - new Float2(0, delta * 10),
                        Float2.Zero,
                        new Float2(_targetViewOffset.X, _textSize.Y - Height));
                }

                return true;
            }
            // No event handled
            return false;
        }

        /// <inheritdoc />
        public override bool OnCharInput(char c)
        {
            if (base.OnCharInput(c))
            {
                return true;
            }

            if (IsReadOnly)
            {
                return false;
            }

            Insert(c);
            return true;
        }

        /// <inheritdoc />
        public override void OnKeyUp(KeyboardKeys key)
        {
            base.OnKeyUp(key);
            KeyUp?.Invoke(key);
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            RootControl window = Root;
            bool shift = window.GetKey(KeyboardKeys.Shift);
            bool ctrl = window.GetKey(KeyboardKeys.Control);
            KeyDown?.Invoke(key);

            // Handle controls that have bindings (the native editor block is commented out).
            // Handle controls without bindings
            switch (key)
            {
                case KeyboardKeys.ArrowRight:
                    MoveRight(shift, ctrl);
                    return true;
                case KeyboardKeys.ArrowLeft:
                    MoveLeft(shift, ctrl);
                    return true;
                case KeyboardKeys.ArrowUp:
                    MoveUp(shift, ctrl);
                    return true;
                case KeyboardKeys.ArrowDown:
                    MoveDown(shift, ctrl);
                    return true;
                case KeyboardKeys.C:
                    if (ctrl)
                    {
                        Copy();
                        return true;
                    }
                    break;
                case KeyboardKeys.V:
                    if (ctrl)
                    {
                        Paste();
                        return true;
                    }
                    break;
                case KeyboardKeys.D:
                    if (ctrl)
                    {
                        Duplicate();
                        return true;
                    }
                    break;
                case KeyboardKeys.X:
                    if (ctrl)
                    {
                        Cut();
                        return true;
                    }
                    break;
                case KeyboardKeys.A:
                    if (ctrl)
                    {
                        SelectAll();
                        return true;
                    }
                    break;
                case KeyboardKeys.Backspace:
                    if (IsReadOnly)
                    {
                        return true;
                    }

                    if (ctrl)
                    {
                        int previous = FindPrevWordBegin();
                        _text = _text.Remove(previous, CaretPosition - previous);
                        SetSelection(previous);
                        OnTextChanged();
                        return true;
                    }
                    int left = SelectionLeft;
                    if (HasSelection)
                    {
                        _text = _text.Remove(left, SelectionLength);
                        SetSelection(left);
                        OnTextChanged();
                    }
                    else if (CaretPosition > 0)
                    {
                        left--;
                        _text = _text.Remove(left, 1);
                        SetSelection(left);
                        OnTextChanged();
                    }
                    return true;
                case KeyboardKeys.Delete:
                    if (IsReadOnly)
                    {
                        return true;
                    }

                    int deleteLeft = SelectionLeft;
                    if (HasSelection)
                    {
                        _text = _text.Remove(deleteLeft, SelectionLength);
                        SetSelection(deleteLeft);
                        OnTextChanged();
                    }
                    else if (TextLength > 0 && deleteLeft < TextLength)
                    {
                        _text = _text.Remove(deleteLeft, 1);
                        SetSelection(deleteLeft);
                        OnTextChanged();
                    }
                    return true;
                case KeyboardKeys.Escape:
                    if (IsReadOnly)
                    {
                        SetSelection(_selectionEnd);
                        return true;
                    }
                    RestoreTextFromStart();
                    if (!IsNavFocused)
                    {
                        RemoveFocus();
                    }

                    return true;
                case KeyboardKeys.Return:
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
                    {
                        return false;
                    }

                    return true;
                case KeyboardKeys.Home:
                    if (shift)
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
                case KeyboardKeys.End:
                    // Move caret after last character
                    SetSelection(TextLength);
                    return true;
                case KeyboardKeys.Tab:
                    // Don't process
                    return false;
            }
            return true;
        }
    }
}

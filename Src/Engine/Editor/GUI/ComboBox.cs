#nullable disable annotations

using System;
using System.Collections.Generic;
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Combo box control allows to choose one item or set of items from the provided collection of options.
    /// </summary>
    /// <remarks>
    /// Difference between <see cref="ComboBox"/> and <see cref="Dropdown"/> is that ComboBox uses native window to show the items list while Dropdown uses a custom panel added to parent window.
    /// This means that Dropdown will work on all platforms that don't support multiple native windows (eg. Android, PS4, Xbox One).
    /// </remarks>
    public class ComboBox : Control
    {
        /// <summary>
        /// The default height of the control.
        /// </summary>
        public const float DefaultHeight = 18.0f;

        /// <summary>
        /// The popup menu. May be null if has not been used yet.
        /// </summary>
        protected ContextMenu _popupMenu;

        /// <summary>
        /// The mouse down flag.
        /// </summary>
        protected bool _mouseDown;

        /// <summary>
        /// The block popup flag.
        /// </summary>
        protected bool _blockPopup;

        /// <summary>
        /// The selected indices.
        /// </summary>
        protected List<int> _selectedIndices = new List<int>(4);

        /// <summary>
        /// The items.
        /// </summary>
        public List<string> Items = new List<string>();

        /// <summary>
        /// The item tooltips (optional).
        /// </summary>
        public List<string> Tooltips = new List<string>();

        /// <summary>
        /// True if sort items before showing the list, otherwise present them in the unchanged order.
        /// </summary>
        public bool Sorted;

        /// <summary>
        /// Gets or sets a value indicating whether support multi items selection.
        /// </summary>
        public bool SupportMultiSelect;

        /// <summary>
        /// Gets or sets the maximum amount of items in the view. If popup has more items to show it uses a additional scroll panel.
        /// </summary>
        public int MaximumItemsInViewCount;

        /// <summary>
        /// Gets or sets the selected item (returns <see cref="string.Empty"/> if no item is being selected or more than one item is selected).
        /// </summary>
        public string SelectedItem
        {
            get => _selectedIndices.Count == 1 ? Items[_selectedIndices[0]] : string.Empty;
            set => SelectedIndex = Items.IndexOf(value);
        }

        /// <summary>
        /// Gets a value indicating whether this combobox has any item selected.
        /// </summary>
        public bool HasSelection => _selectedIndices.Count != 0;

        /// <summary>
        /// Gets or sets the index of the selected. If combobox has more than 1 item selected then it returns invalid index (value -1).
        /// </summary>
        public int SelectedIndex
        {
            get => _selectedIndices.Count == 1 ? _selectedIndices[0] : -1;
            set => SetSelectedIndex(value);
        }

        /// <summary>
        /// Gets or sets the selection.
        /// </summary>
        public List<int> Selection
        {
            get => _selectedIndices;
            set => SetSelection(value);
        }

        /// <summary>
        /// Event fired when selected index gets changed.
        /// </summary>
        public Action<ComboBox> SelectedIndexChanged;

        /// <summary>
        /// Occurs when popup is showing (before event). Can be used to update items collection before showing it to the user.
        /// </summary>
        public Action<ComboBox> PopupShowing;

        /// <summary>
        /// Custom popup creation function.
        /// </summary>
        public Func<ComboBox, ContextMenu> PopupCreate;

        /// <summary>
        /// Gets the popup menu (it may be null if not used - lazy init).
        /// </summary>
        public ContextMenu Popup => _popupMenu;

        /// <summary>
        /// Gets a value indicating whether this popup menu is opened.
        /// </summary>
        public bool IsPopupOpened => _popupMenu != null && _popupMenu.IsOpened;

        /// <summary>
        /// Gets or sets the font used to draw text.
        /// </summary>
        public FontReference Font;

        /// <summary>
        /// Gets or sets the color of the text.
        /// </summary>
        public Color TextColor;

        /// <summary>
        /// Gets or sets the color of the border.
        /// </summary>
        public Color BorderColor;

        /// <summary>
        /// Gets or sets the background color when combobox popup is opened.
        /// </summary>
        public Color BackgroundColorSelected;

        /// <summary>
        /// Gets or sets the border color when combobox popup is opened.
        /// </summary>
        public Color BorderColorSelected;

        /// <summary>
        /// Gets or sets the background color when combobox is highlighted.
        /// </summary>
        public Color BackgroundColorHighlighted;

        /// <summary>
        /// Gets or sets the border color when combobox is highlighted.
        /// </summary>
        public Color BorderColorHighlighted;

        /// <summary>
        /// Gets or sets the image used to render combobox drop arrow icon.
        /// </summary>
        public IBrush ArrowImage;

        /// <summary>
        /// Gets or sets the color used to render combobox drop arrow icon.
        /// </summary>
        public Color ArrowColor;

        /// <summary>
        /// Gets or sets the color used to render combobox drop arrow icon (menu is opened).
        /// </summary>
        public Color ArrowColorSelected;

        /// <summary>
        /// Gets or sets the color used to render combobox drop arrow icon (menu is highlighted).
        /// </summary>
        public Color ArrowColorHighlighted;

        public ComboBox()
            : this(0, 0)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ComboBox"/> class.
        /// </summary>
        /// <param name="x">The x.</param>
        /// <param name="y">The y.</param>
        /// <param name="width">The width.</param>
        public ComboBox(float x, float y, float width = 120.0f)
            : base(x, y, width, DefaultHeight)
        {
            MaximumItemsInViewCount = 20;

            Style style = Style.Current;
            Font = new FontReference(style.FontMedium);
            TextColor = style.Foreground;
            BackgroundColor = style.BackgroundNormal;
            BackgroundColorHighlighted = BackgroundColor;
            BackgroundColorSelected = BackgroundColor;
            BorderColor = style.BorderNormal;
            BorderColorHighlighted = style.BorderSelected;
            BorderColorSelected = BorderColorHighlighted;
            ArrowImage = new SpriteBrush(style.ArrowDown);
            ArrowColor = style.Foreground * 0.6f;
            ArrowColorSelected = style.BackgroundSelected;
            ArrowColorHighlighted = style.Foreground;
        }

        /// <summary>
        /// Clears the items.
        /// </summary>
        public void ClearItems()
        {
            SelectedIndex = -1;
            Items.Clear();
        }

        /// <summary>
        /// Adds the item.
        /// </summary>
        /// <param name="item">The item.</param>
        public void AddItem(string item)
        {
            Items.Add(item);
        }

        /// <summary>
        /// Adds the items.
        /// </summary>
        /// <param name="items">The items.</param>
        public void AddItems(List<string> items)
        {
            Items.AddRange(items);
        }

        /// <summary>
        /// Sets the items.
        /// </summary>
        /// <param name="items">The items.</param>
        public void SetItems(List<string> items)
        {
            SelectedIndex = -1;
            Items.Clear();
            Items.AddRange(items);
        }

        /// <summary>
        /// Determines whether the specified item is selected.
        /// </summary>
        /// <param name="item">The item to check.</param>
        /// <returns><c>true</c> if the item is selected; otherwise, <c>false</c>.</returns>
        public bool IsSelected(string item)
        {
            return IsSelected(Items.IndexOf(item));
        }

        /// <summary>
        /// Determines whether the item at the specified index is selected.
        /// </summary>
        /// <param name="index">The index.</param>
        /// <returns><c>true</c> if the item is selected; otherwise, <c>false</c>.</returns>
        public bool IsSelected(int index)
        {
            return index != -1 && _selectedIndices.Contains(index);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (_popupMenu != null)
            {
                _popupMenu.Hide();
                _popupMenu.Dispose();
                _popupMenu = null;
            }

            if (IsDisposing)
            {
                return;
            }

            _selectedIndices.Clear();
            Items.Clear();

            base.OnDestroy();
        }

        /// <inheritdoc />
        public override void Draw()
        {
            // Cache data
            Rectangle clientRect = new Rectangle(Float2.Zero, Size);
            float margin = clientRect.Height * 0.2f;
            float boxSize = clientRect.Height - margin * 2;
            bool isOpened = IsPopupOpened;
            bool enabled = EnabledInHierarchy;
            Color backgroundColor = BackgroundColor;
            Color borderColor = BorderColor;
            Color arrowColor = ArrowColor;
            if (!enabled)
            {
                backgroundColor *= 0.5f;
                arrowColor *= 0.7f;
            }
            else if (isOpened || _mouseDown)
            {
                backgroundColor = BackgroundColorSelected;
                borderColor = BorderColorSelected;
                arrowColor = ArrowColorSelected;
            }
            else if (IsMouseOver || IsNavFocused)
            {
                backgroundColor = BackgroundColorHighlighted;
                borderColor = BorderColorHighlighted;
                arrowColor = ArrowColorHighlighted;
            }

            // Background
            Render2D.FillRectangle(clientRect, backgroundColor);
            Render2D.DrawRectangle(clientRect.MakeExpanded(-2.0f), borderColor);

            // Check if has selected item
            if (_selectedIndices.Count > 0)
            {
                string text = _selectedIndices.Count == 1
                    ? (_selectedIndices[0] >= 0 && _selectedIndices[0] < Items.Count ? Items[_selectedIndices[0]] : string.Empty)
                    : "Multiple Values";

                // Draw text of the selected item
                float textScale = Height / DefaultHeight;
                Rectangle textRect = new Rectangle(margin, 0, clientRect.Width - boxSize - 2.0f * margin, clientRect.Height);
                Render2D.PushClip(textRect);
                Color textColor = TextColor;
                Render2D.RenderText(Font.GetFont(), text, textRect,
                    enabled ? textColor : textColor * 0.5f,
                    TextAlignment.Near, TextAlignment.Center, TextWrapping.NoWrap, 1.0f, textScale);
                Render2D.PopClip();
            }

            // Arrow
            if (ArrowImage != null)
            {
                ArrowImage.Draw(new Rectangle(clientRect.Width - margin - boxSize, margin, boxSize, boxSize), arrowColor);
            }
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            base.OnLostFocus();

            // Clear flags
            _mouseDown = false;
            _blockPopup = false;
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            // Clear flags
            _mouseDown = false;
            _blockPopup = false;

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                _mouseDown = true;
                Focus();
                return true;
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (_mouseDown && !_blockPopup)
            {
                _mouseDown = false;
                ShowPopup();
            }
            else
            {
                _blockPopup = false;
            }

            return true;
        }

        /// <inheritdoc />
        public override void OnSubmit()
        {
            base.OnSubmit();

            ShowPopup();
        }

        /// <summary>
        /// Called when selected item index gets changed.
        /// </summary>
        protected virtual void OnSelectedIndexChanged()
        {
            if (Tooltips.Count == Items.Count)
            {
                TooltipText = _selectedIndices.Count == 1 ? Tooltips[_selectedIndices[0]] : null;
            }

            SelectedIndexChanged?.Invoke(this);
        }

        /// <summary>
        /// Called when item is clicked.
        /// </summary>
        /// <param name="index">The index.</param>
        protected virtual void OnItemClicked(int index)
        {
            if (SupportMultiSelect)
            {
                if (_selectedIndices.Contains(index))
                {
                    _selectedIndices.Remove(index);
                }
                else
                {
                    _selectedIndices.Add(index);
                }

                OnSelectedIndexChanged();
            }
            else
            {
                SelectedIndex = index;
            }
        }

        /// <summary>
        /// Shows the context menu popup.
        /// </summary>
        protected void ShowPopup()
        {
            // Ensure to have valid menu
            if (_popupMenu == null)
            {
                _popupMenu = OnCreatePopup();
                _popupMenu.MaximumItemsInViewCount = MaximumItemsInViewCount;

                // Bind events
                _popupMenu.VisibleChanged += cm =>
                {
                    RootControl win = Root;
                    _blockPopup = win != null && new Rectangle(Float2.Zero, Size).Contains(PointFromWindow(win.GetMousePosition()));
                    if (!_blockPopup)
                    {
                        Focus();
                    }
                };
                _popupMenu.ButtonClicked += btn =>
                {
                    OnItemClicked((int)btn.Tag);
                    if (SupportMultiSelect)
                    {
                        // Don't hide in multi-select, so user can edit multiple elements instead of just one
                        UpdateButtons();
                        if (_popupMenu != null)
                        {
                            _popupMenu.PerformLayout();
                        }
                    }
                    else
                    {
                        if (_popupMenu != null)
                        {
                            _popupMenu.Hide();
                        }
                    }
                };
            }

            // Check if menu hs been already shown
            if (_popupMenu.Visible)
            {
                if (!SupportMultiSelect)
                {
                    _popupMenu.Hide();
                }

                return;
            }

            PopupShowing?.Invoke(this);

            // Check if has any items
            if (Items.Count > 0)
            {
                UpdateButtons();

                // Show dropdown list
                _popupMenu.MinimumWidth = Width;
                _popupMenu.Show(this, new Float2(1, Height));

                // Adjust menu position if it is not the down direction
                if (_popupMenu.Direction == ContextMenuDirection.RightUp)
                {
                    SE.Window window = _popupMenu.RootWindow().Window;
                    Float2 position = window.Position;
                    window.Position = new Float2(position.X, position.Y - Height);
                }
            }
        }

        /// <summary>
        /// Called when button is created or updated. Can be used to customize the visuals.
        /// </summary>
        /// <param name="button">The button.</param>
        /// <param name="index">The item index.</param>
        /// <param name="construct">true if button is created else it is repainting the button</param>
        protected virtual void OnLayoutMenuButton(ContextMenuButton button, int index, bool construct = false)
        {
            button.SetChecked(_selectedIndices.Contains(index));
            if (Tooltips.Count > index)
            {
                button.TooltipText = Tooltips[index];
            }
        }

        /// <summary>
        /// Creates the popup menu.
        /// </summary>
        protected virtual ContextMenu OnCreatePopup()
        {
            if (PopupCreate != null)
            {
                return PopupCreate(this);
            }

            return new ContextMenu();
        }

        /// <summary>
        /// Updates buttons layout.
        /// </summary>
        private void UpdateButtons()
        {
            if (_popupMenu.Items.Count != Items.Count)
            {
                List<ContextMenuItem> itemControls = _popupMenu.Items;
                foreach (ContextMenuItem item in itemControls)
                {
                    item.Dispose();
                }

                if (Sorted)
                {
                    QuickSortItems(Items);
                }

                for (int i = 0; i < Items.Count; i++)
                {
                    ContextMenuButton button = _popupMenu.AddButton(Items[i]);
                    OnLayoutMenuButton(button, i, true);
                    button.Tag = i;
                }
            }
            else
            {
                List<ContextMenuItem> itemControls = _popupMenu.Items;
                if (Sorted)
                {
                    QuickSortItems(Items);
                }

                for (int i = 0; i < Items.Count; i++)
                {
                    ContextMenuButton button = itemControls[i] as ContextMenuButton;
                    if (button != null)
                    {
                        button.Text = Items[i];
                        OnLayoutMenuButton(button, i, true);
                    }
                }
            }
        }

        private void SetSelectedIndex(int value)
        {
            // Clamp index
            value = Math.Min(value, Items.Count - 1);

            // Check if index will change
            if (value != SelectedIndex)
            {
                // Select
                _selectedIndices.Clear();
                if (value != -1)
                {
                    _selectedIndices.Add(value);
                }

                OnSelectedIndexChanged();
            }
        }

        private void SetSelection(List<int> value)
        {
            SE.Debug.Assert(!(!SupportMultiSelect && value.Count > 1), "!(!SupportMultiSelect && value.Count > 1)");

            for (int i = 0; i < value.Count; i++)
            {
                int index = value[i];
                if (index < 0 || index >= Items.Count)
                {
                    SE.Debug.Assert(false, "Selection index is outside the item range.");
                }
            }

            if (!SequenceEqual(_selectedIndices, value))
            {
                // Select
                _selectedIndices.Clear();
                _selectedIndices.AddRange(value);
                OnSelectedIndexChanged();
            }
        }

        private static bool SequenceEqual(List<int> left, List<int> right)
        {
            if (left.Count != right.Count)
            {
                return false;
            }

            for (int i = 0; i < left.Count; i++)
            {
                if (left[i] != right[i])
                {
                    return false;
                }
            }

            return true;
        }

        private static void QuickSortItems(List<string> data)
        {
            if (data.Count < 2)
            {
                return;
            }

            Stack<int> stack = new Stack<int>();

            // Push left and right
            stack.Push(0);
            stack.Push(data.Count - 1);

            // Keep sorting from stack while is not empty
            while (stack.Count > 0)
            {
                // Pop right and left
                int right = stack.Pop();
                int left = stack.Pop();

                // Partition
                string pivotValue = data[right];
                int i = left - 1;
                for (int j = left; j <= right - 1; j++)
                {
                    if (string.CompareOrdinal(data[j], pivotValue) < 0)
                    {
                        i++;
                        string swapped = data[i];
                        data[i] = data[j];
                        data[j] = swapped;
                    }
                }

                string pivotSwap = data[i + 1];
                data[i + 1] = data[right];
                data[right] = pivotSwap;
                int pivot = i + 1;

                // If there are elements on left side of pivot, then push left side to stack
                if (pivot - 1 > left)
                {
                    stack.Push(left);
                    stack.Push(pivot - 1);
                }

                // If there are elements on right side of pivot, then push right side to stack
                if (pivot + 1 < right)
                {
                    stack.Push(pivot + 1);
                    stack.Push(right);
                }
            }
        }
    }
}

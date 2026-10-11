#nullable disable annotations

using System;
using System.Collections.Generic;
using SE.GUI;

namespace SE.Editor.GUI
{
    public class ContextMenu : ContextMenuBase
    {
        /// <summary>
        /// The items container.
        /// </summary>
        /// <seealso cref="Panel" />
        protected class ItemsPanel : Panel
        {
            private ContextMenu _menu;

            /// <summary>
            /// Initializes a new instance of the <see cref="ItemsPanel"/> class.
            /// </summary>
            /// <param name="menu">The menu.</param>
            public ItemsPanel(ContextMenu menu)
            {
                _menu = menu;
            }

            /// <inheritdoc />
            protected override void Arrange()
            {
                base.Arrange();

                // Arrange controls
                Margin margin = _menu._itemsMargin;
                float y = 0;
                float width = Width - margin.Width;
                for (int i = 0; i < m_Children.Count; i++)
                {
                    Control control = m_Children[i];
                    if (!control.Visible)
                    {
                        continue;
                    }

                    ContextMenuItem item = control as ContextMenuItem;
                    if (item != null)
                    {
                        float height = item.Height;
                        item.Bounds = new Rectangle(margin.Left, y, width, height);
                        y += height + margin.Height;
                    }
                }
            }
        }

        /// <summary>
        /// The items area margin.
        /// </summary>
        protected Margin _itemsAreaMargin = new Margin(0, 0, 3, 3);

        /// <summary>
        /// The items margin.
        /// </summary>
        protected Margin _itemsMargin = new Margin(16, 0, 2, 0);

        /// <summary>
        /// The items panel.
        /// </summary>
        protected ItemsPanel _panel;

        /// <summary>
        /// Gets or sets the items area margin (items container area margin).
        /// </summary>
        public Margin ItemsAreaMargin
        {
            get => _itemsMargin;
            set
            {
                _itemsAreaMargin = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// Gets or sets the items margin.
        /// </summary>
        public Margin ItemsMargin
        {
            get => _itemsMargin;
            set
            {
                _itemsMargin = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// Gets or sets the minimum popup width.
        /// </summary>
        public float MinimumWidth;

        /// <summary>
        /// Gets or sets the maximum amount of items in the view. If popup has more items to show it uses a additional scroll panel.
        /// </summary>
        public int MaximumItemsInViewCount;

        /// <summary>
        /// Gets the items (readonly).
        /// </summary>
        public List<ContextMenuItem> Items => GetItems();

        /// <summary>
        /// Event fired when user clicks on the button.
        /// </summary>
        public event Action<ContextMenuButton> ButtonClicked;

        /// <summary>
        /// Gets the context menu items container control.
        /// </summary>
        public Panel ItemsContainer => _panel;

        /// <summary>
        /// The auto sort property.
        /// </summary>
        public bool AutoSort
        {
            get => _autosort;
            set
            {
                _autosort = value;
                if (_autosort)
                {
                    SortButtons();
                }
            }
        }

        /// <summary>
        /// Sorts all <see cref="ContextMenuButton"/> alphabetically.
        /// </summary>
        /// <param name="force">Overrides <see cref="AutoSort"/> property.</param>
        public void SortButtons(bool force = false)
        {
            if (!_autosort && !force)
            {
                return;
            }

            List<Control> children = _panel.Children;
            if (children.Count < 2)
            {
                return;
            }

            Stack<int> stack = new Stack<int>();
            stack.Push(0);
            stack.Push(children.Count - 1);
            while (stack.Count > 0)
            {
                int right = stack.Pop();
                int left = stack.Pop();
                Control pivotControl = children[right];
                int i = left - 1;
                for (int j = left; j <= right - 1; j++)
                {
                    if (CompareButtons(children[j], pivotControl))
                    {
                        i++;
                        Control swap = children[i];
                        children[i] = children[j];
                        children[j] = swap;
                    }
                }

                Control pivotSwap = children[i + 1];
                children[i + 1] = children[right];
                children[right] = pivotSwap;
                int pivot = i + 1;
                if (pivot - 1 > left)
                {
                    stack.Push(left);
                    stack.Push(pivot - 1);
                }
                if (pivot + 1 < right)
                {
                    stack.Push(pivot + 1);
                    stack.Push(right);
                }
            }
        }

        /// <summary>
        /// Removes all the added items (buttons, separators, etc.).
        /// </summary>
        public void DisposeAllItems()
        {
            for (int i = _panel.ChildrenCount - 1; _panel.ChildrenCount > 0 && i >= 0; i--)
            {
                Control control = _panel.Children[i];
                if (control is ContextMenuItem)
                {
                    control.Dispose();
                }
            }
        }

        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <returns>Created context menu item control.</returns>
        public ContextMenuButton AddButton(string text)
        {
            ContextMenuButton item = new ContextMenuButton(this, text);
            item.Parent = _panel;
            SortButtons();
            return item;
        }

        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="shortKeys">The short keys.</param>
        /// <returns>Created context menu item control.</returns>
        public ContextMenuButton AddButton(string text, string shortKeys)
        {
            ContextMenuButton item = new ContextMenuButton(this, text, shortKeys);
            item.Parent = _panel;
            SortButtons();
            return item;
        }

        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="clicked">On button clicked event.</param>
        /// <returns>Created context menu item control.</returns>
        public ContextMenuButton AddButton(string text, Action clicked)
        {
            ContextMenuButton item = new ContextMenuButton(this, text);
            item.Parent = _panel;
            item.Clicked += clicked;
            SortButtons();
            return item;
        }

        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="shortKeys">The shortKeys.</param>
        /// <param name="clicked">On button clicked event.</param>
        /// <returns>Created context menu item control.</returns>
        public ContextMenuButton AddButton(string text, string shortKeys, Action clicked)
        {
            ContextMenuButton item = new ContextMenuButton(this, text, shortKeys);
            item.Parent = _panel;
            item.Clicked += clicked;
            SortButtons();
            return item;
        }

        /// <summary>
        /// Gets the child menu (with that name).
        /// </summary>
        /// <param name="text">The text.</param>
        /// <returns>Created context menu item control or null if missing.</returns>
        public ContextMenuChildMenu GetChildMenu(string text)
        {
            for (int i = 0; i < _panel.ChildrenCount; i++)
            {
                ContextMenuChildMenu menu = _panel.Children[i] as ContextMenuChildMenu;
                if (menu != null && menu.Text == text)
                {
                    return menu;
                }
            }
            return null;
        }

        /// <summary>
        /// Adds the child menu or gets it if already created (with that name).
        /// </summary>
        /// <param name="text">The text.</param>
        /// <returns>Created context menu item control.</returns>
        public ContextMenuChildMenu GetOrAddChildMenu(string text)
        {
            ContextMenuChildMenu item = GetChildMenu(text);
            if (item == null)
            {
                item = new ContextMenuChildMenu(this, text);
                item.Parent = _panel;
            }
            return item;
        }

        /// <summary>
        /// Adds the child menu.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <returns>Created context menu item control.</returns>
        public ContextMenuChildMenu AddChildMenu(string text)
        {
            ContextMenuChildMenu item = new ContextMenuChildMenu(this, text);
            item.Parent = _panel;
            return item;
        }

        /// <summary>
        /// Adds the separator.
        /// </summary>
        /// <returns>Created context menu item control.</returns>
        public ContextMenuSeparator AddSeparator()
        {
            ContextMenuSeparator item = new ContextMenuSeparator(this);
            item.Parent = _panel;
            return item;
        }

        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="clicked">On button clicked event.</param>
        /// <returns>Created context menu item control.</returns>
        public ContextMenuButton AddButton(string text, Action<ContextMenuButton> clicked)
        {
            ContextMenuButton item = new ContextMenuButton(this, text);
            item.Parent = _panel;
            item.ButtonClicked += clicked;
            SortButtons();
            return item;
        }

        /// <summary>
        /// Called when button get clicked.
        /// </summary>
        /// <param name="button">The button.</param>
        public virtual void OnButtonClicked(ContextMenuButton button)
        {
            ButtonClicked?.Invoke(button);
        }

        /// <inheritdoc />
        public override void Show(Control parent, Float2 location)
        {
            // Remove last separator to make context menu look better
            int lastIndex = _panel.ChildrenCount - 1;
            if (lastIndex >= 0)
            {
                ContextMenuSeparator separator = parent as ContextMenuSeparator;
                if (separator != null)
                {
                    separator.Dispose();
                }
            }

            base.Show(parent, location);
        }

        /// <inheritdoc />
        public override bool ContainsPoint(Float2 location, bool precise = false)
        {
            if (base.ContainsPoint(location, precise))
            {
                return true;
            }

            Float2 childLocation = location - Location;
            for (int i = 0; i < _panel.ChildrenCount; i++)
            {
                if (_panel.Children[i].ContainsPoint(childLocation, precise))
                {
                    return true;
                }
            }
            return false;
        }

        /// <inheritdoc />
        public override bool OnCharInput(char c)
        {
            if (base.OnCharInput(c))
            {
                return true;
            }

            // Find the item that starts with that character
            if (char.IsLetterOrDigit(c) && _panel.vScrollBar != null && _panel.vScrollBar.Visible)
            {
                int startIndex = 0;
                for (int i = 0; i < _panel.ChildrenCount; i++)
                {
                    ContextMenuButton item = _panel.Children[i] as ContextMenuButton;
                    if (item != null && item.Visible && item.IsFocused)
                    {
                        // Start searching from the last hit item
                        startIndex = i + 1;
                        break;
                    }
                }
                for (int i = startIndex; i < _panel.ChildrenCount; i++)
                {
                    ContextMenuButton item = _panel.Children[i] as ContextMenuButton;
                    if (item != null && item.Visible)
                    {
                        bool startsWith = false;
                        for (int j = 0; j < item.Text.Length; j++)
                        {
                            char k = item.Text[j];
                            if (char.ToLowerInvariant(k) == char.ToLowerInvariant(c))
                            {
                                startsWith = true;
                                break;
                            }
                            if (!char.IsWhiteSpace(k) && k != '>')
                            {
                                break;
                            }
                        }
                        if (startsWith)
                        {
                            // Focus found item
                            item.Focus();
                            _panel.ScrollViewTo(item);
                            return true;
                        }
                    }
                }

                if (startIndex > 0 && startIndex <= _panel.ChildrenCount)
                {
                    // No more items found so start from the top if there are matching items
                    _panel.Children[startIndex - 1].Defocus();
                    return OnCharInput(c);
                }
            }
            return false;
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (base.OnKeyDown(key))
            {
                return true;
            }

            switch (key)
            {
                case KeyboardKeys.ArrowDown:
                    for (int i = 0; i < _panel.ChildrenCount; i++)
                    {
                        ContextMenuButton item = _panel.Children[i] as ContextMenuButton;
                        if (item != null && item.Visible && item.Enabled)
                        {
                            item.Focus();
                            _panel.ScrollViewTo(item);
                            return true;
                        }
                    }
                    break;
                case KeyboardKeys.ArrowUp:
                    for (int i = _panel.ChildrenCount - 1; i >= 0; i--)
                    {
                        ContextMenuButton item = _panel.Children[i] as ContextMenuButton;
                        if (item != null && item.Visible && item.Enabled)
                        {
                            item.Focus();
                            _panel.ScrollViewTo(item);
                            return true;
                        }
                    }
                    break;
            }
            return false;
        }

        public ContextMenu()
        {
            MinimumWidth = 10;
            MaximumItemsInViewCount = 20;

            _panel = new ItemsPanel(this);
            _panel.ClipChildren = false;
            _panel.Parent = this;
        }

        /// <inheritdoc />
        protected override void PerformLayoutAfterChildren()
        {
            Float2 prevSize = Size;

            // Calculate size of the context menu (items only)
            float maxWidth = 0;
            float height = _itemsAreaMargin.Height;
            int itemsLeft = MaximumItemsInViewCount;
            int overflowItemCount = 0;
            int itemsCount = 0;
            for (int i = 0; i < _panel.ChildrenCount; i++)
            {
                ContextMenuItem item = _panel.Children[i] as ContextMenuItem;
                if (item != null && item.Visible)
                {
                    itemsCount++;
                    if (itemsLeft > 0)
                    {
                        height += item.Height + _itemsMargin.Height;
                        itemsLeft--;
                    }
                    else
                    {
                        overflowItemCount++;
                    }
                    maxWidth = MathF.Max(maxWidth, item.MinimumWidth);
                }
            }
            if (itemsCount != 0)
            {
                height -= _itemsMargin.Height; // Remove item margin from top and bottom
            }
            maxWidth = MathF.Max(maxWidth + 20, MinimumWidth);

            // Move child arrows to accommodate scroll bar showing
            if (overflowItemCount > 0)
            {
                foreach (Control child in _panel.Children)
                {
                    ContextMenuButton item = child as ContextMenuButton;
                    if (item != null && item.Visible)
                    {
                        item.ExtraAdjustmentAmount = -_panel.vScrollBar.Width;
                    }
                }
            }

            // Resize container
            Size = new Float2(MathF.Ceiling(maxWidth), MathF.Ceiling(height));

            // Arrange items view panel
            Rectangle panelBounds = new Rectangle(Float2.Zero, Size);
            _itemsAreaMargin.ShrinkRectangle(ref panelBounds);
            _panel.Bounds = panelBounds;

            // Check if is visible size get changed
            if (Visible && prevSize != Size)
            {
                // Update window dimensions
                UpdateWindowSize();
            }
        }

        private bool _autosort;

        private static bool CompareButtons(Control control, Control control1)
        {
            bool result = false;
            ContextMenuButton button = control as ContextMenuButton;
            ContextMenuButton other = control1 as ContextMenuButton;
            if (button != null && other != null)
            {
                result = string.CompareOrdinal(button.Text, other.Text) > 0;
            }
            if (button == null)
            {
                result = true;
            }
            return result;
        }

        private List<ContextMenuItem> GetItems()
        {
            List<ContextMenuItem> items = new List<ContextMenuItem>();
            foreach (Control control in _panel.Children)
            {
                ContextMenuItem item = control as ContextMenuItem;
                if (item != null)
                {
                    items.Add(item);
                }
            }
            return items;
        }
    }
}

// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using SE.Editor.Utilities;
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// The custom context menu that shows a list of items and supports searching by name and query results
    /// highlighting.
    /// </summary>
    /// <seealso cref="ContextMenuBase" />
    public class ItemsListContextMenu : ContextMenuBase
    {
        /// <summary>
        /// The single list item control.
        /// </summary>
        /// <seealso cref="SE.GUI.Control" />
        [HideInEditor]
        public class Item : Control
        {
            private bool m_IsStartsWithMatch;
            private bool m_IsFullMatch;

            /// <summary>
            /// The is mouse down flag.
            /// </summary>
            protected bool _isMouseDown;

            /// <summary>
            /// The search query highlights.
            /// </summary>
            protected List<Rectangle> _highlights;

            /// <summary>
            /// The item name.
            /// </summary>
            public string Name;

            /// <summary>
            /// The item category name (optional).
            /// </summary>
            public string Category;

            /// <summary>
            /// A computed score for the context menu order
            /// </summary>
            public float SortScore;

            /// <summary>
            /// Whether the query highlights should be drawn.
            /// </summary>
            public bool DrawHighlights = true;

            /// <summary>
            /// Occurs when items gets clicked by the user.
            /// </summary>
            public event Action<Item> ItemClicked;

            /// <summary>
            /// Occurs when items gets focused.
            /// </summary>
            public event Action<Item> ItemFocused;

            /// <summary>
            /// The tint color of the text.
            /// </summary>
            public Color TintColor = Color.White;

            /// <summary>
            /// Initializes a new instance of the <see cref="Item"/> class.
            /// </summary>
            public Item()
            : base(0, 0, 120, 12)
            {
            }

            /// <summary>
            /// Updates the <see cref="SortScore"/>
            /// </summary>
            public void UpdateScore()
            {
                SortScore = 0;

                if (!Visible)
                {
                    return;
                }

                if (_highlights is { Count: > 0 })
                {
                    SortScore += 1;
                }
                if (m_IsStartsWithMatch)
                {
                    SortScore += 2;
                }
                if (m_IsFullMatch)
                {
                    SortScore += 5;
                }
            }

            /// <summary>
            /// Updates the filter.
            /// </summary>
            /// <param name="filterText">The filter text.</param>
            public void UpdateFilter(string filterText)
            {
                m_IsStartsWithMatch = m_IsFullMatch = false;

                if (string.IsNullOrWhiteSpace(filterText))
                {
                    // Clear filter
                    _highlights?.Clear();
                    Visible = true;
                    return;
                }

                if (QueryFilterHelper.Match(filterText, Name, out QueryFilterHelper.Range[] ranges))
                {
                    // Update highlights
                    if (_highlights == null)
                    {
                        _highlights = new List<Rectangle>(ranges.Length);
                    }
                    else
                    {
                        _highlights.Clear();
                    }
                    Style style = Style.Current;
                    Font font = style.FontSmall;
                    for (int i = 0; i < ranges.Length; i++)
                    {
                        Float2 start = font?.GetCharPosition(Name, ranges[i].StartIndex, TextLayoutOptions.Default)
                            ?? Float2.Zero;
                        Float2 end = font?.GetCharPosition(Name, ranges[i].EndIndex, TextLayoutOptions.Default)
                            ?? Float2.Zero;
                        _highlights.Add(new Rectangle(start.X + 2, 0, end.X - start.X, Height));

                        if (ranges[i].StartIndex <= 0)
                        {
                            m_IsStartsWithMatch = true;
                            if (ranges[i].Length == Name.Length)
                            {
                                m_IsFullMatch = true;
                            }
                        }
                    }
                    Visible = true;
                    return;
                }

                // Hide
                _highlights?.Clear();
                Visible = false;
            }

            /// <summary>
            /// Gets the text rectangle.
            /// </summary>
            /// <param name="rect">The output rectangle.</param>
            protected virtual void GetTextRect(out Rectangle rect)
            {
                rect = new Rectangle(2, 0, Width - 4, Height);

                // Indent for drop panel items is handled by drop panel margin
                if (Parent is not DropPanel)
                {
                    rect.Location += new Float2(EditorIcons.ArrowRight12.Size.X + 2, 0);
                }
            }

            /// <inheritdoc />
            public override void Draw()
            {
                Style style = Style.Current;
                GetTextRect(out Rectangle textRect);

                base.Draw();

                // Overlay
                if (IsMouseOver || IsFocused)
                {
                    Render2D.FillRectangle(new Rectangle(Float2.Zero, Size), style.BackgroundHighlighted);
                }

                // Draw all highlights
                if (DrawHighlights && _highlights != null)
                {
                    Color color = style.ProgressNormal * 0.6f;
                    for (int i = 0; i < _highlights.Count; i++)
                    {
                        Rectangle rect = _highlights[i];
                        rect.Location += textRect.Location;
                        rect.Height = textRect.Height;
                        Render2D.FillRectangle(rect, color);
                    }
                }

                // Draw name
                Render2D.DrawText(style.FontSmall, Name, textRect, TintColor * (Enabled ? style.Foreground : style
                    .ForegroundDisabled), TextAlignment.Near, TextAlignment.Center);
            }

            /// <inheritdoc />
            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left)
                {
                    _isMouseDown = true;
                }

                return base.OnMouseDown(location, button);
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left && _isMouseDown)
                {
                    _isMouseDown = false;
                    ItemClicked?.Invoke(this);
                }

                return base.OnMouseUp(location, button);
            }

            /// <inheritdoc />
            public override void OnMouseLeave()
            {
                _isMouseDown = false;

                base.OnMouseLeave();
            }

            /// <inheritdoc />
            public override void OnGetFocus()
            {
                base.OnGetFocus();

                ItemFocused?.Invoke(this);
            }

            /// <inheritdoc />
            public override int CompareTo(Control other)
            {
                if (other is Item otherItem)
                {
                    int order = -1 * SortScore.CompareTo(otherItem.SortScore);
                    if (order == 0)
                    {
                        order = string.Compare(Name, otherItem.Name, StringComparison.Ordinal);
                    }

                    return order;
                }
                return base.CompareTo(other);
            }
        }

        private readonly SearchBox m_SearchBox;
        private readonly Panel m_ScrollPanel;
        private List<DropPanel> m_CategoryPanels;
        private bool m_WaitingForInput;
        private string m_CustomSearch;

        /// <summary>
        /// Event fired when any item in this popup menu gets clicked.
        /// </summary>
        public event Action<Item> ItemClicked;

        /// <summary>
        /// Event fired when search text in this popup menu gets changed.
        /// </summary>
        public event Action<string> TextChanged;

        /// <summary>
        /// The panel control where you should add your items.
        /// </summary>
        public readonly VerticalPanel ItemsPanel;

        /// <summary>
        /// Gets a list of panels with item categories.
        /// </summary>
        public IEnumerable<DropPanel> CategoryPanels
        {
            get
            {
                return (IEnumerable<DropPanel>)m_CategoryPanels ?? Array
                            .Empty<DropPanel>();
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ItemsListContextMenu"/> class.
        /// </summary>
        /// <param name="width">The control width.</param>
        /// <param name="height">The control height.</param>
        /// <param name="withSearch">Enables search field.</param>
        public ItemsListContextMenu(float width = 320, float height = 220, bool withSearch = true)
        {
            // Context menu dimensions
            Size = new Float2(width, height);

            if (withSearch)
            {
                // Search box
                m_SearchBox = new SearchBox(false, 1, 1)
                {
                    Parent = this,
                    Width = Width - 3,
                };
                m_SearchBox.TextChanged += OnSearchFilterChanged;
                m_SearchBox.ClearSearchButton.Clicked += () => PerformLayout();
            }

            // Panel with scrollbar
            m_ScrollPanel = new Panel(ScrollBars.Vertical)
            {
                Parent = this,
                AnchorPreset = AnchorPresets.StretchAll,
                Bounds = withSearch ? new Rectangle(0, m_SearchBox.Bottom + 1, Width, Height - m_SearchBox.Bottom
                    - 2) : new Rectangle(Float2.Zero, Size),
            };

            // Items list panel
            ItemsPanel = new VerticalPanel
            {
                Parent = m_ScrollPanel,
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                IsScrollable = true,
                Pivot = Float2.Zero,
            };
        }

        private void OnSearchFilterChanged()
        {
            if (IsLayoutLocked)
            {
                return;
            }

            LockChildrenRecursive();

            string searchText = m_SearchBox?.Text ?? m_CustomSearch;
            List<Control> items = ItemsPanel.Children;
            for (int i = 0; i < items.Count; i++)
            {
                if (items[i] is Item item)
                {
                    item.UpdateFilter(searchText);
                    item.UpdateScore();
                }
            }
            if (m_CategoryPanels != null)
            {
                for (int i = 0; i < m_CategoryPanels.Count; i++)
                {
                    DropPanel category = m_CategoryPanels[i];
                    bool anyVisible = false;
                    for (int j = 0; j < category.Children.Count; j++)
                    {
                        if (category.Children[j] is Item item2)
                        {
                            item2.UpdateFilter(searchText);
                            item2.UpdateScore();
                            anyVisible |= item2.Visible;
                        }
                    }
                    category.Visible = anyVisible;
                    if (string.IsNullOrEmpty(searchText))
                    {
                        category.Close(false);
                    }
                    else
                    {
                        category.Open(false);
                    }
                }
            }

            SortItems();

            UnlockChildrenRecursive();
            PerformLayout(true);
            m_SearchBox?.Focus();
            TextChanged?.Invoke(searchText);
        }

        /// <summary>
        /// Scrolls the scroll panel to a specific Item
        /// </summary>
        /// <param name="item">The item to scroll to.</param>
        public void ScrollViewTo(Item item)
        {
            m_ScrollPanel.ScrollViewTo(item, true);
        }

        /// <summary>
        /// Scrolls to the item and focuses it by name.
        /// </summary>
        /// <param name="itemName">The item name.</param>
        public void ScrollToAndHighlightItemByName(string itemName)
        {
            foreach (Control child in ItemsPanel.Children)
            {
                if (child is not ItemsListContextMenu.Item item)
                {
                    continue;
                }
                if (string.Equals(item.Name, itemName, StringComparison.Ordinal))
                {
                    // Highlight and scroll to item
                    item.Focus();
                    ScrollViewTo(item);
                    break;
                }
            }
        }

        /// <summary>
        /// Removes all added items.
        /// </summary>
        public void ClearItems()
        {
            ItemsPanel.DisposeChildren();
        }

        /// <summary>
        /// Sorts the items list (by item name by default).
        /// </summary>
        public void SortItems()
        {
            ItemsPanel.SortChildren();
            if (m_CategoryPanels != null)
            {
                for (int i = 0; i < m_CategoryPanels.Count; i++)
                {
                    m_CategoryPanels[i].SortChildren();
                }
            }
        }

        /// <summary>
        /// Focuses and scroll to the given item to be selected.
        /// </summary>
        /// <param name="item">The item to select.</param>
        public void SelectItem(Item item)
        {
            item.Focus();
            ScrollViewTo(item);
        }

        /// <summary>
        /// Applies custom search text query on the items list. Works even if search field is disabled
        /// </summary>
        /// <param name="text">The custom search text. Null to clear search.</param>
        public void Search(string text)
        {
            if (m_SearchBox != null)
            {
                m_SearchBox.SetText(text);
            }
            else
            {
                m_CustomSearch = text;
                if (VisibleInHierarchy)
                {
                    OnSearchFilterChanged();
                }
            }
        }

        /// <summary>
        /// Adds the item to the view and registers for the click event.
        /// </summary>
        /// <param name="item">The item.</param>
        public void AddItem(Item item)
        {
            item.ItemClicked += OnClickItem;
            ContainerControl parent = ItemsPanel;
            if (!string.IsNullOrEmpty(item.Category))
            {
                if (m_CategoryPanels == null)
                {
                    m_CategoryPanels = new List<DropPanel>();
                }
                for (int i = 0; i < m_CategoryPanels.Count; i++)
                {
                    if (string.Equals(m_CategoryPanels[i].HeaderText, item.Category, StringComparison.Ordinal))
                    {
                        parent = m_CategoryPanels[i];
                        break;
                    }
                }
                if (parent == ItemsPanel)
                {
                    DropPanel categoryPanel = new DropPanel
                    {
                        HeaderText = item.Category,
                        ArrowImageOpened = new SpriteBrush(EditorIcons.ArrowDown12),
                        ArrowImageClosed = new SpriteBrush(EditorIcons.ArrowRight12),
                        EnableDropDownIcon = true,
                        ItemsMargin = new Margin(28, 0, 2, 2),
                        HeaderColor = Style.Current.Background,
                        Parent = parent,
                    };
                    categoryPanel.Close(false);
                    m_CategoryPanels.Add(categoryPanel);
                    parent = categoryPanel;
                }
            }
            item.Parent = parent;
        }

        /// <summary>
        /// Called when user clicks on an item.
        /// </summary>
        /// <param name="item">The item.</param>
        public void OnClickItem(Item item)
        {
            Hide();
            ItemClicked?.Invoke(item);
        }

        /// <summary>
        /// Resets the view.
        /// </summary>
        public void ResetView()
        {
            LockChildrenRecursive();

            List<Control> items = ItemsPanel.Children;
            for (int i = 0; i < items.Count; i++)
            {
                if (items[i] is Item item)
                {
                    item.UpdateFilter(null);
                }
            }
            if (m_CategoryPanels != null)
            {
                for (int i = 0; i < m_CategoryPanels.Count; i++)
                {
                    DropPanel category = m_CategoryPanels[i];
                    for (int j = 0; j < category.Children.Count; j++)
                    {
                        if (category.Children[j] is Item item2)
                        {
                            item2.UpdateFilter(null);
                        }
                    }
                    category.Visible = true;
                    category.Close(false);
                }

                if (m_CategoryPanels.Count == 1 && items.Count == 1)
                {
                    // Expand the only category if there are no items outside of it
                    m_CategoryPanels[0].Open(false);
                }
            }

            m_SearchBox?.Clear();
            UnlockChildrenRecursive();
            PerformLayout(true);
            if (m_CustomSearch != null)
            {
                OnSearchFilterChanged();
            }
        }

        private List<Item> GetVisibleItems(bool ignoreFoldedCategories)
        {
            List<Item> result = new List<Item>();
            List<Control> items = ItemsPanel.Children;
            for (int i = 0; i < items.Count; i++)
            {
                Control currentItem = items[i];
                if (currentItem is Item item && item.Visible)
                {
                    result.Add(item);
                }
                else if (currentItem is DropPanel category && (!ignoreFoldedCategories || !category.IsClosed)
                    && currentItem.Visible)
                {
                    for (int j = 0; j < category.Children.Count; j++)
                    {
                        if (category.Children[j] is Item categoryItem && categoryItem.Visible)
                        {
                            result.Add(categoryItem);
                        }
                    }
                }
            }
            return result;
        }

        private void ExpandToItem(Item item)
        {
            if (item.Parent is DropPanel dropPanel)
            {
                dropPanel.Open(false);
            }
        }

        /// <inheritdoc />
        protected override void OnShow()
        {
            // Prepare
            ResetView();
            Focus();
            m_WaitingForInput = true;

            base.OnShow();
        }

        /// <inheritdoc />
        public override void Hide()
        {
            Focus(null);

            base.Hide();
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            Item focusedItem = RootWindow().GetFocusedControl() as Item;
            switch (key)
            {
                case KeyboardKeys.Escape:
                    Hide();
                    return true;
                case KeyboardKeys.Backspace:
                    // Allow the user to quickly focus the searchbar
                    if (m_SearchBox != null && !m_SearchBox.IsFocused)
                    {
                        m_SearchBox.Focus();
                        m_SearchBox.SelectAll();
                        return true;
                    }
                    break;
                case KeyboardKeys.ArrowDown:
                case KeyboardKeys.ArrowUp:
                    if (RootWindow().GetFocusedControl() == null)
                    {
                        // Focus search box if nothing is focused
                        m_SearchBox?.Focus();
                        return true;
                    }

                    // Get the next item
                    bool controlDown = Root.GetKey(KeyboardKeys.Control);
                    List<Item> items = GetVisibleItems(!controlDown);
                    if (items.Count == 0)
                    {
                        return true;
                    }
                    int focusedIndex = items.IndexOf(focusedItem);

                    int delta = key == KeyboardKeys.ArrowDown ? -1 : 1;
                    int nextIndex = Mathf.Wrap(focusedIndex - delta, 0, items.Count - 1);
                    Item nextItem = items[nextIndex];

                    // Focus the next item
                    nextItem.Focus();

                    // Allow the user to expand groups while scrolling
                    if (controlDown)
                    {
                        ExpandToItem(nextItem);
                    }

                    m_ScrollPanel.ScrollViewTo(nextItem);
                    return true;
                case KeyboardKeys.Return:
                    if (focusedItem != null)
                    {
                        OnClickItem(focusedItem);
                        return true;
                    }
                    else
                    {
                        // Select first item if no item is focused (most likely to be the best result), saves the user
                        // from pressing arrow down first
                        List<Item> visibleItems = GetVisibleItems(true);
                        if (visibleItems.Count > 0)
                        {
                            OnClickItem(visibleItems[0]);
                            return true;
                        }
                    }
                    break;
            }

            if (m_WaitingForInput && m_SearchBox != null)
            {
                m_WaitingForInput = false;
                m_SearchBox.Focus();
                return m_SearchBox.OnKeyDown(key);
            }

            return base.OnKeyDown(key);
        }
    }
}

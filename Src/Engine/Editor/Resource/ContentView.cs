using System;
using System.Collections.Generic;
using SE.GUI;

namespace SE.Editor
{
    public enum ContentViewType
    {
        /// <summary>The uniform tiles.</summary>
        Tiles,

        /// <summary>The vertical list.</summary>
        List,
    }

    /// <summary>The method sort for items.</summary>
    public enum SortType
    {
        /// <summary>The classic alphabetic sort method (A-Z).</summary>
        AlphabeticOrder,

        /// <summary>The reverse alphabetic sort method (Z-A).</summary>
        AlphabeticReverse,
    }

    public class ContentView : ContainerControl, IContentItemOwner
    {
        private readonly List<ContentItem> m_Items = new List<ContentItem>(256);
        private readonly List<ContentItem> m_Selection = new List<ContentItem>();

        private float _viewScale = 1.0f;
        private ContentViewType _viewType = ContentViewType.Tiles;
        private bool _isRubberBandSpanning;
        private Float2 _mousePressLocation;
        private Rectangle _rubberBandRectangle;

        private bool _validDragOver;

        /// <summary>Called when user wants to open the item.</summary>
        public Action<ContentItem> OnOpen;

        /// <summary>Called when user wants to rename the item.</summary>
        public Action<ContentItem> OnRename;

        /// <summary>Called when user wants to delete the item.</summary>
        public Action<List<ContentItem>> OnDelete;

        /// <summary>Called when user wants to paste the files/folders.</summary>
        public Action<List<string>> OnPaste;

        /// <summary>Called when user wants to duplicate the item(s).</summary>
        public Action<List<ContentItem>> OnDuplicate;

        /// <summary>Called when user wants to navigate backward.</summary>
        public Action OnNavigateBack;

        /// <summary>Occurs when view scale gets changed.</summary>
        public Action ViewScaleChanged;

        /// <summary>Occurs when view type gets changed.</summary>
        public Action ViewTypeChanged;

        /// <summary>Gets the items.</summary>
        public List<ContentItem> GetItems()
        {
            return m_Items;
        }

        /// <summary>Gets the items count.</summary>
        public int ItemsCount => m_Items.Count;

        /// <summary>Gets the selected items.</summary>
        public List<ContentItem> GetSelection()
        {
            return m_Selection;
        }

        /// <summary>Gets the selected count.</summary>
        public int SelectedCount => m_Selection.Count;

        /// <summary>Gets a value indicating whether any item is selected.</summary>
        public bool HasSelection => m_Selection.Count > 0;

        /// <summary>Gets or sets the view scale.</summary>
        public float ViewScale
        {
            get => _viewScale;
            set
            {
                value = Math.Clamp(value, 0.3f, 3.0f);
                if (!Mathf.NearEqual(value, _viewScale))
                {
                    _viewScale = value;
                    ViewScaleChanged?.Invoke();
                    PerformLayout();
                }
            }
        }

        /// <summary>Gets or sets the type of the view.</summary>
        public ContentViewType ViewType
        {
            get => _viewType;
            set
            {
                if (_viewType != value)
                {
                    _viewType = value;
                    ViewTypeChanged?.Invoke();
                    PerformLayout();
                }
            }
        }

        /// <summary>Flag is used to indicate if user is searching for items.</summary>
        public bool IsSearching;

        /// <summary>Flag used to indicate whenever show full file names including extensions.</summary>
        public bool ShowFileExtensions;

        /// <summary>Initializes a new instance of the <see cref="ContentView"/> class.</summary>
        public ContentView()
        {
            // Native input actions collection and its bindings are commented out.
        }

        /// <summary>Clears the items in the view.</summary>
        public void ClearItems()
        {
            bool wasLayoutLocked = IsLayoutLocked;
            IsLayoutLocked = true;

            ClearSelection();

            for (int i = 0; i < m_Items.Count; i++)
            {
                ContentItem item = m_Items[i];
                item.Parent = null;
                item.RemoveReference(this);
            }
            m_Items.Clear();

            IsLayoutLocked = wasLayoutLocked;
            PerformLayout();
        }

        /// <summary>Shows the items collection in the view.</summary>
        /// <param name="items">The items to show.</param>
        /// <param name="sortType">The sort method for items.</param>
        /// <param name="additive">If true, items are added to the current list; otherwise, the items list is cleared first.</param>
        /// <param name="keepSelection">If true, selected items are preserved; otherwise, the selection is cleared first.</param>
        public void ShowItems(List<ContentItem> items, SortType sortType, bool additive = false, bool keepSelection = false)
        {
            if (items.Count == 0)
            {
                if (!additive)
                {
                    ClearItems();
                }
                return;
            }

            bool wasLayoutLocked = IsLayoutLocked;
            IsLayoutLocked = true;
            List<ContentItem> selection = !additive && keepSelection
                ? new List<ContentItem>(m_Selection)
                : new List<ContentItem>();

            if (!additive)
            {
                ClearItems();
            }

            for (int i = 0; i < items.Count; i++)
            {
                ContentItem item = items[i];
                if (item.Visible && !m_Items.Contains(item))
                {
                    item.Parent = this;
                    item.AddReference(this);
                    m_Items.Add(item);
                }
            }
            if (selection.Count > 0)
            {
                m_Selection.Clear();
                m_Selection.AddRange(selection);
            }

            QuickSortChildren(sortType);

            IsLayoutLocked = wasLayoutLocked;
            PerformLayout();
        }

        /// <summary>Determines whether the specified item is selected.</summary>
        /// <param name="item">The item.</param>
        /// <returns>True if the specified item is selected; otherwise, false.</returns>
        public bool IsSelected(ContentItem item)
        {
            return m_Selection.Contains(item);
        }

        /// <summary>Clears the selected items collection.</summary>
        public void ClearSelection()
        {
            if (m_Selection.Count == 0)
            {
                return;
            }

            m_Selection.Clear();
        }

        /// <summary>Selects the specified items.</summary>
        /// <param name="items">The items.</param>
        /// <param name="additive">If true, items are added to the current selection; otherwise, the selection is cleared first.</param>
        public void Select(List<ContentItem> items, bool additive = false)
        {
            if (items.Count == 0)
            {
                if (!additive)
                {
                    ClearSelection();
                }
                return;
            }

            bool wasLayoutLocked = IsLayoutLocked;
            IsLayoutLocked = true;

            if (additive)
            {
                for (int i = 0; i < items.Count; i++)
                {
                    if (!m_Selection.Contains(items[i]))
                    {
                        m_Selection.Add(items[i]);
                    }
                }
            }
            else
            {
                m_Selection.Clear();
                m_Selection.AddRange(items);
            }

            IsLayoutLocked = wasLayoutLocked;
            PerformLayout();
        }

        /// <summary>Selects the specified item.</summary>
        /// <param name="item">The item.</param>
        /// <param name="additive">If true, the item is added to the current selection; otherwise, the selection is cleared first.</param>
        public void Select(ContentItem item, bool additive = false)
        {
            bool wasLayoutLocked = IsLayoutLocked;
            IsLayoutLocked = true;

            if (additive)
            {
                if (!m_Selection.Contains(item))
                {
                    m_Selection.Add(item);
                }
            }
            else
            {
                m_Selection.Clear();
                m_Selection.Add(item);
            }

            IsLayoutLocked = wasLayoutLocked;
            PerformLayout();
        }

        /// <summary>Selects all the items.</summary>
        public void SelectAll()
        {
            BulkSelectUpdate(true);
        }

        /// <summary>Deselects all the items.</summary>
        public void DeselectAll()
        {
            BulkSelectUpdate(false);
        }

        /// <summary>Deselects the specified item.</summary>
        /// <param name="item">The item.</param>
        public void Deselect(ContentItem item)
        {
            SE.Debug.Assert(item == null, "Native assertion condition in ContentView.Deselect.");

            bool wasLayoutLocked = IsLayoutLocked;
            IsLayoutLocked = true;

            if (m_Selection.Contains(item))
            {
                m_Selection.Remove(item);
            }

            IsLayoutLocked = wasLayoutLocked;
            PerformLayout();
        }

        /// <summary>Duplicates the selected items.</summary>
        public void Duplicate()
        {
            OnDuplicate?.Invoke(m_Selection);
        }

        /// <summary>Copies the selected items to the system clipboard.</summary>
        public void Copy()
        {
            if (m_Selection.Count == 0)
            {
                return;
            }

            List<string> files = new List<string>();
            foreach (ContentItem item in m_Selection)
            {
                files.Add(item.Path);
            }
            Clipboard.Files = files.ToArray();
        }

        /// <summary>Returns true if user can paste copied files.</summary>
        /// <returns>True if files can be pasted.</returns>
        public bool CanPaste()
        {
            string[] files = Clipboard.Files;
            return files.Length > 0;
        }

        /// <summary>Pastes copied items from the system clipboard.</summary>
        public void Paste()
        {
            List<string> files = new List<string>(Clipboard.Files);
            if (files.Count == 0)
            {
                return;
            }

            OnPaste?.Invoke(files);
        }

        /// <summary>Gives focus and selects the first item in the view.</summary>
        public void SelectFirstItem()
        {
            if (m_Items.Count > 0)
            {
                m_Items[0].Focus();
                Select(m_Items[0]);
            }
            else
            {
                Focus();
            }
        }

        /// <summary>Refreshes thumbnails of all items in the view.</summary>
        public void RefreshThumbnails()
        {
            for (int i = 0; i < m_Items.Count; i++)
            {
                m_Items[i].RefreshThumbnail();
            }
        }

        /// <summary>Called when user clicks on an item.</summary>
        /// <param name="item">The item.</param>
        public void OnItemClick(ContentItem item)
        {
            bool isSelected = m_Selection.Contains(item);

            if (Root.GetKey(KeyboardKeys.Control))
            {
                if (isSelected)
                {
                    Deselect(item);
                }
                else
                {
                    Select(item, true);
                }
            }
            else if (m_Selection.Count != 0 && Root.GetKey(KeyboardKeys.Shift))
            {
                int min = int.MaxValue;
                int max = int.MinValue;
                foreach (ContentItem itemTemp in m_Selection)
                {
                    min = Math.Min(min, itemTemp.IndexInParent);
                    max = Math.Max(max, itemTemp.IndexInParent);
                }

                min = Math.Max(Math.Min(min, item.IndexInParent), 0);
                max = Math.Min(Math.Max(max, item.IndexInParent), m_Children.Count - 1);
                List<ContentItem> selection = new List<ContentItem>(m_Selection);
                for (int i = min; i <= max; i++)
                {
                    ContentItem child = m_Children[i] as ContentItem;
                    if (child != null && !selection.Contains(child))
                    {
                        selection.Add(child);
                    }
                }
                Select(selection);
            }
            else
            {
                Select(item);
            }
        }

        /// <summary>Called when user wants to open an item.</summary>
        /// <param name="item">The item.</param>
        public void OnItemDoubleClick(ContentItem item)
        {
            OnOpen?.Invoke(item);
        }

        /// <inheritdoc />
        public void OnItemDeleted(ContentItem item)
        {
            RemoveItem(item);
        }

        /// <inheritdoc />
        public void OnItemRenamed(ContentItem item)
        {
        }

        /// <inheritdoc />
        public void OnItemReimported(ContentItem item)
        {
        }

        /// <inheritdoc />
        public void OnItemDispose(ContentItem item)
        {
            RemoveItem(item);
        }

        private void RemoveItem(ContentItem item)
        {
            m_Selection.Remove(item);
            m_Items.Remove(item);
            if (item.Parent == this)
            {
                item.Parent = null;
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            Style style = Style.Current;
            if (IsDragOver() && _validDragOver)
            {
                Rectangle bounds = new Rectangle(Float2.One, Float2.One * 2.0f - Size);
                Render2D.FillRectangle(bounds, style.Selection);
                Render2D.DrawRectangle(bounds, style.SelectionBorder, 1.0f);
            }

            if (m_Items.Count == 0)
            {
                Render2D.RenderText(style.FontSmall, IsSearching ? "No results" : "Empty",
                    new Rectangle(Float2.Zero, Size), style.ForegroundDisabled,
                    TextAlignment.Center, TextAlignment.Center);
            }

            if (_isRubberBandSpanning)
            {
                Render2D.FillRectangle(_rubberBandRectangle, style.Selection);
                Render2D.DrawRectangle(_rubberBandRectangle, style.SelectionBorder, 1.0f);
            }
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
            {
                return true;
            }

            if (button == MouseButton.Left)
            {
                _mousePressLocation = location;
                _rubberBandRectangle = new Rectangle(_mousePressLocation.X, _mousePressLocation.Y, 0.0f, 0.0f);
                _isRubberBandSpanning = true;
                StartMouseCapture();
                return true;
            }

            return AutoFocus && Focus(this);
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            if (_isRubberBandSpanning)
            {
                _rubberBandRectangle.Width = location.X - _mousePressLocation.X;
                _rubberBandRectangle.Height = location.Y - _mousePressLocation.Y;
            }

            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (_isRubberBandSpanning)
            {
                _isRubberBandSpanning = false;
                EndMouseCapture();
                if (_rubberBandRectangle.Width < 0 || _rubberBandRectangle.Height < 0)
                {
                    Float2 size = _rubberBandRectangle.Size;
                    _rubberBandRectangle.X = Math.Min(_rubberBandRectangle.X,
                        _rubberBandRectangle.X + _rubberBandRectangle.Width);
                    _rubberBandRectangle.Y = Math.Min(_rubberBandRectangle.Y,
                        _rubberBandRectangle.Y + _rubberBandRectangle.Height);
                    size.X = Math.Abs(size.X);
                    size.Y = Math.Abs(size.Y);
                    _rubberBandRectangle.Size = size;
                }

                List<ContentItem> itemsInRectangle = new List<ContentItem>();
                for (int i = 0; i < m_Items.Count; i++)
                {
                    ContentItem item = m_Items[i];
                    if (_rubberBandRectangle.Intersects(item.Bounds))
                    {
                        itemsInRectangle.Add(item);
                    }
                }

                Select(itemsInRectangle, Input.GetKey(KeyboardKeys.Shift) || Input.GetKey(KeyboardKeys.Control));
                return true;
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseWheel(Float2 location, float delta)
        {
            if (Root.GetKey(KeyboardKeys.Control))
            {
                ViewScale = ViewScale + delta * 0.05f;
                return true;
            }

            return base.OnMouseWheel(location, delta);
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (key == KeyboardKeys.Backspace)
            {
                OnNavigateBack?.Invoke();
                return true;
            }

            // Native InputActions.Process branch is commented out.

            if (HasSelection)
            {
                if (key == KeyboardKeys.Return && m_Selection.Count != 0)
                {
                    foreach (ContentItem selectedItem in m_Selection)
                    {
                        OnOpen?.Invoke(selectedItem);
                    }
                    return true;
                }

                ContentItem root = m_Selection[0];
                Float2 size = root.Size;
                Float2 offset = Float2.Minimum;
                ContentItem item = null;
                if (key == KeyboardKeys.ArrowUp)
                {
                    offset = new Float2(0, -size.Y);
                }
                else if (key == KeyboardKeys.ArrowDown)
                {
                    offset = new Float2(0, size.Y);
                }
                else if (key == KeyboardKeys.ArrowRight)
                {
                    offset = new Float2(size.X, 0);
                }
                else if (key == KeyboardKeys.ArrowLeft)
                {
                    offset = new Float2(-size.X, 0);
                }
                if (offset != Float2.Minimum)
                {
                    item = GetChildAt(size / 2.0f + offset + root.Location) as ContentItem;
                }
                if (item != null)
                {
                    OnItemClick(item);
                    return true;
                }
            }

            return base.OnKeyDown(key);
        }

        /// <inheritdoc />
        public override bool OnCharInput(char c)
        {
            if (base.OnCharInput(c))
            {
                return true;
            }

            if (char.IsLetterOrDigit(c) && m_Items.Count != 0)
            {
                c = char.ToLowerInvariant(c);
                for (int i = 0; i < m_Items.Count; i++)
                {
                    ContentItem item = m_Items[i];
                    string name = item.ShortName;
                    if (name.Length > 0 && char.ToLowerInvariant(name[0]) == c)
                    {
                        Select(item);
                        Panel panel = Parent as Panel;
                        if (panel != null)
                        {
                            panel.ScrollViewTo(item, true);
                        }
                        break;
                    }
                }
            }

            return false;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            DragDropEffect result = base.OnDragEnter(ref location, data);
            if (result != DragDropEffect.None)
            {
                return result;
            }

            if (data is DragDataFiles)
            {
                _validDragOver = true;
                return DragDropEffect.Copy;
            }

            // Native actor drag branch is commented out.
            return DragDropEffect.None;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            _validDragOver = false;
            DragDropEffect result = base.OnDragMove(ref location, data);
            if (result != DragDropEffect.None)
            {
                return result;
            }

            if (data is DragDataFiles)
            {
                _validDragOver = true;
                result = DragDropEffect.Copy;
            }
            // Native actor drag branch is commented out.

            return result;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
        {
            DragDropEffect result = base.OnDragDrop(ref location, data);
            if (result != DragDropEffect.None)
            {
                return result;
            }

            if (data is DragDataFiles files)
            {
                Editor editor = Editor.Instance;
                // Import files
                ContentFolder currentFolder = editor.Windows.ContentWin.CurrentViewFolder;

                if (currentFolder != null)
                {
                    editor.Importing.Import(files.Files, currentFolder);
                }
                result = DragDropEffect.Copy;
            }

            _validDragOver = false;
            return result;
        }

        /// <inheritdoc />
        public override void OnDragLeave()
        {
            _validDragOver = false;
            // Native actor drag leave branch is commented out.
            base.OnDragLeave();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (IsDisposing)
            {
                return;
            }

            ClearItems();
            base.OnDestroy();
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            float width = GetClientArea().Width;
            float x = 0.0f;
            float y = 1.0f;
            float viewScale = _viewScale * 0.97f;

            switch (ViewType)
            {
                case ContentViewType.Tiles:
                {
                    float defaultItemsWidth = ContentItem.DefaultWidth * viewScale;
                    int itemsToFit = (int)MathF.Floor(width / defaultItemsWidth) - 1;
                    if (itemsToFit < 1)
                    {
                        itemsToFit = 1;
                    }
                    int xSpace = 4;
                    float itemsWidth = width / Math.Max(itemsToFit, 1) - xSpace;
                    float itemsHeight = itemsWidth / defaultItemsWidth * (ContentItem.DefaultHeight * viewScale);
                    float flooredItemsWidth = MathF.Floor(itemsWidth);
                    float flooredItemsHeight = MathF.Floor(itemsHeight);
                    x = itemsToFit == 1 ? 1.0f : itemsWidth / itemsToFit + xSpace;
                    for (int i = 0; i < m_Children.Count; i++)
                    {
                        Control child = m_Children[i];
                        child.Bounds = new Rectangle(MathF.Floor(x), MathF.Floor(y),
                            flooredItemsWidth, flooredItemsHeight);

                        x += (itemsWidth + xSpace) + (itemsWidth + xSpace) / itemsToFit;
                        if (x + itemsWidth > width)
                        {
                            x = itemsToFit == 1 ? 1.0f : itemsWidth / itemsToFit + xSpace;
                            y += itemsHeight + 7.0f;
                        }
                    }
                    if (x > 0.0f)
                    {
                        y += itemsHeight;
                    }
                    break;
                }
                case ContentViewType.List:
                {
                    float itemsHeight = 50.0f * viewScale;
                    for (int i = 0; i < m_Children.Count; i++)
                    {
                        Control child = m_Children[i];
                        child.Bounds = new Rectangle(x, y, width, itemsHeight);
                        y += itemsHeight + 1.0f;
                    }
                    y += 40.0f;
                    break;
                }
            }

            if (HasParent)
            {
                y = Math.Max(y, Parent.Height);
            }
            Height = y;

            base.PerformLayoutBeforeChildren();
        }

        private void BulkSelectUpdate(bool select = true)
        {
            bool wasLayoutLocked = IsLayoutLocked;
            IsLayoutLocked = true;

            m_Selection.Clear();
            if (select)
            {
                m_Selection.AddRange(m_Items);
            }

            IsLayoutLocked = wasLayoutLocked;
            PerformLayout();
        }

        private void QuickSortChildren(SortType sortType)
        {
            if (m_Children.Count < 2)
            {
                return;
            }

            Stack<int> stack = new Stack<int>();
            stack.Push(0);
            stack.Push(m_Children.Count - 1);

            while (stack.Count > 0)
            {
                int right = stack.Pop();
                int left = stack.Pop();
                Control pivotControl = m_Children[right];
                int i = left - 1;
                for (int j = left; j <= right - 1; j++)
                {
                    if (CompareForSort(m_Children[j], pivotControl, sortType))
                    {
                        i++;
                        (m_Children[i], m_Children[j]) = (m_Children[j], m_Children[i]);
                    }
                }

                (m_Children[i + 1], m_Children[right]) = (m_Children[right], m_Children[i + 1]);
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

        private static bool CompareForSort(Control control, Control other, SortType sortType)
        {
            if (control == null || other == null)
            {
                return false;
            }
            if (sortType == SortType.AlphabeticReverse)
            {
                if (control.CompareTo(other) > 0)
                {
                    return false;
                }
                if (control.CompareTo(other) == 0)
                {
                    return false;
                }
                return true;
            }
            return control.CompareTo(other) > 0;
        }
    }
}

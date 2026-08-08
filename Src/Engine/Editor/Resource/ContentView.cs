using System;
using System.Collections.Generic;
using System.Linq;
using SE.GUI;

namespace SE.Editor
{
    public enum ContentViewType
    {
        Tiles,
        List,
    }

    public enum ContentSortType
    {
        Name,
        Type,
        Path,
    }

    /// <summary>
    /// Managed content-browser surface. Selection and layout are kept entirely in
    /// C#, so native resource changes are represented by stable ContentItem values.
    /// </summary>
    public sealed class ContentView : ContainerControl, IContentItemOwner
    {
        private readonly List<ContentItem> m_Items = new();
        private readonly HashSet<ContentItem> m_Selection = new();
        private ContentViewType m_ViewType = ContentViewType.Tiles;
        private float m_ViewScale = 1.0f;

        public ContentView()
            : base(new Rectangle(0, 0, 400, 300))
        {
        }

        public IReadOnlyList<ContentItem> Items => m_Items;
        public IReadOnlyCollection<ContentItem> Selection => m_Selection;
        public int SelectedCount => m_Selection.Count;
        public bool HasSelection => m_Selection.Count != 0;
        public bool IsSearching { get; set; }
        public bool ShowFileExtensions { get; set; }
        public ContentSortType SortType { get; private set; } = ContentSortType.Name;

        public ContentViewType ViewType
        {
            get => m_ViewType;
            set
            {
                if (m_ViewType == value)
                    return;
                m_ViewType = value;
                PerformLayout();
            }
        }

        public float ViewScale
        {
            get => m_ViewScale;
            set
            {
                float clamped = Math.Clamp(value, 0.5f, 2.0f);
                if (Math.Abs(m_ViewScale - clamped) < float.Epsilon)
                    return;
                m_ViewScale = clamped;
                PerformLayout();
            }
        }

        public event Action<IReadOnlyCollection<ContentItem>>? SelectionChanged;
        public event Action<ContentItem>? ItemActivated;
        public event Action? DuplicateRequested;
        public event Action? CopyRequested;
        public event Action? PasteRequested;

        public void ClearItems()
        {
            foreach (ContentItem item in m_Items)
            {
                item.RemoveReference(this);
                RemoveChild(item);
            }

            m_Items.Clear();
            ClearSelection();
        }

        public void ShowItems(IEnumerable<ContentItem> items, ContentSortType sortType = ContentSortType.Name, bool additive = false, bool keepSelection = false)
        {
            ArgumentNullException.ThrowIfNull(items);
            HashSet<ContentItem> previousSelection = new(m_Selection);
            if (!additive)
                ClearItems();

            foreach (ContentItem item in items)
            {
                if (m_Items.Contains(item))
                    continue;

                item.ShowFileExtension = ShowFileExtensions;
                item.AddReference(this);
                m_Items.Add(item);
                AddChild(item);
            }

            SortType = sortType;
            SortItems();
            if (keepSelection)
                Select(previousSelection, additive: false);
            PerformLayout();
        }

        public bool IsSelected(ContentItem item) => m_Selection.Contains(item);

        public void ClearSelection()
        {
            if (m_Selection.Count == 0)
                return;

            m_Selection.Clear();
            SelectionChanged?.Invoke(Selection);
        }

        public void Select(IEnumerable<ContentItem> items, bool additive = false)
        {
            ArgumentNullException.ThrowIfNull(items);
            if (!additive)
                m_Selection.Clear();

            foreach (ContentItem item in items)
            {
                if (m_Items.Contains(item))
                    m_Selection.Add(item);
            }

            SelectionChanged?.Invoke(Selection);
        }

        public void Select(ContentItem item, bool additive = false)
        {
            ArgumentNullException.ThrowIfNull(item);
            Select(new[] { item }, additive);
        }

        public void SelectAll() => Select(m_Items);

        public void DeselectAll() => ClearSelection();

        public void Deselect(ContentItem item)
        {
            if (m_Selection.Remove(item))
                SelectionChanged?.Invoke(Selection);
        }

        public void Duplicate() => DuplicateRequested?.Invoke();
        public void Copy() => CopyRequested?.Invoke();
        public bool CanPaste => PasteRequested != null;
        public void Paste() => PasteRequested?.Invoke();

        public void SelectFirstItem()
        {
            if (m_Items.Count != 0)
                Select(m_Items[0]);
        }

        public void RefreshThumbnails()
        {
            foreach (ContentItem item in m_Items)
            {
                item.RefreshThumbnail();
            }
        }

        public void OnItemClick(ContentItem item, bool additive = false)
        {
            Select(item, additive);
        }

        public void OnItemDoubleClick(ContentItem item)
        {
            ItemActivated?.Invoke(item);
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
                ClearSelection();
            return base.OnMouseDown(location, button);
        }

        public override bool OnMouseWheel(Float2 location, float delta)
        {
            if (delta == 0)
                return false;
            ViewScale += delta > 0 ? 0.1f : -0.1f;
            return true;
        }

        public override bool OnKeyDown(KeyboardKeys key)
        {
            // The root input bridge normalizes keys to virtual-key values.
            if (key == KeyboardKeys.A)
            {
                SelectAll();
                return true;
            }

            return base.OnKeyDown(key);
        }

        public void OnItemDeleted(ContentItem item)
        {
            RemoveItem(item);
        }

        public void OnItemRenamed(ContentItem item)
        {
            if (m_Items.Contains(item))
            {
                SortItems();
                PerformLayout();
            }
        }

        public void OnItemReimported(ContentItem item)
        {
            if (m_Items.Contains(item))
                item.RefreshThumbnail();
        }

        public void OnItemDispose(ContentItem item)
        {
            RemoveItem(item);
        }

        protected override void OnLayoutChildren()
        {
            if (m_ViewType == ContentViewType.List)
            {
                float y = 0;
                foreach (ContentItem item in m_Items)
                {
                    item.SetBounds(0, y, Width, 26.0f);
                    y += item.Height;
                }
                return;
            }

            float itemWidth = ContentItem.DefaultWidth * m_ViewScale;
            float itemHeight = ContentItem.DefaultHeight * m_ViewScale;
            int columns = Math.Max(1, (int)MathF.Floor(Math.Max(Width, itemWidth) / itemWidth));
            for (int index = 0; index < m_Items.Count; index++)
            {
                int column = index % columns;
                int row = index / columns;
                m_Items[index].SetBounds(column * itemWidth, row * itemHeight, itemWidth, itemHeight);
            }
        }

        protected override void OnDispose()
        {
            ClearItems();
            base.OnDispose();
        }

        private void RemoveItem(ContentItem item)
        {
            if (!m_Items.Remove(item))
                return;

            item.RemoveReference(this);
            RemoveChild(item);
            m_Selection.Remove(item);
            SelectionChanged?.Invoke(Selection);
            PerformLayout();
        }

        private void SortItems()
        {
            m_Items.Sort(SortType switch
            {
                ContentSortType.Type => static (left, right) => left.ItemType.CompareTo(right.ItemType),
                ContentSortType.Path => static (left, right) => string.Compare(left.Path, right.Path, StringComparison.OrdinalIgnoreCase),
                _ => static (left, right) => string.Compare(left.DisplayName, right.DisplayName, StringComparison.OrdinalIgnoreCase),
            });

            for (int index = 0; index < m_Items.Count; index++)
                m_Items[index].IndexInParent = index;
        }
    }
}

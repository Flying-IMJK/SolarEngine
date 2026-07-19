// Managed editor GUI feature implementation.
using System;
using System.Collections.Generic;
using SE.GUI;

namespace SE.Editor.GUI
{
    public class ComboBox : Control
    {
        public const float DefaultHeight = 18.0f;

        private readonly List<string> _items = new List<string>();
        private readonly List<string> _tooltips = new List<string>();
        private readonly List<int> _selectedIndices = new List<int>(4);
        private ContextMenu? _popupMenu;
        private bool _isPressed;

        public ComboBox(float x, float y, float width = 120.0f)
            : base(new Rectangle(x, y, width, DefaultHeight))
        {
            MaximumItemsInViewCount = 20;
            BackgroundColor = Style.Current.BackgroundNormal;
        }

        public event Action<ComboBox>? SelectedIndexChanged;
        public event Action<ComboBox>? PopupShowing;

        public IList<string> Items => _items;
        public IList<string> Tooltips => _tooltips;
        public bool Sorted { get; set; }
        public bool SupportMultiSelect { get; set; }
        public int MaximumItemsInViewCount { get; set; }
        public Func<ComboBox, ContextMenu>? PopupCreate { get; set; }
        public ContextMenu? Popup => _popupMenu;
        public bool IsPopupOpened => _popupMenu?.IsOpened == true;
        public bool HasSelection => _selectedIndices.Count != 0;
        public string Text { get; private set; } = string.Empty;

        public string SelectedItem
        {
            get => _selectedIndices.Count == 1 ? _items[_selectedIndices[0]] : string.Empty;
            set => SelectedIndex = _items.IndexOf(value);
        }

        public int SelectedIndex
        {
            get => _selectedIndices.Count == 1 ? _selectedIndices[0] : -1;
            set
            {
                int clamped = _items.Count == 0 ? -1 : Math.Min(Math.Max(value, -1), _items.Count - 1);
                if (SelectedIndex == clamped)
                    return;

                _selectedIndices.Clear();
                if (clamped != -1)
                    _selectedIndices.Add(clamped);
                OnSelectedIndexChanged();
            }
        }

        public IReadOnlyList<int> Selection => _selectedIndices;

        public void SetSelection(IEnumerable<int> selection)
        {
            List<int> next = new List<int>();
            foreach (int index in selection)
            {
                if (index < 0 || index >= _items.Count)
                    throw new ArgumentOutOfRangeException(nameof(selection), "Selection index is outside the item range.");
                next.Add(index);
            }

            if (!SupportMultiSelect && next.Count > 1)
                throw new InvalidOperationException("ComboBox does not support multiple selected items.");

            if (SequenceEqual(_selectedIndices, next))
                return;

            _selectedIndices.Clear();
            _selectedIndices.AddRange(next);
            OnSelectedIndexChanged();
        }

        public void ClearItems()
        {
            SelectedIndex = -1;
            _items.Clear();
            RefreshText();
        }

        public void AddItem(string item)
        {
            _items.Add(item);
            RefreshText();
        }

        public void AddItems(IEnumerable<string> items)
        {
            _items.AddRange(items);
            RefreshText();
        }

        public void SetItems(IEnumerable<string> items)
        {
            SelectedIndex = -1;
            _items.Clear();
            _items.AddRange(items);
            RefreshText();
        }

        public bool IsSelected(string item)
        {
            return IsSelected(_items.IndexOf(item));
        }

        public bool IsSelected(int index)
        {
            return index >= 0 && _selectedIndices.Contains(index);
        }

        public override bool OnMouseDown(Float2 location, int button)
        {
            if (button != 1)
                return false;

            _isPressed = true;
            Root?.StartTrackingMouse(this);
            return true;
        }

        public override bool OnMouseUp(Float2 location, int button)
        {
            if (button != 1 || !_isPressed)
                return false;

            _isPressed = false;
            Root?.EndTrackingMouse();
            if (location.X >= 0.0f && location.Y >= 0.0f && location.X <= Width && location.Y <= Height)
                ShowPopup();
            return true;
        }

        public override void ClearState()
        {
            _isPressed = false;
            base.ClearState();
        }

        protected override void OnDraw()
        {
            base.OnDraw();

            Font? font = Style.Current.FontMedium;
            if (ReferenceEquals(font, null) || string.IsNullOrEmpty(Text))
                return;

            Rectangle bounds = ScreenBounds;
            Color color = EnabledInHierarchy ? Style.Current.Foreground : Style.Current.ForegroundDisabled;
            Render2D.RenderText(font, Text, ref bounds, ref color, TextAlignment.Near, TextAlignment.Center, TextWrapping.NoWrap);
        }

        public void ShowPopup()
        {
            _popupMenu ??= OnCreatePopup();
            _popupMenu.MaximumItemsInViewCount = MaximumItemsInViewCount;

            if (_popupMenu.IsOpened)
            {
                if (!SupportMultiSelect)
                    _popupMenu.Hide();
                return;
            }

            PopupShowing?.Invoke(this);
            if (_items.Count == 0)
                return;

            UpdateButtons();
            _popupMenu.MinimumWidth = Width;
            _popupMenu.Show(this, 1.0f, Height);
        }

        protected virtual void OnSelectedIndexChanged()
        {
            RefreshText();
            SelectedIndexChanged?.Invoke(this);
        }

        protected virtual void OnItemClicked(int index)
        {
            if (SupportMultiSelect)
            {
                if (_selectedIndices.Contains(index))
                    _selectedIndices.Remove(index);
                else
                    _selectedIndices.Add(index);
                OnSelectedIndexChanged();
                UpdateButtons();
            }
            else
            {
                SelectedIndex = index;
                _popupMenu?.Hide();
            }
        }

        protected virtual void OnLayoutMenuButton(ContextMenuButton button, int index, bool construct)
        {
            button.SetChecked(_selectedIndices.Contains(index));
            if (_tooltips.Count > index)
                button.Tag = _tooltips[index];
        }

        protected virtual ContextMenu OnCreatePopup()
        {
            return PopupCreate != null ? PopupCreate(this) : new ContextMenu();
        }

        private void UpdateButtons()
        {
            if (_popupMenu == null)
                return;

            _popupMenu.DisposeAllItems();
            if (Sorted)
                _items.Sort(StringComparer.OrdinalIgnoreCase);

            for (int i = 0; i < _items.Count; i++)
            {
                int itemIndex = i;
                ContextMenuButton button = _popupMenu.AddButton(_items[i], _ => OnItemClicked(itemIndex));
                button.Tag = itemIndex;
                OnLayoutMenuButton(button, itemIndex, true);
            }
        }

        private void RefreshText()
        {
            Text = _selectedIndices.Count switch
            {
                0 => string.Empty,
                1 when _selectedIndices[0] >= 0 && _selectedIndices[0] < _items.Count => _items[_selectedIndices[0]],
                1 => string.Empty,
                _ => "Multiple Values",
            };
        }

        private static bool SequenceEqual(IReadOnlyList<int> left, IReadOnlyList<int> right)
        {
            if (left.Count != right.Count)
                return false;

            for (int i = 0; i < left.Count; i++)
            {
                if (left[i] != right[i])
                    return false;
            }

            return true;
        }
    }
}

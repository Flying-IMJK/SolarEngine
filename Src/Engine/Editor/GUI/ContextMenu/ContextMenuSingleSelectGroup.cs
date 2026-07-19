using System;
using System.Collections.Generic;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Synchronizes a single selection across one or more context menus.
    /// </summary>
    public sealed class ContextMenuSingleSelectGroup<T>
    {
        private sealed class Item
        {
            public required string Text { get; init; }
            public required T Value { get; init; }
            public string? Tooltip { get; init; }
            public Action? Selected { get; init; }
            public List<ContextMenuButton> Buttons { get; } = new();
        }

        private readonly List<ContextMenu> _menus = new();
        private readonly List<Item> _items = new();
        private Item? _selected;

        public event Action<T>? SelectedChanged;
        public bool HasSelection => _selected != null;

        public T Selected
        {
            get => _selected == null ? default! : _selected.Value;
            set
            {
                Item? item = _items.Find(candidate => EqualityComparer<T>.Default.Equals(candidate.Value, value));
                if (item != null)
                    SetSelected(item);
            }
        }

        public ContextMenuSingleSelectGroup<T> AddItem(string text, T value, Action? selected = null, string? tooltip = null)
        {
            ArgumentNullException.ThrowIfNull(text);
            var item = new Item { Text = text, Value = value, Selected = selected, Tooltip = tooltip };
            _items.Add(item);
            for (int index = 0; index < _menus.Count; index++)
                AddItemToMenu(_menus[index], item);
            return this;
        }

        public ContextMenuSingleSelectGroup<T> AddItemsToContextMenu(ContextMenu menu)
        {
            ArgumentNullException.ThrowIfNull(menu);
            _menus.Add(menu);
            for (int index = 0; index < _items.Count; index++)
                AddItemToMenu(menu, _items[index]);
            return this;
        }

        private void AddItemToMenu(ContextMenu menu, Item item)
        {
            ContextMenuButton button = menu.AddButton(item.Text, _ => SetSelected(item));
            button.Tag = item.Tooltip;
            button.SetChecked(ReferenceEquals(_selected, item));
            item.Buttons.Add(button);
        }

        private void SetSelected(Item item)
        {
            if (ReferenceEquals(_selected, item))
                return;

            _selected = item;
            for (int index = 0; index < _items.Count; index++)
            {
                List<ContextMenuButton> buttons = _items[index].Buttons;
                bool checkedValue = ReferenceEquals(_items[index], item);
                for (int buttonIndex = 0; buttonIndex < buttons.Count; buttonIndex++)
                    buttons[buttonIndex].SetChecked(checkedValue);
            }
            SelectedChanged?.Invoke(item.Value);
            item.Selected?.Invoke();
        }
    }
}

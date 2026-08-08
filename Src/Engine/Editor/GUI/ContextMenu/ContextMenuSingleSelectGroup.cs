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

        private readonly List<ContextMenu> m_Menus = new();
        private readonly List<Item> m_Items = new();
        private Item? m_Selected;

        public event Action<T>? SelectedChanged;
        public bool HasSelection => m_Selected != null;

        public T Selected
        {
            get => m_Selected == null ? default! : m_Selected.Value;
            set
            {
                Item? item = m_Items.Find(candidate => EqualityComparer<T>.Default.Equals(candidate.Value, value));
                if (item != null)
                    SetSelected(item);
            }
        }

        public ContextMenuSingleSelectGroup<T> AddItem(string text, T value, Action? selected = null, string? tooltip = null)
        {
            ArgumentNullException.ThrowIfNull(text);
            var item = new Item { Text = text, Value = value, Selected = selected, Tooltip = tooltip };
            m_Items.Add(item);
            for (int index = 0; index < m_Menus.Count; index++)
                AddItemToMenu(m_Menus[index], item);
            return this;
        }

        public ContextMenuSingleSelectGroup<T> AddItemsToContextMenu(ContextMenu menu)
        {
            ArgumentNullException.ThrowIfNull(menu);
            m_Menus.Add(menu);
            for (int index = 0; index < m_Items.Count; index++)
                AddItemToMenu(menu, m_Items[index]);
            return this;
        }

        private void AddItemToMenu(ContextMenu menu, Item item)
        {
            ContextMenuButton button = menu.AddButton(item.Text, _ => SetSelected(item));
            button.TooltipText = item.Tooltip ?? string.Empty;
            button.SetChecked(ReferenceEquals(m_Selected, item));
            item.Buttons.Add(button);
        }

        private void SetSelected(Item item)
        {
            if (ReferenceEquals(m_Selected, item))
                return;

            m_Selected = item;
            for (int index = 0; index < m_Items.Count; index++)
            {
                List<ContextMenuButton> buttons = m_Items[index].Buttons;
                bool checkedValue = ReferenceEquals(m_Items[index], item);
                for (int buttonIndex = 0; buttonIndex < buttons.Count; buttonIndex++)
                    buttons[buttonIndex].SetChecked(checkedValue);
            }
            SelectedChanged?.Invoke(item.Value);
            item.Selected?.Invoke();
        }
    }
}

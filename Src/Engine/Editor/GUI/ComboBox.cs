// Managed editor GUI feature implementation.
using System;
using System.Collections.Generic;
using SE.GUI;

namespace SE.Editor.GUI
{
    public class ComboBox : Control
    {
        public const float DefaultHeight = 18.0f;

        private readonly List<string> m_Items = new List<string>();
        private readonly List<string> m_Tooltips = new List<string>();
        private readonly List<int> m_SelectedIndices = new List<int>(4);
        private ContextMenu? m_PopupMenu;
        private bool m_IsPressed;

        public ComboBox()
            : this(0.0f, 0.0f)
        {
        }

        public ComboBox(float x, float y, float width = 120.0f)
            : base(new Rectangle(x, y, width, DefaultHeight))
        {
            MaximumItemsInViewCount = 20;
            BackgroundColor = Style.Current.BackgroundNormal;
        }

        public event Action<ComboBox>? SelectedIndexChanged;
        public event Action<ComboBox>? PopupShowing;

        public IList<string> Items => m_Items;
        public IList<string> Tooltips => m_Tooltips;
        public bool Sorted { get; set; }
        public bool SupportMultiSelect { get; set; }
        public int MaximumItemsInViewCount { get; set; }
        public Func<ComboBox, ContextMenu>? PopupCreate { get; set; }
        public ContextMenu? Popup => m_PopupMenu;
        public bool IsPopupOpened => m_PopupMenu?.IsOpened == true;
        public bool HasSelection => m_SelectedIndices.Count != 0;
        public string Text { get; private set; } = string.Empty;

        public string SelectedItem
        {
            get => m_SelectedIndices.Count == 1 ? m_Items[m_SelectedIndices[0]] : string.Empty;
            set => SelectedIndex = m_Items.IndexOf(value);
        }

        public int SelectedIndex
        {
            get => m_SelectedIndices.Count == 1 ? m_SelectedIndices[0] : -1;
            set
            {
                int clamped = m_Items.Count == 0 ? -1 : Math.Min(Math.Max(value, -1), m_Items.Count - 1);
                if (SelectedIndex == clamped)
                    return;

                m_SelectedIndices.Clear();
                if (clamped != -1)
                    m_SelectedIndices.Add(clamped);
                OnSelectedIndexChanged();
            }
        }

        public IReadOnlyList<int> Selection => m_SelectedIndices;

        public void SetSelection(IEnumerable<int> selection)
        {
            List<int> next = new List<int>();
            foreach (int index in selection)
            {
                if (index < 0 || index >= m_Items.Count)
                    throw new ArgumentOutOfRangeException(nameof(selection), "Selection index is outside the item range.");
                next.Add(index);
            }

            if (!SupportMultiSelect && next.Count > 1)
                throw new InvalidOperationException("ComboBox does not support multiple selected items.");

            if (SequenceEqual(m_SelectedIndices, next))
                return;

            m_SelectedIndices.Clear();
            m_SelectedIndices.AddRange(next);
            OnSelectedIndexChanged();
        }

        public void ClearItems()
        {
            SelectedIndex = -1;
            m_Items.Clear();
            RefreshText();
        }

        public void AddItem(string item)
        {
            m_Items.Add(item);
            RefreshText();
        }

        public void AddItems(IEnumerable<string> items)
        {
            m_Items.AddRange(items);
            RefreshText();
        }

        public void SetItems(IEnumerable<string> items)
        {
            SelectedIndex = -1;
            m_Items.Clear();
            m_Items.AddRange(items);
            RefreshText();
        }

        public bool IsSelected(string item)
        {
            return IsSelected(m_Items.IndexOf(item));
        }

        public bool IsSelected(int index)
        {
            return index >= 0 && m_SelectedIndices.Contains(index);
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button != MouseButton.Left)
                return false;

            m_IsPressed = true;
            Root?.StartTrackingMouse(this);
            return true;
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button != MouseButton.Left || !m_IsPressed)
                return false;

            m_IsPressed = false;
            Root?.EndTrackingMouse();
            if (location.X >= 0.0f && location.Y >= 0.0f && location.X <= Width && location.Y <= Height)
                ShowPopup();
            return true;
        }

        public override void ClearState()
        {
            m_IsPressed = false;
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
            m_PopupMenu ??= OnCreatePopup();
            m_PopupMenu.MaximumItemsInViewCount = MaximumItemsInViewCount;

            if (m_PopupMenu.IsOpened)
            {
                if (!SupportMultiSelect)
                    m_PopupMenu.Hide();
                return;
            }

            PopupShowing?.Invoke(this);
            if (m_Items.Count == 0)
                return;

            UpdateButtons();
            m_PopupMenu.MinimumWidth = Width;
            m_PopupMenu.Show(this, 1.0f, Height);
        }

        protected virtual void OnSelectedIndexChanged()
        {
            TooltipText = m_Tooltips.Count == m_Items.Count && m_SelectedIndices.Count == 1
                ? m_Tooltips[m_SelectedIndices[0]]
                : string.Empty;
            RefreshText();
            SelectedIndexChanged?.Invoke(this);
        }

        protected virtual void OnItemClicked(int index)
        {
            if (SupportMultiSelect)
            {
                if (m_SelectedIndices.Contains(index))
                    m_SelectedIndices.Remove(index);
                else
                    m_SelectedIndices.Add(index);
                OnSelectedIndexChanged();
                UpdateButtons();
            }
            else
            {
                SelectedIndex = index;
                m_PopupMenu?.Hide();
            }
        }

        protected virtual void OnLayoutMenuButton(ContextMenuButton button, int index, bool construct)
        {
            button.SetChecked(m_SelectedIndices.Contains(index));
            if (m_Tooltips.Count > index)
                button.TooltipText = m_Tooltips[index];
        }

        protected virtual ContextMenu OnCreatePopup()
        {
            return PopupCreate != null ? PopupCreate(this) : new ContextMenu();
        }

        private void UpdateButtons()
        {
            if (m_PopupMenu == null)
                return;

            m_PopupMenu.DisposeAllItems();
            if (Sorted)
                m_Items.Sort(StringComparer.OrdinalIgnoreCase);

            for (int i = 0; i < m_Items.Count; i++)
            {
                int itemIndex = i;
                ContextMenuButton button = m_PopupMenu.AddButton(m_Items[i], _ => OnItemClicked(itemIndex));
                button.Tag = itemIndex;
                OnLayoutMenuButton(button, itemIndex, true);
            }
        }

        private void RefreshText()
        {
            Text = m_SelectedIndices.Count switch
            {
                0 => string.Empty,
                1 when m_SelectedIndices[0] >= 0 && m_SelectedIndices[0] < m_Items.Count => m_Items[m_SelectedIndices[0]],
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

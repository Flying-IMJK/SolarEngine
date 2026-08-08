// Managed editor GUI feature implementation.
using System;
using System.Collections.Generic;
using SE.GUI;

namespace SE.Editor.GUI
{
    public enum ContextMenuDirection
    {
        RightDown,
        RightUp,
        LeftDown,
        LeftUp,
    }

    public class ContextMenu
    {
        private readonly List<ContextMenuItem> m_Items = new List<ContextMenuItem>();
        private readonly Dictionary<ContextMenuItem, Control> m_ItemViews = new Dictionary<ContextMenuItem, Control>();
        private bool m_AutoSort;
        private ContextMenu? m_OpenedChild;
        private readonly ContextMenu? m_ParentMenu;
        private Panel? m_View;

        public ContextMenu()
            : this(null)
        {
        }

        internal ContextMenu(ContextMenu? parentMenu)
        {
            m_ParentMenu = parentMenu;
            MinimumWidth = 10.0f;
            MaximumItemsInViewCount = 20;
            Direction = ContextMenuDirection.RightDown;
        }

        public event Action<ContextMenuButton>? ButtonClicked;
        public event Action<ContextMenu>? VisibleChanged;

        public IReadOnlyList<ContextMenuItem> Items => m_Items;
        public bool HasItems => m_Items.Count > 0;
        public bool IsOpened { get; private set; }
        public ContextMenuDirection Direction { get; private set; }
        public SE.GUI.Control? PlacementTarget { get; private set; }
        public float PlacementX { get; private set; }
        public float PlacementY { get; private set; }
        public float MinimumWidth { get; set; }
        public int MaximumItemsInViewCount { get; set; }

        public bool AutoSort
        {
            get => m_AutoSort;
            set
            {
                m_AutoSort = value;
                SortButtons();
            }
        }

        public ContextMenuButton AddButton(string text)
        {
            ContextMenuButton button = new ContextMenuButton(this, text);
            m_Items.Add(button);
            SortButtons();
            RefreshViewIfOpened();
            return button;
        }

        public ContextMenuButton AddButton(string text, string shortKeys)
        {
            ContextMenuButton button = new ContextMenuButton(this, text, shortKeys);
            m_Items.Add(button);
            SortButtons();
            RefreshViewIfOpened();
            return button;
        }

        // Flax AddButton(string, InputBinding, Action) is intentionally disabled until the
        // managed Editor input-binding API is available. Do not replace it with a custom binding type.

        public ContextMenuButton AddButton(string text, Action clicked)
        {
            ContextMenuButton button = AddButton(text);
            button.Clicked += _ => clicked();
            return button;
        }

        public ContextMenuButton AddButton(string text, Action<ContextMenuButton> clicked)
        {
            ContextMenuButton button = AddButton(text);
            button.Clicked += clicked;
            return button;
        }

        public ContextMenuChildMenu? GetChildMenu(string text)
        {
            foreach (ContextMenuItem item in m_Items)
            {
                if (item is ContextMenuChildMenu childMenu && childMenu.Text == text)
                    return childMenu;
            }

            return null;
        }

        public ContextMenuChildMenu GetOrAddChildMenu(string text)
        {
            return GetChildMenu(text) ?? AddChildMenu(text);
        }

        public ContextMenuChildMenu AddChildMenu(string text)
        {
            ContextMenuChildMenu childMenu = new ContextMenuChildMenu(this, text);
            m_Items.Add(childMenu);
            SortButtons();
            RefreshViewIfOpened();
            return childMenu;
        }

        public ContextMenuSeparator AddSeparator()
        {
            ContextMenuSeparator separator = new ContextMenuSeparator(this);
            m_Items.Add(separator);
            RefreshViewIfOpened();
            return separator;
        }

        public void DisposeAllItems()
        {
            m_OpenedChild?.Hide();
            m_OpenedChild = null;
            m_Items.Clear();
            RefreshViewIfOpened();
        }

        public void SortButtons(bool force = false)
        {
            if (!m_AutoSort && !force)
                return;

            m_Items.Sort(static (left, right) =>
            {
                if (left is ContextMenuButton leftButton && right is ContextMenuButton rightButton)
                    return string.Compare(leftButton.Text, rightButton.Text, StringComparison.OrdinalIgnoreCase);
                if (left is ContextMenuButton)
                    return -1;
                if (right is ContextMenuButton)
                    return 1;
                return 0;
            });
            RefreshViewIfOpened();
        }

        public virtual void Show(SE.GUI.Control? parent, float x, float y)
        {
            Hide();

            PlacementTarget = parent;
            PlacementX = x;
            PlacementY = y;
            Direction = ContextMenuDirection.RightDown;
            BuildView();
            if (m_View != null)
            {
                Float2 position = parent != null ? parent.PointToRoot(new Float2(x, y)) : new Float2(x, y);
                m_View.SetBounds(position.X, position.Y, m_View.Width, m_View.Height);
                m_View.Visible = true;
                parent?.Root?.AddChild(m_View);
            }
            IsOpened = true;
            VisibleChanged?.Invoke(this);
        }

        public virtual void Hide()
        {
            if (!IsOpened)
                return;

            foreach (ContextMenuItem item in m_Items)
            {
                if (item is ContextMenuChildMenu childMenu)
                    childMenu.ContextMenu.Hide();
            }

            IsOpened = false;
            PlacementTarget = null;
            if (m_View != null)
            {
                m_View.Visible = false;
                ((SE.GUI.Control)m_View).Parent?.RemoveChild(m_View);
            }
            m_ParentMenu?.OnChildHidden(this);
            VisibleChanged?.Invoke(this);
        }

        internal void OnButtonClicked(ContextMenuButton button)
        {
            ButtonClicked?.Invoke(button);
        }

        internal SE.GUI.Control? GetItemView(ContextMenuItem item)
        {
            return m_ItemViews.TryGetValue(item, out SE.GUI.Control? view) ? view : null;
        }

        internal void ShowChild(ContextMenu child, SE.GUI.Control? parent, float x, float y)
        {
            if (!ReferenceEquals(m_OpenedChild, child))
                m_OpenedChild?.Hide();

            child.Show(parent, x, y);
            m_OpenedChild = child;
        }

        private void OnChildHidden(ContextMenu child)
        {
            if (ReferenceEquals(m_OpenedChild, child))
                m_OpenedChild = null;
        }

        private void RefreshViewIfOpened()
        {
            if (!IsOpened)
                return;

            BuildView();
        }

        private void BuildView()
        {
            m_View ??= new Panel(new Rectangle(0, 0, MinimumWidth, 0));
            m_View.BackgroundColor = SE.GUI.Style.Current.Background;
            m_View.DisposeChildren();
            m_ItemViews.Clear();

            float width = MinimumWidth;
            foreach (ContextMenuItem item in m_Items)
            {
                width = Math.Max(width, item.MinimumWidth);
            }
            float y = 2.0f;
            foreach (ContextMenuItem item in m_Items)
            {
                SE.GUI.Control itemView;
                if (item is ContextMenuButton button)
                {
                    Button buttonView = new Button(new Rectangle(2.0f, y, width - 4.0f, item.Height), button.Text)
                    {
                        Enabled = button.Enabled,
                        TooltipText = button.TooltipText,
                    };
                    buttonView.Visible = button.Visible;
                    buttonView.Clicked += _ => button.Click();
                    itemView = buttonView;
                }
                else
                {
                    Panel separatorView = new Panel(new Rectangle(6.0f, y, Math.Max(0.0f, width - 12.0f), item.Height))
                    {
                        BackgroundColor = SE.GUI.Style.Current.ForegroundDisabled,
                        Enabled = false,
                    };
                    separatorView.Visible = item.Visible;
                    itemView = separatorView;
                }

                m_View.AddChild(itemView);
                m_ItemViews.Add(item, itemView);
                y += item.Height;
            }

            m_View.SetBounds(m_View.X, m_View.Y, width, y + 2.0f);
        }
    }

    public abstract class ContextMenuItem
    {
        protected ContextMenuItem(ContextMenu parentContextMenu, float height)
        {
            ParentContextMenu = parentContextMenu;
            Height = height;
            Enabled = true;
            Visible = true;
        }

        public ContextMenu ParentContextMenu { get; }
        public bool Enabled { get; set; }
        public bool Visible { get; set; }
        public float Height { get; }
        public object? Tag { get; set; }
        public string TooltipText { get; set; } = string.Empty;
        public virtual float MinimumWidth => 0.0f;
    }

    public class ContextMenuButton : ContextMenuItem
    {
        public ContextMenuButton(ContextMenu parentContextMenu, string text, string shortKeys = "")
            : base(parentContextMenu, 22.0f)
        {
            Text = text;
            ShortKeys = shortKeys;
            CloseMenuOnClick = true;
        }

        public event Action<ContextMenuButton>? Clicked;

        public string Text { get; set; }
        public string ShortKeys { get; set; }
        public bool Checked { get; private set; }
        public bool AutoCheck { get; private set; }
        public bool CloseMenuOnClick { get; set; }

        public override float MinimumWidth
        {
            get
            {
                float width = 20.0f + Text.Length * 7.0f;
                if (!string.IsNullOrEmpty(ShortKeys))
                    width += 40.0f + ShortKeys.Length * 7.0f;
                return width;
            }
        }

        public ContextMenuButton SetAutoCheck(bool value)
        {
            AutoCheck = value;
            return this;
        }

        public ContextMenuButton SetChecked(bool value)
        {
            Checked = value;
            return this;
        }

        public virtual void Click()
        {
            if (CloseMenuOnClick)
                ParentContextMenu.Hide();

            if (AutoCheck)
                Checked = !Checked;

            Clicked?.Invoke(this);
            ParentContextMenu.OnButtonClicked(this);
        }
    }

    public sealed class ContextMenuChildMenu : ContextMenuButton
    {
        public ContextMenuChildMenu(ContextMenu parentContextMenu, string text)
            : base(parentContextMenu, text)
        {
            ContextMenu = new ContextMenu(parentContextMenu);
            CloseMenuOnClick = false;
        }

        public ContextMenu ContextMenu { get; }

        public override void Click()
        {
            base.Click();
            ShowChild();
        }

        public void ShowChild()
        {
            SE.GUI.Control? view = ParentContextMenu.GetItemView(this);
            ParentContextMenu.ShowChild(ContextMenu, view, view?.Width ?? 0.0f, 0.0f);
        }
    }

    public sealed class ContextMenuSeparator : ContextMenuItem
    {
        public ContextMenuSeparator(ContextMenu parentContextMenu)
            : base(parentContextMenu, 4.0f)
        {
        }
    }

}

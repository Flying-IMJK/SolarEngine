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
        private readonly List<ContextMenuItem> _items = new List<ContextMenuItem>();
        private readonly Dictionary<ContextMenuItem, Control> _itemViews = new Dictionary<ContextMenuItem, Control>();
        private bool _autoSort;
        private Panel? _view;

        public ContextMenu()
        {
            MinimumWidth = 10.0f;
            MaximumItemsInViewCount = 20;
            Direction = ContextMenuDirection.RightDown;
        }

        public event Action<ContextMenuButton>? ButtonClicked;

        public IReadOnlyList<ContextMenuItem> Items => _items;
        public bool HasItems => _items.Count > 0;
        public bool IsOpened { get; private set; }
        public ContextMenuDirection Direction { get; private set; }
        public SE.GUI.Control? PlacementTarget { get; private set; }
        public float PlacementX { get; private set; }
        public float PlacementY { get; private set; }
        public float MinimumWidth { get; set; }
        public int MaximumItemsInViewCount { get; set; }

        public bool AutoSort
        {
            get => _autoSort;
            set
            {
                _autoSort = value;
                SortButtons();
            }
        }

        public ContextMenuButton AddButton(string text)
        {
            ContextMenuButton button = new ContextMenuButton(this, text);
            _items.Add(button);
            SortButtons();
            RefreshViewIfOpened();
            return button;
        }

        public ContextMenuButton AddButton(string text, string shortKeys)
        {
            ContextMenuButton button = new ContextMenuButton(this, text, shortKeys);
            _items.Add(button);
            SortButtons();
            return button;
        }

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
            foreach (ContextMenuItem item in _items)
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
            _items.Add(childMenu);
            SortButtons();
            RefreshViewIfOpened();
            return childMenu;
        }

        public ContextMenuSeparator AddSeparator()
        {
            ContextMenuSeparator separator = new ContextMenuSeparator(this);
            _items.Add(separator);
            RefreshViewIfOpened();
            return separator;
        }

        public void DisposeAllItems()
        {
            _items.Clear();
            RefreshViewIfOpened();
        }

        public void SortButtons(bool force = false)
        {
            if (!_autoSort && !force)
                return;

            _items.Sort(static (left, right) =>
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
            PlacementTarget = parent;
            PlacementX = x;
            PlacementY = y;
            Direction = ContextMenuDirection.RightDown;
            BuildView();
            if (_view != null)
            {
                Float2 position = parent != null ? parent.PointToRoot(new Float2(x, y)) : new Float2(x, y);
                _view.SetBounds(position.X, position.Y, _view.Width, _view.Height);
                _view.Visible = true;
                parent?.Root?.AddChild(_view);
            }
            IsOpened = true;
        }

        public virtual void Hide()
        {
            foreach (ContextMenuItem item in _items)
            {
                if (item is ContextMenuChildMenu childMenu)
                    childMenu.ContextMenu.Hide();
            }

            IsOpened = false;
            PlacementTarget = null;
            if (_view != null)
            {
                _view.Visible = false;
                ((SE.GUI.Control)_view).Parent?.RemoveChild(_view);
            }
        }

        internal void OnButtonClicked(ContextMenuButton button)
        {
            ButtonClicked?.Invoke(button);
        }

        internal SE.GUI.Control? GetItemView(ContextMenuItem item)
        {
            return _itemViews.TryGetValue(item, out SE.GUI.Control? view) ? view : null;
        }

        private void RefreshViewIfOpened()
        {
            if (!IsOpened)
                return;

            BuildView();
        }

        private void BuildView()
        {
            _view ??= new Panel(new Rectangle(0, 0, MinimumWidth, 0));
            _view.BackgroundColor = SE.GUI.Style.Current.Background;
            _view.DisposeChildren();
            _itemViews.Clear();

            float width = MinimumWidth;
            foreach (ContextMenuItem item in _items)
                width = Math.Max(width, item.MinimumWidth);

            float y = 2.0f;
            foreach (ContextMenuItem item in _items)
            {
                SE.GUI.Control itemView;
                if (item is ContextMenuButton button)
                {
                    Button buttonView = new Button(new Rectangle(2.0f, y, width - 4.0f, item.Height), button.Text)
                    {
                        Enabled = button.Enabled,
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

                _view.AddChild(itemView);
                _itemViews.Add(item, itemView);
                y += item.Height;
            }

            _view.SetBounds(_view.X, _view.Y, width, y + 2.0f);
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
            ContextMenu = new ContextMenu();
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
            ContextMenu.Show(view, view?.Width ?? 0.0f, 0.0f);
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

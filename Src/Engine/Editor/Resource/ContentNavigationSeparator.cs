#nullable disable annotations

using SE.Editor.GUI;
using SE.GUI;

namespace SE.Editor
{
    public sealed class ContentNavigationSeparator : ComboBox
    {
        public ContentNavigationButton Target;

        public ContentNavigationSeparator()
        {
        }

        public ContentNavigationSeparator(ContentNavigationButton target, float x, float y, float height)
        {
            Target = target;
            Bounds = new Rectangle(x, y, 16, height);
            Offsets = new Margin(Bounds.X, Bounds.Width, Bounds.Y, Bounds.Height);
            UpdateTransform();

            MaximumItemsInViewCount = 20;
            Style style = Style.Current;
            BackgroundColor = style.BackgroundNormal;
            BackgroundColorHighlighted = BackgroundColor;
            BackgroundColorSelected = BackgroundColor;
        }

        public override void Draw()
        {
            Style style = Style.Current;
            Rectangle rect = new(Float2.Zero, Size);
            Color color;
            if (IsDragOver())
            {
                color = Color.Transparent;
            }
            else if (_mouseDown)
            {
                color = style.BackgroundSelected;
            }
            else if (IsMouseOver)
            {
                color = style.BackgroundHighlighted;
            }
            else
            {
                color = Color.Transparent;
            }
            Render2D.FillRectangle(rect, color);
            Rectangle arrowRect = new(rect.Location.X, rect.Y + rect.Size.Y * 0.25f, rect.Size.X, rect.Size.X);
            Render2D.DrawSprite(EditorIcons.ArrowRight12, arrowRect, EnabledInHierarchy ? style.Foreground : style.ForegroundDisabled);
        }

        protected override ContextMenu OnCreatePopup()
        {
            // Update items
            ClearItems();
            foreach (Control child in Target.TargetNode.Children)
            {
                if (child is ContentTreeNode node)
                {
                    if (node.GetFolder().VisibleInHierarchy)
                    {
                        AddItem(node.GetFolder().ShortName);
                    }
                }
            }

            return base.OnCreatePopup();
        }

        protected override void OnLayoutMenuButton(ContextMenuButton button, int index, bool construct = false)
        {
            button.Icon = EditorIcons.FolderClosed32;
            if (Tooltips.Count > index)
            {
                button.TooltipText = Tooltips[index];
            }
        }

        protected override void OnItemClicked(int index)
        {
            base.OnItemClicked(index);

            string item = Items[index];
            foreach (Control child in Target.TargetNode.Children)
            {
                if (child is ContentTreeNode node && node.GetFolder().ShortName == item)
                {
                    Editor.Instance.Windows.ContentWin.Navigate(node);
                    return;
                }
            }
        }
    }
}

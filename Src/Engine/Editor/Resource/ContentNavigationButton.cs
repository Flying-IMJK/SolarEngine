using System;
using SE.GUI;

namespace SE.Editor
{
    /// <summary>
    /// Breadcrumb button bound to a managed content-tree node.
    /// </summary>
    public sealed class ContentNavigationButton : Button
    {
        public ContentNavigationButton(ContentTreeNode node, float height)
            : base(new Rectangle(0, 0, MeasureWidth(node.Text), height), node.Text)
        {
            Node = node ?? throw new ArgumentNullException(nameof(node));
            Clicked += _ => Navigate();
        }

        public ContentTreeNode Node { get; }
        public event Action<ContentTreeNode>? NavigationRequested;

        public float MeasureWidth() => MeasureWidth(Text);

        private void Navigate()
        {
            NavigationRequested?.Invoke(Node);
        }

        private static float MeasureWidth(string text) => text.Length * 7.0f + 12.0f;
    }
}

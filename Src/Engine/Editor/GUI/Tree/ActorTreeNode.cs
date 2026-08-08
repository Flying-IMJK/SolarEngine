using System;
using SE.GUI;
namespace SE.Editor.GUI
{
    public class ActorTreeNode : TreeNode
    {
        private readonly DragHandlers m_DragHandlers = new DragHandlers();
        private bool m_HasSearchFilter;

        public ActorTreeNode()
            : base(canChangeOrder: true)
        {
        }

        public SceneGraphNode? ActorGraphNode { get; private set; }

        public virtual void LinkNode(SceneGraphNode node)
        {
            ActorGraphNode = node;
            UpdateText();
        }

        public void OnParentChanged(SceneGraphNode? parentNode)
        {
            UpdateText();
        }

        public void OnOrderInParentChanged()
        {
        }

        public virtual void UpdateText()
        {
            Text = ActorGraphNode?.Name ?? string.Empty;
        }

        public void UpdateFilter(string filterText)
        {
            m_HasSearchFilter = !string.IsNullOrWhiteSpace(filterText);
            Visible = !m_HasSearchFilter || Text.Contains(filterText, StringComparison.OrdinalIgnoreCase);
        }

        public void AddDragHandler(DragHelperBase helper)
        {
            m_DragHandlers.Add(helper);
        }

        public void StartRenaming()
        {
            RenamePopup popup = new RenamePopup(Text, new Float2(160, 22), false);
            popup.Renamed += OnRenamed;
            popup.Show(this, 0, HeaderHeight);
        }

        protected override DragDropEffect OnDragEnterHeader(DragData data)
        {
            return m_DragHandlers.OnDragEnter(data);
        }

        protected override DragDropEffect OnDragMoveHeader(DragData data)
        {
            return m_DragHandlers.Effect;
        }

        protected override DragDropEffect OnDragDropHeader(DragData data)
        {
            m_DragHandlers.OnDragDrop(new DragEventArgs());
            return DragDropEffect.Move;
        }

        protected override void OnDragLeaveHeader()
        {
            m_DragHandlers.OnDragLeave();
        }

        protected override void OnExpandedChanged()
        {
            ActorGraphNode ??= new SceneGraphNode(Guid.NewGuid(), Text);
        }

        private void OnRenamed(RenamePopup popup)
        {
            if (ActorGraphNode != null)
            {
                ActorGraphNode.Name = popup.Text;
                UpdateText();
            }
            else
            {
                Text = popup.Text;
            }
        }
    }
}

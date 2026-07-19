using System;
namespace SE.Editor.GUI
{
    public class ActorTreeNode : TreeNode
    {
        private readonly DragHandlers _dragHandlers = new DragHandlers();
        private bool _hasSearchFilter;

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
            _hasSearchFilter = !string.IsNullOrWhiteSpace(filterText);
            Visible = !_hasSearchFilter || Text.Contains(filterText, StringComparison.OrdinalIgnoreCase);
        }

        public void AddDragHandler(DragHelperBase helper)
        {
            _dragHandlers.Add(helper);
        }

        public void StartRenaming()
        {
            RenamePopup popup = new RenamePopup(Text, new GuiSize(160, 22), false);
            popup.Renamed += OnRenamed;
            popup.Show(this, 0, HeaderHeight);
        }

        protected override DragDropEffect OnDragEnterHeader(DragData data)
        {
            return _dragHandlers.OnDragEnter(data);
        }

        protected override DragDropEffect OnDragMoveHeader(DragData data)
        {
            return _dragHandlers.Effect;
        }

        protected override DragDropEffect OnDragDropHeader(DragData data)
        {
            _dragHandlers.OnDragDrop(new DragEventArgs());
            return DragDropEffect.Move;
        }

        protected override void OnDragLeaveHeader()
        {
            _dragHandlers.OnDragLeave();
        }

        protected override void OnExpandedChanged()
        {
            ActorGraphNode ??= new SceneGraphNode(Text);
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

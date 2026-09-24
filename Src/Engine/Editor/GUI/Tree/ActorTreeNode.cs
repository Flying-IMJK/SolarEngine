using System;
using SE.Editor.SceneGraph;
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Tree presentation node for an <see cref="ActorGraphNode"/>.
    /// Behavior follows the native ActorTreeNode implementation; features that
    /// are commented out there remain intentionally inactive here.
    /// </summary>
    public class ActorTreeNode : TreeNode
    {
        private int m_OrderInParent;
        private DragActors? m_DragActors;
        private DragAssets? m_DragAssets;
        private DragHandlers? m_DragHandlers;
        private bool m_HasSearchFilter;

        public ActorTreeNode()
            : base(canChangeOrder: true)
        {
            ChildrenIndent = 16.0f;
        }

        public ActorGraphNode? ActorGraphNode { get; private set; }

        public SE.Actor? GetActor() => ActorGraphNode?.Actor;

        public ActorGraphNode? GetActorGraphNode() => ActorGraphNode;

        public virtual void LinkNode(ActorGraphNode node)
        {
            ArgumentNullException.ThrowIfNull(node);
            ActorGraphNode = node;
            SE.Actor? actor = node.Actor;
            if (actor != null)
            {
                m_OrderInParent = actor.OrderInParent;
                // Native hide flags are not exposed in the managed editor yet;
                // the active native behavior therefore keeps this visible.
                Visible = true;
            }
            else
            {
                m_OrderInParent = 0;
            }

            UpdateText();
        }

        public void OnParentChanged(ActorGraphNode? parentNode)
        {
            SE.Actor? actor = GetActor();
            m_OrderInParent = actor?.OrderInParent ?? 0;

            ActorTreeNode? parentTreeNode = parentNode?.TreeNode;
            if (parentTreeNode != null)
            {
                bool wasLayoutLocked = parentTreeNode.IsLayoutLocked;
                if (!wasLayoutLocked)
                    parentTreeNode.IsLayoutLocked = true;
                Parent = parentTreeNode;

                // TreeNode reserves child slot zero for its header label.
                if (Parent != null && Parent.Children.Count > 1)
                {
                    int index = Math.Clamp(m_OrderInParent + 1, 1, Parent.Children.Count - 1);
                    IndexInParent = index;
                }

                if (wasLayoutLocked)
                    return;

                parentTreeNode.IsLayoutLocked = false;
                if (parentTreeNode.IsCollapsedInHierarchy)
                {
                    UnlockChildrenRecursive();
                }
                else if (parentTreeNode.ParentTree is Tree tree)
                {
                    tree.Parent?.PerformLayout();
                }
                else
                {
                    parentTreeNode.PerformLayout();
                }
            }
            else
            {
                Parent = null;
            }
        }

        public void OnOrderInParentChanged()
        {
            if (Parent is ActorTreeNode parent)
            {
                bool anyChanged = false;
                foreach (Control control in parent.Children)
                {
                    if (control is not TreeNode child)
                        continue;
                    if (child is not ActorTreeNode actorChild || actorChild.GetActor() == null)
                        continue;

                    int order = actorChild.GetActor()!.OrderInParent;
                    anyChanged |= actorChild.m_OrderInParent != order;
                    if (anyChanged)
                        actorChild.m_OrderInParent = order;
                }

                if (anyChanged)
                    parent.SortChildren();
            }
            else if (GetActor() != null)
            {
                m_OrderInParent = GetActor()!.OrderInParent;
            }
        }

        public virtual void UpdateText()
        {
            Text = ActorGraphNode?.Name ?? string.Empty;
        }

        public void UpdateFilter(string filterText)
        {
            bool noFilter = string.IsNullOrEmpty(filterText);
            m_HasSearchFilter = !noFilter;

            // The native text-match/highlight block is commented out. Keep the
            // same effective rule: a non-empty filter hides this node, while a
            // matching visible child keeps the branch visible.
            bool isThisVisible = noFilter;
            bool isAnyChildVisible = false;
            foreach (Control control in Children)
            {
                if (control is not ActorTreeNode child)
                    continue;
                if (child is ActorTreeNode actorChild)
                {
                    actorChild.UpdateFilter(filterText);
                    isAnyChildVisible |= actorChild.Visible;
                }
            }

            bool isExpanded = isAnyChildVisible;
            if (noFilter && GetActor() != null)
                isExpanded = false;

            if (isExpanded)
                Expand(true);
            else
                Collapse(true);

            Visible = isThisVisible || isAnyChildVisible;
        }

        public override void Update(float deltaTime)
        {
            SE.Actor? actor = GetActor();
            if (actor != null && !m_HasSearchFilter)
                Visible = true;

            base.Update(deltaTime);
        }

        public override void Draw()
        {
            base.Draw();
        }

        public override int Compare(Control? other)
        {
            return other is ActorTreeNode actorNode
                ? m_OrderInParent - actorNode.m_OrderInParent
                : 0;
        }

        public void StartRenaming(EditorWindow? window, Panel? treePanel = null)
        {
            _ = window;
            _ = treePanel;
        }

        protected override Color CacheTextColor()
        {
            if (Parent is not ActorTreeNode)
                return base.CacheTextColor();

            Color color = Style.Current.TextColor;
            SE.Actor? actor = GetActor();
            if (actor != null)
            {
                if (actor.HasPrefabLink)
                    color = Style.Current.ProgressNormal;

                if (!actor.IsActiveInHierarchy)
                    return Style.Current.ForegroundGrey;

                if (actor.HasScene && actor.IsStatic)
                    return color * 0.85f;
            }

            return color;
        }

        protected override void OnExpandedChanged()
        {
            base.OnExpandedChanged();
            // Expanded-state persistence is commented out in native code.
        }

        protected override DragDropEffect OnDragEnterHeader(DragData data)
        {
            m_DragHandlers ??= new DragHandlers();
            m_DragActors ??= new DragActors(ValidateDragActor, FindActorNode);
            if (!m_DragHandlers.Contains(m_DragActors))
                m_DragHandlers.Add(m_DragActors);

            // Asset validation is the native AssetItem virtual hook. Until a
            // managed asset type overrides it, AssetItem's native default is
            // false, while the content browser still resolves drag payloads.
            m_DragAssets ??= new DragAssets(ValidateDragAsset, FindAsset);
            if (!m_DragHandlers.Contains(m_DragAssets))
                m_DragHandlers.Add(m_DragAssets);

            if (m_DragActors.OnDragEnter(data))
                return m_DragActors.Effect;
            if (m_DragAssets.OnDragEnter(data))
                return m_DragAssets.Effect;
            return DragDropEffect.None;
        }

        protected override DragDropEffect OnDragMoveHeader(DragData data)
        {
            _ = data;
            return m_DragHandlers?.Effect ?? DragDropEffect.None;
        }

        protected override void OnDragLeaveHeader()
        {
            m_DragHandlers?.OnDragLeave();
        }

        protected override DragDropEffect OnDragDropHeader(DragData data)
        {
            _ = data;
            SE.Actor? actor = GetActor();
            SE.Actor? newParent = actor;
            int newOrder = -1;

            if (actor == null)
            {
                if (SE.Level.ScenesCount == 0)
                {
                    SE.Debug.LogError("No scene loaded.");
                    return DragDropEffect.None;
                }
                newParent = SE.Level.GetScene(SE.Level.ScenesCount - 1);
            }
            else if (DragOverMode == DragItemPositioning.Above && actor.Parent != null)
            {
                newParent = actor.Parent;
                newOrder = actor.OrderInParent;
            }
            else if (DragOverMode == DragItemPositioning.Below && actor.Parent != null)
            {
                newParent = actor.Parent;
                newOrder = actor.OrderInParent + 1;
            }

            if (newParent == null)
            {
                SE.Debug.LogError("Missing parent actor.");
                return DragDropEffect.None;
            }

            _ = newOrder;
            DragDropEffect result = m_DragHandlers?.HasValidDrag == true
                ? DragDropEffect.Move
                : DragDropEffect.None;
            m_DragHandlers?.OnDragDrop(new DragEventArgs());
            return result;
        }

        private void OnRenamed(RenamePopup popup)
        {
            // Native rename mutation is commented out; do not change Actor.Name.
            _ = popup;
        }

        private static ActorGraphNode? FindActorNode(Guid id)
        {
            return Editor.Instance?.Scene.SceneGraph.FindNode(id) as ActorGraphNode;
        }

        private static ContentItem? FindAsset(string path)
        {
            ManagedContentBrowser? browser = Editor.Instance?.Windows.Content.Browser;
            return browser?.RootFolder.Find(path);
        }

        private static bool ValidateDragActor(ActorGraphNode actorNode)
        {
            // The native validation body is entirely commented out.
            _ = actorNode;
            return false;
        }

        private static bool ValidateDragAsset(ContentItem assetItem)
        {
            // AssetItem.OnEditorDrag is the native virtual extension point. No
            // managed override exists yet, so the native default remains false.
            _ = assetItem;
            return false;
        }

        protected override void OnDispose()
        {
            m_DragHandlers?.Clear();
            m_DragActors = null;
            m_DragAssets = null;
            m_DragHandlers = null;
            ActorGraphNode = null;
            base.OnDispose();
        }
    }
}

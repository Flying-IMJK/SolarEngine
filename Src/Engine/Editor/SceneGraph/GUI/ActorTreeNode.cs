using System;
using System.Collections.Generic;
using SE.Editor.SceneGraph;
using SE.GUI;

namespace SE.Editor.GUI
{
    public class ActorTreeNode : TreeNode
    {
        private int m_OrderInParent;
        private DragActors m_DragActors;
        private DragHandlers m_DragHandlers;
        private RenamePopup m_RenamePopup;
        private readonly List<Rectangle> m_Highlights = new List<Rectangle>();
        private bool m_HasSearchFilter;

        /// <summary>
        /// The actor node that owns this node.
        /// </summary>
        protected ActorGraphNode m_ActorNode;

        /// <summary>
        /// Gets the actor.
        /// </summary>
        public SE.Actor GetActor()
        {
            return m_ActorNode.Actor;
        }

        /// <summary>
        /// Gets the actor node.
        /// </summary>
        public ActorGraphNode GetActorGraphNode()
        {
            return m_ActorNode;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ActorTreeNode"/> class.
        /// </summary>
        public ActorTreeNode()
            : base(true)
        {
            ChildrenIndent = 16.0f;
        }

        public virtual void LinkNode(ActorGraphNode node)
        {
            m_ActorNode = node;
            SE.Actor actor = node.Actor;
            if (actor != null)
            {
                m_OrderInParent = actor.OrderInParent;
                Visible = true; // (actor.HideFlags & HideFlags.HideInHierarchy) == 0;

                // Pick the correct id when inside a prefab window.
                Guid id = actor.HasPrefabLink && !actor.HasScene ? actor.PrefabObjectID : actor.SceneObjectId;
                /*if (Editor.Instance.ProjectCache.IsExpandedActor(id))
                {
                    Expand(true);
                }*/
            }
            else
            {
                m_OrderInParent = 0;
            }

            UpdateText();
        }

        public void OnParentChanged(SE.Actor actor, ActorGraphNode parentNode)
        {
            // Update cached value
            m_OrderInParent = actor.OrderInParent;

            // Update UI (special case if actor is spawned and added to existing scene tree)
            ActorTreeNode parentTreeNode = null;
            if (parentNode != null)
            {
                parentTreeNode = parentNode.TreeNode;
            }

            if (parentTreeNode != null && !parentTreeNode.IsLayoutLocked)
            {
                parentTreeNode.IsLayoutLocked = true;
                Parent = parentTreeNode;
                IndexInParent = m_OrderInParent;
                parentTreeNode.IsLayoutLocked = false;

                // Skip UI update if node won't be in a view
                if (parentTreeNode.IsCollapsedInHierarchy)
                {
                    UnlockChildrenRecursive();
                }
                else
                {
                    // Try to perform layout at the level where it makes it the most performant (the least computations)
                    Tree tree = parentTreeNode.ParentTree;
                    if (tree != null)
                    {
                        if (tree.Parent is Panel treeParent)
                        {
                            treeParent.PerformLayout();
                        }
                        else
                        {
                            tree.PerformLayout();
                        }
                    }
                    else
                    {
                        parentTreeNode.PerformLayout();
                    }
                }
            }
            else
            {
                Parent = parentTreeNode;
            }
        }

        public void OnOrderInParentChanged()
        {
            if (Parent is ActorTreeNode parent)
            {
                bool anyChanged = false;
                foreach (Control control in parent.Children)
                {
                    if (control is ActorTreeNode child && child.GetActor() != null)
                    {
                        int orderInParent = child.GetActor().OrderInParent;
                        anyChanged |= child.m_OrderInParent != orderInParent;
                        if (anyChanged)
                        {
                            child.m_OrderInParent = orderInParent;
                        }
                    }
                }

                if (anyChanged)
                {
                    parent.SortChildren();
                }
            }
            else if (GetActor() != null)
            {
                m_OrderInParent = GetActor().OrderInParent;
            }
        }

        /// <summary>
        /// Updates the tree node text.
        /// </summary>
        public virtual void UpdateText()
        {
            Text = m_ActorNode.Name;
        }

        /// <summary>
        /// Updates the query search filter.
        /// </summary>
        /// <param name="filterText">The filter text.</param>
        public void UpdateFilter(string filterText)
        {
            // Skip hidden actors
            SE.Actor actor = GetActor();
            if (actor != null /* && (actor.HideFlags & HideFlags.HideInHierarchy) != 0 */)
            {
                return;
            }

            bool noFilter = string.IsNullOrEmpty(filterText);
            m_HasSearchFilter = !noFilter;

            // Update itself
            bool isThisVisible;
            if (noFilter)
            {
                // Clear filter
                m_Highlights.Clear();
                isThisVisible = true;
            }
            else
            {
                /* The native query match and highlight implementation is commented out. */
                SE.Debug.Assert(false, "Unreachable code encountered!");
                return;
            }

            // Update children
            bool isAnyChildVisible = false;
            foreach (Control control in Children)
            {
                if (control is ActorTreeNode child)
                {
                    child.UpdateFilter(filterText);
                    isAnyChildVisible |= child.Visible;
                }
            }

            bool isExpanded = isAnyChildVisible;

            // Restore cached state on query filter clear
            if (noFilter && actor != null)
            {
                // Pick the correct id when inside a prefab window.
                Guid id = actor.HasPrefabLink && actor.Scene == null ? actor.PrefabObjectID : actor.SceneObjectId;
                isExpanded = false; // Editor.Instance.ProjectCache.IsExpandedActor(ref id);
            }

            if (isExpanded)
            {
                Expand(true);
            }
            else
            {
                Collapse(true);
            }

            Visible = isThisVisible | isAnyChildVisible;
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            // Update hidden state
            SE.Actor actor = GetActor();
            if (actor != null && !m_HasSearchFilter)
            {
                Visible = true; // (actor.HideFlags & HideFlags.HideInHierarchy) == 0;
            }

            base.Update(deltaTime);
        }

        /// <inheritdoc />
        public override bool OnShowTooltip(out string text, out Float2 location, out Rectangle area)
        {
            // Evaluate tooltip text once it's actually needed
            SE.Actor actor = m_ActorNode.Actor;
            if (string.IsNullOrEmpty(TooltipText) && actor != null)
            {
                // TooltipText = Surface.SurfaceUtils.GetVisualScriptTypeDescription(TypeUtils.GetObjectType(actor));
            }

            return base.OnShowTooltip(out text, out location, out area);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            m_RenamePopup?.Hide();
            m_DragHandlers?.Clear();
            m_Highlights.Clear();

            m_DragHandlers = null;
            m_DragActors = null;

            base.OnDestroy();
        }

        public override int CompareTo(Control other)
        {
            if (other is ActorTreeNode node)
            {
                return m_OrderInParent - node.m_OrderInParent;
            }

            return base.CompareTo(other);
        }

        /// <summary>
        /// Starts the actor renaming action.
        /// </summary>
        public void StartRenaming(EditorWindow window, Panel treePanel = null)
        {
            if (m_ActorNode == null || m_ActorNode.IsDisposed || GetActor() == null ||
                m_ActorNode.ParentScene == null || GetActor().HasPrefabLink || m_RenamePopup != null)
            {
                return;
            }
            Select();
            Rectangle rect = TextRect;
            if (treePanel != null)
            {
                treePanel.ScrollViewTo(this, true);
                rect.Width = Math.Max(rect.Width, treePanel.Width - rect.X);
            }
            if (window is SceneHierarchyWindow hierarchy)
            {
                hierarchy.ScrollingOnSceneTreeView(false);
            }
            m_RenamePopup = RenamePopup.ShowPopup(this, rect, m_ActorNode.Name, false);
            m_RenamePopup.Validate = (RenamePopup popup, ref string value) => !string.IsNullOrWhiteSpace(value);
            m_RenamePopup.Renamed += OnRenamed;
            m_RenamePopup.Closed += popup =>
            {
                m_RenamePopup = null;
                if (window is SceneHierarchyWindow sceneWindow && !sceneWindow.IsDisposed)
                {
                    sceneWindow.ScrollingOnSceneTreeView(true);
                }
            };
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // Draw all highlights
            if (m_Highlights.Count > 0)
            {
                Style style = Style.Current;
                Color color = style.ProgressNormal * 0.6f;
                foreach (Rectangle highlight in m_Highlights)
                {
                    Render2D.FillRectangle(highlight, color);
                }
            }
        }

        protected override void BeginDragDrop()
        {
            if (m_ActorNode == null || !m_ActorNode.CanDrag)
            {
                return;
            }
            Tree tree = ParentTree;
            List<ActorGraphNode> actors = new();
            if (tree != null && tree.Selection.Contains(this))
            {
                foreach (TreeNode selected in tree.Selection)
                {
                    if (selected is not ActorTreeNode actorTree || !actorTree.m_ActorNode.CanDrag)
                    {
                        continue;
                    }
                    bool selectedAncestor = false;
                    for (Control ancestor = selected.Parent; ancestor != null && ancestor != tree; ancestor = ancestor.Parent)
                    {
                        if (ancestor is ActorTreeNode parent && parent.m_ActorNode.CanDrag && tree.Selection.Contains(parent))
                        {
                            selectedAncestor = true;
                            break;
                        }
                    }
                    if (!selectedAncestor)
                    {
                        actors.Add(actorTree.m_ActorNode);
                    }
                }
            }
            else
            {
                actors.Add(m_ActorNode);
            }
            if (actors.Count > 0)
            {
                DoDragDrop(DragActors.GetDragData(actors));
            }
        }

        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (IsFocused && key == KeyboardKeys.F2)
            {
                SceneHierarchyWindow window = Editor.Instance.Windows.SceneHierarchy;
                StartRenaming(window, window.SceneTreePanel);
                return true;
            }
            return base.OnKeyDown(key);
        }

        protected override bool ShowTooltip()
        {
            return true;
        }

        /// <inheritdoc />
        protected override Color CacheTextColor()
        {
            // Update node text color (based on ActorGraphNode.IsActiveInHierarchy but with optimized logic a little)
            if (Parent is ActorTreeNode)
            {
                Color color = Style.Current.TextColor;
                SE.Actor actor = GetActor();
                if (actor != null)
                {
                    if (actor.HasPrefabLink)
                    {
                        // Prefab
                        color = Style.Current.ProgressNormal;
                    }

                    if (!actor.IsActiveInHierarchy)
                    {
                        // Inactive
                        return Style.Current.ForegroundGrey;
                    }

                    if (actor.HasScene /* && Editor.Instance.StateMachine.IsPlayMode */ && actor.IsStatic)
                    {
                        // Static
                        return color * 0.85f;
                    }
                }

                // Default
                return color;
            }

            return base.CacheTextColor();
        }

        /// <inheritdoc />
        protected override void OnExpandedChanged()
        {
            base.OnExpandedChanged();
            SE.Actor actor = GetActor();

            if (!IsLayoutLocked && actor != null)
            {
                // Pick the correct id when inside a prefab window.
                Guid id = actor.HasPrefabLink && !actor.HasScene ? actor.PrefabObjectID : actor.SceneObjectId;
                // Editor.Instance.ProjectCache.SetExpandedActor(ref id, IsExpanded);
            }
        }

        /// <inheritdoc />
        protected override DragDropEffect OnDragEnterHeader(DragData data)
        {
            if (m_ActorNode == null || m_ActorNode.IsDisposed ||
                (m_ActorNode is SceneGraphNode && DragOverMode != DragItemPositioning.At))
            {
                return DragDropEffect.None;
            }
            if (m_DragHandlers == null)
            {
                m_DragHandlers = new DragHandlers();
            }
            if (m_DragActors == null)
            {
                m_DragActors = new DragActors(node => node != null, FindActorNode);
                m_DragHandlers.Add(m_DragActors);
            }
            if (m_DragActors.OnDragEnter(data))
            {
                foreach (ActorGraphNode node in m_DragActors.Objects)
                {
                    if (!ValidateDragActor(node))
                    {
                        m_DragActors.InvalidDrag();
                        return DragDropEffect.None;
                    }
                }
                return m_DragActors.Effect;
            }
            return DragDropEffect.None;
        }

        /// <inheritdoc />
        protected override DragDropEffect OnDragMoveHeader(DragData data)
        {
            return OnDragEnterHeader(data);
        }

        /// <inheritdoc />
        protected override void OnDragLeaveHeader()
        {
            m_DragHandlers?.OnDragLeave();
        }

        /// <inheritdoc />
        protected override DragDropEffect OnDragDropHeader(DragData data)
        {
            try
            {
                // Cached sources and targets can have changed while dragging.
                if (OnDragEnterHeader(data) == DragDropEffect.None)
                {
                    return DragDropEffect.None;
                }
                SE.Actor target = GetActor();
                SE.Actor parent = target;
                int order = -1;
                if (target == null)
                {
                    if (SE.Level.ScenesCount == 0)
                    {
                        return DragDropEffect.None;
                    }
                    parent = SE.Level.GetScene(SE.Level.ScenesCount - 1);
                }
                else if (DragOverMode == DragItemPositioning.Above || DragOverMode == DragItemPositioning.Below)
                {
                    if (!target.HasParent)
                    {
                        return DragDropEffect.None;
                    }
                    parent = target.Parent;
                    order = target.OrderInParent + (DragOverMode == DragItemPositioning.Below ? 1 : 0);
                }
                bool worldPositionsStays = !Root.GetKey(KeyboardKeys.Control);
                return Editor.Instance.Scene.ChangeParent(m_DragActors.Objects, parent, order, worldPositionsStays)
                    ? DragDropEffect.Move : DragDropEffect.None;
            }
            finally
            {
                m_DragHandlers?.OnDragDrop(null);
            }
        }

        private void OnRenamed(RenamePopup renamePopup)
        {
            if (m_ActorNode == null || m_ActorNode.IsDisposed || m_ActorNode.ParentScene == null || GetActor() == null)
            {
                return;
            }
            string name = renamePopup.Text.Trim();
            if (name.Length == 0 || name == GetActor().Name)
            {
                return;
            }
            GetActor().Name = name;
            Editor.Instance.Scene.MarkSceneEdited(m_ActorNode.ParentScene);
        }

        private bool ValidateDragActor(ActorGraphNode actorNode)
        {
            if (actorNode == null || !actorNode.CanDrag || actorNode.Root != m_ActorNode.Root)
            {
                return false;
            }
            SE.Actor target = GetActor();
            return actorNode != m_ActorNode && (target == null ||
                (target.Scene != null && !target.HasPrefabLink && actorNode.Find(target) == null));
        }

        private static ActorGraphNode FindActorNode(Guid id)
        {
            return Editor.Instance.Scene.GetActorNode(id);
        }
    }
}

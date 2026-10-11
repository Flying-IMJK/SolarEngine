using System;
using System.Collections.Generic;
using SE.Editor.SceneGraph;
using SE.GUI;

namespace SE.Editor.GUI
{
    public class ActorTreeNode : TreeNode
    {
        private int _orderInParent;
        private DragActors _dragActors;
        private DragAssets _dragAssets;
        private DragHandlers _dragHandlers;
        private readonly List<Rectangle> _highlights = new List<Rectangle>();
        private bool _hasSearchFilter;

        /// <summary>
        /// The actor node that owns this node.
        /// </summary>
        protected ActorGraphNode _actorNode;

        /// <summary>
        /// Gets the actor.
        /// </summary>
        public SE.Actor GetActor()
        {
            return _actorNode.Actor;
        }

        /// <summary>
        /// Gets the actor node.
        /// </summary>
        public ActorGraphNode GetActorGraphNode()
        {
            return _actorNode;
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
            _actorNode = node;
            SE.Actor actor = node.Actor;
            if (actor != null)
            {
                _orderInParent = actor.OrderInParent;
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
                _orderInParent = 0;
            }

            UpdateText();
        }

        public void OnParentChanged(SE.Actor actor, ActorGraphNode parentNode)
        {
            // Update cached value
            _orderInParent = actor.OrderInParent;

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
                IndexInParent = _orderInParent;
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
                        anyChanged |= child._orderInParent != orderInParent;
                        if (anyChanged)
                        {
                            child._orderInParent = orderInParent;
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
                _orderInParent = GetActor().OrderInParent;
            }
        }

        /// <summary>
        /// Updates the tree node text.
        /// </summary>
        public virtual void UpdateText()
        {
            Text = _actorNode.Name;
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
            _hasSearchFilter = !noFilter;

            // Update itself
            bool isThisVisible;
            if (noFilter)
            {
                // Clear filter
                _highlights.Clear();
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
            if (actor != null && !_hasSearchFilter)
            {
                Visible = true; // (actor.HideFlags & HideFlags.HideInHierarchy) == 0;
            }

            base.Update(deltaTime);
        }

        /// <inheritdoc />
        public override bool OnShowTooltip(string text, out Float2 location, out Rectangle area)
        {
            // Evaluate tooltip text once it's actually needed
            SE.Actor actor = _actorNode.Actor;
            if (string.IsNullOrEmpty(TooltipText) && actor != null)
            {
                // TooltipText = Surface.SurfaceUtils.GetVisualScriptTypeDescription(TypeUtils.GetObjectType(actor));
            }

            return base.OnShowTooltip(text, out location, out area);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _dragHandlers?.Clear();
            _highlights.Clear();

            _dragHandlers = null;
            _dragActors = null;
            _dragAssets = null;

            base.OnDestroy();
        }

        public override int CompareTo(Control other)
        {
            if (other is ActorTreeNode node)
            {
                return _orderInParent - node._orderInParent;
            }

            return base.CompareTo(other);
        }

        /// <summary>
        /// Starts the actor renaming action.
        /// </summary>
        public void StartRenaming(EditorWindow window, Panel treePanel = null)
        {
            // Block renaming during scripts reload
            /*if (Editor.Instance.ProgressReporting.CompileScripts.IsActive)
            {
                return;
            }*/

            Select();

            /* The native rename dialog and scrolling block is commented out. */
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // Draw all highlights
            if (_highlights.Count > 0)
            {
                Style style = Style.Current;
                Color color = style.ProgressNormal * 0.6f;
                foreach (Rectangle highlight in _highlights)
                {
                    Render2D.FillRectangle(highlight, color);
                }
            }
        }

        public override void DoDragDrop(DragData data)
        {
            /* The native drag initiation body is commented out. */
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
            /* The native scene/content editability checks are commented out. */
            if (_dragHandlers == null)
            {
                _dragHandlers = new DragHandlers();
            }

            // Check if drop actors
            if (_dragActors == null)
            {
                _dragActors = new DragActors(ValidateDragActor, FindActorNode);
                _dragHandlers.Add(_dragActors);
            }
            if (_dragActors.OnDragEnter(data))
            {
                return _dragActors.Effect;
            }

            // Check if drag assets
            if (_dragAssets == null)
            {
                _dragAssets = new DragAssets(ValidateDragAsset, FindAsset);
                _dragHandlers.Add(_dragAssets);
            }
            if (_dragAssets.OnDragEnter(data))
            {
                return _dragAssets.Effect;
            }

            return DragDropEffect.None;
        }

        /// <inheritdoc />
        protected override DragDropEffect OnDragMoveHeader(DragData data)
        {
            return _dragHandlers.Effect;
        }

        /// <inheritdoc />
        protected override void OnDragLeaveHeader()
        {
            _dragHandlers.OnDragLeave();
        }

        /// <inheritdoc />
        protected override DragDropEffect OnDragDropHeader(DragData data)
        {
            DragDropEffect result = DragDropEffect.None;

            SE.Actor myActor = GetActor();
            SE.Actor newParent;
            int newOrder = -1;

            // Check if has no actor (only for Root Actor)
            if (myActor == null)
            {
                // Append to the last scene
                if (SE.Level.ScenesCount == 0)
                {
                    SE.Debug.LogError("No scene loaded.");
                    return DragDropEffect.None;
                }
                newParent = SE.Level.GetScene(SE.Level.ScenesCount - 1);
            }
            else
            {
                newParent = myActor;

                // Use drag positioning to change target parent and index
                if (DragOverMode == DragItemPositioning.Above)
                {
                    if (myActor.HasParent)
                    {
                        newParent = myActor.Parent;
                        newOrder = myActor.OrderInParent;
                    }
                }
                else if (DragOverMode == DragItemPositioning.Below)
                {
                    if (myActor.HasParent)
                    {
                        newParent = myActor.Parent;
                        newOrder = myActor.OrderInParent + 1;
                    }
                }
            }

            if (newParent == null)
            {
                SE.Debug.LogError("Missing parent actor.");
                return DragDropEffect.None;
            }

            // Drag actors
            if (_dragActors != null && _dragActors.HasValidDrag)
            {
                bool worldPositionsStays = Root.GetKey(KeyboardKeys.Control) == false;
                SE.SceneObject[] objects = new SE.SceneObject[_dragActors.Objects.Count];
                for (int i = 0; i < _dragActors.Objects.Count; i++)
                {
                    objects[i] = _dragActors.Objects[i].Actor;
                }
                /*var action = new ParentActorsAction(objects, newParent, newOrder, worldPositionsStays);
                _actorNode.Root.Undo?.AddAction(action);
                action.Do();*/
                result = DragDropEffect.Move;
            }
            // Drag assets
            else if (_dragAssets != null && _dragAssets.HasValidDrag)
            {
                SE.Debug.Assert(false, "Unreachable code encountered!");
                /* The native asset spawn block is commented out. */
                result = DragDropEffect.Move;
            }

            // Clear cache
            _dragHandlers.OnDragDrop(null);

            // Check if scene has been modified
            if (result != DragDropEffect.None)
            {
                /*var node = SceneGraph.FindNode(newParent.ID) as ActorGraphNode;
                node?.TreeNode.Expand();*/
            }

            return result;
        }

        private void OnRenamed(RenamePopup renamePopup)
        {
            /*using (new UndoBlock(_actorNode.Root.Undo, Actor, "Rename"))
            {
                Actor.Name = renamePopup.Text.Trim();
            }*/
        }

        private bool ValidateDragActor(ActorGraphNode actorNode)
        {
            return false;
            /* The native actor drag validation block is commented out. */
        }

        private bool ValidateDragAsset(ContentItem assetItem)
        {
            return assetItem is AssetItem asset && asset.OnEditorDrag(this);
        }

        private static ActorGraphNode FindActorNode(Guid id)
        {
            return Editor.Instance.Scene.GetActorNode(id);
        }

        private static ContentItem FindAsset(string path)
        {
            // AssetDatabaseModule::Find(path) has no managed binding yet.
            // return AssetDatabaseModule.Find(path);
            return null;
        }
    }
}

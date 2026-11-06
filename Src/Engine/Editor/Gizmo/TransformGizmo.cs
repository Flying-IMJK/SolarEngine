
using System.Collections.Generic;
using SE.Editor.SceneGraph;

namespace SE.Editor.Gizmo
{
    public sealed class TransformGizmo : TransformGizmoBase
    {
        private readonly List<ScenesGraphNode> m_Selection = new();
        private readonly List<ScenesGraphNode> m_SelectedParents = new();
        private readonly Dictionary<SceneGraphNode, bool> m_StartEditedScenes = new();

        public IReadOnlyList<ScenesGraphNode> Selection => m_Selection;
        public IReadOnlyList<ScenesGraphNode> SelectedParents => m_SelectedParents;
        protected override int SelectionCount => m_SelectedParents.Count;

        public TransformGizmo(IGizmoOwner owner) : base(owner) { }

        public override void OnSelectionChanged(List<ScenesGraphNode> selection)
        {
            CancelTransforming();
            m_Selection.Clear();
            m_Selection.AddRange(selection);
            m_SelectedParents.Clear();
            foreach (ScenesGraphNode node in selection)
            {
                if (node is not ActorGraphNode actor || node is RootGraphNode || node is SceneGraphNode || actor.Actor == null || node.IsDisposed)
                {
                    continue;
                }
                // A selected ancestor already transforms its descendants through the native hierarchy.
                bool selectedParent = false;
                for (ScenesGraphNode parent = node.ParentNode; parent != null; parent = parent.ParentNode)
                {
                    if (selection.Contains(parent) && parent is not SceneGraphNode && parent is not RootGraphNode)
                    {
                        selectedParent = true;
                        break;
                    }
                }
                if (!selectedParent)
                {
                    m_SelectedParents.Add(node);
                }
            }
        }

        protected override ScenesGraphNode GetSelectedObject(int index) => m_SelectedParents[index];
        protected override Transform GetSelectedTransform(int index) => m_SelectedParents[index].Transform;

        protected override void SetSelectedTransform(int index, Transform value)
        {
            ScenesGraphNode node = m_SelectedParents[index];
            if (!node.IsDisposed && node.Transform != value)
            {
                node.Transform = value;
                if (node.ParentScene != null)
                {
                    node.ParentScene.IsEdited = true;
                }
            }
        }

        protected override void OnStartTransforming()
        {
            m_StartEditedScenes.Clear();
            // Owner.Renderer.BeginTransform();
            foreach (ScenesGraphNode node in m_SelectedParents)
            {
                SceneGraphNode scene = node.ParentScene;
                if (scene != null)
                {
                    m_StartEditedScenes.TryAdd(scene, scene.IsEdited);
                }
                // Owner.Renderer.RecordTransform(((ActorGraphNode)node).Actor);
            }
        }

        protected override void OnEndTransforming(bool cancelled, bool changed)
        {
            if (cancelled || !changed)
            {
                foreach (KeyValuePair<SceneGraphNode, bool> scene in m_StartEditedScenes)
                {
                    if (!scene.Key.IsDisposed)
                    {
                        scene.Key.IsEdited = scene.Value;
                    }
                }
                // Owner.Renderer.CancelTransform();
            }
            else
            {
                // One completed drag registers one native TransformObjectsAction.
                // Owner.Renderer.EndTransform();
            }
            m_StartEditedScenes.Clear();
        }

        public override void Pick()
        {
            if (IsTransforming || !Owner.Viewport.IsPointerOverScene)
            {
                return;
            }
            float closest = float.MaxValue;
            ActorGraphNode hit = null;
            PickNode(Owner.SceneGraphRoot, Owner.MouseRay, ref closest, ref hit);
            List<ScenesGraphNode> selection = Owner.IsControlDown
                ? new List<ScenesGraphNode>(Owner.SceneGraphRoot.Selection) : new List<ScenesGraphNode>();
            if (hit != null)
            {
                if (selection.Contains(hit))
                {
                    selection.Remove(hit);
                }
                else
                {
                    selection.Add(hit);
                }
            }
            Owner.Select(selection);
        }

        private static void PickNode(ScenesGraphNode node, Ray ray, ref float closest, ref ActorGraphNode hit)
        {
            if (node == null || node.IsDisposed)
            {
                return;
            }
            if (node is ActorGraphNode actor && node is not SceneGraphNode && node is not RootGraphNode && actor.Actor != null)
            {
                float distance = GizmoRenderer.RayCast(actor.Actor, ray);
                if (distance >= 0.0f && distance < closest)
                {
                    closest = distance;
                    hit = actor;
                }
            }
            foreach (ScenesGraphNode child in node.ChildNodes)
            {
                PickNode(child, ray, ref closest, ref hit);
            }
        }
    }
}

using System;

namespace SE.Editor.SceneGraph
{
    /// <summary>
    /// Scene graph node for a loaded runtime scene.
    /// </summary>
    public sealed class SceneGraphNode : ActorGraphNode
    {
        private bool m_IsEdited;

        public SceneGraphNode(SceneGraphFactory factory, SE.Scene scene)
            : base(factory, scene)
        {
        }

        public bool IsEdited
        {
            get => m_IsEdited;
            set
            {
                if (m_IsEdited == value)
                    return;

                m_IsEdited = value;
                TreeNode.UpdateText();
            }
        }

        public SE.Scene Scene => (SE.Scene)Actor!;

        public override SceneGraphNode ParentScene => this;
    }
}

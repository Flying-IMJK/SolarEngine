namespace SE.Editor.SceneGraph
{
    /// <summary>
    /// Scene graph node for a point light actor.
    /// </summary>
    public sealed class PointLightGraphNode : ActorGraphNode
    {
        public PointLightGraphNode(SceneGraphFactory factory, SE.Actor actor)
            : base(factory, actor)
        {
        }
    }
}

namespace SE.Editor.SceneGraph
{
    /// <summary>
    /// Scene graph node for a directional light actor.
    /// </summary>
    public sealed class DirectionalLightGraphNode : ActorGraphNode
    {
        public DirectionalLightGraphNode(SceneGraphFactory factory, SE.Actor actor)
            : base(factory, actor)
        {
        }
    }
}

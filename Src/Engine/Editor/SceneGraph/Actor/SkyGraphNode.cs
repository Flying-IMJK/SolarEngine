namespace SE.Editor.SceneGraph
{
    /// <summary>
    /// Scene graph node for a sky actor.
    /// </summary>
    public sealed class SkyGraphNode : ActorGraphNode
    {
        public SkyGraphNode(SceneGraphFactory factory, SE.Actor actor)
            : base(factory, actor)
        {
        }
    }
}

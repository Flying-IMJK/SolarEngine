namespace SE.Editor.SceneGraph
{
    /// <summary>
    /// Scene graph node for a static model actor.
    /// </summary>
    public sealed class StaticModelGraphNode : ActorGraphNode
    {
        public StaticModelGraphNode(SceneGraphFactory factory, SE.Actor actor)
            : base(factory, actor)
        {
        }
    }
}

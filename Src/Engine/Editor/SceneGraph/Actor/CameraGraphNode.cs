namespace SE.Editor.SceneGraph
{
    /// <summary>
    /// Scene graph node for a camera actor.
    /// </summary>
    public sealed class CameraGraphNode : ActorGraphNode
    {
        public CameraGraphNode(SceneGraphFactory factory, SE.Actor actor)
            : base(factory, actor)
        {
        }
    }
}

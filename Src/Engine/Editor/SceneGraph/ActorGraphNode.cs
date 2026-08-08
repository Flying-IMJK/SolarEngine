using System;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Managed hierarchy node for a Runtime Actor. Runtime access is always
    /// performed through the Reflector-generated Actor binding.
    /// </summary>
    public sealed class ActorGraphNode : SceneGraphNode
    {
        public ActorGraphNode(SE.Actor actor)
            : base(actor?.SceneObjectId ?? Guid.Empty, actor?.Name ?? string.Empty)
        {
            Actor = actor ?? throw new ArgumentNullException(nameof(actor));
            IsActive = actor.IsActive;
        }

        public SE.Actor Actor { get; private set; }

        internal void Update(SE.Actor actor)
        {
            if (actor == null)
                throw new ArgumentNullException(nameof(actor));

            Actor = actor;
            Name = actor.Name;
            IsActive = actor.IsActive;
        }
    }

    /// <summary>
    /// Root node for all loaded scenes in the managed editor hierarchy.
    /// </summary>
    public sealed class ScenesRootNode : SceneGraphNode
    {
        internal ScenesRootNode()
            : base(Guid.Empty, "Scenes")
        {
        }
    }
}

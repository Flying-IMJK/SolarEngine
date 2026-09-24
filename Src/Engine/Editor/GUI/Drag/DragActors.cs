using System;
using System.Collections.Generic;
using SE.Editor.SceneGraph;
using SE.GUI;

namespace SE.Editor.GUI
{
    public class DragActorsBase<TEventArgs> : DragHelper<ActorGraphNode, TEventArgs>
        where TEventArgs : DragEventArgs
    {
        public const string DragPrefix = "ACTOR!?";

        private readonly Func<Guid, ActorGraphNode?> m_FindNode;

        public DragActorsBase(Func<ActorGraphNode, bool> validateFunction, Func<Guid, ActorGraphNode?> findNode)
            : base(validateFunction)
        {
            m_FindNode = findNode;
        }

        public override DragData ToDragData(ActorGraphNode item)
        {
            return GetDragData(item);
        }

        public override DragData ToDragData(IEnumerable<ActorGraphNode> items)
        {
            return GetDragData(items);
        }

        public static DragData GetDragData(ActorGraphNode item)
        {
            ArgumentNullException.ThrowIfNull(item);
            return new DragDataText(DragPrefix + item.ID.ToString("N"));
        }

        public static DragData GetDragData(IEnumerable<ActorGraphNode> items)
        {
            List<string> names = new List<string>();
            foreach (ActorGraphNode item in items)
            {
                if (item != null)
                    names.Add(item.ID.ToString("N"));
            }

            return new DragDataText(DragPrefix + string.Join("\n", names));
        }

        public override IEnumerable<ActorGraphNode> FromDragData(DragData data)
        {
            if (data is not DragDataText textData || !textData.Text.StartsWith(DragPrefix, StringComparison.Ordinal))
                yield break;

            foreach (string idText in textData.Text.Substring(DragPrefix.Length).Split('\n', StringSplitOptions.RemoveEmptyEntries))
            {
                if (!Guid.TryParseExact(idText, "N", out Guid id))
                    continue;

                ActorGraphNode? node = m_FindNode(id);
                if (node != null)
                    yield return node;
            }
        }
    }

    public sealed class DragActors : DragActorsBase<DragEventArgs>
    {
        public DragActors(Func<ActorGraphNode, bool> validateFunction, Func<Guid, ActorGraphNode?> findNode)
            : base(validateFunction, findNode)
        {
        }
    }
}

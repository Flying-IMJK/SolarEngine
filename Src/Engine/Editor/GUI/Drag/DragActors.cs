using System;
using System.Collections.Generic;
using SE.GUI;

namespace SE.Editor.GUI
{
    public class DragActorsBase<TEventArgs> : DragHelper<SceneGraphNode, TEventArgs>
        where TEventArgs : DragEventArgs
    {
        public const string DragPrefix = "ACTOR!?";

        private readonly Func<string, SceneGraphNode?> m_FindNode;

        public DragActorsBase(Func<SceneGraphNode, bool> validateFunction, Func<string, SceneGraphNode?> findNode)
            : base(validateFunction)
        {
            m_FindNode = findNode;
        }

        public override DragData ToDragData(SceneGraphNode item)
        {
            return GetDragData(item);
        }

        public override DragData ToDragData(IEnumerable<SceneGraphNode> items)
        {
            return GetDragData(items);
        }

        public static DragData GetDragData(SceneGraphNode item)
        {
            return new DragDataText(DragPrefix + item.Name);
        }

        public static DragData GetDragData(IEnumerable<SceneGraphNode> items)
        {
            List<string> names = new List<string>();
            foreach (SceneGraphNode item in items)
            {
                names.Add(item.Name);
            }

            return new DragDataText(DragPrefix + string.Join("\n", names));
        }

        public override IEnumerable<SceneGraphNode> FromDragData(DragData data)
        {
            if (data is not DragDataText textData || !textData.Text.StartsWith(DragPrefix, StringComparison.Ordinal))
                yield break;

            foreach (string id in textData.Text.Substring(DragPrefix.Length).Split('\n', StringSplitOptions.RemoveEmptyEntries))
            {
                SceneGraphNode? node = m_FindNode(id);
                if (node != null)
                    yield return node;
            }
        }
    }

    public sealed class DragActors : DragActorsBase<DragEventArgs>
    {
        public DragActors(Func<SceneGraphNode, bool> validateFunction, Func<string, SceneGraphNode?> findNode)
            : base(validateFunction, findNode)
        {
        }
    }
}

using System;
using System.Collections.Generic;

namespace SE.Editor.GUI
{
    public class DragActorTypeBase<TEventArgs> : DragHelper<Type, TEventArgs>
        where TEventArgs : DragEventArgs
    {
        public const string DragPrefix = "ATYPE!?";

        public DragActorTypeBase(Func<Type, bool> validateFunction)
            : base(validateFunction)
        {
        }

        public override DragData ToDragData(Type item)
        {
            return GetDragData(item);
        }

        public override DragData ToDragData(IEnumerable<Type> items)
        {
            List<string> names = new List<string>();
            foreach (Type item in items)
            {
                names.Add(item.FullName ?? item.Name);
            }

            return new DragDataText(DragPrefix + string.Join("\n", names));
        }

        public static DragData GetDragData(Type item)
        {
            return new DragDataText(DragPrefix + (item.FullName ?? item.Name));
        }

        public override IEnumerable<Type> FromDragData(DragData data)
        {
            if (data is not DragDataText textData || !textData.Text.StartsWith(DragPrefix, StringComparison.Ordinal))
                yield break;

            foreach (string typeName in textData.Text.Substring(DragPrefix.Length).Split('\n', StringSplitOptions.RemoveEmptyEntries))
            {
                Type? type = Type.GetType(typeName, throwOnError: false);
                if (type != null)
                    yield return type;
            }
        }
    }

    public sealed class DragActorType : DragActorTypeBase<DragEventArgs>
    {
        public DragActorType(Func<Type, bool> validateFunction)
            : base(validateFunction)
        {
        }
    }
}

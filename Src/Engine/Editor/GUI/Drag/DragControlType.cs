using System;
using System.Collections.Generic;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Drag helper for managed GUI control types.
    /// </summary>
    public class DragControlType<TEventArgs> : DragHelper<Type, TEventArgs>
        where TEventArgs : DragEventArgs
    {
        public const string DragPrefix = "CTYPE!?";

        public DragControlType(Func<Type, bool> validateFunction)
            : base(validateFunction)
        {
        }

        public override DragData ToDragData(Type item) => GetDragData(item);

        public override DragData ToDragData(IEnumerable<Type> items)
        {
            ArgumentNullException.ThrowIfNull(items);
            var names = new List<string>();
            foreach (Type item in items)
                names.Add(item.AssemblyQualifiedName ?? item.FullName ?? item.Name);
            return new DragDataText(DragPrefix + string.Join("\n", names));
        }

        public override IEnumerable<Type> FromDragData(DragData data)
        {
            if (data is not DragDataText text || !text.Text.StartsWith(DragPrefix, StringComparison.Ordinal))
                yield break;

            string[] names = text.Text.Substring(DragPrefix.Length).Split('\n', StringSplitOptions.RemoveEmptyEntries);
            for (int index = 0; index < names.Length; index++)
            {
                Type? type = Type.GetType(names[index], throwOnError: false);
                if (type != null)
                    yield return type;
            }
        }

        public static DragData GetDragData(Type item)
        {
            ArgumentNullException.ThrowIfNull(item);
            return new DragDataText(DragPrefix + (item.AssemblyQualifiedName ?? item.FullName ?? item.Name));
        }
    }

    public sealed class DragControlType : DragControlType<DragEventArgs>
    {
        public DragControlType(Func<Type, bool> validateFunction)
            : base(validateFunction)
        {
        }
    }
}

using System;
using System.Collections.Generic;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Drag helper for a prefixed collection of string identifiers.
    /// </summary>
    public class DragNames<TEventArgs> : DragHelper<string, TEventArgs>
        where TEventArgs : DragEventArgs
    {
        public DragNames(string prefix, Func<string, bool> validateFunction)
            : base(validateFunction)
        {
            DragPrefix = prefix ?? throw new ArgumentNullException(nameof(prefix));
        }

        public string DragPrefix { get; }

        public override DragData ToDragData(string item) => GetDragData(DragPrefix, item);
        public override DragData ToDragData(IEnumerable<string> items) => GetDragData(DragPrefix, items);

        public override IEnumerable<string> FromDragData(DragData data)
        {
            if (data is not DragDataText text || !text.Text.StartsWith(DragPrefix, StringComparison.Ordinal))
                yield break;

            string[] values = text.Text.Substring(DragPrefix.Length).Split('\n', StringSplitOptions.RemoveEmptyEntries);
            for (int index = 0; index < values.Length; index++)
                yield return values[index];
        }

        public static DragData GetDragData(string prefix, string name)
        {
            ArgumentNullException.ThrowIfNull(prefix);
            ArgumentNullException.ThrowIfNull(name);
            return new DragDataText(prefix + name);
        }

        public static DragData GetDragData(string prefix, IEnumerable<string> names)
        {
            ArgumentNullException.ThrowIfNull(prefix);
            ArgumentNullException.ThrowIfNull(names);
            return new DragDataText(prefix + string.Join("\n", names));
        }
    }

    public sealed class DragNames : DragNames<DragEventArgs>
    {
        public DragNames(string prefix, Func<string, bool> validateFunction)
            : base(prefix, validateFunction)
        {
        }
    }
}

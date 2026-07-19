using System;
using System.Collections.Generic;

namespace SE.Editor.GUI
{
    public class DragItemsBase<TEventArgs> : DragHelper<ContentItem, TEventArgs>
        where TEventArgs : DragEventArgs
    {
        public const string DragPrefix = "ASSET!?";

        private readonly Func<string, ContentItem?> _findItem;

        public DragItemsBase(Func<ContentItem, bool> validateFunction, Func<string, ContentItem?> findItem)
            : base(validateFunction)
        {
            _findItem = findItem;
        }

        public DragData ToDragData(string path)
        {
            return GetDragData(path);
        }

        public override DragData ToDragData(ContentItem item)
        {
            return GetDragData(item);
        }

        public override DragData ToDragData(IEnumerable<ContentItem> items)
        {
            return GetDragData(items);
        }

        public static DragData GetDragData(string path)
        {
            if (string.IsNullOrEmpty(path))
                throw new ArgumentException("Drag path cannot be empty.", nameof(path));

            return new DragDataText(DragPrefix + path);
        }

        public static DragData GetDragData(ContentItem item)
        {
            return GetDragData(item.Path);
        }

        public static DragData GetDragData(IEnumerable<ContentItem> items)
        {
            List<string> paths = new List<string>();
            foreach (ContentItem item in items)
            {
                paths.Add(item.Path);
            }

            return new DragDataText(DragPrefix + string.Join("\n", paths));
        }

        public override IEnumerable<ContentItem> FromDragData(DragData data)
        {
            if (data is not DragDataText textData || !textData.Text.StartsWith(DragPrefix, StringComparison.Ordinal))
                yield break;

            string payload = textData.Text.Substring(DragPrefix.Length);
            string[] paths = payload.Split('\n', StringSplitOptions.RemoveEmptyEntries);
            foreach (string path in paths)
            {
                ContentItem? item = _findItem(path);
                if (item != null)
                    yield return item;
            }
        }
    }

    public sealed class DragItems : DragItemsBase<DragEventArgs>
    {
        public DragItems(Func<ContentItem, bool> validateFunction, Func<string, ContentItem?> findItem)
            : base(validateFunction, findItem)
        {
        }
    }
}

using System;
using System.Collections.Generic;

namespace SE.Editor.GUI
{
    public class DragAssetsBase<TEventArgs> : DragHelper<ContentItem, TEventArgs>
        where TEventArgs : DragEventArgs
    {
        public const string DragPrefix = DragItemsBase<DragEventArgs>.DragPrefix;

        private readonly Func<string, ContentItem?> _findAsset;

        public DragAssetsBase(Func<ContentItem, bool> validateFunction, Func<string, ContentItem?> findAsset)
            : base(validateFunction)
        {
            _findAsset = findAsset;
        }

        public override DragData ToDragData(ContentItem item)
        {
            return GetDragData(item);
        }

        public override DragData ToDragData(IEnumerable<ContentItem> items)
        {
            return GetDragData(items);
        }

        public static DragData GetDragData(ContentItem item)
        {
            return new DragDataText(DragPrefix + item.Path);
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

            foreach (string path in textData.Text.Substring(DragPrefix.Length).Split('\n', StringSplitOptions.RemoveEmptyEntries))
            {
                ContentItem? item = _findAsset(path);
                if (item != null)
                    yield return item;
            }
        }
    }

    public sealed class DragAssets : DragAssetsBase<DragEventArgs>
    {
        public DragAssets(Func<ContentItem, bool> validateFunction, Func<string, ContentItem?> findAsset)
            : base(validateFunction, findAsset)
        {
        }
    }
}

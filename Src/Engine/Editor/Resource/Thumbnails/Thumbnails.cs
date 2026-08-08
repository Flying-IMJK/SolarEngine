using System;
using System.Collections.Generic;

namespace SE.Editor
{
    public enum ThumbnailState
    {
        Created,
        Prepared,
        Rendered,
        Disposed,
        Failed,
    }

    /// <summary>
    /// Managed thumbnail request. The rendered value is opaque on purpose: native
    /// texture handles stay behind the renderer implementation and are never kept
    /// as raw pointers in content items.
    /// </summary>
    public sealed class ThumbnailRequest
    {
        internal ThumbnailRequest(AssetItem item, AssetOperate operate)
        {
            Item = item;
            Operate = operate;
        }

        public ThumbnailState State { get; internal set; } = ThumbnailState.Created;
        public AssetItem Item { get; }
        public AssetOperate Operate { get; }
        public object? Tag { get; set; }
        public bool IsReady => State == ThumbnailState.Prepared;

        public void FinishRender(object? thumbnail)
        {
            if (State is ThumbnailState.Disposed or ThumbnailState.Failed)
                return;
            Item.Thumbnail = thumbnail;
            State = ThumbnailState.Rendered;
        }

        public void Fail()
        {
            if (State != ThumbnailState.Disposed)
                State = ThumbnailState.Failed;
        }

        public void Dispose()
        {
            if (State == ThumbnailState.Disposed)
                return;
            Tag = null;
            State = ThumbnailState.Disposed;
        }
    }

    public interface IThumbnailRenderer
    {
        bool TryRender(ThumbnailRequest request, out object? thumbnail);
    }

    /// <summary>
    /// Managed thumbnail scheduler. A renderer can call the existing native GPU
    /// APIs internally, but scheduling and ownership are deterministic C# state.
    /// </summary>
    public sealed class ThumbnailService : IContentItemOwner
    {
        private readonly Dictionary<AssetItem, ThumbnailRequest> m_Requests = new();

        private ThumbnailService()
        {
        }

        public static ThumbnailService Instance { get; } = new();
        public Func<AssetItem, AssetOperate?>? OperateResolver { get; set; }
        public IThumbnailRenderer? Renderer { get; set; }
        public int PendingCount => m_Requests.Count;

        public void RequestPreview(ContentItem item)
        {
            if (item is not AssetItem asset || m_Requests.ContainsKey(asset))
                return;
            AssetOperate? operate = OperateResolver?.Invoke(asset);
            if (operate == null)
            {
                asset.Thumbnail = asset.DefaultThumbnail;
                return;
            }

            ThumbnailRequest request = new(asset, operate);
            m_Requests.Add(asset, request);
            asset.AddReference(this);
        }

        public void DeletePreview(ContentItem item)
        {
            if (item is not AssetItem asset || !m_Requests.Remove(asset, out ThumbnailRequest? request))
                return;
            request.Dispose();
            asset.RemoveReference(this);
            asset.Thumbnail = asset.DefaultThumbnail;
        }

        public void Update()
        {
            foreach (ThumbnailRequest request in new List<ThumbnailRequest>(m_Requests.Values))
            {
                if (request.State == ThumbnailState.Created)
                {
                    request.Operate.PrepareThumbnail(request);
                    request.State = request.Operate.CanDrawThumbnail(request) ? ThumbnailState.Prepared : ThumbnailState.Failed;
                }

                if (request.State != ThumbnailState.Prepared)
                {
                    Complete(request);
                    continue;
                }

                if (Renderer != null && Renderer.TryRender(request, out object? thumbnail))
                    request.FinishRender(thumbnail);
                else
                    request.Fail();
                Complete(request);
            }
        }

        public void OnItemDeleted(ContentItem item) => DeletePreview(item);
        public void OnItemRenamed(ContentItem item)
        {
            _ = item;
        }

        public void OnItemReimported(ContentItem item)
        {
            DeletePreview(item);
            RequestPreview(item);
        }

        public void OnItemDispose(ContentItem item) => DeletePreview(item);

        private void Complete(ThumbnailRequest request)
        {
            m_Requests.Remove(request.Item);
            request.Item.RemoveReference(this);
            if (request.State == ThumbnailState.Failed)
                request.Item.Thumbnail = request.Item.DefaultThumbnail;
            request.Dispose();
        }
    }
}

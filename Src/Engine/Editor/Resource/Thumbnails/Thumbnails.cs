using System;
using System.Collections.Generic;
using System.IO;

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
    /// Managed thumbnail scheduler. A renderer can call the existing native GPU
    /// APIs internally, but scheduling and ownership are deterministic C# state.
    /// </summary>
    public sealed class Thumbnails : IContentItemOwner
    {
        /// <summary>
        /// The minimum required quality (in range [0;1]) for content streaming resources to be loaded in order to
        /// generate thumbnail for them.
        /// </summary>
        public static readonly float MinimumRequiredResourcesQuality = 0.8f;

        private Editor editor = null;
        private List<PreviewsCache> m_Cache = new List<PreviewsCache>(4);
        private string m_CacheFolder = null;
        private List<ThumbnailRequest> m_Requests = new List<ThumbnailRequest>(128);
        private PreviewRoot m_GuiRoot = null;
        private DateTime m_LastFlushTime;
        private RenderTask m_Task = null;
        private GPUTexture m_Output = null;

        public Thumbnails(Editor editor)
        {
            this.editor = editor;
            m_GuiRoot = new PreviewRoot();
            m_CacheFolder = FileSystem.CombinePaths(EngineContext.ProjectCacheFolder, "Thumbnails");
            m_LastFlushTime = DateTime.UtcNow;
        }


        /// <summary>
        /// Requests the item preview.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <exception cref="System.ArgumentNullException"></exception>
        public void RequestPreview(ContentItem item)
        {
            Debug.Assert(item != null, "Item cannot be null.");

            // Check if use default icon
            if (item.HasDefaultThumbnail)
            {
                item.Thumbnail = item.DefaultThumbnail;
                return;
            }

            // We cache previews only for items with 'ID', for now we support only AssetItems
            AssetItem assetItem = item as AssetItem;
            if (assetItem == null)
            {
                return;
            }

            // Ensure that there is valid proxy for that item
            AssetOperate proxy = editor.AssetDatabase.GetProxy(item) as AssetOperate;
            if (proxy == null)
            {
                Debug.LogWarning($"Cannot generate preview for item {item.Path}. Cannot find proxy for it.");
                return;
            }

            lock (m_Requests)
            {
                // Check if element hasn't been already processed for generating preview
                if (FindRequest(assetItem) == null)
                {
                    // Check each cache atlas
                    for (int i = 0; i < m_Cache.Count; i++)
                    {
                        SpriteHandle sprite = m_Cache[i].FindSlot(assetItem.Id);
                        if (sprite.IsValid)
                        {
                            // Found!
                            item.Thumbnail = sprite;
                            return;
                        }
                    }

                    AddRequest(assetItem, proxy);
                }
            }
        }

        /// <summary>
        /// Deletes the item preview from the cache.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <exception cref="System.ArgumentNullException"></exception>
        public void DeletePreview(ContentItem item)
        {
            if (item == null)
            {
                throw new ArgumentNullException();
            }

            // We cache previews only for items with 'ID', for now we support only AssetItems
            AssetItem assetItem = item as AssetItem;
            if (assetItem == null)
            {
                return;
            }

            lock (m_Requests)
            {
                // Cancel loading
                RemoveRequest(assetItem);

                // Find atlas with preview and remove it
                for (int i = 0; i < m_Cache.Count; i++)
                {
                    if (m_Cache[i].ReleaseSlot(assetItem.Id))
                    {
                        break;
                    }
                }
            }
        }

        /*internal static bool HasMinimumQuality(TextureBase asset)
        {
            if (asset.HasStreamingError)
                return true; // Don't block thumbnails queue when texture fails to stream in (eg. unsupported format)
            var mipLevels = asset.MipLevels;
            var minMipLevels = Mathf.Min(mipLevels, 7);
            return asset.IsLoaded && asset.ResidentMipLevels >= Mathf.Max(minMipLevels, (int)(mipLevels * MinimumRequiredResourcesQuality));
        }

        internal static bool HasMinimumQuality(Model asset)
        {
            if (!asset.IsLoaded)
                return false;
            var lods = asset.LODs.Length;
            var slots = asset.MaterialSlots;
            foreach (var slot in slots)
            {
                if (slot.Material && !HasMinimumQuality(slot.Material))
                    return false;
            }
            return asset.LoadedLODs >= Mathf.Max(1, (int)(lods * MinimumRequiredResourcesQuality));
        }

        internal static bool HasMinimumQuality(SkinnedModel asset)
        {
            var lods = asset.LODs.Length;
            if (asset.IsLoaded && lods == 0)
                return true; // Skeleton-only model
            var slots = asset.MaterialSlots;
            foreach (var slot in slots)
            {
                if (slot.Material && !HasMinimumQuality(slot.Material))
                    return false;
            }
            return asset.LoadedLODs >= Mathf.Max(1, (int)(lods * MinimumRequiredResourcesQuality));
        }

        internal static bool HasMinimumQuality(MaterialBase asset)
        {
            if (asset is MaterialInstance asInstance)
                return HasMinimumQuality(asInstance);
            return HasMinimumQualityInternal(asset);
        }

        internal static bool HasMinimumQuality(Material asset)
        {
            return HasMinimumQualityInternal(asset);
        }

        internal static bool HasMinimumQuality(MaterialInstance asset)
        {
            if (!HasMinimumQualityInternal(asset))
                return false;
            var baseMaterial = asset.BaseMaterial;
            return baseMaterial == null || HasMinimumQualityInternal(baseMaterial);
        }

        private static bool HasMinimumQualityInternal(MaterialBase asset)
        {
            if (!asset.IsLoaded)
                return false;
            var parameters = asset.Parameters;
            foreach (var parameter in parameters)
            {
                if (parameter.Value is TextureBase asTexture && !HasMinimumQuality(asTexture))
                    return false;
            }
            return true;
        }*/

        // IContentItemOwner
        /// <inheritdoc />
        void IContentItemOwner.OnItemDeleted(ContentItem item)
        {
            DeletePreview(item);
        }

        /// <inheritdoc />
        void IContentItemOwner.OnItemRenamed(ContentItem item)
        {
        }

        /// <inheritdoc />
        void IContentItemOwner.OnItemReimported(ContentItem item)
        {
        }

        /// <inheritdoc />
        void IContentItemOwner.OnItemDispose(ContentItem item)
        {
            if (item is AssetItem assetItem)
            {
                lock (m_Requests)
                {
                    RemoveRequest(assetItem);
                }
            }
        }

        /// <inheritdoc />
        public void Init()
        {
            // Create cache folder
            if (!Directory.Exists(m_CacheFolder))
            {
                Directory.CreateDirectory(m_CacheFolder);
            }

            // Find atlases in a Editor cache directory
            string[] files = Directory.GetFiles(m_CacheFolder, "cache_*.sge", SearchOption.TopDirectoryOnly);
            int atlases = 0;
            for (int i = 0; i < files.Length; i++)
            {
                // Load asset
                Asset asset = AssetContent.LoadAsync(files[i]);
                if (asset == null)
                {
                    continue;
                }

                // Validate type
                if (asset is PreviewsCache atlas)
                {
                    // Cache atlas
                    atlases++;
                    m_Cache.Add(atlas);
                }
                else
                {
                    // Skip asset
                    Debug.LogWarning(string
                        .Format(
"Asset \'{0}\' is inside Editor\'s private directory for Assets Thumbnails Cache. Please move it.",
asset.GetPath()));
                }
            }
            Debug.Log(string.Format("Previews cache count: {0} (capacity for {1} icons)", atlases, atlases
                * PreviewsCache.AssetIconsPerAtlas));

            // Prepare at least one atlas
            if (m_Cache.Count == 0)
            {
                GetValidAtlas();
            }

            // Create render task but disabled for now
            m_Output = GPUDevice.instance.CreateTexture("ThumbnailsOutput");
            GPUTextureDescription desc = GPUTextureDescription.New2D(PreviewsCache.AssetIconSize, PreviewsCache
                .AssetIconSize, PreviewsCache.AssetIconsAtlasFormat);
            m_Output.Init(desc);
            m_Task = Object.New<RenderTask>();
            m_Task.Order = 50; // Render this task later
            m_Task.Enabled = false;
            m_Task.Render += Render;
        }

        /// <inheritdoc />
        public void Update()
        {
            // Wait some frames before start generating previews (late init feature)
            if (Time.GetTimeSinceStartup() < 1.0f || HasAllAtlasesLoaded() == false || m_Task == null)
            {
                return;
            }

            lock (m_Requests)
            {
                DateTime now = DateTime.UtcNow;

                // Check if has any request pending
                int count = m_Requests.Count;
                if (count > 0)
                {
                    // Prepare requests
                    bool isAnyReady = false;
                    int checks = Mathf.Min(10, m_Requests.Count);
                    for (int i = 0; i < checks && i < m_Requests.Count; i++)
                    {
                        ThumbnailRequest request = m_Requests[i];
                        try
                        {
                            request.Update();
                            if (request.IsReady)
                            {
                                isAnyReady = true;
                            }
                            else if (request.state == ThumbnailRequest.States.Created)
                            {
                                request.Prepare();
                            }
                            else if (request.state == ThumbnailRequest.States.Failed)
                            {
                                m_Requests.RemoveAt(i--);
                            }
                        }
                        catch (Exception ex)
                        {
                            Debug.LogWarning($"Failed to prepare thumbnail rendering for {request.Item.ShortName}.");
                            Debug.LogWarning(ex);
                            m_Requests.RemoveAt(i--);
                        }
                    }

                    // Check if has no rendering task enabled but should be
                    if (isAnyReady && m_Task.Enabled == false)
                    {
                        // Start generating preview
                        StartPreviewsQueue();
                    }
                }
                // Don't flush every frame
                else if (now - m_LastFlushTime >= TimeSpan.FromSeconds(1))
                {
                    // Flush data
                    m_LastFlushTime = now;
                    Flush();
                }
            }

        }

        /// <inheritdoc />
        public void Exit()
        {
            if (m_Task)
            {
                m_Task.Enabled = false;
            }

            lock (m_Requests)
            {
                // Clear data
                while (m_Requests.Count > 0)
                {
                    RemoveRequest(m_Requests[0]);
                }
                m_Cache.Clear();
            }

            m_GuiRoot.Dispose();
            Object.Destroy(ref m_Task);
            Object.Destroy(ref m_Output);
        }


        private void Render(RenderTask task, GPUContext context)
        {
            lock (m_Requests)
            {
                // Check if there is ready next asset to render thumbnail for it
                // But don't check whole queue, only a few items
                ThumbnailRequest request = GetReadyRequest(10);
                if (request == null)
                {
                    // Disable task
                    m_Task.Enabled = false;
                    return;
                }

                // Find atlas with an free slot
                PreviewsCache atlas = GetValidAtlas();
                if (atlas == null)
                {
                    // Error
                    m_Task.Enabled = false;
                    m_Requests.Clear();
                    Debug.LogError("Failed to get atlas.");
                    return;
                }

                // Wait for atlas being loaded
                if (!atlas.IsReady)
                {
                    return;
                }

                try
                {
                    // Setup
                    m_GuiRoot.RemoveChildren();
                    m_GuiRoot.AccentColor = request.operate.AccentColor;

                    // Call proxy to prepare for thumbnail rendering
                    request.operate.OnThumbnailDrawBegin(request, m_GuiRoot, context);
                    m_GuiRoot.UnlockChildrenRecursive();

                    // Draw preview
                    context.Clear(m_Output.View(), Color.Black);
                    Render2D.CallDrawing(m_GuiRoot, context, m_Output);

                    // Call proxy and cleanup UI (delete create controls, shared controls should be unlinked during
                    // OnThumbnailDrawEnd event)
                    request.operate.OnThumbnailDrawEnd(request, m_GuiRoot);
                }
                catch (Exception ex)
                {
                    // Handle internal errors gracefully (eg. when asset is corrupted and proxy fails)
                    Debug.LogError("Failed to render thumbnail icon for asset: " + request.Item);
                    Debug.LogWarning(ex);
                    request.FinishRender(ref SpriteHandle.Invalid);
                    RemoveRequest(request);
                    return;
                }
                finally
                {
                    m_GuiRoot.DisposeChildren();
                }

                // Copy backbuffer with rendered preview into atlas
                SpriteHandle icon = atlas.OccupySlot(m_Output, request.Item.Id);
                if (!icon.IsValid)
                {
                    // Error
                    m_Task.Enabled = false;
                    m_Requests.Clear();
                    Debug.LogError("Failed to occupy previews cache atlas slot.");
                    return;
                }

                // End
                request.FinishRender(ref icon);
                RemoveRequest(request);
            }
        }

        private void StartPreviewsQueue()
        {
            // Ensure to have valid atlas
            GetValidAtlas();

            // Enable task 
            m_Task.Enabled = true;
        }

        private ThumbnailRequest FindRequest(AssetItem item)
        {
            for (int i = 0; i < m_Requests.Count; i++)
            {
                if (m_Requests[i].Item == item)
                {
                    return m_Requests[i];
                }
            }
            return null;
        }

        private void AddRequest(AssetItem item, AssetOperate proxy)
        {
            ThumbnailRequest request = new ThumbnailRequest(item, proxy);
            m_Requests.Add(request);
            item.AddReference(this);
        }

        private void RemoveRequest(ThumbnailRequest request)
        {
            request.Dispose();
            m_Requests.Remove(request);
            request.Item.RemoveReference(this);
        }

        private void RemoveRequest(AssetItem item)
        {
            ThumbnailRequest request = FindRequest(item);
            if (request != null)
            {
                RemoveRequest(request);
            }
        }

        private ThumbnailRequest GetReadyRequest(int maxChecks)
        {
            maxChecks = Mathf.Min(maxChecks, m_Requests.Count);
            for (int i = 0; i < maxChecks; i++)
            {
                ThumbnailRequest request = m_Requests[i];
                try
                {
                    if (request.IsReady)
                    {
                        return request;
                    }
                }
                catch (Exception ex)
                {
                    Debug.LogWarning($"Failed to prepare thumbnail rendering for {request.Item.ShortName}.");
                    Debug.LogWarning(ex);
                    m_Requests.RemoveAt(i--);
                }
            }

            return null;
        }


        private PreviewsCache CreateAtlas()
        {

            // Create atlas path
            string path = FileSystem.CombinePaths(m_CacheFolder, string.Format("cache_{0:N}.sge", Guid.NewGuid()));

            // Create atlas
            if (!PreviewsCache.Create(path))
            {
                Debug.LogError("Failed to create thumbnails atlas.");
                return null;
            }

            // Load atlas
            PreviewsCache atlas = AssetContent.LoadAsync<PreviewsCache>(path);
            if (atlas == null)
            {
                Debug.LogError("Failed to load thumbnails atlas.");
                return null;
            }

            // Register new atlas
            m_Cache.Add(atlas);

            return atlas;
        }

        private void Flush()
        {
            for (int i = 0; i < m_Cache.Count; i++)
            {
                m_Cache[i].Flush();
            }
        }

        private bool HasAllAtlasesLoaded()
        {
            for (int i = 0; i < m_Cache.Count; i++)
            {
                if (!m_Cache[i].IsReady)
                {
                    return false;
                }
            }
            return true;
        }

        private PreviewsCache GetValidAtlas()
        {
            // Check if has no free slots
            for (int i = 0; i < m_Cache.Count; i++)
            {
                if (m_Cache[i].HasFreeSlot())
                {
                    return m_Cache[i];
                }
            }

            // Create new atlas
            return CreateAtlas();
        }
    }
}

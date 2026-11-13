using SE.Editor.GUI;
using SE.GUI;

using System;

namespace SE.Editor
{
    public sealed class ModelOperate : BinaryAssetOperate
    {
        private ModelPreview m_Preview;

        public override string Name => "Model";
        public override string FileExtension => "sge";
        public override Type AssetTypeId => typeof(Model);

        public override Color AccentColor => Color.FromRGB(0xe67e22);

        public override bool IsProxyFor(Type typeId) => typeId != null && typeof(ModelBase).IsAssignableFrom(typeId);
        public override bool CanReimport(ContentItem item) => item is BinaryAssetItem;

        public override AssetItem ConstructItem(string path, Type typeId, Guid id)
        {
            return new ModelItem(path, id, typeId);
        }


        /// <inheritdoc />
        public override void OnThumbnailDrawPrepare(ThumbnailRequest request)
        {
            if (m_Preview == null)
            {
                m_Preview = new ModelPreview(false)
                {
                    UseAutomaticTaskManagement = false,
                    CustomResolution = new Int2(PreviewsCache.AssetIconSize, PreviewsCache.AssetIconSize),
                };

                // Each atlas entry renders once, without temporal history from a previous model.
                SceneRenderTask task = m_Preview.GetTask();
                RenderView view = task.View;
                view.IsSingleFrame = true;
                task.View = view;
            }
        }

        /// <inheritdoc />
        public override bool CanDrawThumbnail(ThumbnailRequest request)
        {
            Model model = (Model)request.asset;
            if (!model.IsLoaded() || model.GetLoadedLODs() == 0)
            {
                return false;
            }

            int slotsCount = model.GetMaterialSlotsCount();
            for (int slotIndex = 0; slotIndex < slotsCount; slotIndex++)
            {
                MaterialBase material = model.GetMaterialSlotMaterial(slotIndex);
                if (material != null && (!material.IsLoaded() || !material.IsReady()))
                {
                    return false;
                }
            }
            return true;
        }

        /// <inheritdoc />
        public override void OnThumbnailDrawBegin(ThumbnailRequest request, ContainerControl guiRoot, GPUContext context)
        {
            m_Preview.Model = (Model)request.asset;
            m_Preview.Parent = guiRoot;
            m_Preview.SyncBackbufferSize();
            m_Preview.ResetCamera();
            m_Preview.GetTask().OnRender();
        }

        /// <inheritdoc />
        public override void OnThumbnailDrawEnd(ThumbnailRequest request, ContainerControl guiRoot)
        {
            m_Preview.Model = null;
            m_Preview.Parent = null;
        }

        public override void OnThumbnailDrawCleanup(ThumbnailRequest request)
        {
            // Failed rendering skips DrawEnd; unlink before the scheduler disposes its GUI children.
            if (m_Preview != null && m_Preview.Model == request.asset)
            {
                m_Preview.Model = null;
                m_Preview.Parent = null;
            }
        }

        public override void Dispose()
        {
            if (m_Preview != null)
            {
                m_Preview.Parent = null;
                m_Preview.Model = null;
                m_Preview.Dispose();
                m_Preview = null;
            }

            base.Dispose();
        }
    }
}

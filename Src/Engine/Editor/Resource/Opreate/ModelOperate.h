#pragma once
#include "BinaryAssetOperate.h"

namespace SE::Editor
{
    /// <summary>
    /// A <see cref="Model"/> asset proxy object.
    /// </summary>
    SE_CLASS(Reflect)
    class ModelOperate : public BinaryAssetOperate
    {
        SE_DEFINE_CLASS_DEFAULT(ModelOperate, BinaryAssetOperate)
        // private ModelPreview _preview;
    public:

        /// <inheritdoc />
        bool CanReimport(ContentItem* item) override
        {
            return true;
        }

        /// <inheritdoc />
        EditorWindow* Open(const EditorApp* editor, ContentItem* item) override;

        /// <inheritdoc />
        void OnContentWindowContextMenu(ContextMenu* menu, ContentItem* item) override;

        /// <inheritdoc />
        void OnThumbnailDrawPrepare(ThumbnailRequest* request) override;

        /// <inheritdoc />
        bool CanDrawThumbnail(ThumbnailRequest* request) override;

        /// <inheritdoc />
        void OnThumbnailDrawBegin(ThumbnailRequest* request, ContainerControl* guiRoot, GPUContext* context) override;

        /// <inheritdoc />
        void OnThumbnailDrawEnd(ThumbnailRequest* request, ContainerControl* guiRoot) override;

        /// <inheritdoc />
        void Dispose() override;

        TypeID GetAssetType() override;

    protected:
        String __GetName() override;

        Color __GetAccentColor() override;
    };

} // SE

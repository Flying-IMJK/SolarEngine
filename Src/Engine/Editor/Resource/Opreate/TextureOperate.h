#pragma once
#include "BinaryAssetOperate.h"

namespace SE::Editor
{
    class SimpleTexturePreview;
    /// <summary>
    /// A <see cref="Texture"/> asset operate object.
    /// </summary>
    class TextureOperate : public BinaryAssetOperate
    {
        SE_CLASS_DEFAULT(TextureOperate, BinaryAssetOperate)
    public:
        /// <inheritdoc />
        bool CanReimport(ContentItem* item) override
        {
            return true;
        }

        /// <inheritdoc />
        EditorWindow* Open(const EditorApp* editor, ContentItem* item) override;

        /// <inheritdoc />
        TypeID GetAssetType() override;

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

    protected:
        Color __GetAccentColor() override;
        String __GetName() override;

        SimpleTexturePreview* _preview;
    };

} // SE


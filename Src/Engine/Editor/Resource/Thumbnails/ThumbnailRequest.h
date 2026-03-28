#pragma once
#include "Core/Types/Property.h"
#include "Runtime/Render/2D/SpriteAtlas.h"

namespace SE
{
    class Asset;
}

namespace SE::Editor
{
    class AssetOperate;
    class AssetItem;

    /// <summary>
    /// Contains information about asset thumbnail rendering.
    /// </summary>
    class ThumbnailRequest
    {
    public:
        /// <summary>
        /// The request state types.
        /// </summary>
        enum class States
        {
            /// <summary>
            /// The initial state.
            /// </summary>
            Created,

            /// <summary>
            /// Request has been prepared for the rendering but still may wait for resources to load fully.
            /// </summary>
            Prepared,

            /// <summary>
            /// The thumbnail has been rendered. Request can be finalized.
            /// </summary>
            Rendered,

            /// <summary>
            /// The finalized state.
            /// </summary>
            Disposed,

            /// <summary>
            /// The request has failed (eg. asset cannot be loaded).
            /// </summary>
            Failed,
        };

        /// <summary>
        /// Gets the state.
        /// </summary>
        States state = States::Created;

        /// <summary>
        /// The item.
        /// </summary>
        AssetItem* Item = nullptr;

        /// <summary>
        /// The proxy object for the asset item.
        /// </summary>
        AssetOperate* operate = nullptr;

        /// <summary>
        /// The asset reference. May be null if not cached yet.
        /// </summary>
        Asset* asset = nullptr;

        /// <summary>
        /// The custom tag object used by the thumbnails rendering pipeline. Can be used to store the data related to the thumbnail rendering by the asset proxy.
        /// </summary>
        // object Tag;

        /// <summary>
        /// Determines whether thumbnail can be drawn for the item.
        /// </summary>
        PRO_GET(IsReady, ThumbnailRequest, bool, __GetIsReady);

        /// <summary>
        /// Initializes a new instance of the <see cref="ThumbnailRequest"/> class.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <param name="proxy">The proxy.</param>
        ThumbnailRequest(AssetItem* item, AssetOperate* proxy);

        void Update();

        /// <summary>
        /// Prepares this request.
        /// </summary>
        void Prepare();

        /// <summary>
        /// Finishes the rendering and updates the item thumbnail.
        /// </summary>
        /// <param name="icon">The icon.</param>
        void FinishRender(SpriteHandle icon);

        /// <summary>
        /// Finalizes this request.
        /// </summary>
        void Dispose();

    private:
        bool __GetIsReady();
    };

} // SE

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SE.Editor
{
    /// <summary>
    /// Contains information about asset thumbnail rendering.
    /// </summary>
    public class ThumbnailRequest
    {
        /// <summary>
        /// The request state types.
        /// </summary>
        public enum States
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
        public States state = States.Created;

        /// <summary>
        /// The item.
        /// </summary>
        public AssetItem Item = null;

        /// <summary>
        /// The proxy object for the asset item.
        /// </summary>
        public AssetOperate operate = null;

        /// <summary>
        /// The asset reference. May be null if not cached yet.
        /// </summary>
        public Asset asset = null;

        /// <summary>
        /// The custom tag object used by the thumbnails rendering pipeline. Can be used to store the data related to the thumbnail rendering by the asset proxy.
        /// </summary>
        public object Tag;

        /// <summary>
        /// Determines whether thumbnail can be drawn for the item.
        /// </summary>
        public bool IsReady 
        {
            get 
            {
                return state == States.Prepared && asset && asset.IsLoaded() && operate.CanDrawThumbnail(this);
            }
        }


        /// <summary>
        /// Initializes a new instance of the <see cref="ThumbnailRequest"/> class.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <param name="proxy">The proxy.</param>
        public ThumbnailRequest(AssetItem item, AssetOperate proxy)
        {
            Item = item;
            operate = proxy;
        }

        public void Update()
        {
            if (state == States.Prepared && (!asset || asset.LastLoadFailed()))
            {
                state = States.Failed;
            }
        }

        /// <summary>
        /// Prepares this request.
        /// </summary>
        public void Prepare()
        {
            Debug.Assert(state == States.Created, "ThumbnailRequest is not in a created state.");

            asset = AssetContent.LoadAsync<Asset>(Item.Path);
            operate.OnThumbnailDrawPrepare(this);
            state = States.Prepared;
        }

        /// <summary>
        /// Finishes the rendering and updates the item thumbnail.
        /// </summary>
        /// <param name="icon">The icon.</param>
        public void FinishRender(ref SpriteHandle icon)
        {
            Debug.Assert(state == States.Prepared, "ThumbnailRequest is not in a prepared state.");

            Item.Thumbnail = icon;
            state = States.Rendered;
        }

        /// <summary>
        /// Finalizes this request.
        /// </summary>
        public void Dispose()
        {
            Debug.Assert(state != States.Disposed, "ThumbnailRequest is already disposed.");

            if (state != States.Created)
            {
                // Cleanup
                operate.OnThumbnailDrawCleanup(this);
                asset = null;
            }

            // Tag = null;
            state = States.Disposed;
        }
    };
}

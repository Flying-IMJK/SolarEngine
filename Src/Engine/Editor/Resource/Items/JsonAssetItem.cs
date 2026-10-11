using System;

namespace SE.Editor
{
    /// <summary>
    /// Asset item stored in a Json format file.
    /// </summary>
    public class JsonAssetItem : AssetItem
    {
        /// <summary>
        /// Asset icon.
        /// </summary>
        protected SpriteHandle _thumbnail;

        /// <summary>
        /// Initializes a new instance of the <see cref="JsonAssetItem"/> class.
        /// </summary>
        /// <param name="path">The path.</param>
        /// <param name="id">The identifier.</param>
        /// <param name="typeId">Name of the resource type.</param>
        public JsonAssetItem(string path, Guid id, Type typeId) : base(path, typeId, id)
        {
            _thumbnail = EditorIcons.Json128;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="JsonAssetItem"/> class.
        /// </summary>
        /// <param name="path">The path.</param>
        /// <param name="id">The identifier.</param>
        /// <param name="typeId">Name of the resource type.</param>
        /// <param name="thumbnail">Asset icon.</param>
        public JsonAssetItem(string path, Guid id, Type typeId, SpriteHandle thumbnail)
            : base(path, typeId, id)
        {
            _thumbnail = thumbnail;
        }

        public override SpriteHandle DefaultThumbnail => _thumbnail;

        public override bool HasDefaultThumbnail => true;

        public override ContentItemSearchFilter SearchFilter => ContentItemSearchFilter.Json;
    }
}

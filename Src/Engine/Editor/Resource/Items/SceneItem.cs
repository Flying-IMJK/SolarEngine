using System;

namespace SE.Editor
{
    /// <summary>
    /// Content item that contains data.
    /// </summary>
    public class SceneItem : JsonAssetItem
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="SceneItem"/> class.
        /// </summary>
        /// <param name="path">The asset path.</param>
        /// <param name="id">The asset identifier.</param>
        public SceneItem(string path, Guid id) : base(path, id, typeof(Scene))
        {
        }

        public override ContentItemType ItemType => ContentItemType.Scene;

        public override ContentItemSearchFilter SearchFilter => ContentItemSearchFilter.Scene;

        public override SpriteHandle DefaultThumbnail => EditorIcons.Scene128;

        /*/// <inheritdoc />
        public override string TypeDescription => "Scene";

        /// <inheritdoc />
        public override bool IsOfType(Type type)
        {
            return type.IsAssignableFrom(typeof(SceneAsset));
        }*/
    }
}

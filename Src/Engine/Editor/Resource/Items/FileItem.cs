

namespace SE.Editor
{
    /// <summary>
    /// Content item for the auxiliary files.
    /// </summary>
    public class FileItem : ContentItem
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="FileItem"/> class.
        /// </summary>
        /// <param name="path">The path to the file.</param>
        public FileItem(string path) : base(path)
        {
        }

        /// <inheritdoc />
        public override ContentItemType ItemType => ContentItemType.Other;

        /// <inheritdoc />
        public override ContentItemSearchFilter SearchFilter => ContentItemSearchFilter.Other;

        /// <inheritdoc />
        public override SpriteHandle DefaultThumbnail => EditorIcons.Document128;
    }
}

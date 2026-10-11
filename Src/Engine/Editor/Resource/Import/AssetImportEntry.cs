
namespace SE.Editor
{
    /// <summary>
    /// Asset import entry.
    /// </summary>
    public class AssetImportEntry : ImportFileEntry
    {
        /// <inheritdoc />
        public AssetImportEntry(ref Request request) : base(ref request)
        {
        }

        /// <inheritdoc />
        public override bool Import()
        {
            // Use engine backend
            return false;// Editor.Import(SourceUrl, ResultUrl);
        }
    }
}

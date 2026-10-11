using System;
using System.Collections.Generic;
using System.IO;

namespace SE.Editor
{
    /// <summary>
    /// Managed policy for a content type. Operations use C++ exported asset types
    /// and item paths across the interop edge.
    /// </summary>
    public abstract class ContentOperate
    {
        public virtual void Dispose()
        {
        }

        public abstract string Name { get; }
        public virtual string NewItemName => Name;
        public abstract string FileExtension { get; }
        public virtual bool IsAsset => false;
        public virtual bool CanExport => false;

        /// <summary>
        /// Gets the unique accent color for that asset type.
        /// </summary>
        public abstract Color AccentColor { get; }

        public abstract bool IsProxyFor(ContentItem item);
        public virtual bool IsProxyFor(Type typeId) => false;
        public virtual bool CanCreate(ContentFolder targetLocation) => false;
        public virtual bool CanReimport(ContentItem item) => false;
        public virtual bool IsFileNameValid(string filename) => !string.IsNullOrWhiteSpace(filename);
        public virtual void Create(string outputPath, object? argument = null)
        {

        }

        public virtual void Open(ContentItem item)
        {

        }

        public virtual void Export(ContentItem item, string outputPath)
        {

        }
    }
}

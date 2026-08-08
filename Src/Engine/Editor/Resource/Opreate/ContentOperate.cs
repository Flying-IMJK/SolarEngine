using System;
using System.IO;

namespace SE.Editor
{
    /// <summary>
    /// Managed policy for a content type. Operations never retain a native asset
    /// pointer; a stable type id and item path are passed across the interop edge.
    /// </summary>
    public abstract class ContentOperate
    {
        public abstract string Name { get; }
        public virtual string NewItemName => Name;
        public abstract string FileExtension { get; }
        public virtual bool IsAsset => false;
        public virtual bool CanExport => false;

        public abstract bool IsProxyFor(ContentItem item);
        public virtual bool IsProxyFor(string typeId) => false;
        public virtual bool CanCreate(ContentFolder targetLocation) => false;
        public virtual bool CanReimport(ContentItem item) => false;
        public virtual bool IsFileNameValid(string filename) => !string.IsNullOrWhiteSpace(filename);
        public virtual void Create(string outputPath, object? argument = null)
        {
            _ = outputPath;
            _ = argument;
        }

        public virtual void Open(ContentItem item)
        {
            _ = item;
        }

        public virtual void Export(ContentItem item, string outputPath)
        {
            _ = item;
            _ = outputPath;
        }
    }

    public abstract class AssetOperate : ContentOperate
    {
        public override bool IsAsset => true;
        public virtual bool IsVirtual => false;
        public abstract string AssetTypeId { get; }

        public override bool IsProxyFor(ContentItem item)
        {
            return item is AssetItem asset && IsProxyFor(asset.TypeId);
        }

        public override bool IsProxyFor(string typeId)
        {
            return string.Equals(typeId, AssetTypeId, StringComparison.Ordinal);
        }

        public virtual bool AcceptsAsset(string typeId, string path)
        {
            _ = path;
            return IsProxyFor(typeId);
        }

        public abstract AssetItem ConstructItem(string path, string typeId, Guid id);
        public virtual bool CanDrawThumbnail(ThumbnailRequest request) => true;
        public virtual void PrepareThumbnail(ThumbnailRequest request)
        {
            _ = request;
        }
    }

    public abstract class BinaryAssetOperate : AssetOperate
    {
        public override AssetItem ConstructItem(string path, string typeId, Guid id)
        {
            return new BinaryAssetItem(path, typeId, id);
        }
    }

    public sealed class ModelOperate : BinaryAssetOperate
    {
        public override string Name => "Model";
        public override string FileExtension => "model";
        public override string AssetTypeId => "Model";
        public override bool CanReimport(ContentItem item) => item is BinaryAssetItem;

        public override AssetItem ConstructItem(string path, string typeId, Guid id)
        {
            return new BinaryAssetItem(path, typeId, id);
        }
    }

    public sealed class TextureOperate : BinaryAssetOperate
    {
        public override string Name => "Texture";
        public override string FileExtension => "texture";
        public override string AssetTypeId => "Texture";
        public override bool CanReimport(ContentItem item) => item is BinaryAssetItem;

        public override AssetItem ConstructItem(string path, string typeId, Guid id)
        {
            return new BinaryAssetItem(path, typeId, id);
        }
    }

    public sealed class SceneOperate : AssetOperate
    {
        public const string Extension = "scene";
        public override string Name => "Scene";
        public override string FileExtension => Extension;
        public override string AssetTypeId => "SceneAsset";
        public override bool CanCreate(ContentFolder targetLocation) => targetLocation.CanHaveAssets;

        public override void Create(string outputPath, object? argument = null)
        {
            _ = argument;
            string? directory = Path.GetDirectoryName(outputPath);
            if (!string.IsNullOrEmpty(directory))
                Directory.CreateDirectory(directory);
            if (!File.Exists(outputPath))
                File.WriteAllText(outputPath, "{}");
        }

        public override AssetItem ConstructItem(string path, string typeId, Guid id)
        {
            return new SceneItem(path, id);
        }
    }
}

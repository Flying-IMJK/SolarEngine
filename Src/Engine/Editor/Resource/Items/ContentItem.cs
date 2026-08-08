using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using SE.GUI;

namespace SE.Editor
{
    public enum ContentItemType
    {
        Asset,
        Folder,
        Script,
        Scene,
        Other,
    }

    public enum ContentItemSearchFilter
    {
        Model,
        SkinnedModel,
        Material,
        Texture,
        Scene,
        Prefab,
        Script,
        Audio,
        Animation,
        Json,
        Particles,
        Shader,
        Other,
    }

    public enum ContentFolderType
    {
        Content,
        Source,
        Other,
    }

    public interface IContentItemOwner
    {
        void OnItemDeleted(ContentItem item);
        void OnItemRenamed(ContentItem item);
        void OnItemReimported(ContentItem item);
        void OnItemDispose(ContentItem item);
    }

    /// <summary>
    /// Managed presentation model for one entry in the editor content browser.
    /// It deliberately stores paths and managed state only; native assets are
    /// addressed through stable ids instead of raw C++ pointers.
    /// </summary>
    public abstract class ContentItem : Control
    {
        public const int DefaultMarginSize = 4;
        public const int DefaultTextHeight = 24;
        public const int DefaultThumbnailSize = 64;
        public const int DefaultWidth = DefaultThumbnailSize + 2 * DefaultMarginSize;
        public const int DefaultHeight = DefaultThumbnailSize + 2 * DefaultMarginSize + DefaultTextHeight;

        private readonly HashSet<IContentItemOwner> m_Owners = new();
        private bool m_MouseDown;
        private Float2 m_MouseDownStart;

        protected ContentItem(string path)
            : base(new Rectangle(0, 0, DefaultWidth, DefaultHeight))
        {
            UpdatePath(path);
        }

        public abstract ContentItemType ItemType { get; }
        public virtual ContentItemSearchFilter SearchFilter => ContentItemSearchFilter.Other;
        public bool IsAsset => ItemType == ContentItemType.Asset;
        public bool IsFolder => ItemType == ContentItemType.Folder;
        public virtual bool CanHaveChildren => IsFolder;
        public virtual bool CanRename => ParentFolder != null;
        public virtual bool CanDrag => ParentFolder != null;
        public virtual bool Exists => File.Exists(Path) || Directory.Exists(Path);
        public ContentFolder? ParentFolder { get; private set; }
        public string Path { get; private set; } = string.Empty;
        public string FileName { get; private set; } = string.Empty;
        public string ShortName { get; private set; } = string.Empty;
        public string NamePath { get; private set; } = string.Empty;
        public string Tooltip { get; protected set; } = string.Empty;
        public object? Thumbnail { get; internal set; }
        public bool ShowFileExtension { get; set; }
        public int ReferencesCount => m_Owners.Count;
        public virtual object? DefaultThumbnail => null;
        public bool HasDefaultThumbnail => DefaultThumbnail != null;
        public virtual string DisplayName => ShowFileExtension ? FileName : ShortName;

        public event Action<ContentItem>? DoubleClicked;
        public event Action<ContentItem>? Reloaded;

        public virtual void UpdatePath(string path)
        {
            ArgumentException.ThrowIfNullOrWhiteSpace(path);
            Path = path;
            FileName = System.IO.Path.GetFileName(path);
            ShortName = System.IO.Path.GetFileNameWithoutExtension(path);
            NamePath = ShortName;
            Name = DisplayName;
            UpdateTooltipText();
            OnPathChanged();
            Notify(static (owner, item) => owner.OnItemRenamed(item));
        }

        public virtual void RefreshThumbnail()
        {
            ThumbnailService.Instance.RequestPreview(this);
        }

        public virtual void UpdateTooltipText()
        {
            Tooltip = Path;
            TooltipText = Tooltip;
        }

        public virtual ContentItem? Find(string path)
        {
            return string.Equals(Path, path, StringComparison.OrdinalIgnoreCase) ? this : null;
        }

        public virtual bool Find(ContentItem item) => ReferenceEquals(this, item);

        public void AddReference(IContentItemOwner owner)
        {
            ArgumentNullException.ThrowIfNull(owner);
            m_Owners.Add(owner);
        }

        public void RemoveReference(IContentItemOwner owner)
        {
            ArgumentNullException.ThrowIfNull(owner);
            m_Owners.Remove(owner);
        }

        public virtual void OnDelete()
        {
            Notify(static (owner, item) => owner.OnItemDeleted(item));
            ParentFolder?.RemoveChild(this);
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button != MouseButton.Left)
                return base.OnMouseDown(location, button);

            m_MouseDown = true;
            m_MouseDownStart = location;
            FindContentView()?.OnItemClick(this);
            return true;
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            _ = location;
            if (button != MouseButton.Left || !m_MouseDown)
                return base.OnMouseUp(location, button);

            m_MouseDown = false;
            return true;
        }

        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            _ = location;
            if (button != MouseButton.Left)
                return base.OnMouseDoubleClick(location, button);

            FindContentView()?.OnItemDoubleClick(this);
            DoubleClicked?.Invoke(this);
            return true;
        }

        public override void OnMouseMove(Float2 location)
        {
            if (m_MouseDown && (location - m_MouseDownStart).LengthSquared > 16.0f)
                m_MouseDown = false;
            base.OnMouseMove(location);
        }

        protected virtual void OnPathChanged()
        {
        }

        protected virtual void OnReimport()
        {
            Reloaded?.Invoke(this);
            Notify(static (owner, item) => owner.OnItemReimported(item));
        }

        protected override void OnDispose()
        {
            ParentFolder?.RemoveChild(this);
            Notify(static (owner, item) => owner.OnItemDispose(item));
            m_Owners.Clear();
            base.OnDispose();
        }

        internal void SetParentFolder(ContentFolder? value)
        {
            ParentFolder = value;
            OnParentFolderChanged();
        }

        protected virtual void OnParentFolderChanged()
        {
        }

        protected void NotifyReimported() => OnReimport();

        private void Notify(Action<IContentItemOwner, ContentItem> callback)
        {
            var owners = m_Owners.ToArray();
            foreach (IContentItemOwner owner in owners)
            {
                callback(owner, this);
            }
        }

        private ContentView? FindContentView()
        {
            for (Control? control = Parent; control != null; control = control.Parent)
            {
                if (control is ContentView view)
                    return view;
            }

            return null;
        }
    }

    public class ContentFolder : ContentItem
    {
        private readonly List<ContentItem> m_Children = new();

        public ContentFolder(ContentFolderType folderType, string path)
            : base(path)
        {
            FolderType = folderType;
        }

        public override ContentItemType ItemType => ContentItemType.Folder;
        public ContentFolderType FolderType { get; }
        public bool CanHaveAssets => FolderType == ContentFolderType.Content;
        public ContentTreeNode? Node { get; internal set; }
        public IReadOnlyList<ContentItem> Children => m_Children;
        public override bool CanRename => ParentFolder != null;
        public override bool CanDrag => ParentFolder != null;

        public ContentItem AddChild(ContentItem item)
        {
            ArgumentNullException.ThrowIfNull(item);
            if (ReferenceEquals(item, this))
                throw new InvalidOperationException("A folder cannot contain itself.");
            if (item.ParentFolder == this)
                return item;

            item.ParentFolder?.RemoveChild(item);
            m_Children.Add(item);
            item.SetParentFolder(this);
            return item;
        }

        public bool RemoveChild(ContentItem item)
        {
            if (!m_Children.Remove(item))
                return false;

            item.SetParentFolder(null);
            return true;
        }

        public ContentItem? FindChild(string path)
        {
            return m_Children.Find(item => string.Equals(item.Path, path, StringComparison.OrdinalIgnoreCase));
        }

        public bool ContainsChild(string path) => FindChild(path) != null;

        public override void UpdatePath(string path)
        {
            string previousPath = Path;
            base.UpdatePath(path);
            if (string.IsNullOrEmpty(previousPath))
                return;

            foreach (ContentItem child in m_Children)
            {
                string relative = System.IO.Path.GetRelativePath(previousPath, child.Path);
                child.UpdatePath(System.IO.Path.Combine(path, relative));
            }
        }

        public override ContentItem? Find(string path)
        {
            ContentItem? direct = base.Find(path);
            if (direct != null)
                return direct;

            foreach (ContentItem child in m_Children)
            {
                ContentItem? result = child.Find(path);
                if (result != null)
                    return result;
            }

            return null;
        }

        public override bool Find(ContentItem item)
        {
            if (base.Find(item))
                return true;

            foreach (ContentItem child in m_Children)
            {
                if (child.Find(item))
                    return true;
            }

            return false;
        }

        protected override void OnDispose()
        {
            foreach (ContentItem child in m_Children.ToArray())
            {
                child.Dispose();
            }
            m_Children.Clear();
            base.OnDispose();
        }
    }

    public class AssetItem : ContentItem
    {
        public AssetItem(string path, string typeId, Guid id)
            : base(path)
        {
            TypeId = typeId ?? string.Empty;
            Id = id;
        }

        public override ContentItemType ItemType => ContentItemType.Asset;
        public Guid Id { get; }
        public string TypeId { get; }
        public bool IsLoaded { get; private set; }
        public override ContentItemSearchFilter SearchFilter => ContentItemSearchFilter.Other;

        public virtual void SetLoaded(bool value)
        {
            IsLoaded = value;
        }

        public virtual void Reload()
        {
            NotifyReimported();
        }
    }

    public class BinaryAssetItem : AssetItem
    {
        public BinaryAssetItem(string path, string typeId, Guid id, string? sourcePath = null)
            : base(path, typeId, id)
        {
            SourcePath = sourcePath;
        }

        public string? SourcePath { get; private set; }

        public bool TryGetImportPath(out string? path)
        {
            path = SourcePath;
            return !string.IsNullOrEmpty(path);
        }

        public void SetImportPath(string? path)
        {
            SourcePath = path;
        }
    }

    public sealed class FileItem : ContentItem
    {
        public FileItem(string path)
            : base(path)
        {
        }

        public override ContentItemType ItemType => ContentItemType.Other;
        public override ContentItemSearchFilter SearchFilter => ContentItemSearchFilter.Other;
    }

    public sealed class JsonAssetItem : AssetItem
    {
        public JsonAssetItem(string path, string typeId, Guid id)
            : base(path, typeId, id)
        {
        }

        public override ContentItemSearchFilter SearchFilter => ContentItemSearchFilter.Json;
    }

    public sealed class SceneItem : AssetItem
    {
        public SceneItem(string path, Guid id)
            : base(path, "SceneAsset", id)
        {
        }

        public override ContentItemType ItemType => ContentItemType.Scene;
        public override ContentItemSearchFilter SearchFilter => ContentItemSearchFilter.Scene;
    }

    public sealed class NewItem : ContentItem
    {
        public NewItem(string path)
            : base(path)
        {
        }

        public override ContentItemType ItemType => ContentItemType.Other;
        public override bool Exists => false;
        public override bool CanDrag => false;
    }
}

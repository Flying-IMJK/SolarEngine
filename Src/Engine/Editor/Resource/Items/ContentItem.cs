using SE.Editor.GUI;
using SE.GUI;

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace SE.Editor
{
    public enum ContentItemType
    {
        /// <summary>The binary or text asset.</summary>
        Asset,
        /// <summary>The directory.</summary>
        Folder,
        /// <summary>The script file.</summary>
        Script,
        /// <summary>The scene file.</summary>
        Scene,
        /// <summary>The other type.</summary>
        Other,
    }

    /// <summary>Content item filter types used for searching.</summary>
    public enum ContentItemSearchFilter
    {
        /// <summary>The model.</summary>
        Model,
        /// <summary>The skinned model.</summary>
        SkinnedModel,
        /// <summary>The material.</summary>
        Material,
        /// <summary>The texture.</summary>
        Texture,
        /// <summary>The scene.</summary>
        Scene,
        /// <summary>The prefab.</summary>
        Prefab,
        /// <summary>The script.</summary>
        Script,
        /// <summary>The audio.</summary>
        Audio,
        /// <summary>The animation.</summary>
        Animation,
        /// <summary>The json.</summary>
        Json,
        /// <summary>The particles.</summary>
        Particles,
        /// <summary>The shader source files.</summary>
        Shader,
        /// <summary>The other.</summary>
        Other,
    }

    public interface IContentItemOwner
    {
        /// <summary>Called when referenced item gets deleted. Item should not be used after that.</summary>
        void OnItemDeleted(ContentItem item);
        /// <summary>Called when referenced item gets renamed.</summary>
        void OnItemRenamed(ContentItem item);
        /// <summary>Called when item gets reimported or reloaded.</summary>
        void OnItemReimported(ContentItem item);
        /// <summary>Called when referenced item gets disposed. Item should not be used after that.</summary>
        void OnItemDispose(ContentItem item);
    }

    /// <summary>
    /// Content item in the editor content browser.
    /// </summary>
    public abstract class ContentItem : Control
    {
        /// <summary>The default margin size.</summary>
        public const int DefaultMarginSize = 4;
        /// <summary>The default text height.</summary>
        public const int DefaultTextHeight = 24;
        /// <summary>The default thumbnail size.</summary>
        public const int DefaultThumbnailSize = 64;
        /// <summary>The default width.</summary>
        public const int DefaultWidth = DefaultThumbnailSize + 2 * DefaultMarginSize;
        /// <summary>The default height.</summary>
        public const int DefaultHeight = DefaultThumbnailSize + 2 * DefaultMarginSize + DefaultTextHeight;

        private ContentFolder m_ParentFolder;
        private bool m_IsMouseDown;
        private Float2 m_MouseDownStartPos;
        private readonly List<IContentItemOwner> m_References = new(4);
        private SpriteHandle m_ShadowIcon = SpriteHandle.Invalid;

        /// <summary>Gets the type of the item.</summary>
        public abstract ContentItemType ItemType { get; }
        /// <summary>Gets the type of the item searching filter to use.</summary>
        public abstract ContentItemSearchFilter SearchFilter { get; }
        /// <summary>Gets a value indicating whether this instance is asset.</summary>
        public bool IsAsset => ItemType == ContentItemType.Asset;
        /// <summary>Gets a value indicating whether this instance is folder.</summary>
        public bool IsFolder => ItemType == ContentItemType.Folder;
        /// <summary>Gets a value indicating whether this instance can have children.</summary>
        public bool CanHaveChildren => ItemType == ContentItemType.Folder;
        /// <summary>Determines whether this item can be renamed.</summary>
        public virtual bool CanRename => true;
        /// <summary>Gets a value indicating whether this item can be dragged and dropped.</summary>
        public virtual bool CanDrag => Root != null;
        /// <summary>Gets a value indicating whether this item exists on drive.</summary>
        public virtual bool Exists => File.Exists(Path);
        /// <summary>Gets the parent folder.</summary>
        public ContentFolder ParentFolder
        {
            get => m_ParentFolder;
            set
            {
                if (m_ParentFolder == value)
                {
                    return;
                }

                if (m_ParentFolder != null)
                {
                    m_ParentFolder.Children.Remove(this);
                }

                m_ParentFolder = value;

                if (m_ParentFolder != null)
                {
                    m_ParentFolder.Children.Add(this);
                }

                OnParentFolderChanged();
            }
        }

        /// <summary>Gets the path to the item.</summary>
        public string Path;
        /// <summary>Gets the item file name (filename with extension).</summary>
        public string FileName;
        /// <summary>Gets the item short name (filename without extension).</summary>
        public string ShortName;
        /// <summary>Gets the asset name relative to the project root folder (without asset file extension).</summary>
        public string NamePath = string.Empty;
        /// <summary>Gets the default name of the content item thumbnail. Returns null if not used.</summary>
        public virtual SpriteHandle DefaultThumbnail => SpriteHandle.Invalid;
        /// <summary>Gets a value indicating whether this item has default thumbnail.</summary>
        public virtual bool HasDefaultThumbnail => false;
        /// <summary>
        /// Gets or sets the item thumbnail. Warning, thumbnail may not be available if item has no references.
        /// </summary>
        public SpriteHandle Thumbnail = SpriteHandle.Invalid;
        /// <summary>True if force show file extension.</summary>
        public bool ShowFileExtension;

        /// <summary>
        /// Updates the item path. Use with caution or even don't use it. It's dangerous.
        /// </summary>
        /// <param name="value">The new path.</param>
        public virtual void UpdatePath(string value)
        {
            value = value.Replace('\\', '/');
            if (value.Length > 2
                && ((value[0] >= 'A' && value[0] <= 'Z') || (value[0] >= 'a' && value[0] <= 'z'))
                && value[1] == ':')
            {
                value = value.Substring(0, 2) + '/' + value.Substring(3);
            }

            Path = value;
            FileName = System.IO.Path.GetFileName(value);
            ShortName = System.IO.Path.GetFileNameWithoutExtension(value);

            OnPathChanged();
            for (int i = 0; i < m_References.Count; i++)
            {
                m_References[i].OnItemRenamed(this);
            }
        }

        /// <summary>Refreshes the item thumbnail.</summary>
        public virtual void RefreshThumbnail()
        {
            if (HasDefaultThumbnail)
            {
                return;
            }

            Thumbnails thumbnails = Editor.Instance.UI.Thumbnails;

            // Delete old thumbnail and remove it from the cache
            thumbnails.DeletePreview(this);

            // Request new one (if need to)
            if (m_References.Count() > 0)
            {
                thumbnails.RequestPreview(this);
            }
        }

        /// <summary>Updates the tooltip text text.</summary>
        public virtual void UpdateTooltipText()
        {
            // Native tooltip text builder is commented out.
        }

        /// <summary>Tries to find the item at the specified path.</summary>
        /// <param name="path">The path.</param>
        /// <returns>Found item or null if missing.</returns>
        public virtual ContentItem Find(string path)
        {
            return Path == path ? this : null;
        }

        /// <summary>Tries to find a specified item in the assets tree.</summary>
        /// <param name="item">The item.</param>
        /// <returns>True if found, otherwise false.</returns>
        public virtual bool Find(ContentItem item)
        {
            return this == item;
        }

        /// <summary>Tries to find the item with the specified id.</summary>
        /// <param name="id">The id.</param>
        /// <returns>Found item or null if missing.</returns>
        public virtual ContentItem Find(Guid id)
        {
            return null;
        }

        /// <summary>Gets the local space rectangle for element name text area.</summary>
        public Rectangle TextRectangle
        {
            get
            {
                if (!Visible)
                {
                    return Rectangle.Empty;
                }

                ContentView view = Parent as ContentView;
                Float2 size = Size;
                ContentViewType viewType = ContentViewType.Tiles;
                if (view != null)
                {
                    viewType = view.ViewType;
                }

                switch (viewType)
                {
                    case ContentViewType.Tiles:
                        {
                            float textHeight = DefaultTextHeight * size.X / DefaultWidth;
                            return new Rectangle(0, size.Y - textHeight, size.X, textHeight);
                        }
                    case ContentViewType.List:
                        {
                            float thumbnailSize = size.Y - 2 * DefaultMarginSize;
                            float textHeight = Math.Min(size.Y, 24.0f);
                            return new Rectangle(thumbnailSize + DefaultMarginSize * 2,
                                (size.Y - textHeight) * 0.5f,
                                size.X - textHeight - DefaultMarginSize * 3.0f, textHeight);
                        }
                }

                return Rectangle.Empty;
            }
        }

        /// <summary>Draws the item thumbnail.</summary>
        /// <param name="rectangle">The thumbnail rectangle.</param>
        public void DrawThumbnail(Rectangle rectangle)
        {
            // Native DrawShadow branch is commented out.
            if (Thumbnail.IsValid)
            {
                Render2D.DrawSprite(Thumbnail, rectangle, Color.White);
            }
            else
            {
                Render2D.FillRectangle(rectangle, Color.Black);
            }
        }

        /// <summary>Draws the item thumbnail.</summary>
        /// <param name="rectangle">The thumbnail rectangle.</param>
        /// <param name="shadow">Whether or not to draw the shadow.</param>
        public void DrawThumbnail(Rectangle rectangle, bool shadow)
        {
            if (shadow)
            {
                const float thumbnailInShadowSize = 50.0f;
                Rectangle shadowRect = rectangle.MakeExpanded(
                    (DefaultThumbnailSize - thumbnailInShadowSize) * rectangle.Width / DefaultThumbnailSize * 1.3f);
                if (!m_ShadowIcon.IsValid)
                {
                    m_ShadowIcon = EditorIcons.AssetShadow128;
                }
                Render2D.DrawSprite(m_ShadowIcon, shadowRect, Color.White);
            }

            if (Thumbnail.IsValid)
            {
                Render2D.DrawSprite(Thumbnail, rectangle, Color.White);
            }
            else
            {
                Render2D.FillRectangle(rectangle, Color.Black);
            }
        }

        /// <summary>Gets the amount of references to that item.</summary>
        public int ReferencesCount => m_References.Count;

        /// <summary>Adds the reference to the item.</summary>
        /// <param name="obj">The object.</param>
        public void AddReference(IContentItemOwner obj)
        {
            SE.Debug.Assert(obj != null, "Invalid content item owner.");
            SE.Debug.Assert(!m_References.Contains(obj), "Content item owner already exists.");
            m_References.Add(obj);

            if (m_References.Count == 1 && !Thumbnail.IsValid)
            {
                RequestThumbnail();
            }
        }

        /// <summary>Removes the reference from the item.</summary>
        /// <param name="obj">The object.</param>
        public void RemoveReference(IContentItemOwner obj)
        {
            if (m_References.Remove(obj))
            {
                if (m_References.Count == 0 && Thumbnail.IsValid)
                {
                    ReleaseThumbnail();
                }
            }
        }

        /// <summary>Called when context menu is being prepared to show. Can be used to add custom options.</summary>
        /// <param name="menu">The menu.</param>
        public virtual void OnContextMenu(ContextMenu menu)
        {
        }

        /// <summary>Called when item gets renamed or location gets changed (path modification).</summary>
        public virtual void OnPathChanged()
        {
        }

        /// <summary>Called when content item gets removed (by the user or externally).</summary>
        public virtual void OnDelete()
        {
            while (m_References.Count > 0)
            {
                IContentItemOwner reference = m_References[0];
                reference.OnItemDeleted(this);
                RemoveReference(reference);
            }

            if (Thumbnail.IsValid)
            {
                ReleaseThumbnail();
            }
        }

        /// <inheritdoc />
        public override void NavigationFocus()
        {
            base.NavigationFocus();
            if (IsFocused)
            {
                ContentView view = Parent as ContentView;
                if (view != null)
                {
                    view.Select(this);
                }
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            Float2 size = Size;
            Style style = Style.Current;
            ContentView view = Parent as ContentView;
            if (view == null)
            {
                return;
            }

            bool isSelected = view.IsSelected(this);
            Rectangle clientRect = new Rectangle(Float2.Zero, size);
            Rectangle textRect = TextRectangle;
            Rectangle thumbnailRect = Rectangle.Empty;
            TextAlignment nameAlignment = TextAlignment.Near;

            switch (view.ViewType)
            {
                case ContentViewType.Tiles:
                    {
                        float thumbnailSize = size.X;
                        thumbnailRect = new Rectangle(0, 0, thumbnailSize, thumbnailSize);
                        nameAlignment = TextAlignment.Center;

                        if (this is ContentFolder)
                        {
                            Rectangle shadowRect = new Rectangle(2, 2, clientRect.Width + 1, clientRect.Height + 1);
                            Color color = Color.Black.AlphaMultiplied(0.2f);
                            Render2D.FillRectangle(shadowRect, color);
                            Render2D.FillRectangle(clientRect, style.Background.RGBMultiplied(1.25f));

                            float accentHeight = 2 * view.ViewScale;
                            Rectangle barRect = new Rectangle(0, thumbnailRect.Height - accentHeight, clientRect
                                .Width, accentHeight);
                            Render2D.FillRectangle(barRect, Color.DimGray);

                            if (isSelected)
                            {
                                Render2D.FillRectangle(clientRect,
                                    Parent.ContainsFocus ? style.BackgroundSelected : style.LightBackground);
                            }
                            else if (IsMouseOver)
                            {
                                Render2D.FillRectangle(clientRect, style.BackgroundHighlighted);
                            }

                            DrawThumbnail(thumbnailRect, false);
                        }
                        else
                        {
                            Rectangle shadowRect = new Rectangle(2, 2, clientRect.Width + 1, clientRect.Height + 1);
                            Color color = Color.Black.AlphaMultiplied(0.2f);
                            Render2D.FillRectangle(shadowRect, color);
                            Render2D.FillRectangle(clientRect, style.Background.RGBMultiplied(1.25f));
                            Render2D.FillRectangle(TextRectangle, style.LightBackground);
                            DrawThumbnail(thumbnailRect, false);

                            float accentHeight = 2 * view.ViewScale;
                            Rectangle barRect = new Rectangle(0, thumbnailRect.Height - accentHeight, clientRect
                                .Width, accentHeight);
                            Render2D.FillRectangle(barRect, Color.DimGray);

                            if (isSelected)
                            {
                                Render2D.FillRectangle(textRect,
                                    Parent.ContainsFocus ? style.BackgroundSelected : style.LightBackground);
                                Render2D.DrawRectangle(clientRect,
                                    Parent.ContainsFocus ? style.BackgroundSelected : style.LightBackground, 1.0f);
                            }
                            else if (IsMouseOver)
                            {
                                Render2D.FillRectangle(textRect, style.BackgroundHighlighted);
                                Render2D.DrawRectangle(clientRect, style.BackgroundHighlighted, 1.0f);
                            }
                        }
                        break;
                    }
                case ContentViewType.List:
                    {
                        float thumbnailSize = size.Y - 2 * DefaultMarginSize;
                        thumbnailRect = new Rectangle(DefaultMarginSize, DefaultMarginSize, thumbnailSize,
                            thumbnailSize);
                        nameAlignment = TextAlignment.Near;

                        if (isSelected)
                        {
                            Render2D.FillRectangle(clientRect,
                                Parent.ContainsFocus ? style.BackgroundSelected : style.LightBackground);
                        }
                        else if (IsMouseOver)
                        {
                            Render2D.FillRectangle(clientRect, style.BackgroundHighlighted);
                        }

                        DrawThumbnail(thumbnailRect);
                        break;
                    }
            }

            Render2D.PushClip(textRect);
            Render2D.RenderText(style.FontMedium,
                ShowFileExtension || view.ShowFileExtensions ? FileName : ShortName,
                textRect, style.TextColor, nameAlignment, TextAlignment.Center,
                TextWrapping.WrapWords, 1.0f, 0.95f);
            Render2D.PopClip();
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            Focus();
            if (button == MouseButton.Left)
            {
                m_IsMouseDown = true;
                m_MouseDownStartPos = location;
            }
            return true;
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && m_IsMouseDown)
            {
                m_IsMouseDown = false;

                // Mirrors the native TypeTryCast<ContentView>(this) expression.
                ContentView otherItem = (object)this as ContentView;
                if (otherItem != null)
                {
                    otherItem.OnItemClick(this);
                }
            }

            return base.OnMouseUp(location, button);
        }

        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            Focus();
            ContentView otherItem = Parent as ContentView;
            if (otherItem != null)
            {
                otherItem.OnItemDoubleClick(this);
            }
            return true;
        }

        public override void OnMouseMove(Float2 location)
        {
            if (m_IsMouseDown && Float2.Distance(m_MouseDownStartPos, location) > 10.0f)
            {
                m_IsMouseDown = false;
                DoDrag();
            }
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            if (m_IsMouseDown)
            {
                m_IsMouseDown = false;
                DoDrag();
            }

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override void OnSubmit()
        {
            // Native asset opening branch is commented out.
            base.OnSubmit();
        }

        /// <inheritdoc />
        public override int CompareTo(Control other)
        {
            if (other is ContentItem otherItem)
            {
                if (otherItem.IsFolder)
                {
                    return 1;
                }
                return string.CompareOrdinal(ShortName, otherItem.ShortName);
            }

            return base.CompareTo(other);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            while (m_References.Count > 0)
            {
                IContentItemOwner reference = m_References[0];
                reference.OnItemDispose(this);
                RemoveReference(reference);
            }

            if (Thumbnail.IsValid)
            {
                ReleaseThumbnail();
            }

            base.OnDestroy();
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ContentItem"/> class.
        /// </summary>
        /// <param name="path">The path to the item.</param>
        protected ContentItem(string path)
            : base(new Rectangle(0, 0, DefaultWidth, DefaultHeight))
        {
            Path = path;
            FileName = System.IO.Path.GetFileName(path);
            ShortName = System.IO.Path.GetFileNameWithoutExtension(path);
        }

        /// <summary>Called when item parent folder gets changed.</summary>
        protected virtual void OnParentFolderChanged()
        {
        }

        /// <summary>Requests the thumbnail.</summary>
        protected void RequestThumbnail()
        {
            Editor.Instance.UI.Thumbnails.RequestPreview(this);
        }

        /// <summary>Releases the thumbnail.</summary>
        protected void ReleaseThumbnail()
        {
            Thumbnail = SpriteHandle.Invalid;
        }

        /// <summary>Called when item gets reimported or reloaded.</summary>
        protected virtual void OnReimport()
        {
            for (int i = 0; i < m_References.Count; i++)
            {
                m_References[i].OnItemReimported(this);
            }
            RefreshThumbnail();
        }

        /// <summary>Does the drag and drop operation with this asset.</summary>
        protected virtual void DoDrag()
        {
            if (!CanDrag)
            {
                return;
            }

            DragData data;
            if (Parent is ContentView view && view.IsSelected(this))
            {
                data = DragItems.GetDragData(view.GetSelection());
            }
            else
            {
                data = DragItems.GetDragData(this);
            }
            DoDragDrop(data);
        }

        /// <inheritdoc />
        protected override bool ShowTooltip()
        {
            return true;
        }

        /// <summary>Called when building tooltip text.</summary>
        /// <param name="sb">The output string builder.</param>
        protected virtual void OnBuildTooltipText(StringBuilder sb)
        {
            // Native tooltip text building is commented out.
        }
    }

}

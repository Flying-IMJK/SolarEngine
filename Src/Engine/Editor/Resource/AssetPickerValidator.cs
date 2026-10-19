// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace SE.Editor
{

    /// <summary>
    /// Manages and converts the selected content item to the appropriate types. Useful for drag operations.
    /// </summary>
    public class AssetPickerValidator : IContentItemOwner
    {
        private Asset m_Selected;
        private ContentItem m_SelectedItem;
        private Type _type;
        private string m_FileExtension;

        /// <summary>
        /// Gets or sets the selected item.
        /// </summary>
        public ContentItem SelectedItem
        {
            get => m_SelectedItem;
            set
            {
                if (m_SelectedItem == value)
                {
                    return;
                }
                if (value == null)
                {
                    if (m_Selected == null && m_SelectedItem is SceneItem)
                    {
                        // Deselect scene reference
                        m_SelectedItem.RemoveReference(this);
                        m_SelectedItem = null;
                        m_Selected = null;
                        OnSelectedItemChanged();
                        return;
                    }

                    // Deselect
                    m_SelectedItem?.RemoveReference(this);
                    m_SelectedItem = null;
                    m_Selected = null;
                    OnSelectedItemChanged();
                }
                else if (value is SceneItem item)
                {
                    if (m_SelectedItem == item)
                    {
                        return;
                    }
                    if (!IsValid(item))
                    {
                        item = null;
                    }

                    // Change value to scene reference (cannot load asset because scene can be already loaded -
                    // duplicated ID issue)
                    m_SelectedItem?.RemoveReference(this);
                    m_SelectedItem = item;
                    m_Selected = null;
                    m_SelectedItem?.AddReference(this);
                    OnSelectedItemChanged();
                }
                else if (value is AssetItem assetItem)
                {
                    if (!IsValid(assetItem))
                    {
                        return;
                    }
                    Asset loaded = AssetContent.LoadAsync<Asset>(assetItem.Id);
                    if (loaded == null || _type != null && !_type.IsInstanceOfType(loaded))
                    {
                        SE.Debug.Log($"Cannot select asset '{assetItem.Path}' as {_type?.FullName}.");
                        return;
                    }
                    SelectedAsset = loaded;
                }
                else
                {
                    // Change value
                    m_SelectedItem?.RemoveReference(this);
                    m_SelectedItem = value;
                    m_Selected = null;
                    OnSelectedItemChanged();
                }
            }
        }

        /// <summary>
        /// Gets or sets the selected asset identifier.
        /// </summary>
        public Guid SelectedID
        {
            get
            {
                if (m_Selected != null)
                {
                    return m_Selected.ID;
                }
                if (m_SelectedItem is AssetItem assetItem)
                {
                    return assetItem.Id;
                }
                return Guid.Empty;
            }
            set => SelectedItem = Editor.Instance.AssetDatabase.FindAsset(value);
        }

        /// <summary>
        /// Gets or sets the selected content item path.
        /// </summary>
        public string SelectedPath
        {
            get => m_SelectedItem?.Path ?? m_Selected?.GetPath();
            set
            {
                SelectedItem = string.IsNullOrEmpty(value) ? null : Editor.Instance.AssetDatabase.Find(System.IO
                                .Path.GetFullPath(value, EngineContext.ProjectFolder));
            }
        }

        /// <summary>
        /// Gets or sets the selected asset object.
        /// </summary>
        public Asset SelectedAsset
        {
            get => m_Selected;
            set
            {
                // Check if value won't change
                if (value == m_Selected)
                {
                    return;
                }
                if (value != null && _type != null && !_type.IsInstanceOfType(value))
                {
                    return;
                }

                // Find item from content database and check it
                AssetItem item = value ? Editor.Instance.AssetDatabase.FindAsset(value.ID) : null;
                if (item != null && !IsValid(item))
                {
                    item = null;
                }

                // Change value
                m_SelectedItem?.RemoveReference(this);
                m_SelectedItem = item;
                m_Selected = value;
                m_SelectedItem?.AddReference(this);
                OnSelectedItemChanged();
            }
        }

        /// <summary>
        /// Gets or sets the assets types that this picker accepts (it supports types derived from the given type). Use
        /// <see cref="null"/> for generic file picker.
        /// </summary>
        public Type AssetType
        {
            get => _type;
            set
            {
                if (_type != value)
                {
                    _type = value;

                    // Auto deselect if the current value is invalid
                    if (m_SelectedItem != null && !IsValid(m_SelectedItem))
                    {
                        SelectedItem = null;
                    }
                }
            }
        }

        /// <summary>
        /// Gets or sets the content items extensions filter. Null if unused.
        /// </summary>
        public string FileExtension
        {
            get => m_FileExtension;
            set
            {
                if (m_FileExtension != value)
                {
                    m_FileExtension = value;

                    // Auto deselect if the current value is invalid
                    if (m_SelectedItem != null && !IsValid(m_SelectedItem))
                    {
                        SelectedItem = null;
                    }
                }
            }
        }

        /// <summary>
        /// Occurs when selected item gets changed.
        /// </summary>
        public event Action SelectedItemChanged;

        /// <summary>
        /// The custom callback for assets validation. Cane be used to implement a rule for assets to pick.
        /// </summary>
        public Func<ContentItem, bool> CheckValid;

        /// <summary>
        /// Returns whether item is valid.
        /// </summary>
        /// <param name="item"></param>
        /// <returns></returns>
        public bool IsValid(ContentItem item)
        {
            if (m_FileExtension != null && !item.Path.EndsWith(m_FileExtension))
            {
                return false;
            }
            if (CheckValid != null && !CheckValid(item))
            {
                return false;
            }
            if (_type == null)
            {
                return true;
            }

            if (item is AssetItem assetItem)
            {
                // SolarEngine stores the CLR asset type directly on the content item.
                if (_type.IsAssignableFrom(assetItem.TypeId))
                {
                    return true;
                }
                if (assetItem is JsonAssetItem && (_type == typeof(JsonAsset) || _type == typeof(Asset)))
                {
                    return true;
                }

            }

            return false;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AssetPickerValidator"/> class.
        /// </summary>
        public AssetPickerValidator()
        : this(typeof(Asset))
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AssetPickerValidator"/> class.
        /// </summary>
        /// <param name="assetType">The asset types that this picker accepts.</param>
        public AssetPickerValidator(Type assetType)
        {
            _type = assetType;
        }

        /// <summary>
        /// Called when selected item gets changed.
        /// </summary>
        protected virtual void OnSelectedItemChanged()
        {
            SelectedItemChanged?.Invoke();
        }

        /// <inheritdoc />
        public void OnItemDeleted(ContentItem item)
        {
            // Deselect item
            SelectedItem = null;
        }

        /// <inheritdoc />
        public void OnItemRenamed(ContentItem item)
        {
        }

        /// <inheritdoc />
        public void OnItemReimported(ContentItem item)
        {
        }

        /// <inheritdoc />
        public void OnItemDispose(ContentItem item)
        {
            // Deselect item
            SelectedItem = null;
        }

        /// <summary>
        /// Call to remove reference from the selected item.
        /// </summary>
        public void OnDestroy()
        {
            m_SelectedItem?.RemoveReference(this);
            m_SelectedItem = null;
            m_Selected = null;
        }
    }

}

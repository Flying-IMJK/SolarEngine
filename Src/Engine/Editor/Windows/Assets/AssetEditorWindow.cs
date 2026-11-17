using System;

using SE.Editor.GUI;
using SE.GUI;

namespace SE.Editor
{
    /// <summary>Common contract for an editor object with unsaved changes.</summary>
    public interface IEditable
    {
        bool IsEdited { get; }
        event Action OnEdited;
        void MarkAsEdited();
    }

    /// <summary>Owns the content item, edit state and lifetime of an asset editor.</summary>
    public abstract class AssetEditorWindow : EditorWindow, IEditable, IContentItemOwner
    {
        protected AssetItem m_Item;
        protected readonly ToolStrip m_Toolstrip;
        private bool m_IsEdited;

        public AssetItem Item => m_Item;
        public ToolStrip ToolStrip => m_Toolstrip;
        public override string SerializationTypename => m_Item.Id.ToString();
        protected virtual string WindowTitleName => "{0}";
        public event Action OnEdited;

        protected AssetEditorWindow(Editor editor, AssetItem item) : base(editor, false, ScrollBars.None)
        {
            m_Item = item ?? throw new ArgumentNullException(nameof(item));
            m_Item.AddReference(this);
            Name = item.Id.ToString();
            m_Toolstrip = new ToolStrip(24.0f) { Parent = this };
            m_Toolstrip.AddButton("Show in Content", () => Editor.Windows.ContentWin.Select(m_Item));
            UpdateTitle();
        }

        public bool IsEdited
        {
            get => m_IsEdited;
            protected set
            {
                if (value)
                {
                    MarkAsEdited();
                }
                else
                {
                    ClearEditedFlag();
                }
            }
        }

        public void MarkAsEdited()
        {
            if (!m_IsEdited)
            {
                m_IsEdited = true;
                OnEditedState();
                OnEdited?.Invoke();
                OnEditedStateChanged();
            }
        }

        protected void ClearEditedFlag()
        {
            if (m_IsEdited)
            {
                m_IsEdited = false;
                OnEditedStateChanged();
            }
        }

        protected virtual void OnEditedState()
        {
        }

        protected virtual void OnEditedStateChanged()
        {
            UpdateTitle();
            UpdateToolstrip();
        }

        protected void UpdateTitle()
        {
            Title = string.Format(WindowTitleName, m_Item?.ShortName ?? string.Empty) + (IsEdited ? "*" : string.Empty);
        }

        protected virtual void UpdateToolstrip()
        {
        }

        public virtual void Save()
        {
        }

        public override bool IsEditingItem(ContentItem item) => item == m_Item;

        protected override bool OnClosing(ClosingReason reason)
        {
            if (reason == ClosingReason.User && IsEdited && m_Item != null)
            {
                DialogResult result = MessageBox.Show($"Save changes to '{m_Item.Path}'?", "Save before closing?", MessageBoxButtons.YesNoCancel);
                if (result == DialogResult.Yes || result == DialogResult.OK)
                {
                    Save();
                    if (IsEdited)
                    {
                        return true;
                    }
                }
                else if (result == DialogResult.Cancel || result == DialogResult.Abort)
                {
                    return true;
                }
            }
            return base.OnClosing(reason);
        }

        protected virtual void UnlinkItem()
        {
            m_Item.RemoveReference(this);
            m_Item = null;
        }

        protected override void OnClose()
        {
            if (m_Item != null)
            {
                UnlinkItem();
            }
            base.OnClose();
        }

        public override void OnDestroy()
        {
            if (IsDisposing)
            {
                return;
            }
            if (m_Item != null)
            {
                UnlinkItem();
            }
            OnEdited = null;
            base.OnDestroy();
        }

        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (key == KeyboardKeys.S && Root != null && Root.GetKey(KeyboardKeys.Control))
            {
                Save();
                return true;
            }
            return base.OnKeyDown(key);
        }

        public override void OnShowContextMenu(ContextMenu menu)
        {
            base.OnShowContextMenu(menu);
            menu.AddButton("Save", Save).Enabled = IsEdited;
            menu.AddButton("Copy name", () => Clipboard.Text = Item.NamePath);
            menu.AddSeparator();
        }

        public void OnItemDeleted(ContentItem item)
        {
            if (item == m_Item)
            {
                Close();
            }
        }

        public void OnItemRenamed(ContentItem item)
        {
            if (item == m_Item)
            {
                UpdateTitle();
            }
        }

        public virtual void OnItemReimported(ContentItem item)
        {
        }

        public void OnItemDispose(ContentItem item)
        {
            if (item == m_Item)
            {
                Close();
            }
        }
    }

    /// <summary>Loads the typed asset and dispatches its loading lifecycle to the editor.</summary>
    public abstract class AssetEditorWindowBase<T> : AssetEditorWindow where T : Asset
    {
        protected T m_Asset;
        protected bool m_IsWaitingForLoaded;
        protected bool m_IsImporting;
        protected bool m_LastImportFailed;
        private volatile int m_ImportResult;

        public T Asset => m_Asset;

        protected AssetEditorWindowBase(Editor editor, AssetItem item) : base(editor, item)
        {
            AssetContent.AssetReloading += OnAssetReloadingInternal;
            Editor.Importing.ImportFileEnd += OnImportFileEnd;
        }

        protected virtual T LoadAsset() => AssetContent.LoadAsync<T>(m_Item.Id);

        protected virtual void OnAssetLinked()
        {
        }

        protected virtual void OnAssetLoaded()
        {
        }

        protected virtual void OnAssetLoadFailed()
        {
        }

        /// <summary>Ends consumers of old native data before Solar unloads an asset.</summary>
        protected virtual void OnAssetReloading()
        {
        }

        public void RefreshAsset()
        {
            if (m_Asset == null || !m_Asset.WaitForLoaded())
            {
                return;
            }
            OnAssetLoaded();
            MarkAsEdited();
            Save();
        }

        public void ReloadAsset()
        {
            if (m_Asset != null)
            {
                m_Asset.Reload();
                m_IsWaitingForLoaded = true;
            }
        }

        protected override void OnShow()
        {
            if (m_Asset == null && m_Item != null)
            {
                m_Asset = LoadAsset();
                if (m_Asset == null)
                {
                    Debug.LogWarning($"Cannot load asset '{m_Item.Path}' ({typeof(T)}).");
                    Close();
                    return;
                }
                OnAssetLinked();
                m_IsWaitingForLoaded = true;
            }
            base.OnShow();
            UpdateTitle();
            UpdateToolstrip();
            PerformLayout();
        }

        public override void OnUpdate()
        {
            int result = m_ImportResult;
            if (result != 0)
            {
                m_ImportResult = 0;
                m_IsImporting = false;
                m_LastImportFailed = result == 2;
                m_IsWaitingForLoaded = true;
            }
            if (m_IsWaitingForLoaded && !m_IsImporting)
            {
                if (m_Asset == null)
                {
                    m_IsWaitingForLoaded = false;
                }
                else if (m_Asset.IsLoaded())
                {
                    m_IsWaitingForLoaded = false;
                    OnAssetLoaded();
                }
                else if (m_Asset.LastLoadFailed())
                {
                    m_IsWaitingForLoaded = false;
                    OnAssetLoadFailed();
                }
            }
            base.OnUpdate();
        }

        private void OnAssetReloadingInternal(Asset asset)
        {
            if (asset == m_Asset)
            {
                OnAssetReloading();
                m_IsWaitingForLoaded = true;
            }
        }

        private void OnImportFileEnd(IFileEntryAction entry, bool failed)
        {
            // Import callbacks can run on a worker; dispatch GUI work from OnUpdate.
            AssetItem item = m_Item;
            if (item != null && string.Equals(entry.ResultUrl, item.Path, StringComparison.OrdinalIgnoreCase))
            {
                m_ImportResult = failed ? 2 : 1;
            }
        }

        public override void OnItemReimported(ContentItem item)
        {
            if (item == m_Item)
            {
                m_IsWaitingForLoaded = true;
            }
            base.OnItemReimported(item);
        }

        protected override void UnlinkItem()
        {
            AssetContent.AssetReloading -= OnAssetReloadingInternal;
            Editor.Importing.ImportFileEnd -= OnImportFileEnd;
            m_Asset = null;
            base.UnlinkItem();
        }
    }
}

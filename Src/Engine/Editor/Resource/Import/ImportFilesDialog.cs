

using SE.Editor.GUI;
using SE.GUI;

using System;
using System.Collections.Generic;
using System.IO;

namespace SE.Editor
{
    /// <summary>
    /// Dialog used to edit import files settings.
    /// </summary>
    /// <seealso cref="SE.Editor.GUI.Dialog" />
    public class ImportFilesDialog : Dialog
    {
        private TreeNode m_RootNode;
        private CustomEditorPresenter m_SettingsEditor;
        private Tree m_Tree;

        /// <summary>
        /// Gets the entries count.
        /// </summary>
        public int EntriesCount
        {
            get
            {
                int result = 0;
                for (int i = 0; i < m_RootNode.ChildrenCount; i++)
                {
                    if (m_RootNode.Children[i].Tag is ImportFileEntry)
                    {
                        result++;
                    }
                }
                return result;
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ImportFilesDialog"/> class.
        /// </summary>
        /// <param name="entries">The entries to edit settings.</param>
        public ImportFilesDialog(List<ImportFileEntry> entries) : base("Import files settings")
        {
            if (entries == null || entries.Count < 1)
            {
                throw new ArgumentNullException();
            }

            const float totalWidth = 920;
            const float editorHeight = 450;
            Width = totalWidth;

            // Header and help description
            Label headerLabel = new Label
            {
                Text = "Import settings",
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = new Margin(0, 0, 0, 40),
                Parent = this,
                Font = new FontReference(Style.Current.FontTitle)
            };
            Label infoLabel = new Label
            {
                Text = "Specify options for importing files. Every file can have different settings. Select entries on the left panel to modify them.\nPro Tip: hold CTRL key and select entries to edit multiple at once or use CTRL+A to select them all.",
                HorizontalAlignment = TextAlignment.Near,
                Margin = new Margin(7),
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = new Margin(10, -20, 45, 70),
                Parent = this
            };

            // Buttons
            const float buttonsWidth = 60;
            const float buttonsHeight = 24;
            const float buttonsMargin = 8;
            Button importButton = new Button(0, 0, buttonsWidth, buttonsHeight)
            {
                Text = "Import",
                AnchorPreset = AnchorPresets.BottomRight,
                Offsets = new Margin(-buttonsWidth - buttonsMargin, buttonsWidth, -buttonsHeight - buttonsMargin,
                    buttonsHeight),
                Parent = this
            };
            importButton.Clicked += OnSubmit;
            Button cancelButton = new Button(0, 0, buttonsWidth, buttonsHeight)
            {
                Text = "Cancel",
                AnchorPreset = AnchorPresets.BottomRight,
                Offsets = new Margin(-buttonsWidth - buttonsMargin - buttonsWidth - buttonsMargin, buttonsWidth,
                    -buttonsHeight - buttonsMargin, buttonsHeight),
                Parent = this
            };
            cancelButton.Clicked += OnCancel;

            // Split panel for entries list and settings editor
            SplitPanel splitPanel = new SplitPanel(Orientation.Horizontal, ScrollBars.Both, ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(2, 2, infoLabel.Bottom + 2, buttonsHeight + buttonsMargin + buttonsMargin),
                Parent = this
            };

            // Settings editor
            m_SettingsEditor = new CustomEditorPresenter(null);
            m_SettingsEditor.Panel.Parent = splitPanel.Panel2;

            // Setup tree
            m_Tree = new Tree(true)
            {
                Parent = splitPanel.Panel1
            };
            m_Tree.RightClick += OnTreeRightClick;
            m_RootNode = new TreeNode(false);
            for (int i = 0; i < entries.Count; i++)
            {
                ImportFileEntry entry = entries[i];

                // TODO: add icons for textures/models/etc from FileEntry to tree node??
                ItemNode node = new ItemNode(entry)
                {
                    Parent = m_RootNode
                };
            }
            m_RootNode.Expand();
            m_RootNode.ChildrenIndent = 0;
            m_RootNode.Parent = m_Tree;
            m_Tree.Margin = new Margin(0.0f, 0.0f, -16.0f, 2.0f); // Hide root node
            m_Tree.SelectedChanged += OnSelectedChanged;

            // Select the first item
            m_Tree.Select(m_RootNode.Children[0] as TreeNode);

            m_DialogSize = new Float2(totalWidth, editorHeight + splitPanel.Offsets.Height);
        }



        private void OnTreeRightClick(TreeNode node, Float2 location)
        {
            ContextMenu menu = new ContextMenu();
            menu.AddButton("Rename", OnRenameClicked);
            menu.AddButton("Don't import", OnDontImportClicked);
            menu.AddButton("Show in Explorer", OnShowInExplorerClicked);
            menu.Tag = node;
            menu.Show(node, location);
        }

        private void OnRenameClicked(ContextMenuButton button)
        {
            ItemNode node = (ItemNode)button.ParentContextMenu.Tag;
            node.StartRenaming();
        }

        private void OnDontImportClicked(ContextMenuButton button)
        {
            if (EntriesCount == 1)
            {
                OnCancel();
                return;
            }

            ItemNode node = (ItemNode)button.ParentContextMenu.Tag;
            if (m_SettingsEditor.Selection.Count == 1 && m_SettingsEditor.Selection[0] == node.Entry.Settings)
            {
                m_SettingsEditor.Deselect();
            }
            node.Dispose();
        }

        private void OnShowInExplorerClicked(ContextMenuButton button)
        {
            ItemNode node = (ItemNode)button.ParentContextMenu.Tag;
            FileSystem.ShowFileExplorer(Path.GetDirectoryName(node.Entry.SourceUrl));
        }

        private class ItemNode : TreeNode
        {
            public ImportFileEntry Entry => (ImportFileEntry)Tag;

            public ItemNode(ImportFileEntry entry)
            : base(false)
            {
                Text = Path.GetFileName(entry.SourceUrl);
                Tag = entry;
                LinkTooltip(entry.SourceUrl);
            }

            /// <inheritdoc />
            protected override bool OnMouseDoubleClickHeader(ref Float2 location, MouseButton button)
            {
                StartRenaming();
                return true;
            }

            public override bool OnKeyDown(KeyboardKeys key)
            {
                if (base.OnKeyDown(key))
                {
                    return true;
                }
                switch (key)
                {
                    case KeyboardKeys.F2:
                        StartRenaming();
                        return true;
                }
                return false;
            }

            /// <summary>
            /// Shows the rename popup for the item.
            /// </summary>
            public void StartRenaming()
            {
                // Start renaming the folder
                ImportFileEntry entry = (ImportFileEntry)Tag;
                string shortName = Path.GetFileNameWithoutExtension(entry.ResultUrl);
                RenamePopup dialog = RenamePopup.ShowPopup(this, HeaderRect, shortName, false);
                dialog.Tag = Tag;
                dialog.Renamed += OnRenamed;
            }

            private void OnRenamed(RenamePopup popup)
            {
                ImportFileEntry entry = (ImportFileEntry)Tag;
                entry.ModifyResultFilename(popup.Text);
                Text = string.Format("{0} ({1})", Path.GetFileName(entry.SourceUrl), popup.Text);
            }
        }

        private void OnSelectedChanged(List<TreeNode> before, List<TreeNode> after)
        {
            List<object> selection = new List<object>(after.Count);
            for (int i = 0; i < after.Count; i++)
            {
                if (after[i].Tag is ImportFileEntry fileEntry && fileEntry.HasSettings)
                {
                    selection.Add(fileEntry.Settings);
                }
            }

            m_SettingsEditor.Select(selection);
        }

        /// <inheritdoc />
        public override void OnSubmit()
        {
            m_SettingsEditor.Update();

            List<ImportFileEntry> entries = new List<ImportFileEntry>(m_RootNode.ChildrenCount);
            for (int i = 0; i < m_RootNode.ChildrenCount; i++)
            {
                if (m_RootNode.Children[i].Tag is ImportFileEntry fileEntry)
                {
                    entries.Add(fileEntry);
                }
            }
            Editor.Instance.Importing.LetThemBeImportedxD(entries);

            base.OnSubmit();
        }


        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);
        }


        /// <inheritdoc />
        protected override void SetupWindowSettings(ref CreateWindowSettings settings)
        {
            base.SetupWindowSettings(ref settings);

            settings.MinimumSize = new Float2(300, 400);
            settings.HasSizingFrame = true;
        }

        /// <inheritdoc />
        public override void Focus()
        {
            base.Focus();
            m_Tree.SelectedNode?.Focus();
        }
    }
}

#pragma once
#include "ContentItem.h"
#include "fmt/chrono.h"

namespace SE::Editor
{
    class ContentTreeNode;
    class DragItems;

    /// <summary>
    /// Types of content directories.
    /// </summary>
    enum class ContentFolderType
    {
        /// <summary>
        /// The directory with assets.
        /// </summary>
        Content,

        /// <summary>
        /// The directory with source files.
        /// </summary>
        Source,

        /// <summary>
        /// The other type of directory.
        /// </summary>
        Other,
    };

    class ContentFolder : public ContentItem
    {
        SE_DEFINE_CLASS_DEFAULT(ContentFolder, ContentItem)
    private:
        Ref<DragItems> _dragOverItems;
        bool _validDragOver;

    public:
        /// <summary>
        /// Gets the type of the folder.
        /// </summary>
        ContentFolderType FolderType;

        /// <summary>
        /// Returns true if that folder can import/manage scripts.
        /// </summary>
        // bool CanHaveScripts => FolderType == ContentFolderType.Source;

        /// <summary>
        /// Returns true if that folder can import/manage assets.
        /// </summary>
        PRO_GET(CanHaveAssets, ContentFolder, bool, __GetCanHaveAssets);

        /// <summary>
        /// Gets the content node.
        /// </summary>
        ContentTreeNode* Node;

        /// <summary>
        /// The subitems of this folder.
        /// </summary>
        List<ContentItem*> Children = List<ContentItem*>();

        /// <summary>
        /// Initializes a new instance of the <see cref="ContentFolder"/> class.
        /// </summary>
        /// <param name="type">The folder type.</param>
        /// <param name="path">The path to the item.</param>
        /// <param name="node">The folder parent node.</param>
        ContentFolder(ContentFolderType type, StringView path, ContentTreeNode* node);

        /// <summary>
        /// Tries to find child element with given path
        /// </summary>
        /// <param name="path">Element path to find</param>
        /// <returns>Found element of null</returns>
        ContentItem* FindChild(StringView path);

        /// <summary>
        /// Check if folder contains child element with given path
        /// </summary>
        /// <param name="path">Element path to find</param>
        /// <returns>True if contains that element, otherwise false</returns>
        bool ContainsChild(StringView path)
        {
            return FindChild(path) != nullptr;
        }

        /// <inheritdoc />
        void UpdatePath(String value) override;

        /// <inheritdoc />
        ContentItem* Find(String path) override;

        /// <inheritdoc />
        bool Find(ContentItem* item) override;

        /// <inheritdoc />
        ContentItem* Find(UID id) override;

        /// <inheritdoc />
        /*ScriptItem* FindScriptWitScriptName(String scriptName) override
        {
            for (int i = 0; i < Children.Count; i++)
            {
                var result = Children[i].FindScriptWitScriptName(scriptName);
                if (result != null)
                    return result;
            }

            return nullptr;
        }*/

        /// <inheritdoc />
        int Compare(const Control* other) const override;

        /// <inheritdoc />
        void Draw() override;

        /// <inheritdoc />
        DragDropEffect OnDragEnter(const Float2 &location, DragData* data) override;

        /// <inheritdoc />
        DragDropEffect OnDragMove(const Float2 &location, DragData* data) override;

        /// <inheritdoc />
        DragDropEffect OnDragDrop(const Float2 &location, DragData* data) override;

        /// <inheritdoc />
        void OnDragLeave() override;

    private:
        bool ValidateDragItem(ContentItem* item);

    protected:

        /// <inheritdoc />
        void OnBuildTooltipText(StringBuilder sb) override;

        ContentItemSearchFilter __GetSearchFilter() override;

        /// <inheritdoc />
        void OnParentFolderChanged() override;

        /// <inheritdoc />
        ContentItemType __GetItemType() override { return ContentItemType::Folder; }

        /// <inheritdoc />
        // public override ContentItemSearchFilter SearchFilter => ContentItemSearchFilter.Other;

        /// <inheritdoc />
        bool __GetCanRename() override;

        /// <inheritdoc />
        bool __GetCanDrag() override; // Deny rename action for root folders

        /// <inheritdoc />
        bool __GetExists() override;

        /// <inheritdoc />
        // public override String TypeDescription => "Folder";

        /// <inheritdoc />
        SpriteHandle __GetDefaultThumbnail() override;

        bool __GetHasDefaultThumbnail() override;

        bool __GetCanHaveAssets();

    };
} // SE


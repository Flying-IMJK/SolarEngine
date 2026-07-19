#pragma once
#include "Editor/GUI/Tree/TreeNode.h"

namespace SE::Editor
{
    class ContentItem;
    enum class ContentFolderType;
    class ContentFolder;
    class DragItems;

    /// <summary>
    /// Content folder tree node.
    /// </summary>
    /// <seealso cref="TreeNode" />
    SE_CLASS(Reflect)
    class ContentTreeNode : public TreeNode
    {
        SE_DEFINE_CLASS_DEFAULT(ContentTreeNode, TreeNode)
    private:
        Ref<DragItems> _dragOverItems;
        List<Rectangle> _highlights;

    protected:
        /// <summary>
        /// The folder.
        /// </summary>
        ContentFolder* m_Folder;

    public:
        /// <summary>
        /// Whether this node can be deleted.
        /// </summary>
        virtual bool GetCanDelete() { return true; }

        /// <summary>
        /// Whether this node can be duplicated.
        /// </summary>
        virtual bool GetCanDuplicate() { return true; }

        /// <summary>
        /// Gets the content folder item.
        /// </summary>
        ContentFolder* GetFolder() { return m_Folder; }

        /// <summary>
        /// Gets the type of the folder.
        /// </summary>
        ContentFolderType GetFolderType();

        /// <summary>
        /// Returns true if that folder can import/manage assets.
        /// </summary>
        /// <returns>True if can contain assets for project, otherwise false</returns>
        bool GetCanHaveAssets();

        /// <summary>
        /// Gets the parent node.
        /// </summary>
        ContentTreeNode* ParentNode();

        /// <summary>
        /// Gets the folder path.
        /// </summary>
        StringView GetPath();

        /// <summary>
        /// Gets the navigation button label.
        /// </summary>
        virtual StringView GetNavButtonLabel();

        /// <summary>
        /// Initializes a new instance of the <see cref="ContentTreeNode"/> class.
        /// </summary>
        /// <param name="parent">The parent node.</param>
        /// <param name="path">The folder path.</param>
        ContentTreeNode(ContentTreeNode* parent, StringView path);

        /// <summary>
        /// Shows the rename popup for the item.
        /// </summary>
        void StartRenaming();

        /// <summary>
        /// Updates the query search filter.
        /// </summary>
        /// <param name="filterText">The filter text.</param>
        void UpdateFilter(String filterText);

        /// <inheritdoc />
        void Draw() override;

        /// <inheritdoc />
        void OnDestroy() override;

        /// <inheritdoc />
        bool OnKeyDown(KeyboardKeys key) override;

    private:
        DragDropEffect GetDragEffect(DragData* data);

        bool ValidateDragItem(ContentItem* item);

    protected:
        /// <summary>
        /// Initializes a new instance of the <see cref="ContentTreeNode"/> class.
        /// </summary>
        /// <param name="parent">The parent node.</param>
        /// <param name="type">The folder type.</param>
        /// <param name="path">The folder path.</param>
        ContentTreeNode(ContentTreeNode* parent, ContentFolderType type, StringView path);

        /// <inheritdoc />
        DragDropEffect OnDragEnterHeader(DragData* data) override;

        /// <inheritdoc />
        DragDropEffect OnDragMoveHeader(DragData* data) override;

        /// <inheritdoc />
        void OnDragLeaveHeader() override;

        /// <inheritdoc />
        DragDropEffect OnDragDropHeader(DragData* data) override;

        /// <inheritdoc />
        void BeginDragDrop() override;

        /// <inheritdoc />
        void OnLongPress() override;
    };

    /// <summary>
    /// Root tree node for the content workspace.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.ContentTreeNode" />
    SE_CLASS(Reflect)
    class RootContentTreeNode : public ContentTreeNode
    {
        SE_DEFINE_CLASS(RootContentTreeNode, ContentTreeNode)
    public:
        RootContentTreeNode();

        StringView GetNavButtonLabel() override;
    };

} // SE


#pragma once
#include "Editor/GUI/ComboBox.h"
#include "Editor/GUI/NavigationButton.h"

namespace SE::Editor
{
    class ContentItem;
    class ContentTreeNode;
    class DragItems;

    /// <summary>
    /// A navigation button for <see cref="Windows.ContentWindow"/>.
    /// </summary>
    class ContentNavigationButton : public NavigationButton
    {
        SE_DEFINE_CLASS(ContentNavigationButton, NavigationButton)
    private:
         DragItems* _dragOverItems;

    public:
        /// <summary>
        /// Gets the target node.
        /// </summary>
        ContentTreeNode* TargetNode;

        ContentNavigationButton();

        /// <summary>
        /// Initializes a new instance of the <see cref="ContentNavigationButton"/> class.
        /// </summary>
        /// <param name="targetNode">The target node.</param>
        /// <param name="x">The x position.</param>
        /// <param name="y">The y position.</param>
        /// <param name="height">The height.</param>
        ContentNavigationButton(ContentTreeNode* targetNode, float x, float y, float height);

        /// <inheritdoc />
        DragDropEffect OnDragEnter(const Float2& location, DragData* data) override;

        /// <inheritdoc />
        DragDropEffect OnDragMove(const Float2& location, DragData* data) override;

        /// <inheritdoc />
        void OnDragLeave() override;

        /// <inheritdoc />
        DragDropEffect OnDragDrop(const Float2& location, DragData* data) override;

    protected:
        /// <inheritdoc />
        void OnClick() override;

    private:
        DragDropEffect GetDragEffect(DragData* data);

        bool ValidateDragItem(ContentItem* item);

    };

    class ContentNavigationSeparator final : public ComboBox
    {
        SE_DEFINE_CLASS_DEFAULT(ContentNavigationSeparator, ComboBox)
    public:
        ContentNavigationButton* Target;

        ContentNavigationSeparator(ContentNavigationButton* target, float x, float y, float height);

        void Draw() override;

    protected:
        ContextMenu* OnCreatePopup() override;

        void OnLayoutMenuButton(ContextMenuButton* button, int index, bool construct = false) override;

        void OnItemClicked(int index) override;
    };

} // SE

#pragma once

#include <optional>

#include "Runtime/Render/2D/FontReference.h"
#include "Runtime/Render/2D/SpriteAtlas.h"
#include "Runtime/UI/GUI/ContainerControl.h"

namespace SE::Editor
{
    class Tree;

    class TreeNode : public ContainerControl
    {
        SE_CLASS(TreeNode, ContainerControl)
    public:
        /// <summary>
        /// The default drag insert position margin.
        /// </summary>
        static constexpr float DefaultDragInsertPositionMargin = 3.0f;

        /// <summary>
        /// The default node offset on Y axis.
        /// </summary>
        static constexpr float DefaultNodeOffsetY = 0;

    private:
        Tree* _tree;

        bool _opened = false, _canChangeOrder = false;
        float _animationProgress, _cachedHeight;
        bool _mouseOverArrow, _mouseOverHeader;
        float _xOffset, _textWidth;
        float _headerHeight = 16.0f;
        Rectangle _headerRect;
        SpriteHandle _iconCollaped, _iconOpened;
        Margin _margin = Margin(2.0f);
        String _text;
        bool _textChanged;
        bool _isMouseDown;
        float _mouseDownTime;
        Float2 _mouseDownPos;

        DragItemPositioning _dragOverMode;
        bool _isDragOverHeader;
        static uint64 _dragEndFrame;

    public:
        /// <summary>
        /// Gets or sets the text.
        /// </summary>
        // [EditorOrder(10), Tooltip("The node text.")]
        PRO_REF(Text, TreeNode, String, __GetText, __SetText);

        /// <summary>
        /// Gets or sets a value indicating whether this node is expanded.
        /// </summary>
        // [EditorOrder(20), Tooltip("If checked, node is expanded.")]
        PRO(IsExpanded, TreeNode, bool, __GetIsExpanded, __SetIsExpanded);

        /// <summary>
        /// Gets or sets a value indicating whether this node is collapsed.
        /// </summary>
        // [HideInEditor, NoSerialize]
        PRO(IsCollapsed, TreeNode, bool, __GetIsCollapsed, __SetIsCollapsed);


        /// <summary>
        /// Gets a value indicating whether the node is collapsed in the hierarchy (is collapsed or any of its parents is collapsed).
        /// </summary>
        PRO_GET(IsCollapsedInHierarchy, TreeNode, bool, __GetIsCollapsedInHierarchy);

        /// <summary>
        /// Gets or sets the text margin.
        /// </summary>
        // [EditorOrder(30), Tooltip("The margin of the text area.")]
        PRO_REF(TextMargin, TreeNode, Margin, __GetTextMargin, __SetTextMargin);


        /// <summary>
        /// Gets or sets the color of the text.
        /// </summary>
        // [EditorDisplay("Style"), EditorOrder(2000)]
        Color TextColor;

        /// <summary>
        /// Gets or sets the font used to render text.
        /// </summary>
        // [EditorDisplay("Style"), EditorOrder(2000)]
        FontReference TextFont;

        /// <summary>
        /// Gets or sets the color of the background when tree node is selected.
        /// </summary>
        // [EditorDisplay("Style"), EditorOrder(2000)]
        Color BackgroundColorSelected;

        /// <summary>
        /// Gets or sets the color of the background when tree node is highlighted.
        /// </summary>
        // [EditorDisplay("Style"), EditorOrder(2000)]
        Color BackgroundColorHighlighted;

        /// <summary>
        /// Gets or sets the color of the background when tree node is selected but not focused.
        /// </summary>
        // [EditorDisplay("Style"), EditorOrder(2000)]
        Color BackgroundColorSelectedUnfocused;

        /// <summary>
        /// Gets the parent tree control.
        /// </summary>
        PRO_GET(ParentTree, TreeNode, Tree*, __GetParentTree);

        /// <summary>
        /// Gets a value indicating whether this node is root.
        /// </summary>
        PRO_GET(IsRoot, TreeNode, bool, __GetIsRoot);

        /// <summary>
        /// Gets the minimum width of the node sub-tree.
        /// </summary>
        PRO_GET(MinimumWidth, TreeNode, float, __GetMinimumWidth);

        /// <summary>
        /// The indent applied to the child nodes.
        /// </summary>
        // [EditorOrder(30), Tooltip("The indentation applied to the child nodes.")]
        float ChildrenIndent = 12.0f;

        /// <summary>
        /// The height of the tree node header area.
        /// </summary>
        // [EditorOrder(40), Limit(1, 10000, 0.1f), Tooltip("The height of the tree node header area.")]
        PRO(HeaderHeight, TreeNode, float, __GetHeaderHeight, __SetHeaderHeight);

        /// <summary>
        /// Gets or sets the color of the icon.
        /// </summary>
        // [EditorOrder(50), Tooltip("The color of the icon.")]
        Color IconColor = Colors::White;

        /// <summary>
        /// Gets the arrow rectangle.
        /// </summary>
        PRO_GET(ArrowRect, TreeNode, Rectangle, __GetArrowRect);

        /// <summary>
        /// Gets the header rectangle.
        /// </summary>
        PRO_GET(HeaderRect, TreeNode, Rectangle, __GetHeaderRect);

        /// <summary>
        /// Gets the header text rectangle.
        /// </summary>
        PRO_GET(TextRect, TreeNode, Rectangle, __GetTextRect);

        /// <summary>
        /// Custom arrow rectangle within node.
        /// </summary>
        std::optional<Rectangle> CustomArrowRect;

        /// <summary>
        /// Gets the drag over action type.
        /// </summary>
        PRO_GET(DragOverMode, TreeNode, DragItemPositioning, __GetDragOverMode);

        /// <summary>
        /// Gets a value indicating whether this node has any visible child. Returns false if it has no children.
        /// </summary>
        PRO_GET(HasAnyVisibleChild, TreeNode, bool, __GetHasAnyVisibleChild);

        TreeNode();

        /// <summary>
        /// Initializes a new instance of the <see cref="TreeNode"/> class.
        /// </summary>
        /// <param name="canChangeOrder">Enable/disable changing node order in parent tree node.</param>
        TreeNode(bool canChangeOrder);

        /// <summary>
        /// Initializes a new instance of the <see cref="TreeNode"/> class.
        /// </summary>
        /// <param name="canChangeOrder">Enable/disable changing node order in parent tree node.</param>
        /// <param name="iconCollapsed">The icon for node collapsed.</param>
        /// <param name="iconOpened">The icon for node opened.</param>
        TreeNode(bool canChangeOrder, SpriteHandle iconCollapsed, SpriteHandle iconOpened);

        /// <summary>
        /// Expand node.
        /// </summary>
        /// <param name="noAnimation">True if skip node expanding animation.</param>
        void Expand(bool noAnimation = false);

        /// <summary>
        /// Collapse node.
        /// </summary>
        /// <param name="noAnimation">True if skip node expanding animation.</param>
        void Collapse(bool noAnimation = false);

        /// <summary>
        /// Expand node and all the children.
        /// </summary>
        /// <param name="noAnimation">True if skip node expanding animation.</param>
        void ExpandAll(bool noAnimation = false);

        /// <summary>
        /// Collapse node and all the children.
        /// </summary>
        /// <param name="noAnimation">True if skip node expanding animation.</param>
        void CollapseAll(bool noAnimation = false);

        /// <summary>
        /// Ensure that all node parents are expanded.
        /// </summary>
        /// <param name="noAnimation">True if skip node expanding animation.</param>
        void ExpandAllParents(bool noAnimation = false);

        /// <summary>
        /// Ends open/close animation by force.
        /// </summary>
        void EndAnimation();

        /// <summary>
        /// Select node in the tree.
        /// </summary>
        void Select();

        /// <inheritdoc />
        void Update(float deltaTime) override;

        /// <inheritdoc />
        void Draw() override;

        /// <inheritdoc />
        bool OnMouseDown(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        bool OnMouseUp(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        bool OnMouseDoubleClick(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        void OnMouseMove(Float2 location) override;

        /// <inheritdoc />
        void OnMouseLeave() override;

        /// <inheritdoc />
        bool OnKeyDown(KeyboardKeys key) override;

        /// <inheritdoc />
        void OnKeyUp(KeyboardKeys key) override;

        /// <inheritdoc />
        void OnChildResized(Control* control) override;

        /// <inheritdoc />
        DragDropEffect OnDragEnter(const Float2 &location, DragData* data) override;

        /// <inheritdoc />
        DragDropEffect OnDragMove(const Float2 &location, DragData* data) override;

        /// <inheritdoc />
        DragDropEffect OnDragDrop(const Float2 &location, DragData* data) override;

        /// <inheritdoc />
        void OnDragLeave() override;

        /// <inheritdoc />
        bool OnTestTooltipOverControl(Float2 location) override;

        /// <inheritdoc />
        /*bool OnShowTooltip(String &text, Float2 &location, Rectangle &area) override
        {
            text = TooltipText;
            location = _headerRect.Size * new Float2(0.5f, 1.0f);
            area = new Rectangle(Float2.Zero, _headerRect.Size);
            return ShowTooltip;
        }*/


        /// <inheritdoc />
        void PerformLayout(bool force = false) override;

        /// <inheritdoc />
        int Compare(const Control* other) const override;

        /// <inheritdoc />
        void OnDestroy() override;

    protected:

/// <summary>
        /// Called when drag and drop enters the node header area.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <returns>Drag action response.</returns>
        virtual DragDropEffect OnDragEnterHeader(DragData* data)
        {
            return DragDropEffect::None;
        }

        /// <summary>
        /// Called when drag and drop moves over the node header area.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <returns>Drag action response.</returns>
        virtual DragDropEffect OnDragMoveHeader(DragData* data)
        {
            return DragDropEffect::None;
        }

        /// <summary>
        /// Called when drag and drop performs over the node header area.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <returns>Drag action response.</returns>
        virtual DragDropEffect OnDragDropHeader(DragData* data)
        {
            return DragDropEffect::None;
        }

        /// <summary>
        /// Called when drag and drop leaves the node header area.
        /// </summary>
        virtual void OnDragLeaveHeader()
        {
        }

        /// <summary>
        /// Begins the drag drop operation.
        /// </summary>
        virtual void BeginDragDrop()
        {
        }

        /// <summary>
        /// Called when mouse double clicks header.
        /// </summary>
        /// <param name="location">The mouse location.</param>
        /// <param name="button">The button.</param>
        /// <returns>True if event has been handled.</returns>
        virtual bool OnMouseDoubleClickHeader(Float2 &location, MouseButton button);

        /// <summary>
        /// Called when mouse is pressing node header for a long time.
        /// </summary>
        virtual void OnLongPress()
        {
        }

        /// <summary>
        /// Called when expanded/collapsed state changes.
        /// </summary>
        virtual void OnExpandedChanged()
        {
        }

        /// <summary>
        /// Called when expand/collapse animation progress changes.
        /// </summary>
        virtual void OnExpandAnimationChanged();

        /// <summary>
        /// Tests the header hit.
        /// </summary>
        /// <param name="location">The location.</param>
        /// <returns>True if hits it.</returns>
        virtual bool TestHeaderHit(const Float2 &location);

        /// <summary>
        /// Caches the color of the text for this node. Called during update before children nodes but after parent node so it can reuse parent tree node data.
        /// </summary>
        /// <returns>Text color.</returns>
        virtual Color CacheTextColor();

        /// <summary>
        /// Updates the cached width of the text.
        /// </summary>
        void UpdateTextWidth();

        /// <inheritdoc />
        void DrawChildren() override;

        /// <inheritdoc />
        void OnSizeChanged() override;

        /// <inheritdoc />
        bool CanNavigateChild(Control* child) override;

        /// <inheritdoc />
        void OnParentChangedInternal() override;

        /// <inheritdoc />
        void PerformLayoutAfterChildren() override;

        /// <inheritdoc />
        void PerformLayoutBeforeChildren() override;

        virtual float __GetMinimumWidth();

    private:

        /// <summary>
        /// Updates the drag over mode based on the given mouse location.
        /// </summary>
        /// <param name="location">The location.</param>
        void UpdateDragPositioning(const Float2 &location);

        void ClearDragPositioning();

        void UpdateMouseOverFlags(Float2 location);

        bool __GetIsCollapsedInHierarchy();
        Margin& __GetTextMargin() { return _margin; }
        void __SetTextMargin(Margin &value)
        {
            _margin = value;
            PerformLayout();
        }


        String& __GetText() { return _text; }
        void __SetText(String& value);

        bool __GetHasAnyVisibleChild();

        DragItemPositioning __GetDragOverMode();

        Rectangle __GetTextRect();

        float __GetHeaderHeight() { return _headerHeight; }
        void __SetHeaderHeight(float value);
        Rectangle __GetArrowRect();
        Rectangle __GetHeaderRect() { return _headerRect; }

        Tree* __GetParentTree();
        bool __GetIsRoot();


        bool __GetIsCollapsed() { return _opened; }
        void __SetIsCollapsed(bool value);

        bool __GetIsExpanded() { return _opened; }
        void __SetIsExpanded(bool value);

    };

} // SE


#pragma once

#include "HScrollBar.h"
#include "ScrollBar.h"
#include "VScrollBar.h"
#include "Runtime/UI/GUI/ScrollableControl.h"

namespace SE
{

	class SE_API_RUNTIME Panel : public ScrollableControl
	{
		SE_DEFINE_CLASS(Panel, ScrollableControl)
private:
        bool _layoutChanged;
        bool _alwaysShowScrollbars;
        int _layoutUpdateLock;
        ScrollBars _scrollBars;
        float _scrollBarsSize = ScrollBarDefaultSize;
        Margin _scrollMargin;
        Color _scrollbarTrackColor;
        Color _scrollbarThumbColor;
        Color _scrollbarThumbSelectedColor;

protected:
        /// <summary>
        /// The cached scroll area bounds. Used to scroll contents of the panel control. Cached during performing layout.
        /// </summary>
        Rectangle _controlsBounds;

public:
        /// <summary>
        /// The vertical scroll bar.
        /// </summary>
        VScrollBar* vScrollBar;

        /// <summary>
        /// The horizontal scroll bar.
        /// </summary>
        HScrollBar* hScrollBar;

        /// <summary>
        /// Gets the view bottom.
        /// </summary>
        Float2 GetViewBottom() const;

        /// <summary>
        /// Gets the cached scroll area bounds. Used to scroll contents of the panel control. Cached during performing layout.
        /// </summary>
        Rectangle GetControlsBounds() const { return _controlsBounds; };

        /// <summary>
        /// Gets or sets the scroll bars usage by this panel.
        /// </summary>
        // [EditorDisplay("Scrollbar Style"), EditorOrder(1500), Tooltip("The scroll bars usage.")]
        ScrollBars GetScrollBars() const { return _scrollBars; };
	    void SetScrollBars(ScrollBars value);

        /// <summary>
        /// Gets or sets the size of the scroll bars.
        /// </summary>
        // [EditorDisplay("Scrollbar Style"), EditorOrder(1501), Tooltip("Scroll bars size.")]
	    float GetScrollBarsSize() const { return _scrollBarsSize; };
        void SetScrollBarsSize(float value);

        /// <summary>
        /// Gets or sets a value indicating whether always show scrollbars. Otherwise show them only if scrolling is available.
        /// </summary>
        // [EditorDisplay("Scrollbar Style"), EditorOrder(1502), Tooltip("Whether always show scrollbars. Otherwise show them only if scrolling is available.")]
        bool GetAlwaysShowScrollbars() const { return _alwaysShowScrollbars; }
	    void SetAlwaysShowScrollbars(bool value);

        /// <summary>
        /// Gets or sets the scroll margin applies to the child controls area. Can be used to expand the scroll area bounds by adding a margin.
        /// </summary>
        // [EditorDisplay("Scrollbar Style"), EditorOrder(1503), Tooltip("Scroll margin applies to the child controls area. Can be used to expand the scroll area bounds by adding a margin.")]
        Margin GetScrollMargin() const { return _scrollMargin; }
	    void SetScrollMargin(Margin value);

        /// <summary>
        /// The color of the scroll bar track.
        /// </summary>
        // [EditorDisplay("Scrollbar Style"), EditorOrder(1600), ExpandGroups]
        Color GetScrollbarTrackColor() const { return _scrollbarTrackColor; }
	    void SetScrollbarTrackColor(Color value);

        /// <summary>
        /// The color of the scroll bar thumb.
        /// </summary>
        // [EditorDisplay("Scrollbar Style"), EditorOrder(1601)]
	    Color GetScrollbarThumbColor() const { return _scrollbarThumbColor; }
        void SetScrollbarThumbColor(Color value);

        /// <summary>
        /// The color of the scroll bar thumb when selected.
        /// </summary>
        // [EditorDisplay("Scrollbar Style"), EditorOrder(1602)]
	    Color GetScrollbarThumbSelectedColor() const { return _scrollbarThumbSelectedColor; }
        void SetScrollbarThumbSelectedColor(Color value);

		Panel();

        /// <summary>
        /// Initializes a new instance of the <see cref="Panel"/> class.
        /// </summary>
        /// <param name="scrollBars">The scroll bars.</param>
        /// <param name="autoFocus">True if control can accept user focus</param>
        Panel(ScrollBars scrollBars, bool autoFocus = false);

        /// <inheritdoc />
	protected:
	    void SetViewOffset(Float2 value) override;

	public:
        /// <summary>
        /// Cuts the scroll bars value smoothing and imminently goes to the target scroll value.
        /// </summary>
        void FastScroll();

        /// <summary>
        /// Scrolls the view to the given control area.
        /// </summary>
        /// <param name="c">The control.</param>
        /// <param name="fastScroll">True of scroll to the item quickly without smoothing.</param>
        void ScrollViewTo(Control* c, bool fastScroll = false);

        /// <summary>
        /// Scrolls the view to the given location.
        /// </summary>
        /// <param name="location">The location.</param>
        /// <param name="fastScroll">True of scroll to the item quickly without smoothing.</param>
        void ScrollViewTo(Float2 location, bool fastScroll = false);

        /// <summary>
        /// Scrolls the view to the given area.
        /// </summary>
        /// <param name="bounds">The bounds.</param>
        /// <param name="fastScroll">True of scroll to the item quickly without smoothing.</param>
        void ScrollViewTo(Rectangle bounds, bool fastScroll = false);

	    void SetViewOffset(Orientation orientation, float value);

        /// <inheritdoc />
        bool OnMouseDown(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        bool OnMouseWheel(Float2 location, float delta) override;

        /// <inheritdoc />
        void RemoveChildren() override;

        /// <inheritdoc />
        void DisposeChildren() override;

        /// <inheritdoc />
        void OnChildResized(Control* control) override;

        /// <inheritdoc />
        void Draw() override;

        /// <inheritdoc />
        bool ContainsPoint(Float2& location, bool precise = false) override;

        /// <inheritdoc />
        bool IntersectsChildContent(Control* child, Float2 location, Float2& childSpaceLocation) override;

        /// <inheritdoc />
	    void AddChildInternal(Control* child) override;

        /// <inheritdoc />
        void PerformLayout(bool force = false) override;

        /// <inheritdoc />
        Rectangle GetDesireClientArea() override;

        /// <inheritdoc />
        DragDropEffect OnDragMove(const Float2& location, DragData* data) override;

	protected:
	    /// <inheritdoc />
	    void PerformLayoutBeforeChildren() override;

        /// <summary>
        /// Arranges the child controls and gets their bounds.
        /// </summary>
	    virtual void ArrangeAndGetBounds();

        /// <summary>
        /// Arranges the child controls.
        /// </summary>
	    virtual void Arrange();
	};

} // SE


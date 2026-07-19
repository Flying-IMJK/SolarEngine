using System;

namespace SE.GUI
{
    /// <summary>
    /// UI control anchors presets.
    /// </summary>
    public enum AnchorPresets
    {
        Custom,
        TopLeft,
        TopCenter,
        TopRight,
        MiddleLeft,
        MiddleCenter,
        MiddleRight,
        BottomLeft,
        BottomCenter,
        BottomRight,
        VerticalStretchLeft,
        VerticalStretchCenter,
        VerticalStretchRight,
        HorizontalStretchTop,
        HorizontalStretchMiddle,
        HorizontalStretchBottom,
        StretchAll,
    }

    /// <summary>
    /// Specifies which scroll bars are visible on a control.
    /// </summary>
    [Flags]
    public enum ScrollBars
    {
        None = 0,
        Horizontal = 1,
        Vertical = 2,
        Both = Horizontal | Vertical,
    }

    /// <summary>
    /// Specifies the drop indicator position relative to an item.
    /// </summary>
    public enum DragItemPositioning
    {
        None = 0,
        At,
        Above,
        Below,
    }

    /// <summary>
    /// Specifies the orientation of controls or layout elements.
    /// </summary>
    public enum Orientation
    {
        Horizontal = 0,
        Vertical = 1,
    }

    /// <summary>
    /// The navigation directions in a user-interface layout.
    /// </summary>
    public enum NavDirection
    {
        None,
        Up,
        Down,
        Left,
        Right,
        Next,
        Previous,
    }

    /// <summary>
    /// The effect accepted by a drag-and-drop target.
    /// Values intentionally match the platform window drag-and-drop effect values.
    /// </summary>
    public enum DragDropEffect
    {
        None = 0,
        Copy = 1,
        Move = 2,
        Link = 3,
    }
}

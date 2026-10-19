// Copyright (c) Wojciech Figat. All rights reserved.

using SE.GUI;

namespace SE.Editor
{
    /// <summary>
    /// Custom property name label that fires mouse events for label and supports dragging.
    /// </summary>
    /// <seealso cref="SE.Editor.PropertyNameLabel" />
    [HideInEditor]
    public class DraggablePropertyNameLabel : ClickablePropertyNameLabel
    {
        private bool m_IsLeftMouseButtonDown;

        /// <summary>
        /// Mouse drag action delegate.
        /// </summary>
        /// <param name="label">The label.</param>
        /// <returns>The drag data or null if not use drag.</returns>
        public delegate DragData DragDelegate(DraggablePropertyNameLabel label);

        /// <summary>
        /// The mouse starts the drag. Callbacks gets the drag data.
        /// </summary>
        public DragDelegate Drag;

        /// <inheritdoc />
        public DraggablePropertyNameLabel(string name)
        : base(name)
        {
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                m_IsLeftMouseButtonDown = false;
            }

            base.OnMouseUp(location, button);
            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                m_IsLeftMouseButtonDown = true;
            }

            base.OnMouseDown(location, button);
            return true;
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            base.OnMouseLeave();

            if (m_IsLeftMouseButtonDown)
            {
                m_IsLeftMouseButtonDown = false;

                DragData data = Drag?.Invoke(this);
                if (data != null)
                {
                    DoDragDrop(data);
                }
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            // Unlink event
            Drag = null;

            base.OnDestroy();
        }
    }
}

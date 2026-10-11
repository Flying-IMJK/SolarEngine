#nullable disable annotations

using System;

namespace SE.Editor
{
    /// <summary>
    /// Draggable property name label with drag events.
    /// </summary>
    public class DraggablePropertyNameLabel : PropertyNameLabel
    {
        /// <summary>Drag event delegate.</summary>
        public delegate void DragDelegate(DraggablePropertyNameLabel label, Float2 location);

        /// <summary>Event fired when drag starts.</summary>
        public DragDelegate DragStart;

        /// <summary>Event fired when drag moves.</summary>
        public DragDelegate DragMove;

        /// <summary>Event fired when drag ends.</summary>
        public Action<DraggablePropertyNameLabel> DragEnd;

        private bool m_isDragging = false;
        private Float2 m_dragStartPos;

        public DraggablePropertyNameLabel(string name) : base(name)
        {
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                m_isDragging = true;
                m_dragStartPos = location;
                if (DragStart != null)
                {
                    DragStart(this, location);
                }
            }
            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 mousePos)
        {
            if (m_isDragging && DragMove != null)
            {
                DragMove(this, mousePos);
            }
            base.OnMouseMove(mousePos);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && m_isDragging)
            {
                m_isDragging = false;
                if (DragEnd != null)
                {
                    DragEnd(this);
                }
                return true;
            }
            return base.OnMouseUp(location, button);
        }
    }
}

// Copyright (c) Wojciech Figat. All rights reserved.

namespace SE.GUI
{
    /// <summary>
    /// A panel that evenly divides up available space between all of its children.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    public class UniformGridPanel : ContainerControl
    {
        private Margin m_SlotPadding;
        private int m_SlotsV, m_SlotsH;
        private Float2 m_SlotSpacing;

        /// <summary>
        /// Gets or sets the padding given to each slot.
        /// </summary>
        [EditorOrder(0), Tooltip("The padding margin applied to each item slot.")]
        public Margin SlotPadding
        {
            get => m_SlotPadding;
            set
            {
                m_SlotPadding = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// Gets or sets the amount of slots horizontally. Use 0 to don't limit it.
        /// </summary>
        [EditorOrder(10), Limit(0, 100000, 0.1f), Tooltip("The amount of slots horizontally. Use 0 to don't limit it.")]
        public int SlotsHorizontally
        {
            get => m_SlotsH;
            set
            {
                value = Mathf.Clamp(value, 0, 1000000);
                if (value != m_SlotsH)
                {
                    m_SlotsH = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the amount of slots vertically. Use 0 to don't limit it.
        /// </summary>
        [EditorOrder(20), Limit(0, 100000, 0.1f), Tooltip("The amount of slots vertically. Use 0 to don't limit it.")]
        public int SlotsVertically
        {
            get => m_SlotsV;
            set
            {
                value = Mathf.Clamp(value, 0, 1000000);
                if (value != m_SlotsV)
                {
                    m_SlotsV = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets grid slot spacing.
        /// </summary>
        [EditorOrder(30), Limit(0), Tooltip("The Grid slot spacing.")]
        public Float2 SlotSpacing
        {
            get => m_SlotSpacing;
            set
            {
                if (!Float2.NearEqual(ref m_SlotSpacing, ref value))
                {
                    m_SlotSpacing = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="UniformGridPanel"/> class.
        /// </summary>
        public UniformGridPanel()
        : this(0)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="UniformGridPanel"/> class.
        /// </summary>
        /// <param name="slotPadding">The slot padding.</param>
        public UniformGridPanel(float slotPadding)
        {
            AutoFocus = false;
            m_SlotPadding = new Margin(slotPadding);
            m_SlotSpacing = new Float2(2);
            m_SlotsH = m_SlotsV = 5;
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();

            int slotsV = m_SlotsV;
            int slotsH = m_SlotsH;
            Float2 slotSize;
            Float2 size = Size;
            bool applySpacing = true;
        APPLY_SPACING:
            if (m_SlotsV + m_SlotsH == 0)
            {
                slotSize = HasChildren ? Children[0].Size : new Float2(32);
                slotsH = Mathf.CeilToInt(size.X / slotSize.X);
                slotsV = Mathf.CeilToInt(size.Y / slotSize.Y);
            }
            else if (slotsH == 0)
            {
                slotSize = new Float2(size.Y / slotsV);
            }
            else if (slotsV == 0)
            {
                slotSize = new Float2(size.X / slotsH);
            }
            else
            {
                slotSize = new Float2(size.X / slotsH, size.Y / slotsV);
            }
            if (applySpacing && m_SlotSpacing != Float2.Zero)
            {
                applySpacing = false;
                size -= m_SlotSpacing * new Float2(slotsH > 1 ? slotsH - 1 : 0, slotsV > 1 ? slotsV - 1 : 0);
                goto APPLY_SPACING;
            }

            int i = 0;
            for (int y = 0; y < slotsV; y++)
            {
                int end = Mathf.Min(i + slotsH, m_Children.Count) - i;
                if (end <= 0)
                {
                    break;
                }

                for (int x = 0; x < end; x++)
                {
                    Rectangle slotBounds = new Rectangle((slotSize + m_SlotSpacing) * new Float2(x, y), slotSize);
                    m_SlotPadding.ShrinkRectangle(ref slotBounds);

                    Control c = m_Children[i++];
                    c.Bounds = slotBounds;
                }
            }
        }
    }
}

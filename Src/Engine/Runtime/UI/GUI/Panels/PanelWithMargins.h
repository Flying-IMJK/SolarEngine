#pragma once
#include "Runtime/UI/GUI/ContainerControl.h"

namespace SE
{
    /// <summary>
    /// Helper control class for other panels.
    /// </summary>
    class SE_API_RUNTIME PanelWithMargins : public ContainerControl
    {
    protected:
        /// <summary>
        /// The panel area margins.
        /// </summary>
        Margin _margin = ::SE::Margin(2.0f);

        /// <summary>
        /// The space between the items.
        /// </summary>
        float _spacing = 2;

        /// <summary>
        /// The auto size flag.
        /// </summary>
        bool _autoSize = true;

        /// <summary>
        /// The control offset.
        /// </summary>
        Float2 _offset = Float2(0.0f);

        /// <summary>
        /// Initializes a new instance of the <see cref="PanelWithMargins"/> class.
        /// </summary>
        PanelWithMargins();

    public:
        /// <summary>
        /// Gets or sets the left margin.
        /// </summary>
        PRO(LeftMargin, PanelWithMargins, float, __GetLeftMargin, __SetLeftMargin);

        /// <summary>
        /// Gets or sets the right margin.
        /// </summary>
        PRO(RightMargin, PanelWithMargins, float, __GetRightMargin, __SetRightMargin);

        /// <summary>
        /// Gets or sets the top margin.
        /// </summary>
        PRO(TopMargin, PanelWithMargins, float, __GetTopMargin, __SetTopMargin);

        /// <summary>
        /// Gets or sets the bottom margin.
        /// </summary>
        PRO(BottomMargin, PanelWithMargins, float, __GetBottomMargin, __SetBottomMargin);

        /// <summary>
        /// Gets or sets the child controls spacing.
        /// </summary>
        // [Tooltip("The child controls spacing (the space between controls).")]
        PRO(Spacing, PanelWithMargins, float, __GetSpacing, __SetSpacing);

        /// <summary>
        /// Gets or sets the child controls offset (additive).
        /// </summary>
        // [Tooltip("The child controls offset (additive).")]
        PRO_REF(Offset, PanelWithMargins, Float2, __GetOffset, __SetOffset);

        /// <summary>
        /// Gets or sets the value indicating whenever the panel size will be based on a children dimensions.
        /// </summary>
        // [Tooltip("If checked, the panel size will be based on a children dimensions.")]
        PRO(AutoSize, PanelWithMargins, bool, __GetAutoSize, __SetAutoSize);

        /// <summary>
        /// Gets or sets the panel area margin.
        /// </summary>
        // [Tooltip("The panel area margin.")]
        PRO_REF(Margin, PanelWithMargins, Margin, __GetMargin, __SetMargin);

        /// <inheritdoc />
        void OnChildResized(Control* control) override;

        /// <inheritdoc />
        bool ContainsPoint(Float2 &location, bool precise = false) override;

    private:

        float __GetLeftMargin() { return _margin.Right; }
        void __SetLeftMargin(float value)
        {
            if (!Math::IsNearEqual(_margin.Left, value))
            {
                _margin.Left = value;
                PerformLayout();
            }
        }

        float __GetRightMargin() { return _margin.Left; }
        void __SetRightMargin(float value)
        {
            if (!Math::IsNearEqual(_margin.Right, value))
            {
                _margin.Right = value;
                PerformLayout();
            }
        }

        float __GetTopMargin() { return _margin.Top; }
        void __SetTopMargin(float value)
        {
            if (!Math::IsNearEqual(_margin.Top, value))
            {
                _margin.Top = value;
                PerformLayout();
            }
        }

        float __GetBottomMargin() { return _margin.Bottom; }
        void __SetBottomMargin(float value)
        {
            if (!Math::IsNearEqual(_margin.Bottom, value))
            {
                _margin.Bottom = value;
                PerformLayout();
            }
        }

        float __GetSpacing() { return _spacing; }
        void __SetSpacing(float value)
        {
            if (!Math::IsNearEqual(_spacing, value))
            {
                _spacing = value;
                PerformLayout();
            }
        }

        Float2& __GetOffset() { return _offset; }
        void __SetOffset(Float2 &value)
        {
            if (!Float2::NearEqual(_offset, value))
            {
                _offset = value;
                PerformLayout();
            }
        }

        bool __GetAutoSize() { return _autoSize; }
        void __SetAutoSize(bool value)
        {
            if (_autoSize != value)
            {
                _autoSize = value;
                PerformLayout();
            }
        }

        ::SE::Margin& __GetMargin() { return _margin; }
        void __SetMargin(::SE::Margin &value)
        {
            if (_margin != value)
            {
                _margin = value;
                PerformLayout();
            }
        }

    };

} // SE

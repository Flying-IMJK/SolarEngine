#pragma once

#include "Runtime/UI/GUI/ContainerControl.h"

namespace SE::Editor
{
    /// <summary>
    /// The viewport widget location.
    /// </summary>
    enum ViewportWidgetLocation
    {
        /// <summary>
        /// The upper left corner of the parent container.
        /// </summary>
        UpperLeft,

        /// <summary>
        /// The upper right corner of the parent container.
        /// </summary>
        UpperRight,
    };

    /// <summary>
    /// Viewport Widgets Container control
    /// </summary>
    class ViewportWidgetsContainer : public ContainerControl
    {
    public:
        /// <summary>
        /// The widgets margin.
        /// </summary>
        constexpr static float WidgetsMargin = 4;

        /// <summary>
        /// The widgets height.
        /// </summary>
        constexpr static float WidgetsHeight = 18;

        /// <summary>
        /// The widgets icon size.
        /// </summary>
        constexpr static float WidgetsIconSize = 16;

        /// <summary>
        /// Gets the widget location.
        /// </summary>
        ViewportWidgetLocation WidgetLocation;

        /// <summary>
        /// Initializes a new instance of the <see cref="ViewportWidgetsContainer"/> class.
        /// </summary>
        /// <param name="location">The location.</param>
        ViewportWidgetsContainer(ViewportWidgetLocation location);

        /// <inheritdoc />
        void Draw() override;

        /// <inheritdoc />
        void OnChildResized(Control* control) override
        {
            ContainerControl::OnChildResized(control);

            PerformLayout();
        }

        /// <summary>
        /// Arranges the widgets of the control.
        /// </summary>
        /// <param name="control">The control.</param>
        static void ArrangeWidgets(ContainerControl control);

    protected:
        /// <inheritdoc />
        void PerformLayoutBeforeChildren() override;
    };

} // SE

#pragma once
#include "PanelWithMargins.h"

namespace SE
{
    /// <summary>
    /// This panel arranges child controls vertically.
    /// </summary>
    class SE_API_RUNTIME VerticalPanel : public PanelWithMargins
    {
    public:
        /// <summary>
        /// Initializes a new instance of the <see cref="VerticalPanel"/> class.
        /// </summary>
        VerticalPanel()
        {
        }

    protected:
        /// <inheritdoc />
        void PerformLayoutBeforeChildren() override;

        /// <inheritdoc />
        void PerformLayoutAfterChildren() override;
    };

} // SE

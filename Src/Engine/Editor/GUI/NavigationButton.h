#pragma once
#include "Runtime/UI/GUI/Common/Button.h"

namespace SE::Editor
{
    /// <summary>
    /// A navigation bar button. Allows to change the current location and view the path.
    /// </summary>
    class NavigationButton : public Button
    {
        SE_DEFINE_CLASS(NavigationButton, Button)
    protected:
        /// <summary>
        /// The valid drag is over flag.
        /// </summary>
        bool m_ValidDragOver = false;

    public:
        /// <summary>
        /// The default margin (horizontal).
        /// </summary>
        static constexpr float DefaultMargin = 6.0f;

        NavigationButton();

        /// <summary>
        /// Initializes a new instance of the <see cref="NavigationButton"/> class.
        /// </summary>
        /// <param name="x">The x position.</param>
        /// <param name="y">The y position.</param>
        /// <param name="height">The height.</param>
        NavigationButton(float x, float y, float height);

        /// <inheritdoc />
        void Draw() override;

        /// <inheritdoc />
        void PerformLayout(bool force = false) override;

    };

} // SE


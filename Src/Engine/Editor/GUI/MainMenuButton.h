#pragma once

#include "Runtime/UI/GUI/Control.h"

namespace SE::Editor
{
    class ContextMenu;

    class MainMenuButton : public Control
    {
        SE_CLASS_DEFAULT(MainMenuButton, Control)
    public:
        /// <summary>
        /// The button text.
        /// </summary>
        String Text;

        /// <summary>
        /// The context menu.
        /// </summary>
    	ContextMenu* ContextMenu = nullptr;

        /// <summary>
        /// The background color when mouse is over.
        /// </summary>
        Color BackgroundColorMouseOver = Color(0, 0, 0, 0);

        /// <summary>
        /// The background color when mouse is over and context menu is opened.
        /// </summary>
        Color BackgroundColorMouseOverOpened = Color(0, 0, 0, 0);

        /// <summary>
        /// Initializes a new instance of the <see cref="MainMenuButton"/> class.
        /// </summary>
        /// <param name="text">The text.</param>
        MainMenuButton(String text);

        /// <inheritdoc />
        void Draw() override;

        /// <inheritdoc />
        bool OnMouseDown(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        void OnMouseEnter(Float2 location) override;

        /// <inheritdoc />
        void PerformLayout(bool force = false) override;
    };

} // SE


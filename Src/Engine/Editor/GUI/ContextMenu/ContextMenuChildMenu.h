#pragma once

#include "ContextMenuButton.h"

namespace SE::Editor
{
    class ContextMenu;

	class ContextMenuChildMenu : public ContextMenuButton
    {
		SE_CLASS_DEFAULT(ContextMenuChildMenu, ContextMenuButton)
	public:
        /// <summary>
        /// The child context menu.
        /// </summary>
	    ContextMenu* ContextMenu = nullptr;

        /// <summary>
        /// Initializes a new instance of the <see cref="ContextMenuChildMenu"/> class.
        /// </summary>
        /// <param name="parent">The parent context menu.</param>
        /// <param name="text">The text.</param>
        ContextMenuChildMenu(::SE::Editor::ContextMenu* parent, String text);

        /// <inheritdoc />
	    void Draw() override;

        /// <inheritdoc />
        void OnMouseEnter(Float2 location) override;

        /// <inheritdoc />
	    bool OnMouseUp(Float2 location, MouseButton button) override;

	private:
	    void ShowChild(::SE::Editor::ContextMenu* parentContextMenu);

    };

} // SE


#pragma once
#include "Runtime/Render/2D/SpriteAtlas.h"
#include "Runtime/UI/GUI/ContainerControl.h"

namespace SE::Editor
{
    class ToolStripSeparator;
    class ToolStripButton;

    /// <summary>
    /// Tool strip with child items.
    /// </summary>
    SE_CLASS(Reflect)
    class ToolStrip : public ContainerControl
    {
        SE_DEFINE_CLASS_DEFAULT(ToolStrip, ContainerControl)
    public:
        /// <summary>
        /// The default margin vertically.
        /// </summary>
        constexpr static int DefaultMarginV = 1;

        /// <summary>
        /// The default margin horizontally.
        /// </summary>
        constexpr static int DefaultMarginH = 2;

        /// <summary>
        /// Event fired when button gets clicked with the primary mouse button.
        /// </summary>
        Function<void(ToolStripButton*)> ButtonClicked;

        /// <summary>
        /// Event fired when button gets clicked with the secondary mouse button.
        /// </summary>
        Function<void(ToolStripButton*)> SecondaryButtonClicked;

        /// <summary>
        /// Tries to get the last button.
        /// </summary>
        PRO_GET(LastButton, ToolStrip, ToolStripButton*, __GetLastButton);

        /// <summary>
        /// Gets amount of buttons that has been added
        /// </summary>
        PRO_GET(ButtonsCount, ToolStrip, int, __GetButtonsCount);


        /// <summary>
        /// Gets the height for the items.
        /// </summary>
        PRO_GET(ItemsHeight, ToolStrip, float, __GetItemsHeight);

        /// <summary>
        /// Initializes a new instance of the <see cref="ToolStrip"/> class.
        /// </summary>
        /// <param name="height">The toolstrip height.</param>
        /// <param name="y">The toolstrip Y position.</param>
        ToolStrip(float height, float y = 0);

        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="sprite">The icon sprite.</param>
        /// <param name="onClick">The custom action to call on button clicked.</param>
        /// <returns>The button.</returns>
        ToolStripButton* AddButton(SpriteHandle sprite, Function<void()> onClick = Function<void()>());

        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="sprite">The icon sprite.</param>
        /// <param name="text">The text.</param>
        /// <param name="onClick">The custom action to call on button clicked.</param>
        /// <returns>The button.</returns>
        ToolStripButton* AddButton(SpriteHandle sprite, String text, Function<void()> onClick = Function<void()>());

        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="onClick">The custom action to call on button clicked.</param>
        /// <returns>The button.</returns>
        ToolStripButton* AddButton(String text, Function<void()> onClick = Function<void()>());

        /// <summary>
        /// Adds the separator.
        /// </summary>
        /// <returns>The separator.</returns>
        ToolStripSeparator* AddSeparator();

        void OnButtonClicked(ToolStripButton* button)
        {
            if (ButtonClicked.IsBinded())
            {
                ButtonClicked(button);
            }
        }

        void OnSecondaryButtonClicked(ToolStripButton* button)
        {
            if (SecondaryButtonClicked.IsBinded())
            {
                SecondaryButtonClicked(button);
            }
        }

        /// <inheritdoc />
        void OnChildResized(Control* control) override;

        /// <inheritdoc />
        bool OnKeyDown(KeyboardKeys key) override;

    protected:
        /// <inheritdoc />
        void PerformLayoutBeforeChildren() override;

    private:
        ToolStripButton* __GetLastButton();

        int __GetButtonsCount();

        float __GetItemsHeight();
    };

} // SE


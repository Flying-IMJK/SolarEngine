#pragma once
#include "Runtime/Render/2D/SpriteAtlas.h"
#include "Runtime/UI/GUI/Control.h"

namespace SE::Editor
{
    class ContextMenu;

    /// <summary>
    /// Tool strip button control.
    /// </summary>
    class ToolStripButton : public Control
    {
        SE_DEFINE_CLASS_DEFAULT(ToolStripButton, Control)
    private:
        SpriteHandle _icon;
        String _text;
        bool _primaryMouseDown;
        bool _secondaryMouseDown;

    public:
        /// <summary>
        /// The default margin for button parts (icon, text, etc.).
        /// </summary>
        constexpr static int DefaultMargin = 2;

        /// <summary>
        /// Event fired when user clicks the button.
        /// </summary>
        Action Clicked;

        /// <summary>
        /// Event fired when user clicks the button.
        /// </summary>
        Action SecondaryClicked;

        /// <summary>
        /// The checked state.
        /// </summary>
        bool Checked;

        /// <summary>
        /// The automatic check mode.
        /// </summary>
        bool AutoCheck;

        /// <summary>
        /// Gets or sets the button text.
        /// </summary>
        PRO_REF(Text, ToolStripButton, String, __GetText, __SetText);

        /// <summary>
        /// The icon.
        /// </summary>
        PRO_REF(Icon, ToolStripButton, SpriteHandle, __GetIcon, __SetIcon);

        /// <summary>
        /// A reference to a context menu to raise when the secondary mouse button is pressed.
        /// </summary>
        ContextMenu* ContextMenu;

        /// <summary>
        /// Initializes a new instance of the <see cref="ToolStripButton"/> class.
        /// </summary>
        /// <param name="height">The height.</param>
        /// <param name="icon">The icon.</param>
        ToolStripButton(float height, SpriteHandle icon);

        /// <summary>
        /// Sets the automatic check mode.
        /// </summary>
        /// <param name="value">True if use auto check, otherwise false.</param>
        /// <returns>This button.</returns>
        ToolStripButton* SetAutoCheck(bool value);

        /// <summary>
        /// Sets the checked state.
        /// </summary>
        /// <param name="value">True if check it, otherwise false.</param>
        /// <returns>This button.</returns>
        ToolStripButton* SetChecked(bool value)
        {
            Checked = value;
            return this;
        }

        /// <inheritdoc />
        void Draw() override;

        /// <inheritdoc />
        void PerformLayout(bool force = false) override;

        /// <inheritdoc />
        bool OnMouseDown(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        bool OnMouseUp(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        void OnMouseLeave() override;

        /// <inheritdoc />
        void OnLostFocus() override;

    private:
        String& __GetText() { return _text; }
        void __SetText(String &value)
        {
            _text = value;
            PerformLayout();
        }

        SpriteHandle& __GetIcon() { return _icon; }
        void __SetIcon(SpriteHandle &value)
        {
            _icon = value;
            PerformLayout();
        }
    };

} // SE

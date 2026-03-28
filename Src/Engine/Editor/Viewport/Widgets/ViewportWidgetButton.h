#pragma once
#include "Runtime/Render/2D/SpriteAtlas.h"
#include "Runtime/UI/GUI/Control.h"

namespace SE::Editor
{
    class ContextMenu;

    class ViewportWidgetButton : public Control
    {
    private:
        String _text;
        ContextMenu* _cm;
        bool _checked;
        bool _autoCheck;
        bool _isMosueDown;
        float _forcedTextWidth;

    public:
        /// <summary>
        /// Event fired when user toggles checked state.
        /// </summary>
        Delegate<ViewportWidgetButton*> Toggled;

        /// <summary>
        /// Event fired when user click the button.
        /// </summary>
        Delegate<ViewportWidgetButton*> Clicked;

        /// <summary>
        /// The icon.
        /// </summary>
        SpriteHandle Icon;

        /// <summary>
        /// Gets or sets the text.
        /// </summary>
        PRO_REF(Text, ViewportWidgetButton, String, __GetText, __SetText);


        /// <summary>
        /// Gets or sets a value indicating whether this <see cref="ViewportWidgetButton"/> is checked.
        /// </summary>
        bool Checked;

        /// <summary>
        /// Initializes a new instance of the <see cref="ViewportWidgetButton"/> class.
        /// </summary>
        /// <param name="text">The text.</param>
        /// <param name="icon">The icon.</param>
        /// <param name="contextMenu">The context menu.</param>
        /// <param name="autoCheck">If set to <c>true</c> will be automatic checked on mouse click.</param>
        /// <param name="textWidth">Forces the text to be drawn with the specified width.</param>
        ViewportWidgetButton(StringView text, SpriteHandle icon, ContextMenu* contextMenu = nullptr, bool autoCheck = false, float textWidth = 0.0f);

        /// <inheritdoc />
        void Draw() override;

        /// <inheritdoc />
        bool OnMouseDown(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        bool OnMouseUp(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        void PerformLayout(bool force = false) override;

    private:
        void CmOnVisibleChanged(Control* control);

        static float CalculateButtonWidth(float textWidth, bool hasIcon);

        String& __GetText() { return _text; }
        void __SetText(String& value);
    };
} // SE

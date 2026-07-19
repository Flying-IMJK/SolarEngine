#pragma once
#include "Runtime/Core/Math/Color.h"
#include "Runtime/Render/2D/Font.h"
#include "Runtime/Render/2D/FontReference.h"
#include "Runtime/Render/2D/SpriteAtlas.h"

namespace SE
{
    class Tooltip;
    class Texture;

    class SE_API_RUNTIME Style
    {
    private:

        FontReference _fontTitle;
        FontReference _fontLarge;
        FontReference _fontMedium;
        FontReference _fontSmall;

    public:

        /// <summary>
        /// Style for the Statusbar
        /// </summary>
        struct StatusbarStyle
        {
            /// <summary>
            /// Color of the Statusbar when in Play Mode
            /// </summary>
            Color PlayMode;

            /// <summary>
            /// Color of the Statusbar when in loading state (e.g. when importing assets)
            /// </summary>
            Color Loading;

            /// <summary>
            /// Color of the Statusbar in its failed state (e.g. with compilation errors)
            /// </summary>
            Color Failed;
        };

        
        /// <summary>
        /// Global GUI style used by all the controls.
        /// </summary>
        static Style* Current;

        /// <summary>
        /// The font title.
        /// </summary>
        PRO(FontTitle, Style, Font*, __GetFontTitle, __SetFontTitle);

        /// <summary>
        /// The font large.
        /// </summary>
        PRO(FontLarge, Style, Font*, __GetFontLarge, __SetFontLarge);

        /// <summary>
        /// The font medium.
        /// </summary>
        PRO(FontMedium, Style, Font*, __GetFontMedium, __SetFontMedium);

        /// <summary>
        /// The font small.
        /// </summary>
        PRO(FontSmall, Style, Font*, __GetFontSmall, __SetFontSmall);

        /// <summary>
        /// The background color.
        /// </summary>
        Color Background;

        /// <summary>
        /// The light background color.
        /// </summary>
        Color LightBackground;

        /// <summary>
        /// The drag window color.
        /// </summary>
        Color DragWindow;

        /// <summary>
        /// The foreground color.
        /// </summary>
        Color Foreground;

        /// <summary>
        /// The foreground grey.
        /// </summary>
        Color ForegroundGrey;

        /// <summary>
        /// The foreground disabled.
        /// </summary>
        Color ForegroundDisabled;

        /// <summary>
        /// The foreground color in viewports (usually have a dark background)
        /// </summary>
        Color ForegroundViewport;

        /// <summary>
        /// The background highlighted color.
        /// </summary>
        Color BackgroundHighlighted;

        /// <summary>
        /// The border highlighted color.
        /// </summary>
        Color BorderHighlighted;

        /// <summary>
        /// The background selected color.
        /// </summary>
        Color BackgroundSelected;

        /// <summary>
        /// The border selected color.
        /// </summary>
        Color BorderSelected;

        /// <summary>
        /// The background normal color.
        /// </summary>
        Color BackgroundNormal;

        /// <summary>
        /// The border normal color.
        /// </summary>
        Color BorderNormal;

        /// <summary>
        /// The text box background color.
        /// </summary>
        Color TextColor;

        /// <summary>
        /// The text box background color.
        /// </summary>
        Color TextBoxBackground;

        /// <summary>
        /// The text box background selected color.
        /// </summary>
        Color TextBoxBackgroundSelected;

        /// <summary>
        /// The collection background color.
        /// </summary>
        Color CollectionBackgroundColor;

        /// <summary>
        /// The progress normal color.
        /// </summary>
        Color ProgressNormal;

        /// <summary>
        /// The selection and drag drop highlights colors.
        /// </summary>
        Color Selection;

        /// <summary>
        /// The selection and drag drop highlights border colors.
        /// </summary>
        Color SelectionBorder;

        /// <summary>
        /// The status bar style
        /// </summary>
        StatusbarStyle Statusbar;

        /// <summary>
        /// The arrow right icon.
        /// </summary>
        SpriteHandle ArrowRight;

        /// <summary>
        /// The arrow down icon.
        /// </summary>
        SpriteHandle ArrowDown;

        /// <summary>
        /// The search icon.
        /// </summary>
        SpriteHandle Search;

        /// <summary>
        /// The settings icon.
        /// </summary>
        SpriteHandle Settings;

        /// <summary>
        /// The cross icon.
        /// </summary>
        SpriteHandle* Cross;

        /// <summary>
        /// The CheckBox intermediate icon.
        /// </summary>
        SpriteHandle CheckBoxIntermediate;

        /// <summary>
        /// The CheckBox tick icon.
        /// </summary>
        SpriteHandle CheckBoxTick;

        /// <summary>
        /// The status bar size grip icon.
        /// </summary>
        SpriteHandle StatusBarSizeGrip;

        /// <summary>
        /// The translate icon.
        /// </summary>
        SpriteHandle Translate;

        /// <summary>
        /// The rotate icon.
        /// </summary>
        SpriteHandle Rotate;

        /// <summary>
        /// The scale icon.
        /// </summary>
        SpriteHandle Scale;

        /// <summary>
        /// The scalar icon.
        /// </summary>
        SpriteHandle Scalar;

        /// <summary>
        /// The shared tooltip control used by the controls if no custom tooltip is provided.
        /// </summary>
        Tooltip* SharedTooltip;

    private:
        Font* __GetFontTitle() { return _fontTitle.GetFont(); }
        void __SetFontTitle(Font* value) { _fontTitle = FontReference(value); }

        Font* __GetFontLarge() { return _fontLarge.GetFont(); }
        void __SetFontLarge(Font* value) { _fontLarge = FontReference(value); }

        Font* __GetFontMedium() { return _fontMedium.GetFont(); }
        void __SetFontMedium(Font* value) { _fontMedium = FontReference(value); }

        Font* __GetFontSmall() { return _fontSmall.GetFont(); }
        void __SetFontSmall(Font* value) { _fontSmall = FontReference(value); }
    };
} // SE

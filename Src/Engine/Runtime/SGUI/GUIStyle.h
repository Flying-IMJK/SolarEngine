#pragma once

#include "Core/Math/Color.h"

#include "Runtime/API.h"


//-------------------------------------------------------------------------

namespace SE::GUI
{
    struct SE_API_RUNTIME Style
    {
        friend class GUISystem;

    public:

        // Base colors
        //-------------------------------------------------------------------------

        static Color const s_colorGray0; // Brightest
        static Color const s_colorGray1;
        static Color const s_colorGray2;
        static Color const s_colorGray3;
        static Color const s_colorGray4;
        static Color const s_colorGray5;
        static Color const s_colorGray6;
        static Color const s_colorGray7;
        static Color const s_colorGray8;
        static Color const s_colorGray9; // Darkest

        static Color const s_colorText;
        static Color const s_colorTextDisabled;

        // Accents
        //-------------------------------------------------------------------------

        static Color const s_colorAccent0;  // Brightest
        static Color const s_colorAccent1;
        static Color const s_colorAccent2;  // Darkest

        // Misc settings
        //-------------------------------------------------------------------------

        constexpr static float const s_toolTipDelay = 0.2f;

    public:

        static void Apply();
    };

	struct SE_API_RUNTIME TextColorScope
	{
		TextColorScope(const Color &color);
		~TextColorScope();
	};
}
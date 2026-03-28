#include "ReflectorSettingsAndUtils.h"

//-------------------------------------------------------------------------

namespace SE::ReflectTool
{
    static Char const* g_macroNames[] =
    {
        SE_TEXT("ENGINE_REFLECT_MODULE"),
        SE_TEXT("SE_ENUM"),
        SE_TEXT("SE_CLASS"),
        SE_TEXT("SE_PROPERTY"),
        SE_TEXT("SE_FUNC"),
        SE_TEXT("SE_META"),
    };

    //-------------------------------------------------------------------------

    Char const* GetReflectionMacroText( ReflectionMacroType macro )
    {
        uint32_t const macroIdx = static_cast<uint32_t>(macro);
        ENGINE_ASSERT( macroIdx < static_cast<uint32_t>(ReflectionMacroType::NumMacros));
        return g_macroNames[macroIdx];
    }
}
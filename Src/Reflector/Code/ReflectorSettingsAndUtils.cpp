#include "ReflectorSettingsAndUtils.h"

//-------------------------------------------------------------------------

namespace SE::ReflectTool
{
    /*SE_TEXT("DEFINE_CLASS"),           // DefineClass
    SE_TEXT("DEFINE_CLASS_DEFAULT"),   // DefineClassDefault
    SE_TEXT("ENGINE_REFLECT_MODULE"),  // ReflectModule*/
    static Char const* g_macroNames[] =
    {
        SE_TEXT("SE_META"),               // ReflectMeta

        SE_TEXT("SE_CLASS"),              // SEClass
        SE_TEXT("SE_STRUCT"),             // SEStruct
        SE_TEXT("SE_INTERFACE"),          // SEInterface
        SE_TEXT("SE_ENUM"),               // SEEnum
        SE_TEXT("SE_PROPERTY"),           // SEProperty
        SE_TEXT("SE_FUNCTION"),           // SEFunction
        SE_TEXT("SE_EVENT"),              // SEEvent
    };

    //-------------------------------------------------------------------------

    Char const* GetReflectionMacroText( ReflectionMacroType macro )
    {
        switch ( macro )
        {
        case ReflectionMacroType::ReflectMeta:
            return SE_TEXT("SE_META");
        case ReflectionMacroType::SEClass:
            return SE_TEXT("SE_CLASS");
        case ReflectionMacroType::SEStruct:
            return SE_TEXT("SE_STRUCT");
        case ReflectionMacroType::SEInterface:
            return SE_TEXT("SE_INTERFACE");
        case ReflectionMacroType::SEEnum:
            return SE_TEXT("SE_ENUM");
        case ReflectionMacroType::SEProperty:
            return SE_TEXT("SE_PROPERTY");
        case ReflectionMacroType::SEFunction:
            return SE_TEXT("SE_FUNCTION");
        case ReflectionMacroType::SEEvent:
            return SE_TEXT("SE_EVENT");
        case ReflectionMacroType::NumMacros:
            break;
        }
        return SE_TEXT("");
    }
}
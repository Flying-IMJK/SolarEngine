#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "e:/engineproject/solarengine/src/engine/core/input/enums.h"
//-------------------------------------------------------------------------
// Enum Info: SE:: CursorLockMode
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeEnumInfo.h"


namespace SE
{
    template<>
    class TTypeEnumInfo<::SE::CursorLockMode> final : public TypeEnumInfo
    {
    public:
        static TTypeEnumInfo<::SE::CursorLockMode> * s_pTypeInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo == nullptr);
            s_pTypeInfo = New<TTypeEnumInfo<::SE::CursorLockMode>>();

            Types::RegisterEnum(s_pTypeInfo);
        };

        // Static unregistration Function
        //-------------------------------------------------------------------------
        static void UnregisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo != nullptr);
            Types::UnregisterEnum(s_pTypeInfo);
            Delete(s_pTypeInfo);
        };

        // Constructor
        //-------------------------------------------------------------------------
        TTypeEnumInfo()
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::CursorLockMode"));
            size = sizeof(::SE::CursorLockMode);
            alignment = alignof(::SE::CursorLockMode);
            name = SE_TEXT("CursorLockMode");
            fullName = SE_TEXT("SE::CursorLockMode");
            underlyingType = TypeIDCore::Int32;

            TypeEnumInfo::ConstantInfo constantInfo;
            constantInfo.id = StringID(SE_TEXT("None"));
            constantInfo.value = 0;
            constantInfo.alphabeticalOrder = 2;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The default mode. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Locked"));
            constantInfo.value = 1;
            constantInfo.alphabeticalOrder = 1;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; Cursor position is locked to the center of the game window. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Clipped"));
            constantInfo.value = 2;
            constantInfo.alphabeticalOrder = 0;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; Cursor position is confined to the bounds of the game window. &lt;/summary&gt;");
            constants.Add(constantInfo);
        };
    };

    TTypeEnumInfo<::SE::CursorLockMode> * TTypeEnumInfo<::SE::CursorLockMode>::s_pTypeInfo = nullptr;
}

//-------------------------------------------------------------------------
// Enum Info: SE:: MouseButton
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeEnumInfo.h"


namespace SE
{
    template<>
    class TTypeEnumInfo<::SE::MouseButton> final : public TypeEnumInfo
    {
    public:
        static TTypeEnumInfo<::SE::MouseButton> * s_pTypeInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo == nullptr);
            s_pTypeInfo = New<TTypeEnumInfo<::SE::MouseButton>>();

            Types::RegisterEnum(s_pTypeInfo);
        };

        // Static unregistration Function
        //-------------------------------------------------------------------------
        static void UnregisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo != nullptr);
            Types::UnregisterEnum(s_pTypeInfo);
            Delete(s_pTypeInfo);
        };

        // Constructor
        //-------------------------------------------------------------------------
        TTypeEnumInfo()
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::MouseButton"));
            size = sizeof(::SE::MouseButton);
            alignment = alignof(::SE::MouseButton);
            name = SE_TEXT("MouseButton");
            fullName = SE_TEXT("SE::MouseButton");
            underlyingType = TypeIDCore::Int32;

            TypeEnumInfo::ConstantInfo constantInfo;
            constantInfo.id = StringID(SE_TEXT("None"));
            constantInfo.value = 0;
            constantInfo.alphabeticalOrder = 5;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; No button. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Left"));
            constantInfo.value = 1;
            constantInfo.alphabeticalOrder = 2;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; Left button. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Middle"));
            constantInfo.value = 2;
            constantInfo.alphabeticalOrder = 4;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; Middle button. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Right"));
            constantInfo.value = 3;
            constantInfo.alphabeticalOrder = 6;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; Right button. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Extended1"));
            constantInfo.value = 4;
            constantInfo.alphabeticalOrder = 0;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; Extended button 1 (or XButton1). &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Extended2"));
            constantInfo.value = 5;
            constantInfo.alphabeticalOrder = 1;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; Extended button 2 (or XButton2). &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MAX"));
            constantInfo.value = 6;
            constantInfo.alphabeticalOrder = 3;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; Extended button 2 (or XButton2). &lt;/summary&gt;");
            constants.Add(constantInfo);
        };
    };

    TTypeEnumInfo<::SE::MouseButton> * TTypeEnumInfo<::SE::MouseButton>::s_pTypeInfo = nullptr;
}

//-------------------------------------------------------------------------
// Enum Info: SE:: GamepadAxis
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeEnumInfo.h"


namespace SE
{
    template<>
    class TTypeEnumInfo<::SE::GamepadAxis> final : public TypeEnumInfo
    {
    public:
        static TTypeEnumInfo<::SE::GamepadAxis> * s_pTypeInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo == nullptr);
            s_pTypeInfo = New<TTypeEnumInfo<::SE::GamepadAxis>>();

            Types::RegisterEnum(s_pTypeInfo);
        };

        // Static unregistration Function
        //-------------------------------------------------------------------------
        static void UnregisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo != nullptr);
            Types::UnregisterEnum(s_pTypeInfo);
            Delete(s_pTypeInfo);
        };

        // Constructor
        //-------------------------------------------------------------------------
        TTypeEnumInfo()
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::GamepadAxis"));
            size = sizeof(::SE::GamepadAxis);
            alignment = alignof(::SE::GamepadAxis);
            name = SE_TEXT("GamepadAxis");
            fullName = SE_TEXT("SE::GamepadAxis");
            underlyingType = TypeIDCore::Int32;

            TypeEnumInfo::ConstantInfo constantInfo;
            constantInfo.id = StringID(SE_TEXT("None"));
            constantInfo.value = 0;
            constantInfo.alphabeticalOrder = 4;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; No axis. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("LeftStickX"));
            constantInfo.value = 1;
            constantInfo.alphabeticalOrder = 0;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The X-Axis of the left thumb stick. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("LeftStickY"));
            constantInfo.value = 2;
            constantInfo.alphabeticalOrder = 1;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The Y-Axis of the left thumb stick. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("RightStickX"));
            constantInfo.value = 3;
            constantInfo.alphabeticalOrder = 5;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The X-Axis of the right thumb stick. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("RightStickY"));
            constantInfo.value = 4;
            constantInfo.alphabeticalOrder = 6;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The Y-Axis of the right thumb stick. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("LeftTrigger"));
            constantInfo.value = 5;
            constantInfo.alphabeticalOrder = 2;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The left trigger. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("RightTrigger"));
            constantInfo.value = 6;
            constantInfo.alphabeticalOrder = 7;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The right trigger. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MAX"));
            constantInfo.value = 7;
            constantInfo.alphabeticalOrder = 3;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The right trigger. &lt;/summary&gt;");
            constants.Add(constantInfo);
        };
    };

    TTypeEnumInfo<::SE::GamepadAxis> * TTypeEnumInfo<::SE::GamepadAxis>::s_pTypeInfo = nullptr;
}

//-------------------------------------------------------------------------
// Enum Info: SE:: GamepadButton
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeEnumInfo.h"


namespace SE
{
    template<>
    class TTypeEnumInfo<::SE::GamepadButton> final : public TypeEnumInfo
    {
    public:
        static TTypeEnumInfo<::SE::GamepadButton> * s_pTypeInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo == nullptr);
            s_pTypeInfo = New<TTypeEnumInfo<::SE::GamepadButton>>();

            Types::RegisterEnum(s_pTypeInfo);
        };

        // Static unregistration Function
        //-------------------------------------------------------------------------
        static void UnregisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo != nullptr);
            Types::UnregisterEnum(s_pTypeInfo);
            Delete(s_pTypeInfo);
        };

        // Constructor
        //-------------------------------------------------------------------------
        TTypeEnumInfo()
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::GamepadButton"));
            size = sizeof(::SE::GamepadButton);
            alignment = alignof(::SE::GamepadButton);
            name = SE_TEXT("GamepadButton");
            fullName = SE_TEXT("SE::GamepadButton");
            underlyingType = TypeIDCore::Int32;

            TypeEnumInfo::ConstantInfo constantInfo;
            constantInfo.id = StringID(SE_TEXT("None"));
            constantInfo.value = 0;
            constantInfo.alphabeticalOrder = 15;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; No buttons. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("DPadUp"));
            constantInfo.value = 1;
            constantInfo.alphabeticalOrder = 6;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; PadUp button (DPad / Directional Pad). &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("DPadDown"));
            constantInfo.value = 2;
            constantInfo.alphabeticalOrder = 3;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; PadDown button (DPad / Directional Pad). &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("DPadLeft"));
            constantInfo.value = 3;
            constantInfo.alphabeticalOrder = 4;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; PadLeft button (DPad / Directional Pad). &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("DPadRight"));
            constantInfo.value = 4;
            constantInfo.alphabeticalOrder = 5;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; PadRight button (DPad / Directional Pad). &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Start"));
            constantInfo.value = 5;
            constantInfo.alphabeticalOrder = 23;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; Start button. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Back"));
            constantInfo.value = 6;
            constantInfo.alphabeticalOrder = 2;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; Back button. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("LeftThumb"));
            constantInfo.value = 7;
            constantInfo.alphabeticalOrder = 12;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; Left thumbstick button. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("RightThumb"));
            constantInfo.value = 8;
            constantInfo.alphabeticalOrder = 21;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; Right thumbstick button. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("LeftShoulder"));
            constantInfo.value = 9;
            constantInfo.alphabeticalOrder = 7;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; Left shoulder button. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("RightShoulder"));
            constantInfo.value = 10;
            constantInfo.alphabeticalOrder = 16;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; Right shoulder button. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("LeftTrigger"));
            constantInfo.value = 11;
            constantInfo.alphabeticalOrder = 13;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; Left trigger button. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("RightTrigger"));
            constantInfo.value = 12;
            constantInfo.alphabeticalOrder = 22;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; Right trigger button. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("A"));
            constantInfo.value = 13;
            constantInfo.alphabeticalOrder = 0;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; A (face button down). &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("B"));
            constantInfo.value = 14;
            constantInfo.alphabeticalOrder = 1;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; B (face button right). &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("X"));
            constantInfo.value = 15;
            constantInfo.alphabeticalOrder = 24;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; X (face button left). &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Y"));
            constantInfo.value = 16;
            constantInfo.alphabeticalOrder = 25;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; Y (face button up). &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("LeftStickUp"));
            constantInfo.value = 17;
            constantInfo.alphabeticalOrder = 11;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The left stick up. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("LeftStickDown"));
            constantInfo.value = 18;
            constantInfo.alphabeticalOrder = 8;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The left stick down. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("LeftStickLeft"));
            constantInfo.value = 19;
            constantInfo.alphabeticalOrder = 9;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The left stick left. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("LeftStickRight"));
            constantInfo.value = 20;
            constantInfo.alphabeticalOrder = 10;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The left stick right. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("RightStickUp"));
            constantInfo.value = 21;
            constantInfo.alphabeticalOrder = 20;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The right stick up. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("RightStickDown"));
            constantInfo.value = 22;
            constantInfo.alphabeticalOrder = 17;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The right stick down. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("RightStickLeft"));
            constantInfo.value = 23;
            constantInfo.alphabeticalOrder = 18;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The right stick left. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("RightStickRight"));
            constantInfo.value = 24;
            constantInfo.alphabeticalOrder = 19;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The right stick right. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MAX"));
            constantInfo.value = 25;
            constantInfo.alphabeticalOrder = 14;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The right stick right. &lt;/summary&gt;");
            constants.Add(constantInfo);
        };
    };

    TTypeEnumInfo<::SE::GamepadButton> * TTypeEnumInfo<::SE::GamepadButton>::s_pTypeInfo = nullptr;
}

//-------------------------------------------------------------------------
// Enum Info: SE:: InputActionMode
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeEnumInfo.h"


namespace SE
{
    template<>
    class TTypeEnumInfo<::SE::InputActionMode> final : public TypeEnumInfo
    {
    public:
        static TTypeEnumInfo<::SE::InputActionMode> * s_pTypeInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo == nullptr);
            s_pTypeInfo = New<TTypeEnumInfo<::SE::InputActionMode>>();

            Types::RegisterEnum(s_pTypeInfo);
        };

        // Static unregistration Function
        //-------------------------------------------------------------------------
        static void UnregisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo != nullptr);
            Types::UnregisterEnum(s_pTypeInfo);
            Delete(s_pTypeInfo);
        };

        // Constructor
        //-------------------------------------------------------------------------
        TTypeEnumInfo()
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::InputActionMode"));
            size = sizeof(::SE::InputActionMode);
            alignment = alignof(::SE::InputActionMode);
            name = SE_TEXT("InputActionMode");
            fullName = SE_TEXT("SE::InputActionMode");
            underlyingType = TypeIDCore::Int32;

            TypeEnumInfo::ConstantInfo constantInfo;
            constantInfo.id = StringID(SE_TEXT("Pressing"));
            constantInfo.value = 0;
            constantInfo.alphabeticalOrder = 1;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; User is pressing the key/button. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Press"));
            constantInfo.value = 1;
            constantInfo.alphabeticalOrder = 0;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; User pressed the key/button (but wasn&apos;t pressing it in the previous frame). &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Release"));
            constantInfo.value = 2;
            constantInfo.alphabeticalOrder = 2;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; User released the key/button (was pressing it in the previous frame). &lt;/summary&gt;");
            constants.Add(constantInfo);
        };
    };

    TTypeEnumInfo<::SE::InputActionMode> * TTypeEnumInfo<::SE::InputActionMode>::s_pTypeInfo = nullptr;
}

//-------------------------------------------------------------------------
// Enum Info: SE:: InputActionState
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeEnumInfo.h"


namespace SE
{
    template<>
    class TTypeEnumInfo<::SE::InputActionState> final : public TypeEnumInfo
    {
    public:
        static TTypeEnumInfo<::SE::InputActionState> * s_pTypeInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo == nullptr);
            s_pTypeInfo = New<TTypeEnumInfo<::SE::InputActionState>>();

            Types::RegisterEnum(s_pTypeInfo);
        };

        // Static unregistration Function
        //-------------------------------------------------------------------------
        static void UnregisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo != nullptr);
            Types::UnregisterEnum(s_pTypeInfo);
            Delete(s_pTypeInfo);
        };

        // Constructor
        //-------------------------------------------------------------------------
        TTypeEnumInfo()
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::InputActionState"));
            size = sizeof(::SE::InputActionState);
            alignment = alignof(::SE::InputActionState);
            name = SE_TEXT("InputActionState");
            fullName = SE_TEXT("SE::InputActionState");
            underlyingType = TypeIDCore::Int32;

            TypeEnumInfo::ConstantInfo constantInfo;
            constantInfo.id = StringID(SE_TEXT("None"));
            constantInfo.value = 0;
            constantInfo.alphabeticalOrder = 0;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The key/button is not assigned. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Waiting"));
            constantInfo.value = 1;
            constantInfo.alphabeticalOrder = 4;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The key/button is waiting for input. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Pressing"));
            constantInfo.value = 2;
            constantInfo.alphabeticalOrder = 2;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; User is pressing the key/button. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Press"));
            constantInfo.value = 3;
            constantInfo.alphabeticalOrder = 1;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; User pressed the key/button (but wasn&apos;t pressing it in the previous frame). &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Release"));
            constantInfo.value = 4;
            constantInfo.alphabeticalOrder = 3;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; User released the key/button (was pressing it in the previous frame). &lt;/summary&gt;");
            constants.Add(constantInfo);
        };
    };

    TTypeEnumInfo<::SE::InputActionState> * TTypeEnumInfo<::SE::InputActionState>::s_pTypeInfo = nullptr;
}

//-------------------------------------------------------------------------
// Enum Info: SE:: InputGamepadIndex
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeEnumInfo.h"


namespace SE
{
    template<>
    class TTypeEnumInfo<::SE::InputGamepadIndex> final : public TypeEnumInfo
    {
    public:
        static TTypeEnumInfo<::SE::InputGamepadIndex> * s_pTypeInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo == nullptr);
            s_pTypeInfo = New<TTypeEnumInfo<::SE::InputGamepadIndex>>();

            Types::RegisterEnum(s_pTypeInfo);
        };

        // Static unregistration Function
        //-------------------------------------------------------------------------
        static void UnregisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo != nullptr);
            Types::UnregisterEnum(s_pTypeInfo);
            Delete(s_pTypeInfo);
        };

        // Constructor
        //-------------------------------------------------------------------------
        TTypeEnumInfo()
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::InputGamepadIndex"));
            size = sizeof(::SE::InputGamepadIndex);
            alignment = alignof(::SE::InputGamepadIndex);
            name = SE_TEXT("InputGamepadIndex");
            fullName = SE_TEXT("SE::InputGamepadIndex");
            underlyingType = TypeIDCore::Int32;

            TypeEnumInfo::ConstantInfo constantInfo;
            constantInfo.id = StringID(SE_TEXT("All"));
            constantInfo.value = -1;
            constantInfo.alphabeticalOrder = 0;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; All detected gamepads. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Gamepad0"));
            constantInfo.value = 0;
            constantInfo.alphabeticalOrder = 1;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The gamepad no. 0. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Gamepad1"));
            constantInfo.value = 1;
            constantInfo.alphabeticalOrder = 2;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The gamepad no. 1. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Gamepad2"));
            constantInfo.value = 2;
            constantInfo.alphabeticalOrder = 3;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The gamepad no. 2. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Gamepad3"));
            constantInfo.value = 3;
            constantInfo.alphabeticalOrder = 4;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The gamepad no. 3. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Gamepad4"));
            constantInfo.value = 4;
            constantInfo.alphabeticalOrder = 5;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The gamepad no. 4. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Gamepad5"));
            constantInfo.value = 5;
            constantInfo.alphabeticalOrder = 6;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The gamepad no. 5. &lt;/summary&gt;");
            constants.Add(constantInfo);
        };
    };

    TTypeEnumInfo<::SE::InputGamepadIndex> * TTypeEnumInfo<::SE::InputGamepadIndex>::s_pTypeInfo = nullptr;
}

//-------------------------------------------------------------------------
// Enum Info: SE:: InputAxisType
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeEnumInfo.h"


namespace SE
{
    template<>
    class TTypeEnumInfo<::SE::InputAxisType> final : public TypeEnumInfo
    {
    public:
        static TTypeEnumInfo<::SE::InputAxisType> * s_pTypeInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo == nullptr);
            s_pTypeInfo = New<TTypeEnumInfo<::SE::InputAxisType>>();

            Types::RegisterEnum(s_pTypeInfo);
        };

        // Static unregistration Function
        //-------------------------------------------------------------------------
        static void UnregisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo != nullptr);
            Types::UnregisterEnum(s_pTypeInfo);
            Delete(s_pTypeInfo);
        };

        // Constructor
        //-------------------------------------------------------------------------
        TTypeEnumInfo()
        {
            // Create type info
            id = TypeID(SE_TEXT("SE::InputAxisType"));
            size = sizeof(::SE::InputAxisType);
            alignment = alignof(::SE::InputAxisType);
            name = SE_TEXT("InputAxisType");
            fullName = SE_TEXT("SE::InputAxisType");
            underlyingType = TypeIDCore::Int32;

            TypeEnumInfo::ConstantInfo constantInfo;
            constantInfo.id = StringID(SE_TEXT("MouseX"));
            constantInfo.value = 0;
            constantInfo.alphabeticalOrder = 10;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The mouse X-Axis (mouse delta position scaled by the sensitivity). &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MouseY"));
            constantInfo.value = 1;
            constantInfo.alphabeticalOrder = 11;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The mouse Y-Axis (mouse delta position scaled by the sensitivity). &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MouseWheel"));
            constantInfo.value = 2;
            constantInfo.alphabeticalOrder = 9;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The mouse wheel (mouse wheel delta scaled by the sensitivity). &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("GamepadLeftStickX"));
            constantInfo.value = 3;
            constantInfo.alphabeticalOrder = 2;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The gamepad X-Axis of the left thumb stick. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("GamepadLeftStickY"));
            constantInfo.value = 4;
            constantInfo.alphabeticalOrder = 3;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The gamepad Y-Axis of the left thumb stick. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("GamepadRightStickX"));
            constantInfo.value = 5;
            constantInfo.alphabeticalOrder = 5;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The gamepad X-Axis of the right thumb stick. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("GamepadRightStickY"));
            constantInfo.value = 6;
            constantInfo.alphabeticalOrder = 6;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The gamepad Y-Axis of the right thumb stick. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("GamepadLeftTrigger"));
            constantInfo.value = 7;
            constantInfo.alphabeticalOrder = 4;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The gamepad left trigger. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("GamepadRightTrigger"));
            constantInfo.value = 8;
            constantInfo.alphabeticalOrder = 7;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The gamepad right trigger. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("KeyboardOnly"));
            constantInfo.value = 9;
            constantInfo.alphabeticalOrder = 8;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The keyboard only mode. For key inputs. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("GamepadDPadX"));
            constantInfo.value = 10;
            constantInfo.alphabeticalOrder = 0;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; Pad X axis - left/right (DPad / Directional Pad). &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("GamepadDPadY"));
            constantInfo.value = 11;
            constantInfo.alphabeticalOrder = 1;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; Pad Y axis - up/down (DPad / Directional Pad). &lt;/summary&gt;");
            constants.Add(constantInfo);
        };
    };

    TTypeEnumInfo<::SE::InputAxisType> * TTypeEnumInfo<::SE::InputAxisType>::s_pTypeInfo = nullptr;
}


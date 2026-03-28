#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "e:/engineproject/solarengine/src/engine/runtime/graphics/textures/gpusamplerdescription.h"
//-------------------------------------------------------------------------
// Enum Info: SE:: GPUSamplerFilter
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeEnumInfo.h"


namespace SE
{
    template<>
    class TTypeEnumInfo<::SE::GPUSamplerFilter> final : public TypeEnumInfo
    {
    public:
        static TTypeEnumInfo<::SE::GPUSamplerFilter> * s_pTypeInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo == nullptr);
            s_pTypeInfo = New<TTypeEnumInfo<::SE::GPUSamplerFilter>>();

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
            id = TypeID(SE_TEXT("SE::GPUSamplerFilter"));
            size = sizeof(::SE::GPUSamplerFilter);
            alignment = alignof(::SE::GPUSamplerFilter);
            name = SE_TEXT("GPUSamplerFilter");
            fullName = SE_TEXT("SE::GPUSamplerFilter");
            underlyingType = TypeIDCore::Int32;

            TypeEnumInfo::ConstantInfo constantInfo;
            constantInfo.id = StringID(SE_TEXT("Point"));
            constantInfo.value = 0;
            constantInfo.alphabeticalOrder = 3;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt;Filter using the nearest found pixel. Texture appears pixelated.&lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Bilinear"));
            constantInfo.value = 1;
            constantInfo.alphabeticalOrder = 1;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt;Filter using the linear average of the nearby pixels. Texture appears blurry.&lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Trilinear"));
            constantInfo.value = 2;
            constantInfo.alphabeticalOrder = 4;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt;Filter using the linear average of the nearby pixels and nearby mipmaps. Texture appears blurry.&lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Anisotropic"));
            constantInfo.value = 3;
            constantInfo.alphabeticalOrder = 0;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt;Filter using the anisotropic filtering that improves quality when viewing textures at a steep angles. Texture appears sharp at extreme viewing angles.&lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MAX"));
            constantInfo.value = 4;
            constantInfo.alphabeticalOrder = 2;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt;Filter using the anisotropic filtering that improves quality when viewing textures at a steep angles. Texture appears sharp at extreme viewing angles.&lt;/summary&gt;");
            constants.Add(constantInfo);
        };
    };

    TTypeEnumInfo<::SE::GPUSamplerFilter> * TTypeEnumInfo<::SE::GPUSamplerFilter>::s_pTypeInfo = nullptr;
}

//-------------------------------------------------------------------------
// Enum Info: SE:: GPUSamplerAddressMode
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeEnumInfo.h"


namespace SE
{
    template<>
    class TTypeEnumInfo<::SE::GPUSamplerAddressMode> final : public TypeEnumInfo
    {
    public:
        static TTypeEnumInfo<::SE::GPUSamplerAddressMode> * s_pTypeInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo == nullptr);
            s_pTypeInfo = New<TTypeEnumInfo<::SE::GPUSamplerAddressMode>>();

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
            id = TypeID(SE_TEXT("SE::GPUSamplerAddressMode"));
            size = sizeof(::SE::GPUSamplerAddressMode);
            alignment = alignof(::SE::GPUSamplerAddressMode);
            name = SE_TEXT("GPUSamplerAddressMode");
            fullName = SE_TEXT("SE::GPUSamplerAddressMode");
            underlyingType = TypeIDCore::Int32;

            TypeEnumInfo::ConstantInfo constantInfo;
            constantInfo.id = StringID(SE_TEXT("Wrap"));
            constantInfo.value = 0;
            constantInfo.alphabeticalOrder = 4;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt;Texture coordinates wrap back to the valid range.&lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Clamp"));
            constantInfo.value = 1;
            constantInfo.alphabeticalOrder = 1;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt;Texture coordinates are clamped within the valid range.&lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Mirror"));
            constantInfo.value = 2;
            constantInfo.alphabeticalOrder = 3;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt;Texture coordinates flip every time the size of the valid range is passed.&lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Border"));
            constantInfo.value = 3;
            constantInfo.alphabeticalOrder = 0;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt;Texture coordinates outside of the valid range will return a separately set border color.&lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MAX"));
            constantInfo.value = 4;
            constantInfo.alphabeticalOrder = 2;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt;Texture coordinates outside of the valid range will return a separately set border color.&lt;/summary&gt;");
            constants.Add(constantInfo);
        };
    };

    TTypeEnumInfo<::SE::GPUSamplerAddressMode> * TTypeEnumInfo<::SE::GPUSamplerAddressMode>::s_pTypeInfo = nullptr;
}

//-------------------------------------------------------------------------
// Enum Info: SE:: GPUSamplerCompareFunction
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeEnumInfo.h"


namespace SE
{
    template<>
    class TTypeEnumInfo<::SE::GPUSamplerCompareFunction> final : public TypeEnumInfo
    {
    public:
        static TTypeEnumInfo<::SE::GPUSamplerCompareFunction> * s_pTypeInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo == nullptr);
            s_pTypeInfo = New<TTypeEnumInfo<::SE::GPUSamplerCompareFunction>>();

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
            id = TypeID(SE_TEXT("SE::GPUSamplerCompareFunction"));
            size = sizeof(::SE::GPUSamplerCompareFunction);
            alignment = alignof(::SE::GPUSamplerCompareFunction);
            name = SE_TEXT("GPUSamplerCompareFunction");
            fullName = SE_TEXT("SE::GPUSamplerCompareFunction");
            underlyingType = TypeIDCore::Int32;

            TypeEnumInfo::ConstantInfo constantInfo;
            constantInfo.id = StringID(SE_TEXT("Never"));
            constantInfo.value = 0;
            constantInfo.alphabeticalOrder = 2;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt;Never pass the comparison.&lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Less"));
            constantInfo.value = 1;
            constantInfo.alphabeticalOrder = 0;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt;If the source data is less than the destination data, the comparison passes.&lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MAX"));
            constantInfo.value = 2;
            constantInfo.alphabeticalOrder = 1;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt;If the source data is less than the destination data, the comparison passes.&lt;/summary&gt;");
            constants.Add(constantInfo);
        };
    };

    TTypeEnumInfo<::SE::GPUSamplerCompareFunction> * TTypeEnumInfo<::SE::GPUSamplerCompareFunction>::s_pTypeInfo = nullptr;
}

//-------------------------------------------------------------------------
// Enum Info: SE:: GPUSamplerBorderColor
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeEnumInfo.h"


namespace SE
{
    template<>
    class TTypeEnumInfo<::SE::GPUSamplerBorderColor> final : public TypeEnumInfo
    {
    public:
        static TTypeEnumInfo<::SE::GPUSamplerBorderColor> * s_pTypeInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo == nullptr);
            s_pTypeInfo = New<TTypeEnumInfo<::SE::GPUSamplerBorderColor>>();

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
            id = TypeID(SE_TEXT("SE::GPUSamplerBorderColor"));
            size = sizeof(::SE::GPUSamplerBorderColor);
            alignment = alignof(::SE::GPUSamplerBorderColor);
            name = SE_TEXT("GPUSamplerBorderColor");
            fullName = SE_TEXT("SE::GPUSamplerBorderColor");
            underlyingType = TypeIDCore::Int32;

            TypeEnumInfo::ConstantInfo constantInfo;
            constantInfo.id = StringID(SE_TEXT("TransparentBlack"));
            constantInfo.value = 0;
            constantInfo.alphabeticalOrder = 3;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; Indicates black, with the alpha component as fully transparent. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("OpaqueBlack"));
            constantInfo.value = 1;
            constantInfo.alphabeticalOrder = 1;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; Indicates black, with the alpha component as fully opaque. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("OpaqueWhite"));
            constantInfo.value = 2;
            constantInfo.alphabeticalOrder = 2;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; Indicates white, with the alpha component as fully opaque. &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("MAX"));
            constantInfo.value = 3;
            constantInfo.alphabeticalOrder = 0;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; Indicates white, with the alpha component as fully opaque. &lt;/summary&gt;");
            constants.Add(constantInfo);
        };
    };

    TTypeEnumInfo<::SE::GPUSamplerBorderColor> * TTypeEnumInfo<::SE::GPUSamplerBorderColor>::s_pTypeInfo = nullptr;
}


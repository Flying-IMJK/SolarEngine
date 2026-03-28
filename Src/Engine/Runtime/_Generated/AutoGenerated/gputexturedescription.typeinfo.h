#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "e:/engineproject/solarengine/src/engine/runtime/graphics/textures/gputexturedescription.h"
//-------------------------------------------------------------------------
// Enum Info: SE:: TextureFormatType
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeEnumInfo.h"


namespace SE
{
    template<>
    class TTypeEnumInfo<::SE::TextureFormatType> final : public TypeEnumInfo
    {
    public:
        static TTypeEnumInfo<::SE::TextureFormatType> * s_pTypeInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo == nullptr);
            s_pTypeInfo = New<TTypeEnumInfo<::SE::TextureFormatType>>();

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
            id = TypeID(SE_TEXT("SE::TextureFormatType"));
            size = sizeof(::SE::TextureFormatType);
            alignment = alignof(::SE::TextureFormatType);
            name = SE_TEXT("TextureFormatType");
            fullName = SE_TEXT("SE::TextureFormatType");
            underlyingType = TypeIDCore::Uint8;

            TypeEnumInfo::ConstantInfo constantInfo;
            constantInfo.id = StringID(SE_TEXT("Unknown"));
            constantInfo.value = 0;
            constantInfo.alphabeticalOrder = 7;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "Invalid value.");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("ColorRGB"));
            constantInfo.value = 1;
            constantInfo.alphabeticalOrder = 0;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "The color with RGB channels.");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("ColorRGBA"));
            constantInfo.value = 2;
            constantInfo.alphabeticalOrder = 1;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "The color with RGBA channels.");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("NormalMap"));
            constantInfo.value = 3;
            constantInfo.alphabeticalOrder = 6;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "Normal map data (packed and compressed).");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("GrayScale"));
            constantInfo.value = 4;
            constantInfo.alphabeticalOrder = 3;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "The gray scale (R channel).");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("HdrRGBA"));
            constantInfo.value = 5;
            constantInfo.alphabeticalOrder = 5;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "The HDR color (RGBA channels).");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("HdrRGB"));
            constantInfo.value = 6;
            constantInfo.alphabeticalOrder = 4;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "The HDR color (RGB channels).");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("CubeMap"));
            constantInfo.value = 7;
            constantInfo.alphabeticalOrder = 2;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
        };
    };

    TTypeEnumInfo<::SE::TextureFormatType> * TTypeEnumInfo<::SE::TextureFormatType>::s_pTypeInfo = nullptr;
}

//-------------------------------------------------------------------------
// Enum Info: SE:: TextureDimensions
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeEnumInfo.h"


namespace SE
{
    template<>
    class TTypeEnumInfo<::SE::TextureDimensions> final : public TypeEnumInfo
    {
    public:
        static TTypeEnumInfo<::SE::TextureDimensions> * s_pTypeInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo == nullptr);
            s_pTypeInfo = New<TTypeEnumInfo<::SE::TextureDimensions>>();

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
            id = TypeID(SE_TEXT("SE::TextureDimensions"));
            size = sizeof(::SE::TextureDimensions);
            alignment = alignof(::SE::TextureDimensions);
            name = SE_TEXT("TextureDimensions");
            fullName = SE_TEXT("SE::TextureDimensions");
            underlyingType = TypeIDCore::Int32;

            TypeEnumInfo::ConstantInfo constantInfo;
            constantInfo.id = StringID(SE_TEXT("Texture"));
            constantInfo.value = 0;
            constantInfo.alphabeticalOrder = 1;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The texture (2d). &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("VolumeTexture"));
            constantInfo.value = 1;
            constantInfo.alphabeticalOrder = 2;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The volume texture (3d texture). &lt;/summary&gt;");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("CubeTexture"));
            constantInfo.value = 2;
            constantInfo.alphabeticalOrder = 0;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "&lt;summary&gt; The cube texture (2d texture array of 6 items). &lt;/summary&gt;");
            constants.Add(constantInfo);
        };
    };

    TTypeEnumInfo<::SE::TextureDimensions> * TTypeEnumInfo<::SE::TextureDimensions>::s_pTypeInfo = nullptr;
}


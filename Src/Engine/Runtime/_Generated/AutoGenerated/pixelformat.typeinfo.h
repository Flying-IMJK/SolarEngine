#pragma once
//*************************************************************************
// This is an auto-generated file - DO NOT edit
//*************************************************************************
#include "e:/engineproject/solarengine/src/engine/runtime/graphics/base/pixelformat.h"
//-------------------------------------------------------------------------
// Enum Info: SE:: PixelFormat
//-------------------------------------------------------------------------

#include "Core/TypeSystem/Info/TypeEnumInfo.h"


namespace SE
{
    template<>
    class TTypeEnumInfo<::SE::PixelFormat> final : public TypeEnumInfo
    {
    public:
        static TTypeEnumInfo<::SE::PixelFormat> * s_pTypeInfo;

        // Static registration Function
        //-------------------------------------------------------------------------
        static void RegisterType()
        {
            ENGINE_ASSERT(s_pTypeInfo == nullptr);
            s_pTypeInfo = New<TTypeEnumInfo<::SE::PixelFormat>>();

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
            id = TypeID(SE_TEXT("SE::PixelFormat"));
            size = sizeof(::SE::PixelFormat);
            alignment = alignof(::SE::PixelFormat);
            name = SE_TEXT("PixelFormat");
            fullName = SE_TEXT("SE::PixelFormat");
            underlyingType = TypeIDCore::Int32;

            TypeEnumInfo::ConstantInfo constantInfo;
            constantInfo.id = StringID(SE_TEXT("Undefined"));
            constantInfo.value = 0;
            constantInfo.alphabeticalOrder = 66;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R32G32B32A32_Float"));
            constantInfo.value = 1;
            constantInfo.alphabeticalOrder = 40;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R32G32B32A32_UInt"));
            constantInfo.value = 2;
            constantInfo.alphabeticalOrder = 42;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R32G32B32A32_SInt"));
            constantInfo.value = 3;
            constantInfo.alphabeticalOrder = 41;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R32G32B32_Float"));
            constantInfo.value = 4;
            constantInfo.alphabeticalOrder = 43;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R32G32B32_UInt"));
            constantInfo.value = 5;
            constantInfo.alphabeticalOrder = 45;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R32G32B32_SInt"));
            constantInfo.value = 6;
            constantInfo.alphabeticalOrder = 44;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R16G16B16A16_Float"));
            constantInfo.value = 7;
            constantInfo.alphabeticalOrder = 25;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R16G16B16A16_UNorm"));
            constantInfo.value = 8;
            constantInfo.alphabeticalOrder = 29;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R16G16B16A16_UInt"));
            constantInfo.value = 9;
            constantInfo.alphabeticalOrder = 28;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R16G16B16A16_SNorm"));
            constantInfo.value = 10;
            constantInfo.alphabeticalOrder = 27;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R16G16B16A16_SInt"));
            constantInfo.value = 11;
            constantInfo.alphabeticalOrder = 26;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R32G32_Float"));
            constantInfo.value = 12;
            constantInfo.alphabeticalOrder = 46;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R32G32_UInt"));
            constantInfo.value = 13;
            constantInfo.alphabeticalOrder = 48;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R32G32_SInt"));
            constantInfo.value = 14;
            constantInfo.alphabeticalOrder = 47;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("D32_Float_S8X24_UInt"));
            constantInfo.value = 15;
            constantInfo.alphabeticalOrder = 19;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (32-bit) + stencil (8-bit) | SRV: R32_Float (default or depth aspect), R8_UInt (stencil aspect)");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R10G10B10A2_UNorm"));
            constantInfo.value = 16;
            constantInfo.alphabeticalOrder = 23;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (32-bit) + stencil (8-bit) | SRV: R32_Float (default or depth aspect), R8_UInt (stencil aspect)");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R10G10B10A2_UInt"));
            constantInfo.value = 17;
            constantInfo.alphabeticalOrder = 22;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (32-bit) + stencil (8-bit) | SRV: R32_Float (default or depth aspect), R8_UInt (stencil aspect)");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R11G11B10_Float"));
            constantInfo.value = 18;
            constantInfo.alphabeticalOrder = 24;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (32-bit) + stencil (8-bit) | SRV: R32_Float (default or depth aspect), R8_UInt (stencil aspect)");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R8G8B8A8_UNorm"));
            constantInfo.value = 19;
            constantInfo.alphabeticalOrder = 55;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (32-bit) + stencil (8-bit) | SRV: R32_Float (default or depth aspect), R8_UInt (stencil aspect)");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R8G8B8A8_UNorm_SRGB"));
            constantInfo.value = 20;
            constantInfo.alphabeticalOrder = 56;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (32-bit) + stencil (8-bit) | SRV: R32_Float (default or depth aspect), R8_UInt (stencil aspect)");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R8G8B8A8_UInt"));
            constantInfo.value = 21;
            constantInfo.alphabeticalOrder = 54;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (32-bit) + stencil (8-bit) | SRV: R32_Float (default or depth aspect), R8_UInt (stencil aspect)");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R8G8B8A8_SNorm"));
            constantInfo.value = 22;
            constantInfo.alphabeticalOrder = 53;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (32-bit) + stencil (8-bit) | SRV: R32_Float (default or depth aspect), R8_UInt (stencil aspect)");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R8G8B8A8_SInt"));
            constantInfo.value = 23;
            constantInfo.alphabeticalOrder = 52;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (32-bit) + stencil (8-bit) | SRV: R32_Float (default or depth aspect), R8_UInt (stencil aspect)");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("B8G8R8A8_UNorm"));
            constantInfo.value = 24;
            constantInfo.alphabeticalOrder = 0;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (32-bit) + stencil (8-bit) | SRV: R32_Float (default or depth aspect), R8_UInt (stencil aspect)");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("B8G8R8A8_UNorm_SRGB"));
            constantInfo.value = 25;
            constantInfo.alphabeticalOrder = 1;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (32-bit) + stencil (8-bit) | SRV: R32_Float (default or depth aspect), R8_UInt (stencil aspect)");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R16G16_Float"));
            constantInfo.value = 26;
            constantInfo.alphabeticalOrder = 30;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (32-bit) + stencil (8-bit) | SRV: R32_Float (default or depth aspect), R8_UInt (stencil aspect)");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R16G16_UNorm"));
            constantInfo.value = 27;
            constantInfo.alphabeticalOrder = 34;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (32-bit) + stencil (8-bit) | SRV: R32_Float (default or depth aspect), R8_UInt (stencil aspect)");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R16G16_UInt"));
            constantInfo.value = 28;
            constantInfo.alphabeticalOrder = 33;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (32-bit) + stencil (8-bit) | SRV: R32_Float (default or depth aspect), R8_UInt (stencil aspect)");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R16G16_SNorm"));
            constantInfo.value = 29;
            constantInfo.alphabeticalOrder = 32;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (32-bit) + stencil (8-bit) | SRV: R32_Float (default or depth aspect), R8_UInt (stencil aspect)");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R16G16_SInt"));
            constantInfo.value = 30;
            constantInfo.alphabeticalOrder = 31;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (32-bit) + stencil (8-bit) | SRV: R32_Float (default or depth aspect), R8_UInt (stencil aspect)");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("D32_Float"));
            constantInfo.value = 31;
            constantInfo.alphabeticalOrder = 18;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (32-bit) | SRV: R32_Float");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R32_Float"));
            constantInfo.value = 32;
            constantInfo.alphabeticalOrder = 49;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (32-bit) | SRV: R32_Float");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R32_UInt"));
            constantInfo.value = 33;
            constantInfo.alphabeticalOrder = 51;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (32-bit) | SRV: R32_Float");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R32_SInt"));
            constantInfo.value = 34;
            constantInfo.alphabeticalOrder = 50;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (32-bit) | SRV: R32_Float");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("D24_UNorm_S8_UInt"));
            constantInfo.value = 35;
            constantInfo.alphabeticalOrder = 17;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (24-bit) + stencil (8-bit) | SRV: R24_INTERNAL (default or depth aspect), R8_UInt (stencil aspect)");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R9G9B9E5_SHAREDEXP"));
            constantInfo.value = 36;
            constantInfo.alphabeticalOrder = 65;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (24-bit) + stencil (8-bit) | SRV: R24_INTERNAL (default or depth aspect), R8_UInt (stencil aspect)");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R8G8_UNorm"));
            constantInfo.value = 37;
            constantInfo.alphabeticalOrder = 60;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (24-bit) + stencil (8-bit) | SRV: R24_INTERNAL (default or depth aspect), R8_UInt (stencil aspect)");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R8G8_UInt"));
            constantInfo.value = 38;
            constantInfo.alphabeticalOrder = 59;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (24-bit) + stencil (8-bit) | SRV: R24_INTERNAL (default or depth aspect), R8_UInt (stencil aspect)");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R8G8_SNorm"));
            constantInfo.value = 39;
            constantInfo.alphabeticalOrder = 58;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (24-bit) + stencil (8-bit) | SRV: R24_INTERNAL (default or depth aspect), R8_UInt (stencil aspect)");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R8G8_SInt"));
            constantInfo.value = 40;
            constantInfo.alphabeticalOrder = 57;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (24-bit) + stencil (8-bit) | SRV: R24_INTERNAL (default or depth aspect), R8_UInt (stencil aspect)");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R16_Float"));
            constantInfo.value = 41;
            constantInfo.alphabeticalOrder = 35;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (24-bit) + stencil (8-bit) | SRV: R24_INTERNAL (default or depth aspect), R8_UInt (stencil aspect)");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("D16_UNorm"));
            constantInfo.value = 42;
            constantInfo.alphabeticalOrder = 16;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "depth (16-bit) | SRV: R16_UNorm");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R16_UNorm"));
            constantInfo.value = 43;
            constantInfo.alphabeticalOrder = 39;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R16_UInt"));
            constantInfo.value = 44;
            constantInfo.alphabeticalOrder = 38;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R16_SNorm"));
            constantInfo.value = 45;
            constantInfo.alphabeticalOrder = 37;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R16_SInt"));
            constantInfo.value = 46;
            constantInfo.alphabeticalOrder = 36;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R8_UNorm"));
            constantInfo.value = 47;
            constantInfo.alphabeticalOrder = 64;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R8_UInt"));
            constantInfo.value = 48;
            constantInfo.alphabeticalOrder = 63;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R8_SNorm"));
            constantInfo.value = 49;
            constantInfo.alphabeticalOrder = 62;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("R8_SInt"));
            constantInfo.value = 50;
            constantInfo.alphabeticalOrder = 61;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("BC1_UNorm"));
            constantInfo.value = 51;
            constantInfo.alphabeticalOrder = 2;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "Formats that are not usable in render pass must be below because formats in render pass must be encodable as 6 bits:");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("BC1_UNorm_SRGB"));
            constantInfo.value = 52;
            constantInfo.alphabeticalOrder = 3;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "Formats that are not usable in render pass must be below because formats in render pass must be encodable as 6 bits:");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("BC2_UNorm"));
            constantInfo.value = 53;
            constantInfo.alphabeticalOrder = 4;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "Formats that are not usable in render pass must be below because formats in render pass must be encodable as 6 bits:");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("BC2_UNorm_SRGB"));
            constantInfo.value = 54;
            constantInfo.alphabeticalOrder = 5;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "Formats that are not usable in render pass must be below because formats in render pass must be encodable as 6 bits:");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("BC3_UNorm"));
            constantInfo.value = 55;
            constantInfo.alphabeticalOrder = 6;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "Formats that are not usable in render pass must be below because formats in render pass must be encodable as 6 bits:");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("BC3_UNorm_SRGB"));
            constantInfo.value = 56;
            constantInfo.alphabeticalOrder = 7;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "Formats that are not usable in render pass must be below because formats in render pass must be encodable as 6 bits:");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("BC4_UNorm"));
            constantInfo.value = 57;
            constantInfo.alphabeticalOrder = 9;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "Formats that are not usable in render pass must be below because formats in render pass must be encodable as 6 bits:");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("BC4_SNorm"));
            constantInfo.value = 58;
            constantInfo.alphabeticalOrder = 8;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "Formats that are not usable in render pass must be below because formats in render pass must be encodable as 6 bits:");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("BC5_UNorm"));
            constantInfo.value = 59;
            constantInfo.alphabeticalOrder = 11;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "Formats that are not usable in render pass must be below because formats in render pass must be encodable as 6 bits:");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("BC5_SNorm"));
            constantInfo.value = 60;
            constantInfo.alphabeticalOrder = 10;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "Formats that are not usable in render pass must be below because formats in render pass must be encodable as 6 bits:");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("BC6H_UF16"));
            constantInfo.value = 61;
            constantInfo.alphabeticalOrder = 13;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "Formats that are not usable in render pass must be below because formats in render pass must be encodable as 6 bits:");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("BC6H_SF16"));
            constantInfo.value = 62;
            constantInfo.alphabeticalOrder = 12;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "Formats that are not usable in render pass must be below because formats in render pass must be encodable as 6 bits:");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("BC7_UNorm"));
            constantInfo.value = 63;
            constantInfo.alphabeticalOrder = 14;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "Formats that are not usable in render pass must be below because formats in render pass must be encodable as 6 bits:");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("BC7_UNorm_SRGB"));
            constantInfo.value = 64;
            constantInfo.alphabeticalOrder = 15;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "Formats that are not usable in render pass must be below because formats in render pass must be encodable as 6 bits:");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("NV12"));
            constantInfo.value = 65;
            constantInfo.alphabeticalOrder = 21;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "Formats that are not usable in render pass must be below because formats in render pass must be encodable as 6 bits:");
            constants.Add(constantInfo);
            constantInfo.id = StringID(SE_TEXT("Max"));
            constantInfo.value = 66;
            constantInfo.alphabeticalOrder = 20;
            SE_DEVELOPMENT_ONLY( constantInfo.description = "Formats that are not usable in render pass must be below because formats in render pass must be encodable as 6 bits:");
            constants.Add(constantInfo);
        };
    };

    TTypeEnumInfo<::SE::PixelFormat> * TTypeEnumInfo<::SE::PixelFormat>::s_pTypeInfo = nullptr;
}


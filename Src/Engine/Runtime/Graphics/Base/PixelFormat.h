#pragma once
#include "Runtime/Core/TypeSystem/IType.h"

namespace SE
{
	/*
	 * @param UNorm [0,1] 范围内的无符号标准化值
	 * @param SNorm [-1,1] 范围内的有符号标准化值
	 * @param UInt [0, 2^(n -)] 范围内的无符号整数值
	 * @param SInt [-2^(n-1) ,2^(n-1) -1)] 范围内的有符号整数值
	 * @param SFloat 有符号浮点数
	 * @param SRGB RGB分量是无符号归一化值，使用sRGB非线性编码，而A（如果存在）是无符号归一化值
	 */
	SE_ENUM(Reflect)
	enum class PixelFormat : int32
	{
		Undefined = 0,
		R32G32B32A32_Float,
		R32G32B32A32_UInt,
		R32G32B32A32_SInt,

		R32G32B32_Float,
		R32G32B32_UInt,
		R32G32B32_SInt,

		R16G16B16A16_Float,
		R16G16B16A16_UNorm,
		R16G16B16A16_UInt,
		R16G16B16A16_SNorm,
		R16G16B16A16_SInt,

		R32G32_Float,
		R32G32_UInt,
		R32G32_SInt,
		// depth (32-bit) + stencil (8-bit) | SRV: R32_Float (default or depth aspect), R8_UInt (stencil aspect)
		D32_Float_S8X24_UInt,
		R10G10B10A2_UNorm,
		R10G10B10A2_UInt,
		R11G11B10_Float,
		R8G8B8A8_UNorm,
		R8G8B8A8_UNorm_SRGB,
		R8G8B8A8_UInt,
		R8G8B8A8_SNorm,
		R8G8B8A8_SInt,
		B8G8R8A8_UNorm,
		B8G8R8A8_UNorm_SRGB,
		R16G16_Float,
		R16G16_UNorm,
		R16G16_UInt,
		R16G16_SNorm,
		R16G16_SInt,
		// depth (32-bit) | SRV: R32_Float
		D32_Float,
		R32_Float,
		R32_UInt,
		R32_SInt,
		// depth (24-bit) + stencil (8-bit) | SRV: R24_INTERNAL (default or depth aspect), R8_UInt (stencil aspect)
		D24_UNorm_S8_UInt,
		R9G9B9E5_SHAREDEXP,
		R8G8_UNorm = 37,
		R8G8_UInt = 38,
		R8G8_SNorm = 39,
		R8G8_SInt = 40,
		R16_Float,
		D16_UNorm, // depth (16-bit) | SRV: R16_UNorm
		R16_UNorm,
		R16_UInt,
		R16_SNorm,
		R16_SInt,
		R8_UNorm,
		R8_UInt,
		R8_SNorm,
		R8_SInt,
		// Formats that are not usable in render pass must be below because formats in render pass must be encodable as 6 bits:
		BC1_UNorm,
		BC1_UNorm_SRGB,
		BC2_UNorm,
		BC2_UNorm_SRGB,
		BC3_UNorm,
		BC3_UNorm_SRGB,
		BC4_UNorm,
		BC4_SNorm,
		BC5_UNorm,
		BC5_SNorm,
		BC6H_UF16,
		BC6H_SF16,
		BC7_UNorm,
		BC7_UNorm_SRGB,
		NV12,
		Max
	};

	constexpr bool IsPixelFormatValid(PixelFormat format)
	{
		return format > PixelFormat::Undefined && format < PixelFormat::Max;
	}

	constexpr bool IsPixelFormatUNorm(PixelFormat format)
	{
		switch (format)
		{
		case PixelFormat::R16G16B16A16_UNorm:
		case PixelFormat::R10G10B10A2_UNorm:
		case PixelFormat::R8G8B8A8_UNorm:
		case PixelFormat::R8G8B8A8_UNorm_SRGB:
		case PixelFormat::B8G8R8A8_UNorm:
		case PixelFormat::B8G8R8A8_UNorm_SRGB:
		case PixelFormat::R16G16_UNorm:
		case PixelFormat::D24_UNorm_S8_UInt:
		case PixelFormat::R8G8_UNorm:
		case PixelFormat::D16_UNorm:
		case PixelFormat::R16_UNorm:
		case PixelFormat::R8_UNorm:
		case PixelFormat::BC1_UNorm:
		case PixelFormat::BC1_UNorm_SRGB:
		case PixelFormat::BC2_UNorm:
		case PixelFormat::BC2_UNorm_SRGB:
		case PixelFormat::BC3_UNorm:
		case PixelFormat::BC3_UNorm_SRGB:
		case PixelFormat::BC4_UNorm:
		case PixelFormat::BC5_UNorm:
		case PixelFormat::BC7_UNorm:
		case PixelFormat::BC7_UNorm_SRGB:
			return true;
		default:
			return false;
		}
	}
	constexpr bool IsFormatBlockCompressed(PixelFormat format)
	{
		switch (format)
		{
		case PixelFormat::BC1_UNorm:
		case PixelFormat::BC1_UNorm_SRGB:
		case PixelFormat::BC2_UNorm:
		case PixelFormat::BC2_UNorm_SRGB:
		case PixelFormat::BC3_UNorm:
		case PixelFormat::BC3_UNorm_SRGB:
		case PixelFormat::BC4_UNorm:
		case PixelFormat::BC4_SNorm:
		case PixelFormat::BC5_UNorm:
		case PixelFormat::BC5_SNorm:
		case PixelFormat::BC6H_UF16:
		case PixelFormat::BC6H_SF16:
		case PixelFormat::BC7_UNorm:
		case PixelFormat::BC7_UNorm_SRGB:
			return true;
		default:
			return false;
		}
	}

	constexpr PixelFormat FindDepthStencilFormat(const PixelFormat format)
	{
/*		switch (format)
		{
		case PixelFormat::R24G8_Typeless:
		case PixelFormat::R24_UNorm_X8_Typeless:
			return PixelFormat::D24_UNorm_S8_UInt;
		case PixelFormat::R32_Typeless:
			return PixelFormat::D32_Float;
		case PixelFormat::R16_Typeless:
			return PixelFormat::D16_UNorm;
		}*/
		return format;
	}

	constexpr bool PixelFormatIsCompressed(const PixelFormat format)
	{
		switch (format)
		{
		case PixelFormat::BC1_UNorm:
		case PixelFormat::BC1_UNorm_SRGB:
		case PixelFormat::BC2_UNorm:
		case PixelFormat::BC2_UNorm_SRGB:
		case PixelFormat::BC3_UNorm:
		case PixelFormat::BC3_UNorm_SRGB:
		case PixelFormat::BC4_UNorm:
		case PixelFormat::BC4_SNorm:
		case PixelFormat::BC5_UNorm:
		case PixelFormat::BC5_SNorm:
		case PixelFormat::BC6H_UF16:
		case PixelFormat::BC6H_SF16:
		case PixelFormat::BC7_UNorm:
		case PixelFormat::BC7_UNorm_SRGB:
/*		case PixelFormat::ASTC_4x4_UNorm:
		case PixelFormat::ASTC_4x4_UNorm_SRGB:
		case PixelFormat::ASTC_6x6_UNorm:
		case PixelFormat::ASTC_6x6_UNorm_SRGB:
		case PixelFormat::ASTC_8x8_UNorm:
		case PixelFormat::ASTC_8x8_UNorm_SRGB:
		case PixelFormat::ASTC_10x10_UNorm:
		case PixelFormat::ASTC_10x10_UNorm_SRGB:*/
			return true;
		default:
			return false;
		}
	}

	constexpr bool PixelFormatIsDepthSupport(PixelFormat format)
	{
		switch (format)
		{
		case PixelFormat::D16_UNorm:
		case PixelFormat::D32_Float:
		case PixelFormat::D32_Float_S8X24_UInt:
		case PixelFormat::D24_UNorm_S8_UInt:
			return true;
		default:
			return false;
		}
	}
	constexpr bool PixelFormatIsStencilSupport(PixelFormat format)
	{
		switch (format)
		{
		case PixelFormat::D32_Float_S8X24_UInt:
		case PixelFormat::D24_UNorm_S8_UInt:
			return true;
		default:
			return false;
		}
	}
	constexpr uint32 PixelFormatGetBlockSize(PixelFormat format)
	{
		if (IsFormatBlockCompressed(format))
		{
			return 4u;
		}
		return 1u;
	}

	constexpr uint32 PixelFormatGetSizeInBits(PixelFormat format)
	{
		switch (format)
		{
			case PixelFormat::R8_SInt:
			case PixelFormat::R8_SNorm:
			case PixelFormat::R8_UInt:
			case PixelFormat::R8_UNorm:
			case PixelFormat::BC2_UNorm:
			case PixelFormat::BC2_UNorm_SRGB:
			case PixelFormat::BC3_UNorm:
			case PixelFormat::BC3_UNorm_SRGB:
			case PixelFormat::BC5_SNorm:
			case PixelFormat::BC5_UNorm:
			case PixelFormat::BC7_UNorm:
			case PixelFormat::BC7_UNorm_SRGB:
				return 8U;

			case PixelFormat::D16_UNorm:
			case PixelFormat::R16_Float:
			case PixelFormat::R16_SInt:
			case PixelFormat::R16_SNorm:
			case PixelFormat::R16_UInt:
			case PixelFormat::R16_UNorm:
			case PixelFormat::R8G8_SInt:
			case PixelFormat::R8G8_SNorm:
			case PixelFormat::R8G8_UInt:
			case PixelFormat::R8G8_UNorm:
				return 16U;

			case PixelFormat::D24_UNorm_S8_UInt:
			case PixelFormat::D32_Float:
			case PixelFormat::D32_Float_S8X24_UInt:
			case PixelFormat::R10G10B10A2_UInt:
			case PixelFormat::R10G10B10A2_UNorm:
			case PixelFormat::R11G11B10_Float:
			case PixelFormat::R16G16_Float:
			case PixelFormat::R16G16_SInt:
			case PixelFormat::R16G16_SNorm:
			case PixelFormat::R16G16_UInt:
			case PixelFormat::R16G16_UNorm:
			case PixelFormat::R32_Float:
			case PixelFormat::R32_SInt:
			case PixelFormat::R32_UInt:
			case PixelFormat::R8G8B8A8_SInt:
			case PixelFormat::R8G8B8A8_SNorm:
			case PixelFormat::R8G8B8A8_UInt:
			case PixelFormat::R8G8B8A8_UNorm:
			case PixelFormat::R8G8B8A8_UNorm_SRGB:
			case PixelFormat::B8G8R8A8_UNorm:
			case PixelFormat::B8G8R8A8_UNorm_SRGB:
				return 32U;

			case PixelFormat::R16G16B16A16_Float:
			case PixelFormat::R16G16B16A16_SInt:
			case PixelFormat::R16G16B16A16_SNorm:
			case PixelFormat::R16G16B16A16_UInt:
			case PixelFormat::R16G16B16A16_UNorm:
			case PixelFormat::R32G32_Float:
			case PixelFormat::R32G32_SInt:
			case PixelFormat::R32G32_UInt:
				return 64U;

			case PixelFormat::R32G32B32_Float:
			case PixelFormat::R32G32B32_SInt:
			case PixelFormat::R32G32B32_UInt:
				return 96U;

			case PixelFormat::R32G32B32A32_Float:
			case PixelFormat::R32G32B32A32_SInt:
			case PixelFormat::R32G32B32A32_UInt:
				return 128U;

			case PixelFormat::BC1_UNorm:
			case PixelFormat::BC1_UNorm_SRGB:
			case PixelFormat::BC4_SNorm:
			case PixelFormat::BC4_UNorm:
				return 4U;
		default:
			 ENGINE_ASSERT(0);
			return 16u;
		}
	}

	constexpr uint32 PixelFormatGetSizeInBytes(PixelFormat format)
	{
		return PixelFormatGetSizeInBits(format) / 8;
	}

	constexpr PixelFormat PixelFormatGetNonSRGB(PixelFormat format)
	{
		switch (format)
		{
		case PixelFormat::R8G8B8A8_UNorm_SRGB:
			return PixelFormat::R8G8B8A8_UNorm;
		case PixelFormat::B8G8R8A8_UNorm_SRGB:
			return PixelFormat::B8G8R8A8_UNorm;
		case PixelFormat::BC1_UNorm_SRGB:
			return PixelFormat::BC1_UNorm;
		case PixelFormat::BC2_UNorm_SRGB:
			return PixelFormat::BC2_UNorm;
		case PixelFormat::BC3_UNorm_SRGB:
			return PixelFormat::BC3_UNorm;
		case PixelFormat::BC7_UNorm_SRGB:
			return PixelFormat::BC7_UNorm;
		default:
			return format;
		}
	}

	constexpr bool PixelFormatIsSRGB(const PixelFormat format)
	{
		switch (format)
		{
		case PixelFormat::R8G8B8A8_UNorm_SRGB:
		case PixelFormat::BC1_UNorm_SRGB:
		case PixelFormat::BC2_UNorm_SRGB:
		case PixelFormat::BC3_UNorm_SRGB:
		case PixelFormat::BC7_UNorm_SRGB:
/*		case PixelFormat::ASTC_4x4_UNorm_SRGB:
		case PixelFormat::ASTC_6x6_UNorm_SRGB:
		case PixelFormat::ASTC_8x8_UNorm_SRGB:
		case PixelFormat::ASTC_10x10_UNorm_SRGB:*/
			return true;
		default:
			return false;
		}
	}

	constexpr PixelFormat PixelFormatToSRGB(PixelFormat format)
	{
		switch (format)
		{
		case PixelFormat::R8G8B8A8_UNorm:
		case PixelFormat::R8G8B8A8_UNorm_SRGB:
			return PixelFormat::R8G8B8A8_UNorm_SRGB;
		case PixelFormat::B8G8R8A8_UNorm:
		case PixelFormat::B8G8R8A8_UNorm_SRGB:
			return PixelFormat::B8G8R8A8_UNorm_SRGB;
		case PixelFormat::BC1_UNorm:
		case PixelFormat::BC1_UNorm_SRGB:
			return PixelFormat::BC1_UNorm_SRGB;
		case PixelFormat::BC2_UNorm:
		case PixelFormat::BC2_UNorm_SRGB:
			return PixelFormat::BC2_UNorm_SRGB;
		case PixelFormat::BC3_UNorm:
		case PixelFormat::BC3_UNorm_SRGB:
			return PixelFormat::BC3_UNorm_SRGB;
		case PixelFormat::BC7_UNorm:
		case PixelFormat::BC7_UNorm_SRGB:
			return PixelFormat::BC7_UNorm_SRGB;
		default:
			return PixelFormat::Undefined;
		}
	}

	constexpr const Char *PixelFormatGetString(PixelFormat format)
	{
		switch (format)
		{
		case PixelFormat::Undefined:
			return SE_TEXT("UNKNOWN");
		case PixelFormat::R32G32B32A32_Float:
			return SE_TEXT("R32G32B32A32_Float");
		case PixelFormat::R32G32B32A32_UInt:
			return SE_TEXT("R32G32B32A32_UInt");
		case PixelFormat::R32G32B32A32_SInt:
			return SE_TEXT("R32G32B32A32_SInt");
		case PixelFormat::R32G32B32_Float:
			return SE_TEXT("R32G32B32_Float");
		case PixelFormat::R32G32B32_UInt:
			return SE_TEXT("R32G32B32_UInt");
		case PixelFormat::R32G32B32_SInt:
			return SE_TEXT("R32G32B32_SInt");
		case PixelFormat::R16G16B16A16_Float:
			return SE_TEXT("R16G16B16A16_Float");
		case PixelFormat::R16G16B16A16_UNorm:
			return SE_TEXT("R16G16B16A16_UNorm");
		case PixelFormat::R16G16B16A16_UInt:
			return SE_TEXT("R16G16B16A16_UInt");
		case PixelFormat::R16G16B16A16_SNorm:
			return SE_TEXT("R16G16B16A16_SNorm");
		case PixelFormat::R16G16B16A16_SInt:
			return SE_TEXT("R16G16B16A16_SInt");
		case PixelFormat::R32G32_Float:
			return SE_TEXT("R32G32_Float");
		case PixelFormat::R32G32_UInt:
			return SE_TEXT("R32G32_UInt");
		case PixelFormat::R32G32_SInt:
			return SE_TEXT("R32G32_SInt");
		case PixelFormat::D32_Float_S8X24_UInt:
			return SE_TEXT("D32_Float_S8X24_UInt");
		case PixelFormat::R10G10B10A2_UNorm:
			return SE_TEXT("R10G10B10A2_UNorm");
		case PixelFormat::R10G10B10A2_UInt:
			return SE_TEXT("R10G10B10A2_UInt");
		case PixelFormat::R11G11B10_Float:
			return SE_TEXT("R11G11B10_Float");
		case PixelFormat::R8G8B8A8_UNorm:
			return SE_TEXT("R8G8B8A8_UNorm");
		case PixelFormat::R8G8B8A8_UNorm_SRGB:
			return SE_TEXT("R8G8B8A8_UNorm_SRGB");
		case PixelFormat::R8G8B8A8_UInt:
			return SE_TEXT("R8G8B8A8_UInt");
		case PixelFormat::R8G8B8A8_SNorm:
			return SE_TEXT("R8G8B8A8_SNorm");
		case PixelFormat::R8G8B8A8_SInt:
			return SE_TEXT("R8G8B8A8_SInt");
		case PixelFormat::B8G8R8A8_UNorm:
			return SE_TEXT("B8G8R8A8_UNorm");
		case PixelFormat::B8G8R8A8_UNorm_SRGB:
			return SE_TEXT("B8G8R8A8_UNorm_SRGB");
		case PixelFormat::R16G16_Float:
			return SE_TEXT("R16G16_Float");
		case PixelFormat::R16G16_UNorm:
			return SE_TEXT("R16G16_UNorm");
		case PixelFormat::R16G16_UInt:
			return SE_TEXT("R16G16_UInt");
		case PixelFormat::R16G16_SNorm:
			return SE_TEXT("R16G16_SNorm");
		case PixelFormat::R16G16_SInt:
			return SE_TEXT("R16G16_SInt");
		case PixelFormat::D32_Float:
			return SE_TEXT("D32_Float");
		case PixelFormat::R32_Float:
			return SE_TEXT("R32_Float");
		case PixelFormat::R32_UInt:
			return SE_TEXT("R32_UInt");
		case PixelFormat::R32_SInt:
			return SE_TEXT("R32_SInt");
		case PixelFormat::D24_UNorm_S8_UInt:
			return SE_TEXT("D24_UNorm_S8_UInt");
		case PixelFormat::R9G9B9E5_SHAREDEXP:
			return SE_TEXT("R9G9B9E5_SHAREDEXP");
		case PixelFormat::R8G8_UNorm:
			return SE_TEXT("R8G8_UNorm");
		case PixelFormat::R8G8_UInt:
			return SE_TEXT("R8G8_UInt");
		case PixelFormat::R8G8_SNorm:
			return SE_TEXT("R8G8_SNorm");
		case PixelFormat::R8G8_SInt:
			return SE_TEXT("R8G8_SInt");
		case PixelFormat::R16_Float:
			return SE_TEXT("R16_Float");
		case PixelFormat::D16_UNorm:
			return SE_TEXT("D16_UNorm");
		case PixelFormat::R16_UNorm:
			return SE_TEXT("R16_UNorm");
		case PixelFormat::R16_UInt:
			return SE_TEXT("R16_UInt");
		case PixelFormat::R16_SNorm:
			return SE_TEXT("R16_SNorm");
		case PixelFormat::R16_SInt:
			return SE_TEXT("R16_SInt");
		case PixelFormat::R8_UNorm:
			return SE_TEXT("R8_UNorm");
		case PixelFormat::R8_UInt:
			return SE_TEXT("R8_UInt");
		case PixelFormat::R8_SNorm:
			return SE_TEXT("R8_SNorm");
		case PixelFormat::R8_SInt:
			return SE_TEXT("R8_SInt");
		case PixelFormat::BC1_UNorm:
			return SE_TEXT("BC1_UNorm");
		case PixelFormat::BC1_UNorm_SRGB:
			return SE_TEXT("BC1_UNorm_SRGB");
		case PixelFormat::BC2_UNorm:
			return SE_TEXT("BC2_UNorm");
		case PixelFormat::BC2_UNorm_SRGB:
			return SE_TEXT("BC2_UNorm_SRGB");
		case PixelFormat::BC3_UNorm:
			return SE_TEXT("BC3_UNorm");
		case PixelFormat::BC3_UNorm_SRGB:
			return SE_TEXT("BC3_UNorm_SRGB");
		case PixelFormat::BC4_UNorm:
			return SE_TEXT("BC4_UNorm");
		case PixelFormat::BC4_SNorm:
			return SE_TEXT("BC4_SNorm");
		case PixelFormat::BC5_UNorm:
			return SE_TEXT("BC5_UNorm");
		case PixelFormat::BC5_SNorm:
			return SE_TEXT("BC5_SNorm");
		case PixelFormat::BC6H_UF16:
			return SE_TEXT("BC6H_UF16");
		case PixelFormat::BC6H_SF16:
			return SE_TEXT("BC6H_SF16");
		case PixelFormat::BC7_UNorm:
			return SE_TEXT("BC7_UNorm");
		case PixelFormat::BC7_UNorm_SRGB:
			return SE_TEXT("BC7_UNorm_SRGB");
		case PixelFormat::NV12:
			return SE_TEXT("NV12");
		default:
			return SE_TEXT("");
		}
	}

	constexpr bool PixelFormatIsCompressedBC(PixelFormat format)
	{
		switch (format)
		{
		case PixelFormat::BC1_UNorm:
		case PixelFormat::BC1_UNorm_SRGB:
		case PixelFormat::BC2_UNorm:
		case PixelFormat::BC2_UNorm_SRGB:
		case PixelFormat::BC3_UNorm:
		case PixelFormat::BC3_UNorm_SRGB:
		case PixelFormat::BC4_UNorm:
		case PixelFormat::BC4_SNorm:
		case PixelFormat::BC5_UNorm:
		case PixelFormat::BC5_SNorm:
		case PixelFormat::BC6H_UF16:
		case PixelFormat::BC6H_SF16:
		case PixelFormat::BC7_UNorm:
		case PixelFormat::BC7_UNorm_SRGB:
			return true;
		default:
			return false;
		}
	}

	constexpr bool PixelFormatIsCompressedASTC(PixelFormat format)
	{
		switch (format)
		{
/*		case PixelFormat::ASTC_4x4_UNorm:
		case PixelFormat::ASTC_4x4_UNorm_SRGB:
		case PixelFormat::ASTC_6x6_UNorm:
		case PixelFormat::ASTC_6x6_UNorm_SRGB:
		case PixelFormat::ASTC_8x8_UNorm:
		case PixelFormat::ASTC_8x8_UNorm_SRGB:
		case PixelFormat::ASTC_10x10_UNorm:
		case PixelFormat::ASTC_10x10_UNorm_SRGB:
			return true;*/
		default:
			return false;
		}
	}

	constexpr int32 PixelFormatComputeComponentsCount(const PixelFormat format)
	{
		switch (format)
		{
		case PixelFormat::R32G32B32A32_Float:
		case PixelFormat::R32G32B32A32_UInt:
		case PixelFormat::R32G32B32A32_SInt:
		case PixelFormat::R16G16B16A16_Float:
		case PixelFormat::R16G16B16A16_UNorm:
		case PixelFormat::R16G16B16A16_UInt:
		case PixelFormat::R16G16B16A16_SNorm:
		case PixelFormat::R16G16B16A16_SInt:
		case PixelFormat::R10G10B10A2_UNorm:
		case PixelFormat::R10G10B10A2_UInt:
		case PixelFormat::R8G8B8A8_UNorm:
		case PixelFormat::R8G8B8A8_UNorm_SRGB:
		case PixelFormat::R8G8B8A8_UInt:
		case PixelFormat::R8G8B8A8_SNorm:
		case PixelFormat::R8G8B8A8_SInt:
		case PixelFormat::BC1_UNorm:
		case PixelFormat::BC1_UNorm_SRGB:
		case PixelFormat::BC2_UNorm:
		case PixelFormat::BC2_UNorm_SRGB:
		case PixelFormat::BC3_UNorm:
		case PixelFormat::BC3_UNorm_SRGB:
		case PixelFormat::B8G8R8A8_UNorm:
		case PixelFormat::B8G8R8A8_UNorm_SRGB:
/*		case PixelFormat::ASTC_4x4_UNorm:
		case PixelFormat::ASTC_4x4_UNorm_sRGB:
		case PixelFormat::ASTC_6x6_UNorm:
		case PixelFormat::ASTC_6x6_UNorm_sRGB:
		case PixelFormat::ASTC_8x8_UNorm:
		case PixelFormat::ASTC_8x8_UNorm_sRGB:
		case PixelFormat::ASTC_10x10_UNorm:
		case PixelFormat::ASTC_10x10_UNorm_sRGB:*/
			return 4;
		case PixelFormat::R32G32B32_Float:
		case PixelFormat::R32G32B32_UInt:
		case PixelFormat::R32G32B32_SInt:
		case PixelFormat::R11G11B10_Float:
			return 3;
		case PixelFormat::R32G32_Float:
		case PixelFormat::R32G32_UInt:
		case PixelFormat::R32G32_SInt:
		case PixelFormat::R16G16_Float:
		case PixelFormat::R16G16_UNorm:
		case PixelFormat::R16G16_UInt:
		case PixelFormat::R16G16_SNorm:
		case PixelFormat::R16G16_SInt:
		case PixelFormat::R8G8_UNorm:
		case PixelFormat::R8G8_UInt:
		case PixelFormat::R8G8_SNorm:
		case PixelFormat::R8G8_SInt:
		case PixelFormat::BC5_UNorm:
		case PixelFormat::BC5_SNorm:
			return 2;
		case PixelFormat::D32_Float:
		case PixelFormat::R32_Float:
		case PixelFormat::R32_UInt:
		case PixelFormat::R32_SInt:
		case PixelFormat::D24_UNorm_S8_UInt:
		case PixelFormat::R16_Float:
		case PixelFormat::D16_UNorm:
		case PixelFormat::R16_UNorm:
		case PixelFormat::R16_UInt:
		case PixelFormat::R16_SNorm:
		case PixelFormat::R16_SInt:
		case PixelFormat::R8_UNorm:
		case PixelFormat::R8_UInt:
		case PixelFormat::R8_SNorm:
		case PixelFormat::R8_SInt:
		case PixelFormat::BC4_UNorm:
		case PixelFormat::BC4_SNorm:
			return 1;
		default:
			return 0;
		}
	}
}


#include "GPUUtils.h"
#include "GPUEnums.h"
#include "Runtime/Core/Types/Hash.h"

namespace SE
{
	void GPUUtils::ComputePitch(PixelFormat format, int32 width, int32 height, uint32& rowPitch, uint32& slicePitch)
	{
		switch (format)
		{
		case PixelFormat::BC1_UNorm:
		case PixelFormat::BC1_UNorm_SRGB:
		case PixelFormat::BC4_UNorm:
		case PixelFormat::BC4_SNorm:
			ENGINE_ASSERT(IsFormatBlockCompressed(format));
			{
				uint32 nbw = Math::Max<uint32>(1, (width + 3) / 4);
				uint32 nbh = Math::Max<uint32>(1, (height + 3) / 4);
				rowPitch = nbw * 8;
				slicePitch = rowPitch * nbh;
			}
			break;
		case PixelFormat::BC2_UNorm:
		case PixelFormat::BC2_UNorm_SRGB:
		case PixelFormat::BC3_UNorm:
		case PixelFormat::BC3_UNorm_SRGB:
		case PixelFormat::BC5_UNorm:
		case PixelFormat::BC5_SNorm:
		case PixelFormat::BC6H_UF16:
		case PixelFormat::BC6H_SF16:
		case PixelFormat::BC7_UNorm:
		case PixelFormat::BC7_UNorm_SRGB:
			ENGINE_ASSERT(IsFormatBlockCompressed(format));
			{
				uint32 nbw = Math::Max<uint32>(1, (width + 3) / 4);
				uint32 nbh = Math::Max<uint32>(1, (height + 3) / 4);
				rowPitch = nbw * 16;
				slicePitch = rowPitch * nbh;
			}
			break;
/*		case PixelFormat::ASTC_4x4_UNorm:
		case PixelFormat::ASTC_4x4_UNorm_SRGB:
		case PixelFormat::ASTC_6x6_UNorm:
		case PixelFormat::ASTC_6x6_UNorm_SRGB:
		case PixelFormat::ASTC_8x8_UNorm:
		case PixelFormat::ASTC_8x8_UNorm_SRGB:
		case PixelFormat::ASTC_10x10_UNorm:
		case PixelFormat::ASTC_10x10_UNorm_SRGB:
		{
			const int32 blockSize = PixelFormatExtensions::ComputeBlockSize(format);
			uint32 nbw = Math::Max<uint32>(1, Math::DivideAndRoundUp(width, blockSize));
			uint32 nbh = Math::Max<uint32>(1, Math::DivideAndRoundUp(height, blockSize));
			rowPitch = nbw * 16; // All ASTC blocks use 128 bits
			slicePitch = rowPitch * nbh;
		}
			break;*/
		default:
			ENGINE_ASSERT(IsPixelFormatValid(format));
			ENGINE_ASSERT(!IsFormatBlockCompressed(format));
			{
				uint32 bpp = PixelFormatGetSizeInBits(format);

				// Default byte alignment
				rowPitch = (width * bpp + 7) / 8;
				slicePitch = rowPitch * height;
			}
			break;
		}
	}

	int32 GPUUtils::MipLevelsCount(int32 width, bool useMipLevels)
	{
		if (!useMipLevels)
			return 1;

		int32 result = 1;
		while (width > 1)
		{
			width >>= 1;
			result++;
		}

		return result;
	}

	int32 GPUUtils::MipLevelsCount(int32 width, int32 height, bool useMipLevels)
	{
		// Check if use mip maps
		if (!useMipLevels)
		{
			// No mipmaps chain, only single mip map
			return 1;
		}

		// Count mip maps
		int32 result = 1;
		while (width > 1 || height > 1)
		{
			if (width > 1)
				width >>= 1;
			if (height > 1)
				height >>= 1;
			result++;
		}
		return result;
	}

	int32 GPUUtils::MipLevelsCount(int32 width, int32 height, int32 depth, bool useMipLevels)
	{
		if (!useMipLevels)
			return 1;

		int32 result = 1;
		while (width > 1 || height > 1 || depth > 1)
		{
			if (width > 1)
				width >>= 1;
			if (height > 1)
				height >>= 1;
			if (depth > 1)
				depth >>= 1;
			result++;
		}

		return result;
	}


	uint64 GPUUtils::CalculateTextureMemoryUsage(PixelFormat format, int32 width, int32 height, int32 mipLevels)
	{
		uint64 result = 0;

		if (mipLevels == 0)
			mipLevels = 69;

		uint32 rowPitch, slicePitch;
		while (mipLevels > 0 && (width >= 1 || height >= 1))
		{
			ComputePitch(format, width, height, rowPitch, slicePitch);
			result += slicePitch;

			if (width > 1)
				width >>= 1;
			if (height > 1)
				height >>= 1;

			mipLevels--;
		}

		return result;
	}

	uint64 GPUUtils::CalculateTextureMemoryUsage(PixelFormat format, int32 width, int32 height, int32 depth, int32 mipLevels)
	{
		return CalculateTextureMemoryUsage(format, width, height, mipLevels) * depth;
	}

	FeatureLevel GPUUtils::GetFeatureLevel(ShaderProfile profile)
	{
		switch (profile)
		{
		case ShaderProfile::DirectX_SM6:
		case ShaderProfile::PS5:
			return FeatureLevel::SM6;
		case ShaderProfile::DirectX_SM5:
		case ShaderProfile::Vulkan_SM5:
		case ShaderProfile::PS4:
			return FeatureLevel::SM5;
		case ShaderProfile::DirectX_SM4:
			return FeatureLevel::SM4;
		case ShaderProfile::GLSL_440:
		case ShaderProfile::GLSL_410:
		case ShaderProfile::Unknown:
			return FeatureLevel::ES2;
		default:
			return FeatureLevel::ES2;
		}
	}

	bool GPUUtils::CanSupportTessellation(ShaderProfile profile)
	{
		switch (profile)
		{
		case ShaderProfile::Vulkan_SM5:
		case ShaderProfile::DirectX_SM6:
		case ShaderProfile::DirectX_SM5:
		case ShaderProfile::PS4:
		case ShaderProfile::PS5:
			return true;
		default:
			return false;
		}
	}

#pragma region

	StringView ToString(ShaderProfile value)
	{
		const Char* result;
		switch (value)
		{
		case ShaderProfile::Unknown:
			result = SE_TEXT("Unknown");
			break;
		case ShaderProfile::DirectX_SM4:
			result = SE_TEXT("DirectX SM4");
			break;
		case ShaderProfile::DirectX_SM5:
			result = SE_TEXT("DirectX SM5");
			break;
		case ShaderProfile::DirectX_SM6:
			result = SE_TEXT("DirectX SM6");
			break;
		case ShaderProfile::GLSL_410:
			result = SE_TEXT("GLSL 410");
			break;
		case ShaderProfile::GLSL_440:
			result = SE_TEXT("GLSL 440");
			break;
		case ShaderProfile::Vulkan_SM5:
			result = SE_TEXT("Vulkan SM5");
			break;
		case ShaderProfile::PS4:
			result = SE_TEXT("PS4");
			break;
		case ShaderProfile::PS5:
			result = SE_TEXT("PS5");
			break;
		default:
			result = SE_TEXT("?");
		}
		return result;
	}

	const Char* ToString(FeatureLevel value)
	{
		const Char* result;
		switch (value)
		{
		case FeatureLevel::ES2:
			result = SE_TEXT("ES2");
			break;
		case FeatureLevel::ES3:
			result = SE_TEXT("ES3");
			break;
		case FeatureLevel::ES3_1:
			result = SE_TEXT("ES3_1");
			break;
		case FeatureLevel::SM4:
			result = SE_TEXT("SM4");
			break;
		case FeatureLevel::SM5:
			result = SE_TEXT("SM5");
			break;
		case FeatureLevel::SM6:
			result = SE_TEXT("SM6");
			break;
		default:
			result = SE_TEXT("?");
		}
		return result;
	}

	StringView ToString(MSAALevel value)
	{
		const Char* result;
		switch (value)
		{
		case MSAALevel::None:
			result = SE_TEXT("None");
			break;
		case MSAALevel::X2:
			result = SE_TEXT("X2");
			break;
		case MSAALevel::X4:
			result = SE_TEXT("X4");
			break;
		case MSAALevel::X8:
			result = SE_TEXT("X8");
			break;
		default:
			result = SE_TEXT("?");
		}
		return result;
	}

	bool BlendingMode::operator==(const BlendingMode& other) const
	{
#define EQUAL(x) x == other.x &&
		return EQUAL(BlendEnable)
			EQUAL(SrcBlend)
			EQUAL(DestBlend)
			EQUAL(BlendOp)
			EQUAL(SrcBlendAlpha)
			EQUAL(DestBlendAlpha)
			EQUAL(BlendOpAlpha)
			EQUAL(RenderTargetWriteMask)
			AlphaToCoverageEnable == other.AlphaToCoverageEnable;
#undef EQUAL
	}

	uint32 GetHash(const BlendingMode& key)
	{
		uint32 hash = key.AlphaToCoverageEnable ? 1 : 0;
		HashCombine(hash, key.BlendEnable ? 1 : 0);
		HashCombine(hash, (uint32)key.SrcBlend);
		HashCombine(hash, (uint32)key.DestBlend);
		HashCombine(hash, (uint32)key.BlendOp);
		HashCombine(hash, (uint32)key.SrcBlendAlpha);
		HashCombine(hash, (uint32)key.DestBlendAlpha);
		HashCombine(hash, (uint32)key.BlendOpAlpha);
		HashCombine(hash, (uint32)key.RenderTargetWriteMask);
		return hash;
	}

	BlendingMode BlendingMode::Opaque =
	{
		false, // AlphaToCoverageEnable
		false, // BlendEnable
		Blend::One, // SrcBlend
		Blend::Zero, // DestBlend
		Operation::Add, // BlendOp
		Blend::One, // SrcBlendAlpha
		Blend::Zero, // DestBlendAlpha
		Operation::Add, // BlendOpAlpha
		ColorWrite::All, // RenderTargetWriteMask
	};

	BlendingMode BlendingMode::Additive =
	{
		false, // AlphaToCoverageEnable
		true, // BlendEnable
		Blend::SrcAlpha, // SrcBlend
		Blend::One, // DestBlend
		Operation::Add, // BlendOp
		Blend::SrcAlpha, // SrcBlendAlpha
		Blend::One, // DestBlendAlpha
		Operation::Add, // BlendOpAlpha
		ColorWrite::All, // RenderTargetWriteMask
	};

	BlendingMode BlendingMode::AlphaBlend =
	{
		false, // AlphaToCoverageEnable
		true, // BlendEnable
		Blend::SrcAlpha, // SrcBlend
		Blend::InvSrcAlpha, // DestBlend
		Operation::Add, // BlendOp
		Blend::One, // SrcBlendAlpha
		Blend::InvSrcAlpha, // DestBlendAlpha
		Operation::Add, // BlendOpAlpha
		ColorWrite::All, // RenderTargetWriteMask
	};

	BlendingMode BlendingMode::Add =
	{
		false, // AlphaToCoverageEnable
		true, // BlendEnable
		Blend::One, // SrcBlend
		Blend::One, // DestBlend
		Operation::Add, // BlendOp
		Blend::One, // SrcBlendAlpha
		Blend::One, // DestBlendAlpha
		Operation::Add, // BlendOpAlpha
		ColorWrite::All, // RenderTargetWriteMask
	};

	BlendingMode BlendingMode::Multiply =
	{
		false, // AlphaToCoverageEnable
		true, // BlendEnable
		Blend::Zero, // SrcBlend
		Blend::SrcColor, // DestBlend
		Operation::Add, // BlendOp
		Blend::Zero, // SrcBlendAlpha
		Blend::SrcAlpha, // DestBlendAlpha
		Operation::Add, // BlendOpAlpha
		ColorWrite::All, // RenderTargetWriteMask
	};

#pragma endregion
} // SE
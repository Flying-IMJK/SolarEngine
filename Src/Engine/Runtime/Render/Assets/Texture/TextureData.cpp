
#include "TextureData.h"

#include "Runtime/Core/Math/Color.h"
#include "Runtime/Core/TypeSystem/CoreTypeConversions.h"
#include "Runtime/Utilities/Texture/TextureUtils.h"

namespace SE
{
	TextureMipData::TextureMipData()
		: RowPitch(0)
		, DepthPitch(0)
		, Lines(0)
	{
	}

	TextureMipData::TextureMipData(const TextureMipData& other)
		: RowPitch(other.RowPitch)
		, DepthPitch(other.DepthPitch)
		, Lines(other.Lines)
		, Data(other.Data)
	{
	}

	TextureMipData::TextureMipData(TextureMipData&& other) noexcept
		: RowPitch(other.RowPitch)
		, DepthPitch(other.DepthPitch)
		, Lines(other.Lines)
		, Data(MoveTemp(other.Data))
	{
	}

	TextureMipData& TextureMipData::operator=(const TextureMipData& other)
	{
		if (this == &other)
			return *this;
		RowPitch = other.RowPitch;
		DepthPitch = other.DepthPitch;
		Lines = other.Lines;
		Data = other.Data;
		return *this;
	}

	TextureMipData& TextureMipData::operator=(TextureMipData&& other) noexcept
	{
		if (this == &other)
			return *this;
		RowPitch = other.RowPitch;
		DepthPitch = other.DepthPitch;
		Lines = other.Lines;
		Data = MoveTemp(other.Data);
		return *this;
	}

	bool TextureMipData::GetPixels(List<Color32>& pixels, int32 width, int32 height, PixelFormat format) const
	{
		const int32 size = width * height;
		if (Data.IsInvalid() || size < 1)
			return true;
		pixels.Resize(size);
		byte* dst = (byte*)pixels.Get();
		const int32 dstRowSize = width * sizeof(Color32);
		const byte* src = Data.Get();
		const int32 srcRowSize = RowPitch;
		switch (format)
		{
		case PixelFormat::R8G8B8A8_SInt:
		case PixelFormat::R8G8B8A8_SNorm:
		case PixelFormat::R8G8B8A8_UInt:
		case PixelFormat::R8G8B8A8_UNorm:
		case PixelFormat::R8G8B8A8_UNorm_SRGB:
		case PixelFormat::B8G8R8A8_UNorm:
		case PixelFormat::B8G8R8A8_UNorm_SRGB:
			if (srcRowSize == dstRowSize)
				Platform::MemoryCopy(dst, src, RowPitch * Lines);
			else
			{
				for (uint32 row = 0; row < Lines; row++)
				{
					Platform::MemoryCopy(dst, src, dstRowSize);
					dst += dstRowSize;
					src += srcRowSize;
				}
			}
			break;
		default:
		{
			// Try to use texture sampler utility
			auto sampler = TextureUtils::GetSampler(format);
			if (sampler)
			{
				for (int32 y = 0; y < height; y++)
				{
					for (int32 x = 0; x < width; x++)
					{
						Color c = TextureUtils::SamplePoint(sampler, x, y, src, RowPitch);
						*(Color32*)(dst + dstRowSize * y + x * sizeof(Color32)) = Color32(c);
					}
				}
				return false;
			}
			LOG_ERROR("Graphic", "Unsupported texture data format {0}.",  Types::GetEnumString(format));
			return true;
		}
		}
		return false;
	}

	bool TextureMipData::GetPixels(List<Color>& pixels, int32 width, int32 height, PixelFormat format) const
	{
		const int32 size = width * height;
		if (Data.IsInvalid() || size < 1)
			return true;
		pixels.Resize(size);
		byte* dst = (byte*)pixels.Get();
		const int32 dstRowSize = width * sizeof(Color);
		const byte* src = Data.Get();
		const int32 srcRowSize = RowPitch;
		switch (format)
		{
		case PixelFormat::R32G32B32A32_Float:
			if (srcRowSize == dstRowSize)
				Platform::MemoryCopy(dst, src, RowPitch * Lines);
			else
			{
				for (uint32 row = 0; row < Lines; row++)
				{
					Platform::MemoryCopy(dst, src, dstRowSize);
					dst += dstRowSize;
					src += srcRowSize;
				}
			}
			break;
		default:
		{
			// Try to use texture sampler utility
			// TODO
			auto sampler = TextureUtils::GetSampler(format);
			if (sampler)
			{
				for (int32 y = 0; y < height; y++)
				{
					for (int32 x = 0; x < width; x++)
					{
						Color c = TextureUtils::SamplePoint(sampler, x, y, src, RowPitch);
						*(Color*)(dst + dstRowSize * y + x * sizeof(Color)) = c;
					}
				}
				return false;
			}
			LOG_ERROR("Graphic", "Unsupported texture data format {0}.", ConvertEnumToString<PixelFormat>(format));
			return true;
		}
		}
		return false;
	}
} // SE
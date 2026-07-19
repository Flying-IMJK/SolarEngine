

#include "TextureUtils.h"
#include "Runtime/Core/Logging/Logging.h"
#include "Runtime/Core/Math/Color.h"
#include "Runtime/Core/Math/Vector2.h"
#include "Runtime/Core/Serialization/FileWriteStream.h"
#include "Runtime/Core/Platform/File.h"

#include "Runtime/Render/Assets/Texture/TextureData.h"
#include "Runtime/Graphics/Base/GPUUtils.h"
#include "Runtime/ThirdParty/stb/stb_image_write.h"
#include "Runtime/ThirdParty/stb/stb_image_resize.h"
#include "Runtime/ThirdParty/stb/stb_image.h"
#include "Runtime/ThirdParty/stb/stb_dxt.h"
#include "Runtime/ThirdParty/detex/detex.h"
#include "Runtime/ThirdParty/bc7enc16/bc7enc16.h"
#include "Runtime/Utilities/AnsiPathTempFile.h"

#if SE_EDITOR
// Import tinyexr library
// Source: https://github.com/syoyo/tinyexr
#define TINYEXR_IMPLEMENTATION
#define TINYEXR_USE_MINIZ 1
#define TINYEXR_USE_STB_ZLIB 0
#define TINYEXR_USE_THREAD 0
#define TINYEXR_USE_OPENMP 0
#undef min
#undef max
#include "Runtime/ThirdParty/tinyexr/tinyexr.h"

#endif

namespace SE
{

	static void stbWrite(void* context, void* data, int size)
	{
		auto file = (FileWriteStream*)context;
		file->WriteBytes(data, (uint32)size);
	}

	#if SE_EDITOR
	
	static TextureData const* stbDecompress(const TextureData& textureData, TextureData& decompressed)
	{
		if (!PixelFormatIsCompressed(textureData.Format))
			return &textureData;
		const bool srgb = PixelFormatIsSRGB(textureData.Format);
		switch (textureData.Format)
		{
		case PixelFormat::BC4_UNorm:
		case PixelFormat::BC4_SNorm:
			decompressed.Format = PixelFormat::R8_UNorm;
			break;
		case PixelFormat::BC5_UNorm:
		case PixelFormat::BC5_SNorm:
			decompressed.Format = PixelFormat::R8G8_UNorm;
			break;
		default:
			decompressed.Format = srgb ? PixelFormat::R8G8B8A8_UNorm_SRGB : PixelFormat::R8G8B8A8_UNorm;
			break;
		}
		decompressed.Width = textureData.Width;
		decompressed.Height = textureData.Height;
		decompressed.Depth = textureData.Depth;
		decompressed.Items.Resize(1);
		decompressed.Items[0].Mips.Resize(1);
	
		TextureMipData* decompressedData = decompressed.GetData(0, 0);
		decompressedData->RowPitch = textureData.Width * PixelFormatGetSizeInBytes(decompressed.Format);
		decompressedData->Lines = textureData.Height;
		decompressedData->DepthPitch = decompressedData->RowPitch * decompressedData->Lines;
		decompressedData->Data.Allocate(decompressedData->DepthPitch);
		byte* decompressedBytes = decompressedData->Data.Get();
	
		Color32 colors[16];
		int32 blocksWidth = textureData.Width / 4;
		int32 blocksHeight = textureData.Height / 4;
		const TextureMipData* blocksData = textureData.GetData(0, 0);
		const byte* blocksBytes = blocksData->Data.Get();
	
		typedef bool (*detexDecompressBlockFuncType)(const uint8_t* bitstring, uint32_t mode_mask, uint32_t flags, uint8_t* pixel_buffer);
		detexDecompressBlockFuncType detexDecompressBlockFunc;
		int32 pixelSize, blockSize;
		switch (textureData.Format)
		{
		case PixelFormat::BC1_UNorm:
		case PixelFormat::BC1_UNorm_SRGB:
			detexDecompressBlockFunc = detexDecompressBlockBC1;
			pixelSize = 4;
			blockSize = 8;
			break;
		case PixelFormat::BC2_UNorm:
		case PixelFormat::BC2_UNorm_SRGB:
			detexDecompressBlockFunc = detexDecompressBlockBC2;
			pixelSize = 4;
			blockSize = 16;
			break;
		case PixelFormat::BC3_UNorm:
		case PixelFormat::BC3_UNorm_SRGB:
			detexDecompressBlockFunc = detexDecompressBlockBC3;
			pixelSize = 4;
			blockSize = 16;
			break;
		case PixelFormat::BC4_UNorm:
			detexDecompressBlockFunc = detexDecompressBlockRGTC1;
			pixelSize = 1;
			blockSize = 8;
			break;
		case PixelFormat::BC5_UNorm:
			detexDecompressBlockFunc = detexDecompressBlockRGTC2;
			pixelSize = 2;
			blockSize = 16;
			break;
		case PixelFormat::BC7_UNorm:
		case PixelFormat::BC7_UNorm_SRGB:
			detexDecompressBlockFunc = detexDecompressBlockBPTC;
			pixelSize = 4;
			blockSize = 16;
			break;
		default:
			LOG_WARNING("Graphic", "Texture data format {0} is not supported by detex library.", (int32)textureData.Format);
			return nullptr;
		}
	
		uint8 blockBuffer[DETEX_MAX_BLOCK_SIZE];
		for (int32 y = 0; y < blocksHeight; y++)
		{
			int32 rows;
			if (y * 4 + 3 >= textureData.Height)
				rows = textureData.Height - y * 4;
			else
				rows = 4;
			for (int32 x = 0; x < blocksWidth; x++)
			{
				const byte* block = blocksBytes + y * blocksData->RowPitch + x * blockSize;
				if (!detexDecompressBlockFunc(block, DETEX_MODE_MASK_ALL, 0, blockBuffer))
					memset(blockBuffer, 0, DETEX_MAX_BLOCK_SIZE);
				uint8* pixels = decompressedBytes + y * 4 * textureData.Width * pixelSize + x * 4 * pixelSize;
				int32 columns;
				if (x * 4 + 3  >= textureData.Width)
					columns = textureData.Width - x * 4;
				else
					columns = 4;
				for (int32 row = 0; row < rows; row++)
					memcpy(pixels + row * textureData.Width * pixelSize, blockBuffer + row * 4 * pixelSize, columns * pixelSize);
			}
		}
	
		return &decompressed;
	}
	
	#endif

	bool TextureUtils::ExportTextureStb(ImageType type, const StringView& path, const TextureData& textureData)
	{
		if (textureData.GetArraySize() != 1)
		{
			LOG_WARNING("Graphic", "Exporting texture arrays and cubemaps is not supported.");
		}
		TextureData const* texture = &textureData;
	
	#if SE_EDITOR
		// Handle compressed textures
		TextureData decompressed;
		texture = stbDecompress(textureData, decompressed);
		if (!texture)
			return true;
	#endif
	
		// Convert into RGBA8
		const auto sampler = GetSampler(texture->Format);
		if (sampler == nullptr)
		{
			LOG_WARNING("Graphic", "Texture data format {0} is not supported.", (int32)textureData.Format);
			return false;
		}
		const auto srcData = texture->GetData(0, 0);
		const int comp = 4;
		List<byte> data;
		bool sRGB = PixelFormatIsSRGB(texture->Format);
		if (type == ImageType::HDR)
		{
			data.Resize(sizeof(float) * comp * texture->Width * texture->Height);
	
			auto ptr = (Float4*)data.Get();
			for (int32 y = 0; y < texture->Height; y++)
			{
				for (int32 x = 0; x < texture->Width; x++)
				{
					Color color = SamplePoint(sampler, x, y, srcData->Data.Get(), srcData->RowPitch);
					if (sRGB)
						color = Color::SrgbToLinear(color);
					*(ptr + x + y * texture->Width) = color.ToFloat4();
				}
			}
		}
		else
		{
			data.Resize(sizeof(Color32) * comp * texture->Width * texture->Height);
	
			auto ptr = (Color32*)data.Get();
			for (int32 y = 0; y < texture->Height; y++)
			{
				for (int32 x = 0; x < texture->Width; x++)
				{
					Color color = SamplePoint(sampler, x, y, srcData->Data.Get(), srcData->RowPitch);
					if (sRGB)
						color = Color::SrgbToLinear(color);
					*(ptr + x + y * texture->Width) = Color32(color);
				}
			}
		}
	
		const auto file = FileWriteStream::Open(path);
		if (!file)
		{
			LOG_WARNING("Graphic", "Failed to open file.");
			return false;
		}

		int32 result = 99;
		switch (type)
		{
		case ImageType::BMP:
			result = stbi_write_bmp_to_func(stbWrite, file, texture->Width, texture->Height, comp, data.Get());
			break;
		case ImageType::JPEG:
			result = stbi_write_jpg_to_func(stbWrite, file,texture->Width, texture->Height, comp, data.Get(), 90);
			break;
		case ImageType::TGA:
			result = stbi_write_tga_to_func(stbWrite, file,texture->Width, texture->Height, comp, data.Get());
			break;
		case ImageType::HDR:
			result = stbi_write_hdr_to_func(stbWrite, file,texture->Width, texture->Height, comp, (float*)data.Get());
			break;
		case ImageType::PNG:
		{
			if (!stbi_write_png_to_func(stbWrite, file, texture->Width, texture->Height, comp, data.Get(),  0))
			{
				result = 0;
			}
			else
			{
				result = 99;
			}
			break;
		}
		case ImageType::GIF:
			LOG_WARNING("Graphic", "GIF format is not supported.");
			break;
		case ImageType::TIFF:
			LOG_WARNING("Graphic", "GIF format is not supported.");
			break;
		case ImageType::DDS:
			LOG_WARNING("Graphic", "DDS format is not supported.");
			break;
		case ImageType::RAW:
			LOG_WARNING("Graphic", "RAW format is not supported.");
			break;
		case ImageType::EXR:
			LOG_WARNING("Graphic", "EXR format is not supported.");
			break;
		default:
			LOG_WARNING("Graphic", "Unknown format.");
			break;
		}
	
		if (result != 0)
		{
			LOG_WARNING("Graphic", "Saving texture failed. Error from stb library: {0}", result);
		}
	
		file->Close();
		Delete(file);
	
		return result == 0;
	}
	
	bool TextureUtils::ImportTextureStb(ImageType type, const StringView& path, TextureData& textureData, bool& hasAlpha)
	{
		List<byte> fileData;
		if (!File::ReadAllBytes(path, fileData))
		{
			LOG_WARNING("Graphic", "Failed to read data from file.");
			return false;
		}
	
		switch (type)
		{
		case ImageType::PNG:
		case ImageType::BMP:
		case ImageType::GIF:
		case ImageType::JPEG:
		case ImageType::HDR:
		case ImageType::TGA:
		{
			int width, height, components;
			void* stbData;
			if (stbi_is_16_bit_from_memory(fileData.Get(), fileData.Count()))
			{
				stbData = stbi_load_16_from_memory(fileData.Get(), fileData.Count(), &width, &height, &components, 4);
			}
			else
			{
				stbData = stbi_load_from_memory(fileData.Get(), fileData.Count(), &width, &height, &components, 4);
			}

			if (!stbData)
			{
				LOG_WARNING("Graphic", "Failed to load image. {0}", String(stbi_failure_reason()));
				return false;
			}
			fileData.Resize(0);
	
			// Setup texture data
			textureData.Width = width;
			textureData.Height = height;
			textureData.Depth = 1;
			textureData.Format = PixelFormat::R8G8B8A8_UNorm;
			textureData.Items.Resize(1);
			textureData.Items[0].Mips.Resize(1);
			auto& mip = textureData.Items[0].Mips[0];
			mip.RowPitch = sizeof(Color32) * width;
			mip.DepthPitch = mip.RowPitch * height;
			mip.Lines = height;
			mip.Data.Copy((byte*)stbData, mip.DepthPitch);
	
	#if SE_EDITOR
			// Detect alpha channel usage
			auto ptrAlpha = (Color32*)mip.Data.Get();
			for (int32 y = 0; y < height && !hasAlpha; y++)
			{
				for (int32 x = 0; x < width && !hasAlpha; x++)
				{
					hasAlpha |= ptrAlpha->a < 255;
					ptrAlpha++;
				}
			}
	#endif
	
			stbi_image_free(stbData);
	
			break;
		}
		case ImageType::RAW:
		{
			// Assume 16-bit, grayscale .RAW file in little-endian byte order
	
			// Check size
			const auto size = (int32)Math::Sqrt(fileData.Count() / 2.0f);
			if (fileData.Count() != size * size * 2)
			{
				LOG_WARNING("Graphic", "Invalid RAW file data size or format. Use 16-bit .RAW file in little-endian byte order (square dimensions).");
				return false;
			}
	
			// Setup texture data
			textureData.Width = size;
			textureData.Height = size;
			textureData.Depth = 1;
			textureData.Format = PixelFormat::R16_UNorm;
			textureData.Items.Resize(1);
			textureData.Items[0].Mips.Resize(1);
			auto& mip = textureData.Items[0].Mips[0];
			mip.RowPitch = fileData.Count() / size;
			mip.DepthPitch = fileData.Count();
			mip.Lines = size;
			mip.Data.Copy(fileData);
	
			break;
		}
		case ImageType::EXR:
		{
	#if SE_EDITOR
			// Load exr file
			// AnsiPathTempFile tempFile(path);
			float* pixels;
			int width, height;
			const char* err = nullptr;
			int ret = LoadEXRFromMemory(&pixels, &width, &height, fileData.Get(), fileData.Count(), &err);
			// int ret = LoadEXR(&pixels, &width, &height, tempFile.Path.Get(), &err);
			if (ret != TINYEXR_SUCCESS)
			{
				if (err)
				{
					LOG_WARNING("Resource", "Import texture error {0}", StringAnsiView(err));
					FreeEXRErrorMessage(err);
				}
				return false;
			}
			
			// Setup texture data
			textureData.Width = width;
			textureData.Height = height;
			textureData.Depth = 1;
			textureData.Format = PixelFormat::R32G32B32A32_Float;
			textureData.Items.Resize(1);
			textureData.Items[0].Mips.Resize(1);
			auto& mip = textureData.Items[0].Mips[0];
			mip.RowPitch = width * sizeof(Float4);
			mip.DepthPitch = mip.RowPitch * height;
			mip.Lines = height;
			mip.Data.Copy((const byte*)pixels, mip.DepthPitch);
	
			free(pixels);
	#else
			LOG_WARNING("Graphic", "EXR format is not supported.");
	#endif
			break;
		}
		case ImageType::DDS:
			LOG_WARNING("Graphic", "DDS format is not supported.");
			return false;
		case ImageType::TIFF:
			LOG_WARNING("Graphic", "TIFF format is not supported.");
			return false;
		default:
			LOG_WARNING("Graphic", "Unknown format.");
			return false;
		}
	
		return true;
	}
	
	bool TextureUtils::ImportTextureStb(ImageType type, const StringView& path, TextureData& textureData, const Options& options, String& errorMsg, bool& hasAlpha)
	{
		// Load image data
		if (type == ImageType::Internal)
		{
			if (!options.InternalLoad.IsBinded() || options.InternalLoad(textureData))
				return true;
			if (options.FlipY)
			{
				// TODO: impl this
				errorMsg = SE_TEXT("Flipping images imported from Internal source is not supported by stb.");
				return true;
			}
		}
		else
		{
			stbi_set_flip_vertically_on_load_thread(options.FlipY);
			bool failed = !ImportTextureStb(type, path, textureData, hasAlpha);
			stbi_set_flip_vertically_on_load_thread(false);
			if (failed)
			{
				return false;
			}
		}

		// split data to 6 array
		if (options.Type == TextureFormatType::CubeMap)
		{
			TextureMipData& srcData = textureData.Items[0].Mips[0];
			int width = textureData.Width / 6;
			int height = textureData.Height;

			int newRowPitch = srcData.RowPitch / 6;

			textureData.Width = width;
			textureData.Height = height;

			textureData.Items.Resize(6);

			for (int i = 1; i < 6; i++)
			{
				TextureData::ArrayEntry& arrayEntry = textureData.Items[i];
				arrayEntry.Mips.Resize(1);

				TextureMipData& arrayMipData = arrayEntry.Mips[0];
				arrayMipData.RowPitch = newRowPitch;
				arrayMipData.DepthPitch = arrayMipData.RowPitch * height;
				arrayMipData.Lines = height;
				arrayMipData.Data.Allocate(arrayMipData.DepthPitch);

				for (int j = 0; j < height; j++)
				{
					int srcOffset = i * arrayMipData.RowPitch + j * srcData.RowPitch;
					int destOffset = j * arrayMipData.RowPitch;
					Platform::MemoryCopy(arrayMipData.Data.Get() + destOffset, srcData.Data.Get() + srcOffset, arrayMipData.RowPitch);
				}
			}


			for (int j = 1; j < height; j++)
			{
				int srcOffset = j * srcData.RowPitch;
				int destOffset = j * newRowPitch;
				Platform::MemoryCopy(srcData.Data.Get() + destOffset, srcData.Data.Get() + srcOffset, newRowPitch);
			}

			srcData.RowPitch = newRowPitch;
			srcData.DepthPitch = newRowPitch * height;
			srcData.Data.SetLength(srcData.DepthPitch);
			srcData.Lines = height;
		}

		// Use two data containers for texture importing for more optimzied performance
		TextureData textureDataTmp;
		TextureData* textureDataSrc = &textureData;
		TextureData* textureDataDst = &textureDataTmp;
	
		// Check if resize source image
		const int32 sourceWidth = textureData.Width;
		const int32 sourceHeight = textureData.Height;
		int32 width = Math::Clamp(options.Resize ? options.SizeX : static_cast<int32>(sourceWidth * options.Scale), 1, options.MaxSize);
		int32 height = Math::Clamp(options.Resize ? options.SizeY : static_cast<int32>(sourceHeight * options.Scale), 1, options.MaxSize);
		if (sourceWidth != width || sourceHeight != height)
		{
			// During resizing we need to keep texture aspect ratio
			const bool keepAspectRatio = false; // TODO: expose as import option
			if (keepAspectRatio)
			{
				const float aspectRatio = static_cast<float>(sourceWidth) / sourceHeight;
				if (width >= height)
					height = Math::CeilToInt(width / aspectRatio);
				else
					width = Math::CeilToInt(height / aspectRatio);
			}
	
			// Resize source texture
			LOG_INFO("Graphic", "Resizing texture from {0}x{1} to {2}x{3}.", sourceWidth, sourceHeight, width, height);
			if (ResizeStb(*textureDataDst, *textureDataSrc, width, height))
			{
				errorMsg = String::Format(SE_TEXT("Cannot resize texture."));
				return false;
			}
			::Swap(textureDataSrc, textureDataDst);
		}
	
		// Cache data
		float alphaThreshold = 0.3f;
		bool isPowerOfTwo = Math::IsPowerOf2(width) && Math::IsPowerOf2(height);
		PixelFormat targetFormat = ToPixelFormat(options.Type, width, height, options.Compress);
		if (options.sRGB)
			targetFormat = PixelFormatToSRGB(targetFormat);
	
		// Check mip levels
		int32 sourceMipLevels = textureDataSrc->GetMipLevels();
		bool hasSourceMipLevels = isPowerOfTwo && sourceMipLevels > 1;
		bool useMipLevels = isPowerOfTwo && (options.GenerateMipMaps || hasSourceMipLevels) && (width > 1 || height > 1);
		int32 arraySize = (int32)textureDataSrc->GetArraySize();
		int32 mipLevels = GPUUtils::MipLevelsCount(width, height, useMipLevels);
		if (useMipLevels && !options.GenerateMipMaps && mipLevels != sourceMipLevels)
		{
			errorMsg = String::Format(SE_TEXT("Imported texture has not full mip chain, loaded mips count: {0}, expected: {1}"), sourceMipLevels, mipLevels);
			return false;
		}
	
		// Decompress if texture is compressed (next steps need decompressed input data, for eg. mip maps generation or format changing)
		if (PixelFormatIsCompressed(textureDataSrc->Format))
		{
			// TODO: implement texture decompression
			errorMsg = String::Format(SE_TEXT("Imported texture used compressed format {0}. Not supported for importing on this platform.."), (int32)textureDataSrc->Format);
			return false;
		}
	
		// Generate mip maps chain
		if (useMipLevels && options.GenerateMipMaps)
		{
			for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
			{
				auto& slice = textureDataSrc->Items[arrayIndex];
				slice.Mips.Resize(mipLevels);
				for (int32 mipIndex = 1; mipIndex < mipLevels; mipIndex++)
				{
					const auto& srcMip = slice.Mips[mipIndex - 1];
					auto& dstMip = slice.Mips[mipIndex];
					auto dstMipWidth = Math::Max(textureDataSrc->Width >> mipIndex, 1);
					auto dstMipHeight = Math::Max(textureDataSrc->Height >> mipIndex, 1);
					if (ResizeStb(textureDataSrc->Format, dstMip, srcMip, dstMipWidth, dstMipHeight))
					{
						errorMsg = SE_TEXT("Failed to generate mip texture.");
						return false;
					}
				}
			}
		}
	
		// Preserve mipmap alpha coverage (if requested)
		if (/*PixelFormatGetSRGB() HasAlpha(textureDataSrc->Format) && */options.PreserveAlphaCoverage && useMipLevels)
		{
			// TODO: implement alpha coverage preserving
			errorMsg = SE_TEXT("Importing textures with alpha coverage preserving is not supported on this platform.");
			return false;
		}
	
		// Compress mip maps or convert image
		if (targetFormat != textureDataSrc->Format)
		{
			if (!ConvertStb(*textureDataDst, *textureDataSrc, targetFormat))
			{
				errorMsg = String::Format(SE_TEXT("Cannot convert/compress texture."));
				return false;
			}
			::Swap(textureDataSrc, textureDataDst);
		}
	
		// Copy data to the output if not in the result container
		if (textureDataSrc != &textureData)
		{
			textureData = textureDataTmp;
		}
	
		return true;
	}
	
	bool TextureUtils::ConvertStb(TextureData& dst, const TextureData& src, const PixelFormat dstFormat)
	{
		TextureData const* textureData = &src;
	
	#if SE_EDITOR
		// Handle compressed textures
		TextureData decompressed;
		textureData = stbDecompress(src, decompressed);
		if (!textureData)
			return false;
	#endif
	
		// Setup
		auto arraySize = textureData->GetArraySize();
		dst.Width = textureData->Width;
		dst.Height = textureData->Height;
		dst.Depth = textureData->Depth;
		dst.Format = dstFormat;
		dst.Items.Resize(arraySize, false);
		auto formatSize = PixelFormatGetSizeInBytes(textureData->Format);
		auto components = PixelFormatComputeComponentsCount(textureData->Format);
		auto sampler = TextureUtils::GetSampler(textureData->Format);
		if (!sampler)
		{
			LOG_WARNING("Graphic", "Cannot convert image. Unsupported format {0}", static_cast<int32>(textureData->Format));
			return false;
		}
	
	#if SE_EDITOR
		if (PixelFormatIsCompressedBC(dstFormat))
		{
			int32 bytesPerBlock;
			switch (dstFormat)
			{
			case PixelFormat::BC1_UNorm:
			case PixelFormat::BC1_UNorm_SRGB:
			case PixelFormat::BC4_UNorm:
				bytesPerBlock = 8;
				break;
			default:
				bytesPerBlock = 16;
				break;
			}
			bool isDstSRGB = PixelFormatIsSRGB(dstFormat);
	
			// bc7enc init
			bc7enc16_compress_block_params params;
			if (dstFormat == PixelFormat::BC7_UNorm || dstFormat == PixelFormat::BC7_UNorm_SRGB)
			{
				bc7enc16_compress_block_params_init(&params);
				bc7enc16_compress_block_init();
			}
	
			// Compress all array slices
			for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
			{
				const auto& srcSlice = textureData->Items[arrayIndex];
				auto& dstSlice = dst.Items[arrayIndex];
				auto mipLevels = srcSlice.Mips.Count();
				dstSlice.Mips.Resize(mipLevels, false);
	
				// Compress all mip levels
				for (int32 mipIndex = 0; mipIndex < mipLevels; mipIndex++)
				{
					const auto& srcMip = srcSlice.Mips[mipIndex];
					auto& dstMip = dstSlice.Mips[mipIndex];
					auto mipWidth = Math::Max(textureData->Width >> mipIndex, 1);
					auto mipHeight = Math::Max(textureData->Height >> mipIndex, 1);
					auto blocksWidth = Math::Max(Math::DivideAndRoundUp(mipWidth, 4), 1);
					auto blocksHeight = Math::Max(Math::DivideAndRoundUp(mipHeight, 4), 1);
	
					// Allocate memory
					dstMip.RowPitch = blocksWidth * bytesPerBlock;
					dstMip.DepthPitch = dstMip.RowPitch * blocksHeight;
					dstMip.Lines = blocksHeight;
					dstMip.Data.Allocate(dstMip.DepthPitch);
	
					// Compress texture
					for (int32 yBlock = 0; yBlock < blocksHeight; yBlock++)
					{
						for (int32 xBlock = 0; xBlock < blocksWidth; xBlock++)
						{
							// Sample source texture 4x4 block
							Color32 srcBlock[16];
							for (int32 y = 0; y < 4; y++)
							{
								for (int32 x = 0; x < 4; x++)
								{
									Color color = TextureUtils::SamplePoint(sampler, xBlock * 4 + x, yBlock * 4 + y, srcMip.Data.Get(), srcMip.RowPitch);
									if (isDstSRGB)
										color = Color::LinearToSrgb(color);
									srcBlock[y * 4 + x] = Color32(color);
								}
							}

							// Compress block
							byte* dstBlock = dstMip.Data.Get() + (yBlock * blocksWidth + xBlock) * bytesPerBlock;
							switch (dstFormat)
							{
							case PixelFormat::BC1_UNorm:
							case PixelFormat::BC1_UNorm_SRGB:
								stb_compress_dxt_block(dstBlock, (byte*)&srcBlock, 0, STB_DXT_HIGHQUAL);
								break;
							case PixelFormat::BC3_UNorm:
							case PixelFormat::BC3_UNorm_SRGB:
								stb_compress_dxt_block(dstBlock, (byte*)&srcBlock, 1, STB_DXT_HIGHQUAL);
								break;
							case PixelFormat::BC4_UNorm:
								for (int32 i = 1; i < 16; i++)
								{
									((byte*)&srcBlock)[i] = srcBlock[i].r;
								}
								stb_compress_bc4_block(dstBlock, (byte*)&srcBlock);
								break;
							case PixelFormat::BC5_UNorm:
								for (int32 i = 0; i < 16; i++)
								{
									((uint16*)&srcBlock)[i] = srcBlock[i].r << 8 | srcBlock[i].g;
								}
								stb_compress_bc5_block(dstBlock, (byte*)&srcBlock);
								break;
							case PixelFormat::BC7_UNorm:
							case PixelFormat::BC7_UNorm_SRGB:
								bc7enc16_compress_block(dstBlock, &srcBlock, &params);
								break;
							default:
								LOG_WARNING("Graphic", "Cannot compress image. Unsupported format {0}", static_cast<int32>(dstFormat));
								return false;
							}
						}
					}
				}
			}
		}
		else if (PixelFormatIsCompressedASTC(dstFormat))
		{
	#if COMPILE_WITH_ASTC
			if (ConvertAstc(dst, *textureData, dstFormat))
	#else
			LOG_ERROR("Graphic", "Missing ASTC texture format compression lib.");
	#endif
			{
				return true;
			}
		}
		else
	#endif
		{
			int32 bytesPerPixel = PixelFormatGetSizeInBytes(dstFormat);
			auto dstSampler = TextureUtils::GetSampler(dstFormat);
			if (!dstSampler)
			{
				LOG_WARNING("Graphic", "Cannot convert image. Unsupported format {0}", static_cast<int32>(dstFormat));
				return false;
			}
	
			// Convert all array slices
			for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
			{
				const auto& srcSlice = textureData->Items[arrayIndex];
				auto& dstSlice = dst.Items[arrayIndex];
				auto mipLevels = srcSlice.Mips.Count();
				dstSlice.Mips.Resize(mipLevels, false);
	
				// Convert all mip levels
				for (int32 mipIndex = 0; mipIndex < mipLevels; mipIndex++)
				{
					const auto& srcMip = srcSlice.Mips[mipIndex];
					auto& dstMip = dstSlice.Mips[mipIndex];
					auto mipWidth = Math::Max(textureData->Width >> mipIndex, 1);
					auto mipHeight = Math::Max(textureData->Height >> mipIndex, 1);
	
					// Allocate memory
					dstMip.RowPitch = mipWidth * bytesPerPixel;
					dstMip.DepthPitch = dstMip.RowPitch * mipHeight;
					dstMip.Lines = mipHeight;
					dstMip.Data.Allocate(dstMip.DepthPitch);
	
					// Convert texture
					for (int32 y = 0; y < mipHeight; y++)
					{
						for (int32 x = 0; x < mipWidth; x++)
						{
							// Sample source texture
							Color color = TextureUtils::SamplePoint(sampler, x, y, srcMip.Data.Get(), srcMip.RowPitch);
	
							// Store destination texture
							TextureUtils::Store(dstSampler, x, y, dstMip.Data.Get(), dstMip.RowPitch, color);
						}
					}
				}
			}
		}
	
		return true;
	}
	
	bool TextureUtils::ResizeStb(PixelFormat format, TextureMipData& dstMip, const TextureMipData& srcMip, int32 dstMipWidth, int32 dstMipHeight)
	{
		// Setup
		auto formatSize = PixelFormatGetSizeInBytes(format);
		auto components = PixelFormatComputeComponentsCount(format);
		auto srcMipWidth = srcMip.RowPitch / formatSize;
		auto srcMipHeight = srcMip.DepthPitch / srcMip.RowPitch;
		auto sampler = GetSampler(format);
	
		// Allocate memory
		dstMip.RowPitch = dstMipWidth * formatSize;
		dstMip.DepthPitch = dstMip.RowPitch * dstMipHeight;
		dstMip.Lines = dstMipHeight;
		dstMip.Data.Allocate(dstMip.DepthPitch);
	
		// Resize texture
		switch (format)
		{
		case PixelFormat::R8_SInt:
		case PixelFormat::R8_SNorm:
		case PixelFormat::R8G8_SInt:
		case PixelFormat::R8G8_SNorm:
		case PixelFormat::R8G8B8A8_UNorm:
		case PixelFormat::R8G8B8A8_UInt:
		case PixelFormat::R8G8B8A8_SNorm:
		case PixelFormat::R8G8B8A8_SInt:
		case PixelFormat::B8G8R8A8_UNorm:
		{
			if (!stbir_resize_uint8((const uint8*)srcMip.Data.Get(), srcMipWidth, srcMipHeight, srcMip.RowPitch, (uint8*)dstMip.Data.Get(), dstMipWidth, dstMipHeight, dstMip.RowPitch, components))
			{
				LOG_WARNING("Graphic", "Cannot resize image.");
				return true;
			}
			break;
		}
		case PixelFormat::R8G8B8A8_UNorm_SRGB:
		case PixelFormat::B8G8R8A8_UNorm_SRGB:
		{
			auto alphaChannel = 3;
			if (!stbir_resize_uint8_srgb((const uint8*)srcMip.Data.Get(), srcMipWidth, srcMipHeight, srcMip.RowPitch, (uint8*)dstMip.Data.Get(), dstMipWidth, dstMipHeight, dstMip.RowPitch, components, alphaChannel, 0))
			{
				LOG_WARNING("Graphic", "Cannot resize image.");
				return true;
			}
			break;
		}
		case PixelFormat::R32_Float:
		case PixelFormat::R32G32_Float:
		case PixelFormat::R32G32B32_Float:
		case PixelFormat::R32G32B32A32_Float:
		{
			if (!stbir_resize_float((const float*)srcMip.Data.Get(), srcMipWidth, srcMipHeight, srcMip.RowPitch, (float*)dstMip.Data.Get(), dstMipWidth, dstMipHeight, dstMip.RowPitch, components))
			{
				LOG_WARNING("Graphic", "Cannot resize image.");
				return true;
			}
			break;
		}
		default:
			if (sampler)
			{
				const Int2 srcSize(srcMipWidth, srcMipHeight);
				for (int32 y = 0; y < dstMipHeight; y++)
				{
					for (int32 x = 0; x < dstMipWidth; x++)
					{
						const Float2 uv((float)x / dstMipWidth, (float)y / dstMipHeight);
						Color color = SamplePoint(sampler, uv, srcMip.Data.Get(), srcSize, srcMip.RowPitch);
						Store(sampler, x, y, dstMip.Data.Get(), dstMip.RowPitch, color);
					}
				}
				return false;
			}
			LOG_WARNING("Graphic", "Cannot resize image. Unsupported format {0}", static_cast<int32>(format));
			return true;
		}
	
		return false;
	}
	
	bool TextureUtils::ResizeStb(TextureData& dst, const TextureData& src, int32 dstWidth, int32 dstHeight)
	{
		// Setup
		auto arraySize = src.GetArraySize();
		dst.Width = dstWidth;
		dst.Height = dstHeight;
		dst.Depth = src.Depth;
		dst.Format = src.Format;
		dst.Items.Resize(arraySize, false);
		auto formatSize = PixelFormatGetSizeInBytes(src.Format);
		auto components = PixelFormatComputeComponentsCount(src.Format);
	
		// Resize all array slices
		for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
		{
			const auto& srcSlice = src.Items[arrayIndex];
			auto& dstSlice = dst.Items[arrayIndex];
			auto mipLevels = srcSlice.Mips.Count();
			dstSlice.Mips.Resize(mipLevels, false);
	
			// Resize all mip levels
			for (int32 mipIndex = 0; mipIndex < mipLevels; mipIndex++)
			{
				const auto& srcMip = srcSlice.Mips[mipIndex];
				auto& dstMip = dstSlice.Mips[mipIndex];
				auto srcMipWidth = srcMip.RowPitch / formatSize;
				auto srcMipHeight = srcMip.DepthPitch / srcMip.RowPitch;
				auto dstMipWidth = Math::Max(dstWidth >> mipIndex, 1);
				auto dstMipHeight = Math::Max(dstHeight >> mipIndex, 1);
				if (ResizeStb(src.Format, dstMip, srcMip, dstMipWidth, dstMipHeight))
					return true;
			}
		}
	
		return false;
	}
}


#include "TextureUtils.h"
#include "Packed.h"
#include "Core/Logging/Logging.h"
#include "Core/TypeSystem/Types.h"
#include "Core/Types/DateTime.h"
#include "Core/Types/TimeSpan.h"
#include "Core/Math/Color.h"
#include "Core/Math/Vector2.h"
#include "Core/Profiler/ProfilerCPU.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Serialization/JsonWriter.h"
#include "Core/Serialization/JsonTools.h"
#include "Runtime/Graphics/Base/GPUUtils.h"

#if SE_EDITOR
#include "Core/Types/Collections/Dictionary.h"
#endif

namespace SE
{
#if SE_EDITOR
	namespace
	{
		Dictionary<String, bool> TexturesHasAlphaCache;
	}
#endif

	String TextureUtils::Options::ToString() const
	{
		return String::Format(SE_TEXT(
				"Type: {}, IsAtlas: {}, NeverStream: {}, IndependentChannels: {}, sRGB: {}, GenerateMipMaps: {}, FlipY: {}, InvertGreen: {} Scale: {}, MaxSize: {}, Resize: {}, PreserveAlphaCoverage: {}, PreserveAlphaCoverageReference: {}, SizeX: {}, SizeY: {}"),
			Types::GetEnumString<TextureFormatType>(Type),
			IsAtlas,
			NeverStream,
			IndependentChannels,
			sRGB,
			GenerateMipMaps,
			FlipY,
			InvertGreenChannel,
			Scale,
			MaxSize,
			MaxSize,
			Resize,
			PreserveAlphaCoverage,
			PreserveAlphaCoverageReference,
			SizeX,
			SizeY
		);
	}

	void TextureUtils::Options::Serialize(SerializeContext& context)
	{
		context.stream.JKEY("Type");
		context.stream.Enum(Type);

		context.stream.JKEY("IsAtlas");
		context.stream.Bool(IsAtlas);

		context.stream.JKEY("NeverStream");
		context.stream.Bool(NeverStream);

		context.stream.JKEY("Compress");
		context.stream.Bool(Compress);

		context.stream.JKEY("IndependentChannels");
		context.stream.Bool(IndependentChannels);

		context.stream.JKEY("sRGB");
		context.stream.Bool(sRGB);

		context.stream.JKEY("GenerateMipMaps");
		context.stream.Bool(GenerateMipMaps);

		context.stream.JKEY("FlipY");
		context.stream.Bool(FlipY);

		context.stream.JKEY("InvertGreenChannel");
		context.stream.Bool(InvertGreenChannel);

		context.stream.JKEY("Resize");
		context.stream.Bool(Resize);

		context.stream.JKEY("PreserveAlphaCoverage");
		context.stream.Bool(PreserveAlphaCoverage);

		context.stream.JKEY("PreserveAlphaCoverageReference");
		context.stream.Float(PreserveAlphaCoverageReference);

		context.stream.JKEY("TextureGroup");
		context.stream.Int(TextureGroup);

		context.stream.JKEY("Scale");
		context.stream.Float(Scale);

		context.stream.JKEY("MaxSize");
		context.stream.Int(MaxSize);

		context.stream.JKEY("SizeX");
		context.stream.Int(SizeX);

		context.stream.JKEY("SizeY");
		context.stream.Int(SizeY);

		context.stream.JKEY("Sprites");
		context.stream.StartArray();
		for (int32 i = 0; i < Sprites.Count(); i++)
		{
			auto& s = Sprites[i];

			context.stream.StartObject();

			context.stream.JKEY("Position");
			context.stream.Float2(s.Area.Location);

			context.stream.JKEY("Size");
			context.stream.Float2(s.Area.Size);

			context.stream.JKEY("Name");
			context.stream.String(s.Name);

			context.stream.EndObject();
		}
		context.stream.EndArray();
	}

	void TextureUtils::Options::Deserialize(DeserializeContext& context)
	{
		// Restore general import options
		Type = JsonTools::GetEnum(*context.stream, "Type", Type);
		IsAtlas = JsonTools::GetBool(*context.stream, "IsAtlas", IsAtlas);
		NeverStream = JsonTools::GetBool(*context.stream, "NeverStream", NeverStream);
		Compress = JsonTools::GetBool(*context.stream, "Compress", Compress);
		IndependentChannels = JsonTools::GetBool(*context.stream, "IndependentChannels", IndependentChannels);
		sRGB = JsonTools::GetBool(*context.stream, "sRGB", sRGB);
		GenerateMipMaps = JsonTools::GetBool(*context.stream, "GenerateMipMaps", GenerateMipMaps);
		FlipY = JsonTools::GetBool(*context.stream, "FlipY", FlipY);
		InvertGreenChannel = JsonTools::GetBool(*context.stream, "InvertGreenChannel", InvertGreenChannel);
		Resize = JsonTools::GetBool(*context.stream, "Resize", Resize);
		PreserveAlphaCoverage = JsonTools::GetBool(*context.stream, "PreserveAlphaCoverage", PreserveAlphaCoverage);
		PreserveAlphaCoverageReference = JsonTools::GetFloat(*context.stream, "PreserveAlphaCoverageReference", PreserveAlphaCoverageReference);
		TextureGroup = JsonTools::GetInt(*context.stream, "TextureGroup", TextureGroup);
		Scale = JsonTools::GetFloat(*context.stream, "Scale", Scale);
		SizeX = JsonTools::GetInt(*context.stream, "SizeX", SizeX);
		SizeY = JsonTools::GetInt(*context.stream, "SizeY", SizeY);
		MaxSize = JsonTools::GetInt(*context.stream, "MaxSize", MaxSize);

		// Load sprites
		// Note: we use it if no sprites in texture header has been loaded earlier
	    auto* spritesMember = context.stream->FindMember("Sprites");
	    if (spritesMember != context.stream->MemberEnd() && Sprites.IsEmpty())
	    {
	        auto& spritesArray = spritesMember->value;
	        ENGINE_ASSERT(spritesArray.IsArray());
	        Sprites.EnsureCapacity(spritesArray.Size());

	        for (uint32 i = 0; i < spritesArray.Size(); i++)
	        {
	            Sprite s;
	            auto& stData = spritesArray[i];

	            s.Area.Location = JsonTools::GetFloat2(stData, "Position", Float2::Zero);
	            s.Area.Size = JsonTools::GetFloat2(stData, "Size", Float2::One);
	            s.Name = JsonTools::GetString(stData, "Name");

	            Sprites.Add(s);
	        }
	    }
	}

#if SE_EDITOR

	bool TextureUtils::HasAlpha(const StringView& path)
	{
		// Try to hit the cache (eg. if texture was already imported before)
		if (!TexturesHasAlphaCache.ContainsKey(path))
		{
			TextureData textureData;
			if (ImportTexture(path, textureData))
				return false;
		}
		return TexturesHasAlphaCache[path];
	}

#endif

	bool TextureUtils::ImportTexture(const StringView& path, TextureData& textureData)
	{
		PROFILE_CPU();
		LOG_INFO("Graphic", "Importing texture from \'{0}\'", path);
		const auto startTime = DateTime::NowUTC();

		// Detect texture format type
		ImageType type;
		if (GetImageType(path, type))
			return true;

		// Import
		bool hasAlpha = false;
#if COMPILE_USE_DIRECTXTEX
		const auto failed = ImportTextureDirectXTex(type, path, textureData, hasAlpha);
#elif COMPILE_USE_STB
		const auto failed = ImportTextureStb(type, path, textureData, hasAlpha);
#else
		const auto failed = true;
		LOG_WARNING("Graphic", "Importing textures is not supported on this platform.");
#endif

		if (failed)
		{
			LOG_WARNING("Graphic", "Importing texture failed.");
		}
		else
		{
#if SE_EDITOR
			TexturesHasAlphaCache[path] = hasAlpha;
#endif
			LOG_INFO("Graphic", "Texture imported in {0} ms", static_cast<int32>((DateTime::NowUTC() - startTime).GetTotalMilliseconds()));
		}

		return failed;
	}

	bool TextureUtils::ImportTexture(const StringView& path, TextureData& textureData, Options options, String& errorMsg)
	{
		PROFILE_CPU();
		LOG_INFO("Graphic", "Importing texture from \'{0}\'. Options: {1}", path, options.ToString());
		const auto startTime = DateTime::NowUTC();

		// Detect texture format type
		ImageType type;
		if (options.InternalLoad.IsBinded())
		{
			type = ImageType::Internal;
		}
		else
		{
			if (!GetImageType(path, type))
			{
				return false;
			}
		}

		// Clamp values
		options.MaxSize = Math::Clamp(options.MaxSize, 1, GPU_MAX_TEXTURE_SIZE);
		options.SizeX = Math::Clamp(options.SizeX, 1, GPU_MAX_TEXTURE_SIZE);
		options.SizeY = Math::Clamp(options.SizeY, 1, GPU_MAX_TEXTURE_SIZE);

		// Import
		bool hasAlpha = false;
#if COMPILE_USE_DIRECTXTEX
		const auto failed = ImportTextureDirectXTex(type, path, textureData, options, errorMsg, hasAlpha);
#elif COMPILE_USE_STB
		const auto success = ImportTextureStb(type, path, textureData, options, errorMsg, hasAlpha);
#else
		const auto success = false;
		LOG_WARNING("Graphic", "Importing textures is not supported on this platform.");
#endif

		if (success)
		{
#if SE_EDITOR
			TexturesHasAlphaCache[path] = hasAlpha;
#endif
			LOG_INFO("Graphic", "Texture imported in {0} ms", static_cast<int32>((DateTime::NowUTC() - startTime).GetTotalMilliseconds()));
		}
		else
		{
			LOG_WARNING("Graphic", "Importing texture failed. {0}", errorMsg);
		}

		return success;
	}

	bool TextureUtils::ExportTexture(const StringView& path, const TextureData& textureData)
	{
		PROFILE_CPU();
		LOG_INFO("Graphic", "Exporting texture to \'{0}\'.", path);
		const auto startTime = DateTime::NowUTC();
		ImageType type;
		if (GetImageType(path, type))
			return true;
		if (textureData.Items.IsEmpty())
		{
			LOG_WARNING("Graphic", "Missing texture data.");
			return true;
		}

#if COMPILE_USE_DIRECTXTEX
		const auto failed = ExportTextureDirectXTex(type, path, textureData);
#elif COMPILE_USE_STB
		const auto failed = ExportTextureStb(type, path, textureData);
#else
		const auto failed = true;
		LOG_WARNING("Graphic", "Exporting textures is not supported on this platform.");
#endif

		if (failed)
		{
			LOG_WARNING("Graphic", "Exporting failed.");
		}
		else
		{
			LOG_INFO("Graphic", "Texture exported in {0} ms", static_cast<int32>((DateTime::NowUTC() - startTime).GetTotalMilliseconds()));
		}

		return failed;
	}

	bool TextureUtils::Convert(TextureData& dst, const TextureData& src, const PixelFormat dstFormat)
	{
		if (src.GetMipLevels() == 0)
		{
			LOG_WARNING("Graphic", "Missing source data.");
			return true;
		}
		if (src.Format == dstFormat)
		{
			LOG_WARNING("Graphic", "Source data and destination format are the same. Cannot perform conversion.");
			return true;
		}
		if (src.Depth != 1)
		{
			LOG_WARNING("Graphic", "Converting volume texture data is not supported.");
			return true;
		}
		PROFILE_CPU();

#if COMPILE_USE_DIRECTXTEX
		return ConvertDirectXTex(dst, src, dstFormat);
#elif COMPILE_USE_STB
		return ConvertStb(dst, src, dstFormat);
#else
		LOG_WARNING("Graphic", "Converting textures is not supported on this platform.");
		return true;
#endif
	}

	bool TextureUtils::Resize(TextureData& dst, const TextureData& src, int32 dstWidth, int32 dstHeight)
	{
		if (src.GetMipLevels() == 0)
		{
			LOG_WARNING("Graphic", "Missing source data.");
			return true;
		}
		if (src.Width == dstWidth && src.Height == dstHeight)
		{
			LOG_WARNING("Graphic", "Source data and destination dimensions are the same. Cannot perform resizing.");
			return true;
		}
		if (src.Depth != 1)
		{
			LOG_WARNING("Graphic", "Resizing volume texture data is not supported.");
			return true;
		}
		PROFILE_CPU();
#if COMPILE_USE_DIRECTXTEX
		return ResizeDirectXTex(dst, src, dstWidth, dstHeight);
#elif COMPILE_USE_STB
		return ResizeStb(dst, src, dstWidth, dstHeight);
#else
		LOG_WARNING("Graphic", "Resizing textures is not supported on this platform.");
		return true;
#endif
	}

	TextureUtils::PixelFormatSampler PixelFormatSamplers[] =
		{
			{
				PixelFormat::R32G32B32A32_Float,
				sizeof(Float4),
				[](const void* ptr)
				{
				  return Color(*(Float4*)ptr);
				},
				[](const void* ptr, const Color& color)
				{
				  *(Float4*)ptr = color.ToFloat4();
				},
			},
			{
				PixelFormat::R32G32B32_Float,
				sizeof(Float3),
				[](const void* ptr)
				{
				  return Color(*(Float3*)ptr, 1.0f);
				},
				[](const void* ptr, const Color& color)
				{
				  *(Float3*)ptr = color.ToFloat3();
				},
			},
			{
				PixelFormat::R16G16B16A16_Float,
				sizeof(Half4),
				[](const void* ptr)
				{
				  return Color(((Half4*)ptr)->ToFloat4());
				},
				[](const void* ptr, const Color& color)
				{
				  *(Half4*)ptr = Half4(color.r, color.g, color.b, color.a);
				},
			},
			{
				PixelFormat::R16G16B16A16_UNorm,
				sizeof(RGBA16UNorm),
				[](const void* ptr)
				{
				  return Color(((RGBA16UNorm*)ptr)->ToFloat4());
				},
				[](const void* ptr, const Color& color)
				{
				  *(RGBA16UNorm*)ptr = RGBA16UNorm(color.r, color.g, color.b, color.a);
				}
			},
			{
				PixelFormat::R32G32_Float,
				sizeof(Float2),
				[](const void* ptr)
				{
				  return Color(((Float2*)ptr)->x, ((Float2*)ptr)->y, 1.0f);
				},
				[](const void* ptr, const Color& color)
				{
				  *(Float2*)ptr = Float2(color.r, color.g);
				},
			},
			{
				PixelFormat::R8G8B8A8_UNorm,
				sizeof(Color32),
				[](const void* ptr)
				{
				  return Color(*(Color32*)ptr);
				},
				[](const void* ptr, const Color& color)
				{
				  *(Color32*)ptr = Color32(color);
				},
			},
			{
				PixelFormat::R8G8B8A8_UNorm_SRGB,
				sizeof(Color32),
				[](const void* ptr)
				{
				  return Color::SrgbToLinear(Color(*(Color32*)ptr));
				},
				[](const void* ptr, const Color& color)
				{
				  Color srgb = Color::LinearToSrgb(color);
				  *(Color32*)ptr = Color32(srgb);
				},
			},
			{
				PixelFormat::R8G8_UNorm,
				sizeof(uint16),
				[](const void* ptr)
				{
				  const uint8* rg = (const uint8*)ptr;
				  return Color((float)rg[0] / Max_uint8, (float)rg[1] / Max_uint8, 0, 1);
				},
				[](const void* ptr, const Color& color)
				{
				  uint8* rg = (uint8*)ptr;
				  rg[0] = (uint8)(color.r * Max_uint8);
				  rg[1] = (uint8)(color.g * Max_uint8);
				},
			},
			{
				PixelFormat::R16G16_Float,
				sizeof(Half2),
				[](const void* ptr)
				{
				  const Float2 rg = ((Half2*)ptr)->ToFloat2();
				  return Color(rg.x, rg.y, 0, 1);
				},
				[](const void* ptr, const Color& color)
				{
				  *(Half2*)ptr = Half2(color.r, color.g);
				},
			},
			{
				PixelFormat::R16G16_UNorm,
				sizeof(RG16UNorm),
				[](const void* ptr)
				{
				  const Float2 rg = ((RG16UNorm*)ptr)->ToFloat2();
				  return Color(rg.x, rg.y, 0, 1);
				},
				[](const void* ptr, const Color& color)
				{
				  *(RG16UNorm*)ptr = RG16UNorm(color.r, color.g);
				},
			},
			{
				PixelFormat::R32_Float,
				sizeof(float),
				[](const void* ptr)
				{
				  return Color(*(float*)ptr, 0, 0, 1);
				},
				[](const void* ptr, const Color& color)
				{
				  *(float*)ptr = color.r;
				},
			},
			{
				PixelFormat::R16_Float,
				sizeof(Half),
				[](const void* ptr)
				{
				  return Color(Float16Compressor::Decompress(*(Half*)ptr), 0, 0, 1);
				},
				[](const void* ptr, const Color& color)
				{
				  *(Half*)ptr = Float16Compressor::Compress(color.r);
				},
			},
			{
				PixelFormat::R16_UNorm,
				sizeof(uint16),
				[](const void* ptr)
				{
				  return Color((float)*(uint16*)ptr / Max_uint16, 0, 0, 1);
				},
				[](const void* ptr, const Color& color)
				{
				  *(uint16*)ptr = (uint16)(color.r * Max_uint16);
				},
			},
			{
				PixelFormat::R8_UNorm,
				sizeof(uint8),
				[](const void* ptr)
				{
				  return Color((float)*(byte*)ptr / Max_uint8, 0, 0, 1);
				},
				[](const void* ptr, const Color& color)
				{
				  *(byte*)ptr = (byte)(color.r * Max_uint8);
				},
			},
			{
				PixelFormat::B8G8R8A8_UNorm,
				sizeof(Color32),
				[](const void* ptr)
				{
				  const Color32 bgra = *(Color32*)ptr;
				  return Color(Color32(bgra.b, bgra.g, bgra.r, bgra.a));
				},
				[](const void* ptr, const Color& color)
				{
				  *(Color32*)ptr = Color32(byte(color.b * Max_uint8), byte(color.g * Max_uint8), byte(color.r * Max_uint8), byte(color.a * Max_uint8));
				},
			},
			{
				PixelFormat::B8G8R8A8_UNorm_SRGB,
				sizeof(Color32),
				[](const void* ptr)
				{
				  const Color32 bgra = *(Color32*)ptr;
				  return Color::SrgbToLinear(Color(Color32(bgra.b, bgra.g, bgra.r, bgra.a)));
				},
				[](const void* ptr, const Color& color)
				{
				  Color srgb = Color::LinearToSrgb(color);
				  *(Color32*)ptr = Color32(byte(srgb.b * Max_uint8), byte(srgb.g * Max_uint8), byte(srgb.r * Max_uint8), byte(srgb.a * Max_uint8));
				},
			},
			{
				PixelFormat::R11G11B10_Float,
				sizeof(FloatR11G11B10),
				[](const void* ptr)
				{
				  const Float3 rgb = ((FloatR11G11B10*)ptr)->ToFloat3();
				  return Color(rgb.x, rgb.y, rgb.z);
				},
				[](const void* ptr, const Color& color)
				{
				  *(FloatR11G11B10*)ptr = FloatR11G11B10(color.r, color.g, color.b);
				},
			},
			{
				PixelFormat::R10G10B10A2_UNorm,
				sizeof(Float1010102),
				[](const void* ptr)
				{
				  const Float3 rgb = ((Float1010102*)ptr)->ToFloat3();
				  return Color(rgb.x, rgb.y, rgb.z);
				},
				[](const void* ptr, const Color& color)
				{
				  *(Float1010102*)ptr = Float1010102(color.r, color.g, color.b, color.a);
				},
			},
		};

	const TextureUtils::PixelFormatSampler* TextureUtils::GetSampler(PixelFormat format)
	{
		for (auto& sampler : PixelFormatSamplers)
		{
			if (sampler.Format == format)
				return &sampler;
		}
		return nullptr;
	}

	void TextureUtils::Store(const PixelFormatSampler* sampler, int32 x, int32 y, const void* data, int32 rowPitch, const Color& color)
	{
		ASSERT_LOW_LAYER(sampler);
		sampler->Store((byte*)data + rowPitch * y + sampler->PixelSize * x, color);
	}

	Color TextureUtils::SamplePoint(const PixelFormatSampler* sampler, const Float2& uv, const void* data, const Int2& size, int32 rowPitch)
	{
		ASSERT_LOW_LAYER(sampler);

		const Int2 end = size - 1;
		const Int2 uvFloor(Math::Min(Math::FloorToInt(uv.x * size.x), end.x), Math::Min(Math::FloorToInt(uv.y * size.y), end.y));

		return sampler->Sample((byte*)data + rowPitch * uvFloor.y + sampler->PixelSize * uvFloor.x);
	}

	Color TextureUtils::SamplePoint(const PixelFormatSampler* sampler, int32 x, int32 y, const void* data, int32 rowPitch)
	{
		ASSERT_LOW_LAYER(sampler);
		return sampler->Sample((byte*)data + rowPitch * y + sampler->PixelSize * x);
	}

	Color TextureUtils::SampleLinear(const PixelFormatSampler* sampler, const Float2& uv, const void* data, const Int2& size, int32 rowPitch)
	{
		ASSERT_LOW_LAYER(sampler);

		const Int2 end = size - 1;
		const Int2 uvFloor(Math::Min(Math::FloorToInt(uv.x * size.x), end.x), Math::Min(Math::FloorToInt(uv.y * size.y), end.y));
		const Int2 uvNext(Math::Min(uvFloor.x + 1, end.x), Math::Min(uvFloor.y + 1, end.y));
		const Float2 uvFraction(uv.x * size.y - uvFloor.x, uv.y * size.y - uvFloor.y);

		const Color v00 = sampler->Sample((byte*)data + rowPitch * uvFloor.y + sampler->PixelSize * uvFloor.x);
		const Color v01 = sampler->Sample((byte*)data + rowPitch * uvFloor.y + sampler->PixelSize * uvNext.x);
		const Color v10 = sampler->Sample((byte*)data + rowPitch * uvNext.y + sampler->PixelSize * uvFloor.x);
		const Color v11 = sampler->Sample((byte*)data + rowPitch * uvNext.y + sampler->PixelSize * uvNext.x);

		return Color::Lerp(Color::Lerp(v00, v01, uvFraction.x), Color::Lerp(v10, v11, uvFraction.x), uvFraction.y);
	}

	PixelFormat TextureUtils::ToPixelFormat(TextureFormatType format, int32 width, int32 height, bool canCompress)
	{
		const bool canUseBlockCompression = width % 4 == 0 && height % 4 == 0;
		if (canCompress && canUseBlockCompression)
		{
			switch (format)
			{
			case TextureFormatType::ColorRGB:
				return PixelFormat::BC1_UNorm;
			case TextureFormatType::ColorRGBA:
				return PixelFormat::BC3_UNorm;
			case TextureFormatType::NormalMap:
				return PixelFormat::BC5_UNorm;
			case TextureFormatType::GrayScale:
				return PixelFormat::BC4_UNorm;
			case TextureFormatType::HdrRGBA:
				return PixelFormat::BC7_UNorm;
			case TextureFormatType::HdrRGB:
#if PLATFORM_LINUX
				// TODO: support BC6H compression for Linux Editor
				return PixelFormat::BC7_UNorm;
#else
				return PixelFormat::BC6H_UF16;
#endif
			case TextureFormatType::CubeMap:
				return PixelFormat::BC7_UNorm;
			default:
				return PixelFormat::Undefined;
			}
		}

		switch (format)
		{
		case TextureFormatType::ColorRGB:
			return PixelFormat::R8G8B8A8_UNorm;
		case TextureFormatType::ColorRGBA:
			return PixelFormat::R8G8B8A8_UNorm;
		case TextureFormatType::NormalMap:
			return PixelFormat::R16G16_UNorm;
		case TextureFormatType::GrayScale:
			return PixelFormat::R8_UNorm;
		case TextureFormatType::HdrRGBA:
			return PixelFormat::R16G16B16A16_Float;
		case TextureFormatType::HdrRGB:
			return PixelFormat::R11G11B10_Float;
		case TextureFormatType::CubeMap:
			return PixelFormat::R32G32B32A32_Float;
		default:
			return PixelFormat::Undefined;
		}
	}

	bool TextureUtils::GetImageType(const StringView& path, ImageType& type)
	{
		const auto extension = FileSystem::GetExtension(path).ToLower();
		if (extension == SE_TEXT("tga"))
		{
			type = ImageType::TGA;
		}
		else if (extension == SE_TEXT("dds"))
		{
			type = ImageType::DDS;
		}
		else if (extension == SE_TEXT("png"))
		{
			type = ImageType::PNG;
		}
		else if (extension == SE_TEXT("bmp"))
		{
			type = ImageType::BMP;
		}
		else if (extension == SE_TEXT("gif"))
		{
			type = ImageType::GIF;
		}
		else if (extension == SE_TEXT("tiff") || extension == SE_TEXT("tif"))
		{
			type = ImageType::TIFF;
		}
		else if (extension == SE_TEXT("hdr"))
		{
			type = ImageType::HDR;
		}
		else if (extension == SE_TEXT("jpeg") || extension == SE_TEXT("jpg"))
		{
			type = ImageType::JPEG;
		}
		else if (extension == SE_TEXT("raw"))
		{
			type = ImageType::RAW;
		}
		else if (extension == SE_TEXT("exr"))
		{
			type = ImageType::EXR;
		}
		else
		{
			LOG_WARNING("Graphic", "Unknown file type.");
			return false;
		}

		return true;
	}

	bool TextureUtils::Transform(TextureData& texture, const Function<void(Color&)>& transformation)
	{
		PROFILE_CPU();
		auto sampler = TextureUtils::GetSampler(texture.Format);
		if (!sampler)
			return true;
		for (auto& slice : texture.Items)
		{
			for (int32 mipIndex = 0; mipIndex < slice.Mips.Count(); mipIndex++)
			{
				auto& mip = slice.Mips[mipIndex];
				auto mipWidth = Math::Max(texture.Width >> mipIndex, 1);
				auto mipHeight = Math::Max(texture.Height >> mipIndex, 1);
				for (int32 y = 0; y < mipHeight; y++)
				{
					for (int32 x = 0; x < mipWidth; x++)
					{
						Color color = TextureUtils::SamplePoint(sampler, x, y, mip.Data.Get(), mip.RowPitch);
						transformation(color);
						TextureUtils::Store(sampler, x, y, mip.Data.Get(), mip.RowPitch, color);
					}
				}
			}
		}
		return false;
	}
}


#pragma once

#include "Runtime/Render/2D/SpriteAtlas.h"
#include "Runtime/Graphics/Textures/GPUTexture.h"
#include "Runtime/Render/Assets/Texture/TextureData.h"
#include "Core/Serialization/ISerializable.h"

namespace SE
{
	class JsonWriter;

	/// <summary>
	/// Textures importing, processing and exporting utilities.
	/// </summary>
	class SE_API_RUNTIME TextureUtils
	{
	public:
		/// <summary>
		/// Texture import options.
		/// </summary>
		struct SE_API_RUNTIME Options : IType, ISerializable
		{
			SE_CLASS_DEFAULT(Options, IType);

			// Texture format type.
			// SE_PROPERTY(Attributes = "EditorOrder(0)")
			TextureFormatType Type = TextureFormatType::ColorRGBA;

			// True if texture should be imported as a texture atlas (with sprites).
			// SE_PROPERTY(Attributes = "EditorOrder(10)")
			bool IsAtlas = false;

			// True if disable dynamic texture streaming.
			// SE_PROPERTY(Attributes = "EditorOrder(20)")
			bool NeverStream = false;

			// True if disable dynamic texture streaming.
			// SE_PROPERTY(Attributes = "EditorOrder(30)")
			bool Compress = true;

			// True if texture channels have independent data (for compression methods).
			// SE_PROPERTY(Attributes = "EditorOrder(40)")
			bool IndependentChannels = false;

			// True if use sRGB format for texture data. Recommended for color maps and diffuse color textures.
			// SE_PROPERTY(Attributes = "EditorOrder(50), EditorDisplay(null, \"sRGB\")")
			bool sRGB = false;

			// True if generate mip maps chain for the texture.
			// SE_PROPERTY(Attributes = "EditorOrder(60)")
			bool GenerateMipMaps = true;

			// True if flip Y coordinate of the texture.
			// SE_PROPERTY(Attributes = "EditorOrder(70)")
			bool FlipY = false;

			// True if to invert the green channel on a normal map. Good for OpenGL to DirectX conversion.
			// SE_PROPERTY(Attributes = "EditorOrder(71)")
			bool InvertGreenChannel = false;

			// Texture size scale. Allows increasing or decreasing the imported texture resolution. Default is 1.
			// SE_PROPERTY(Attributes = "EditorOrder(80), Limit(0.0001f, 1000.0f, 0.01f)")
			float Scale = 1.0f;

			// Maximum size of the texture (for both width and height). Higher resolution textures will be resized during importing process. Used to clip textures that are too big.
			// SE_PROPERTY(Attributes = "HideInEditor")
			int32 MaxSize = 8192;

			// True if resize texture on import. Use SizeX/SizeY properties to define texture width and height. Texture scale property will be ignored.
			// SE_PROPERTY(Attributes = "EditorOrder(100)")
			bool Resize = false;

			// The width of the imported texture. If Resize property is set to true then texture will be resized during the import to this value during the import, otherwise it will be ignored.
			// SE_PROPERTY(Attributes = "HideInEditor")
			int32 SizeX = 1024;

			// The height of the imported texture. If Resize property is set to true then texture will be resized during the import to this value during the import, otherwise it will be ignored.
			// SE_PROPERTY(Attributes = "HideInEditor")
			int32 SizeY = 1024;

			// Check to preserve alpha coverage in generated mips for alpha test reference. Scales mipmap alpha values to preserve alpha coverage based on an alpha test reference value.
			// SE_PROPERTY(Attributes = "EditorOrder(200)")
			bool PreserveAlphaCoverage = false;

			// The reference value for the alpha coverage preserving.
			// SE_PROPERTY(Attributes = "EditorOrder(210), VisibleIf(\"PreserveAlphaCoverage\")")
			float PreserveAlphaCoverageReference = 0.5f;

			// The texture group for streaming (negative if unused). See Streaming Settings.
			// SE_PROPERTY(Attributes = "EditorOrder(300), CustomEditorAlias(\"FlaxEditor.CustomEditors.Dedicated.TextureGroupEditor\")")
			int32 TextureGroup = -1;

			// The sprites for the sprite sheet import mode.
			// SE_PROPERTY(Attributes = "HideInEditor")
			List<Sprite> Sprites;

			// Function used for fast importing textures used by internal parts of the engine
			Function<bool(TextureData&)> InternalLoad;

		public:
			String ToString() const;

			// [ISerializable]
			void Serialize(SerializeContext& context) override;
			void Deserialize(DeserializeContext& context) override;
		};

#if SE_EDITOR
		/// <summary>
		/// Checks whenever the given texture file contains alpha channel data with values different than solid fill of 1 (non fully opaque).
		/// </summary>
		/// <param name="path">The file path.</param>
		/// <returns>True if has alpha channel, otherwise false.</returns>
		static bool HasAlpha(const StringView& path);
#endif

		/// <summary>
		/// Imports the texture.
		/// </summary>
		/// <param name="path">The file path.</param>
		/// <param name="textureData">The output data.</param>
		/// <returns>True if fails, otherwise false.</returns>
		static bool ImportTexture(const StringView& path, TextureData& textureData);

		/// <summary>
		/// Imports the texture.
		/// </summary>
		/// <param name="path">The file path.</param>
		/// <param name="textureData">The output data.</param>
		/// <param name="options">The import options.</param>
		/// <param name="errorMsg">The error message container.</param>
		/// <returns>True if fails, otherwise false.</returns>
		static bool ImportTexture(const StringView& path, TextureData& textureData, Options options, String& errorMsg);

		/// <summary>
		/// Exports the texture.
		/// </summary>
		/// <param name="path">The output file path.</param>
		/// <param name="textureData">The output data.</param>
		/// <returns>True if fails, otherwise false.</returns>
		static bool ExportTexture(const StringView& path, const TextureData& textureData);

		/// <summary>
		/// Converts the specified source texture data into an another format.
		/// </summary>
		/// <param name="dst">The destination data.</param>
		/// <param name="src">The source data.</param>
		/// <param name="dstFormat">The destination data format. Must be other than source data type.</param>
		/// <returns>True if fails, otherwise false.</returns>
		static bool Convert(TextureData& dst, const TextureData& src, const PixelFormat dstFormat);

		/// <summary>
		/// Resizes the specified source texture data into an another dimensions.
		/// </summary>
		/// <param name="dst">The destination data.</param>
		/// <param name="src">The source data.</param>
		/// <param name="dstWidth">The destination data width.</param>
		/// <param name="dstHeight">The destination data height.</param>
		/// <returns>True if fails, otherwise false.</returns>
		static bool Resize(TextureData& dst, const TextureData& src, int32 dstWidth, int32 dstHeight);

	public:
		typedef Color (* ReadPixel)(const void*);
		typedef void (* WritePixel)(const void*, const Color&);

		struct PixelFormatSampler
		{
			PixelFormat Format;
			int32 PixelSize;
			ReadPixel Sample;
			WritePixel Store;
		};

		/// <summary>
		/// Determines whether this tool can sample the specified format to read texture samples and returns the sampler object.
		/// </summary>
		/// <param name="format">The format.</param>
		/// <returns>The pointer to the sampler object or null if cannot sample the given format.</returns>
		static const PixelFormatSampler* GetSampler(PixelFormat format);

		/// <summary>
		/// Stores the color into the specified texture data (uses no interpolation).
		/// </summary>
		/// <remarks>
		/// Use GetSampler for the texture sampler.
		/// </remarks>
		/// <param name="sampler">The texture data sampler.</param>
		/// <param name="x">The X texture coordinates (normalized to range 0-width).</param>
		/// <param name="y">The Y texture coordinates (normalized to range 0-height).</param>
		/// <param name="data">The data pointer for the texture slice (1D or 2D image).</param>
		/// <param name="rowPitch">The row pitch (in bytes). The offset between each image rows.</param>
		/// <param name="color">The color to store (linear).</param>
		static void Store(const PixelFormatSampler* sampler, int32 x, int32 y, const void* data, int32 rowPitch, const Color& color);

		/// <summary>
		/// Samples the specified texture data (uses no interpolation).
		/// </summary>
		/// <remarks>
		/// Use GetSampler for the texture sampler.
		/// </remarks>
		/// <param name="sampler">The texture data sampler.</param>
		/// <param name="uv">The texture coordinates (normalized to range 0-1).</param>
		/// <param name="data">The data pointer for the texture slice (1D or 2D image).</param>
		/// <param name="size">The size of the input texture (in pixels).</param>
		/// <param name="rowPitch">The row pitch (in bytes). The offset between each image rows.</param>
		/// <returns>The sampled color (linear).</returns>
		static Color SamplePoint(const PixelFormatSampler* sampler, const Float2& uv, const void* data, const Int2& size, int32 rowPitch);

		/// <summary>
		/// Samples the specified texture data (uses no interpolation).
		/// </summary>
		/// <remarks>
		/// Use GetSampler for the texture sampler.
		/// </remarks>
		/// <param name="sampler">The texture data sampler.</param>
		/// <param name="x">The X texture coordinates (normalized to range 0-width).</param>
		/// <param name="y">The Y texture coordinates (normalized to range 0-height).</param>
		/// <param name="data">The data pointer for the texture slice (1D or 2D image).</param>
		/// <param name="rowPitch">The row pitch (in bytes). The offset between each image rows.</param>
		/// <returns>The sampled color (linear).</returns>
		static Color SamplePoint(const PixelFormatSampler* sampler, int32 x, int32 y, const void* data, int32 rowPitch);

		/// <summary>
		/// Samples the specified texture data (uses linear interpolation).
		/// </summary>
		/// <remarks>
		/// Use GetSampler for the texture sampler.
		/// </remarks>
		/// <param name="sampler">The texture data sampler.</param>
		/// <param name="uv">The texture coordinates (normalized to range 0-1).</param>
		/// <param name="data">The data pointer for the texture slice (1D or 2D image).</param>
		/// <param name="size">The size of the input texture (in pixels).</param>
		/// <param name="rowPitch">The row pitch (in bytes). The offset between each image rows.</param>
		/// <returns>The sampled color (linear).</returns>
		static Color SampleLinear(const PixelFormatSampler* sampler, const Float2& uv, const void* data, const Int2& size, int32 rowPitch);

		static PixelFormat ToPixelFormat(TextureFormatType format, int32 width, int32 height, bool canCompress);

	private:
		enum class ImageType
		{
			DDS,
			TGA,
			PNG,
			BMP,
			GIF,
			TIFF,
			JPEG,
			HDR,
			RAW,
			EXR,
			Internal,
		};

		static bool GetImageType(const StringView& path, ImageType& type);
		static bool Transform(TextureData& texture, const Function<void(Color&)>& transformation);

#if COMPILE_USE_DIRECTXTEX
		static bool ExportTextureDirectXTex(ImageType type, const StringView& path, const TextureData& textureData);
		static bool ImportTextureDirectXTex(ImageType type, const StringView& path, TextureData& textureData, bool& hasAlpha);
		static bool ImportTextureDirectXTex(ImageType type, const StringView& path, TextureData& textureData, const Options& options, String& errorMsg, bool& hasAlpha);
		static bool ConvertDirectXTex(TextureData& dst, const TextureData& src, const PixelFormat dstFormat);
		static bool ResizeDirectXTex(TextureData& dst, const TextureData& src, int32 dstWidth, int32 dstHeight);
#endif

		static bool ExportTextureStb(ImageType type, const StringView& path, const TextureData& textureData);
		static bool ImportTextureStb(ImageType type, const StringView& path, TextureData& textureData, bool& hasAlpha);
		static bool ImportTextureStb(ImageType type, const StringView& path, TextureData& textureData, const Options& options, String& errorMsg, bool& hasAlpha);
		static bool ConvertStb(TextureData& dst, const TextureData& src, const PixelFormat dstFormat);
		static bool ResizeStb(PixelFormat format, TextureMipData& dstMip, const TextureMipData& srcMip, int32 dstMipWidth, int32 dstMipHeight);
		static bool ResizeStb(TextureData& dst, const TextureData& src, int32 dstWidth, int32 dstHeight);

#if COMPILE_WITH_ASTC
		static bool ConvertAstc(TextureData& dst, const TextureData& src, const PixelFormat dstFormat);
#endif
	};

}

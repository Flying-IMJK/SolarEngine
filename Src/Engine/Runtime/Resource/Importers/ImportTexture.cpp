
#include "ImportTexture.h"

#include "Core/Logging/Logging.h"
#include "Core/Serialization/Serialization.h"
#include "Core/Serialization/JsonWriters.hpp"
#include "Core/Serialization/MemoryWriteStream.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Platform/File.h"
#include "Core/TypeSystem/Types.h"

#include "Runtime/Render/Assets/Texture/TextureData.h"
#include "Runtime/Graphics/Base/GPUUtils.h"
#include "Runtime/Resource/Storage/AssetStorages.h"
/*#include "Engine/ContentImporters/ImportIES.h"
#include "Engine/Content/Assets/CubeTexture.h"
#include "Engine/Content/Assets/IESProfile.h"*/
#include "Runtime/Render/2D/SpriteAtlas.h"
#include "Runtime/Render/Assets/Texture/CubeTexture.h"
#include "Runtime/Render/Assets/Texture/Texture.h"


namespace SE
{

	bool IsSpriteAtlasOrTexture(const TypeID type)
	{
		return type == Typeof<Texture>() || type == Typeof<SpriteAtlas>();
	}

	bool ImportTexture::TryGetImportOptions(const StringView& path, Options& options)
	{
#if IMPORT_TEXTURE_CACHE_OPTIONS
		if (FileSystem::FileExists(path))
		{
			auto tmpFile = AssetStorages::GetStorage(path);
			AssetInitData data;
			if (tmpFile
				&& tmpFile->GetEntriesCount() == 1
				&& IsSpriteAtlasOrTexture(tmpFile->GetEntry(0).Type)
				&& !tmpFile->LoadAssetHeader(0, data)
				&& data.SerializedVersion >= 4)
			{
				// For sprite atlas try to get sprites from the last chunk
/*				if (tmpFile->GetEntry(0).Type == SpriteAtlas::TypeName)
				{
					auto chunk15 = data.Header.Chunks[15];
					if (chunk15 != nullptr && !tmpFile->LoadAssetChunk(chunk15) && chunk15->Data.IsValid())
					{
						MemoryReadStream stream(chunk15->Data.Get(), chunk15->Data.Length());
						int32 tilesVersion, tilesCount;
						stream.ReadInt32(&tilesVersion);
						if (tilesVersion == 1)
						{
							options.Sprites.Clear();
							stream.ReadInt32(&tilesCount);
							for (int32 i = 0; i < tilesCount; i++)
							{
								// Load sprite
								Sprite t;
								stream.Read(t.Area);
								stream.ReadString(&t.Name, 49);
								options.Sprites.Add(t);
							}
						}
					}
				}*/

				// Check import meta
				Json::Document metadata;
				metadata.Parse((const char*)data.Metadata.Get(), data.Metadata.Length());
				if (metadata.HasParseError() == false)
				{
					DeserializeContext deserializeContext(metadata, nullptr);
					options.Deserialize(deserializeContext);
					return true;
				}
			}
		}
#endif
		return false;
	}

	void ImportTexture::InitOptions(CreateAssetContext& context, Options& options)
	{
		// Gather import options
		if (context.CustomArg != nullptr)
		{
			// Copy options
			options = *static_cast<Options*>(context.CustomArg);
			ASSERT_LOW_LAYER(options.Sprites.Count() >= 0);
		}
		else
		{
			// Restore the previous settings or use default ones
			if (!TryGetImportOptions(context.TargetAssetPath, options))
			{
				LOG_WARNING("Resource", "Missing texture import options. Using default values.");
			}
		}

		// Tweak options
		if (options.IsAtlas)
		{
			// Disable streaming for atlases
			// TODO: maybe we could use streaming for atlases?
			options.NeverStream = true;

			// Add default tile if has no sprites
			if (options.Sprites.IsEmpty())
			{
				options.Sprites.Add(Sprite{Rectangle(Float2::Zero, Float2::One), SE_TEXT("Default")});
			}
		}
		options.MaxSize = Math::Min(options.MaxSize, GPU_MAX_TEXTURE_SIZE);
	}

	CreateAssetResult ImportTexture::Create(CreateAssetContext& context, const TextureData& textureData, Options& options)
	{
		// Check data
		bool isCubeMap = false;
		if (textureData.GetArraySize() != 1)
		{
			if (options.IsAtlas)
			{
				LOG_WARNING("Resource", "Cannot import sprite atlas texture that has more than one array slice.");
				return CreateAssetResult::Error;
			}
			if (textureData.GetArraySize() != 6)
			{
				LOG_WARNING("Resource", "Cannot import texture that has {0} array slices. Use single plane images (single 2D) or cube maps (6 slices).", textureData.GetArraySize());
				return CreateAssetResult::Error;
			}
			else
			{
				isCubeMap = true;

				if (textureData.Width != textureData.Height)
				{
					LOG_WARNING("Resource", "Invalid cube texture size.");
					return CreateAssetResult::Error;
				}
			}
		}

		// Base
		if (isCubeMap)
		{
			IMPORT_SETUP(CubeTexture, ASSET_VERSION_CUBETEXTURE);
		}
		else if (options.IsAtlas)
		{
			IMPORT_SETUP(SpriteAtlas,  ASSET_VERSION_SPRITEATLAS);
		}
		else
		{
			IMPORT_SETUP(Texture, ASSET_VERSION_TEXTURE);
		}

		// Fill texture header
		TextureHeader textureHeader;
		textureHeader.NeverStream = options.NeverStream;
		textureHeader.Width = textureData.Width;
		textureHeader.Height = textureData.Height;
		textureHeader.Format = textureData.Format;
		textureHeader.Type = options.Type;
		textureHeader.MipLevels = textureData.GetMipLevels();
		textureHeader.IsSRGB = PixelFormatIsSRGB(textureHeader.Format);
		textureHeader.IsCubeMap = isCubeMap;
		textureHeader.TextureGroup = options.TextureGroup;
		ENGINE_ASSERT(textureHeader.MipLevels <= GPU_MAX_TEXTURE_MIP_LEVELS);

		// Save header
		context.Data.CustomData.Copy(&textureHeader);

		// Save atlas sprites data
		if (options.IsAtlas)
		{
/*			MemoryWriteStream stream(256);
			stream.WriteInt32(1); // Version
			stream.WriteInt32(options.Sprites.Count()); // Amount of tiles
			for (int32 i = 0; i < options.Sprites.Count(); i++)
			{
				auto& sprite = options.Sprites[i];
				stream.Write(sprite.Area);
				stream.WriteString(sprite.Name, 49);
			}
			if (context.AllocateChunk(15))
				return CreateAssetResult::CannotAllocateChunk;
			context.Data.Header.Chunks[15]->Data.Copy(stream.GetHandle(), stream.GetPosition());*/
		}

		// Save mip maps
		if (!isCubeMap)
		{
			for (int32 mipIndex = 0; mipIndex < textureHeader.MipLevels; mipIndex++)
			{
				auto mipData = textureData.GetData(0, mipIndex);

				if (!context.AllocateChunk(mipIndex))
					return CreateAssetResult::CannotAllocateChunk;
				context.Data.Header.Chunks[mipIndex]->Data.Copy(mipData->Data.Get(), static_cast<uint32>(mipData->DepthPitch));
			}
		}
		else
		{
			// Allocate memory for a temporary buffer
			const uint32 imageSize = textureData.GetData(0, 0)->DepthPitch * 6;
			MemoryWriteStream imageData(imageSize);

			// Copy cube sides for every mip into separate chunks
			for (int32 mipLevelIndex = 0; mipLevelIndex < textureHeader.MipLevels; mipLevelIndex++)
			{
				// Write array slices to the stream
				imageData.SetPosition(0);
				for (int32 cubeFaceIndex = 0; cubeFaceIndex < 6; cubeFaceIndex++)
				{
					// Get image
					const auto image = textureData.GetData(cubeFaceIndex, mipLevelIndex);
					if (image == nullptr)
					{
						LOG_WARNING("Resource", "Cannot create cube texture '{0}'. Missing image slice.", context.InputPath);
						return CreateAssetResult::Error;
					}
					ENGINE_ASSERT(image->DepthPitch < Max_int32);

					// Copy data
					imageData.WriteBytes(image->Data.Get(), image->Data.Length());
				}

				// Copy mip
				if (!context.AllocateChunk(mipLevelIndex))
					return CreateAssetResult::CannotAllocateChunk;
				context.Data.Header.Chunks[mipLevelIndex]->Data.Copy(imageData.GetHandle(), imageData.GetPosition());
			}
		}

#if IMPORT_TEXTURE_CACHE_OPTIONS

		// Create json with import context
		Json::StringBuffer importOptionsMetaBuffer;
		importOptionsMetaBuffer.Reserve(256);
		CompactJsonWriter importOptionsMeta(importOptionsMetaBuffer);
		importOptionsMeta.StartObject();
		{
			context.AddMeta(importOptionsMeta);
			SerializeContext serializeContext(importOptionsMeta);
			options.Serialize(serializeContext);
		}
		importOptionsMeta.EndObject();
		context.Data.Metadata.Copy((const byte*)importOptionsMetaBuffer.GetString(), (uint32)importOptionsMetaBuffer.GetSize());

#endif

		return CreateAssetResult::Ok;
	}

	CreateAssetResult ImportTexture::Create(CreateAssetContext& context, const TextureInitData& textureData, Options& options)
	{
		// Check data
		bool isCubeMap = false;
		if (textureData.ArraySize != 1)
		{
			if (options.IsAtlas)
			{
				LOG_WARNING("Resource", "Cannot import sprite atlas texture that has more than one array slice.");
				return CreateAssetResult::Error;
			}
			if (textureData.ArraySize != 6)
			{
				LOG_WARNING("Resource", "Cannot import texture that has {0} array slices. Use single plane images (single 2D) or cube maps (6 slices).", textureData.ArraySize);
				return CreateAssetResult::Error;
			}
			else
			{
				isCubeMap = true;

				if (textureData.Width != textureData.Height)
				{
					LOG_WARNING("Resource", "Invalid cube texture size.");
					return CreateAssetResult::Error;
				}
			}
		}

		// Base
		if (isCubeMap)
		{
			IMPORT_SETUP(CubeTexture, ASSET_VERSION_CUBETEXTURE);
		}
		else if (options.IsAtlas)
		{
			IMPORT_SETUP(SpriteAtlas, ASSET_VERSION_SPRITEATLAS);
		}
		else
		{
			IMPORT_SETUP(Texture, ASSET_VERSION_TEXTURE);
		}

		// Fill texture header
		TextureHeader textureHeader;
		textureHeader.NeverStream = options.NeverStream;
		textureHeader.Width = textureData.Width;
		textureHeader.Height = textureData.Height;
		textureHeader.Format = textureData.Format;
		textureHeader.Type = options.Type;
		textureHeader.MipLevels = textureData.Mips.Count();
		textureHeader.IsSRGB = PixelFormatIsSRGB(textureHeader.Format);
		textureHeader.IsCubeMap = isCubeMap;
		textureHeader.TextureGroup = options.TextureGroup;
		ENGINE_ASSERT(textureHeader.MipLevels <= GPU_MAX_TEXTURE_MIP_LEVELS);

		// Save header
		context.Data.CustomData.Copy(&textureHeader);

		// Save atlas sprites data
		if (options.IsAtlas)
		{
			/*MemoryWriteStream stream(256);
			stream.WriteInt32(1); // Version
			stream.WriteInt32(options.Sprites.Count()); // Amount of tiles
			for (int32 i = 0; i < options.Sprites.Count(); i++)
			{
				auto& sprite = options.Sprites[i];
				stream.Write(sprite.Area);
				stream.WriteString(sprite.Name, 49);
			}
			if (context.AllocateChunk(15))
				return CreateAssetResult::CannotAllocateChunk;
			context.Data.Header.Chunks[15]->Data.Copy(stream.GetHandle(), stream.GetPosition());*/
		}

		// Save mip maps
		if (!isCubeMap)
		{
			for (int32 mipIndex = 0; mipIndex < textureHeader.MipLevels; mipIndex++)
			{
				auto& mipData = textureData.Mips[mipIndex];

				if (!context.AllocateChunk(mipIndex))
					return CreateAssetResult::CannotAllocateChunk;
				context.Data.Header.Chunks[mipIndex]->Data.Copy(mipData.Data.Get(), static_cast<uint32>(mipData.SlicePitch));
			}
		}
		else
		{
			// Allocate memory for a temporary buffer
			const uint32 imageSize = textureData.Mips[0].SlicePitch * 6;
			MemoryWriteStream imageData(imageSize);

			// Copy cube sides for every mip into separate chunks
			for (int32 mipLevelIndex = 0; mipLevelIndex < textureHeader.MipLevels; mipLevelIndex++)
			{
				// Write array slices to the stream
				imageData.SetPosition(0);
				for (int32 cubeFaceIndex = 0; cubeFaceIndex < 6; cubeFaceIndex++)
				{
					// Get image
					auto& image = textureData.Mips[mipLevelIndex];
					const auto data = image.Data.Get() + image.SlicePitch * cubeFaceIndex;

					// Copy data
					imageData.WriteBytes(data, static_cast<int32>(image.SlicePitch));
				}

				// Copy mip
				if (!context.AllocateChunk(mipLevelIndex))
					return CreateAssetResult::CannotAllocateChunk;
				context.Data.Header.Chunks[mipLevelIndex]->Data.Copy(imageData.GetHandle(), imageData.GetPosition());
			}
		}

#if IMPORT_TEXTURE_CACHE_OPTIONS

		// Create json with import context
		Json::StringBuffer importOptionsMetaBuffer;
		importOptionsMetaBuffer.Reserve(256);
		CompactJsonWriter importOptionsMeta(importOptionsMetaBuffer);
		importOptionsMeta.StartObject();
		{
			context.AddMeta(importOptionsMeta);
			SerializeContext serializeContext(importOptionsMeta);
			options.Serialize(serializeContext);
		}
		importOptionsMeta.EndObject();
		context.Data.Metadata.Copy((const byte*)importOptionsMetaBuffer.GetString(), (uint32)importOptionsMetaBuffer.GetSize());

#endif

		return CreateAssetResult::Ok;
	}

	CreateAssetResult ImportTexture::Import(CreateAssetContext& context)
	{
		Options options;
		InitOptions(context, options);

		// Import
		TextureData textureData;
		String errorMsg;
		if (!TextureUtils::ImportTexture(context.InputPath, textureData, options, errorMsg))
		{
			LOG_ERROR("Resource", "Cannot import texture. {0}", errorMsg);
			return CreateAssetResult::Error;
		}

		return Create(context, textureData, options);
	}

	CreateAssetResult ImportTexture::ImportAsTextureData(CreateAssetContext& context)
	{
		ENGINE_ASSERT(context.CustomArg != nullptr);
		return Create(context, static_cast<TextureData*>(context.CustomArg));
	}

	CreateAssetResult ImportTexture::ImportAsInitData(CreateAssetContext& context)
	{
		ENGINE_ASSERT(context.CustomArg != nullptr);
		return Create(context, static_cast<TextureInitData*>(context.CustomArg));
	}

	CreateAssetResult ImportTexture::Create(CreateAssetContext& context, TextureData* textureData)
	{
		Options options;
		return Create(context, *textureData, options);
	}

	CreateAssetResult ImportTexture::Create(CreateAssetContext& context, TextureInitData* initData)
	{
		Options options;
		return Create(context, *initData, options);
	}

	CreateAssetResult ImportTexture::ImportCube(CreateAssetContext& context)
	{
		ENGINE_ASSERT(context.CustomArg != nullptr);
		return CreateCube(context, static_cast<TextureData*>(context.CustomArg));
	}

	CreateAssetResult ImportTexture::CreateCube(CreateAssetContext& context, TextureData* textureData)
	{
		// Validate
		if (textureData == nullptr)
		{
			LOG_WARNING("Resource", "Missing argument.");
			return CreateAssetResult::Error;
		}
		if (textureData->GetArraySize() != 6)
		{
			LOG_WARNING("Resource", "Invalid cube texture array size.");
			return CreateAssetResult::Error;
		}
		if (textureData->Width != textureData->Height)
		{
			LOG_WARNING("Resource", "Invalid cube texture size.");
			return CreateAssetResult::Error;
		}

		// Base
		// IMPORT_SETUP(CubeTexture, 4);

		// Cache data
		int32 size = textureData->Width;
		PixelFormat format = textureData->Format;
		bool isSRGB = PixelFormatIsSRGB(format);
		int32 mipLevels = textureData->GetMipLevels();

		// Fill texture header
		TextureHeader textureHeader;
		textureHeader.IsSRGB = isSRGB;
		textureHeader.Width = size;
		textureHeader.Height = size;
		textureHeader.IsCubeMap = true;
		textureHeader.NeverStream = true; // TODO: could we support streaming for cube textures?
		textureHeader.Type = TextureFormatType::Unknown;
		textureHeader.Format = format;
		textureHeader.MipLevels = mipLevels;
		ENGINE_ASSERT(textureHeader.MipLevels <= GPU_MAX_TEXTURE_MIP_LEVELS);

		// Log info
		LOG_INFO("Resource",
		"Importing cube texture '{0}': size: {1}, format: {2}, mip levels: {3}, sRGB: {4}",
			context.TargetAssetPath,
			size,
			static_cast<int32>(format),
			static_cast<int32>(textureHeader.MipLevels),
			textureHeader.IsSRGB & 0x1);

		// Save header
		context.Data.CustomData.Copy(&textureHeader);

		// Allocate memory for a temporary buffer
		const uint32 imageSize = textureData->GetData(0, 0)->DepthPitch * 6;
		MemoryWriteStream imageData(imageSize);

		// Copy cube sides for every mip into separate chunks
		for (int32 mipLevelIndex = 0; mipLevelIndex < mipLevels; mipLevelIndex++)
		{
			// Write array slices to the stream
			imageData.SetPosition(0);
			for (int32 cubeFaceIndex = 0; cubeFaceIndex < 6; cubeFaceIndex++)
			{
				// Get image
				auto image = textureData->GetData(cubeFaceIndex, mipLevelIndex);
				if (image == nullptr)
				{
					LOG_WARNING("Resource", "Cannot create cube texture '{0}'. Missing image slice.", context.InputPath);
					return CreateAssetResult::Error;
				}
				ENGINE_ASSERT(image->DepthPitch < Max_int32);

				// Copy data
				imageData.WriteBytes(image->Data.Get(), image->Data.Length());
			}

			// Copy mip
			if (!context.AllocateChunk(mipLevelIndex))
				return CreateAssetResult::CannotAllocateChunk;
			context.Data.Header.Chunks[mipLevelIndex]->Data.Copy(imageData.GetHandle(), imageData.GetPosition());
		}

		return CreateAssetResult::Ok;
	}

}

#include "TextureBase.h"
#include "TextureData.h"

#include "Core/Math/Color.h"
#include "Core/Profiler/ProfilerCPU.h"
#include "Core/Thread/Threading.h"
#include "Core/TypeSystem/Types.h"
#include "Core/Logging/Exceptions/ExceptionInclude.h"
#include "Core/Math/Vector2.h"
#include "Core/Platform/FileSystem.h"

#include "Runtime/Utilities/Texture/TextureUtils.h"
#include "Runtime/Graphics/Base/GPUUtils.h"
#include "Runtime/Graphics/Textures/GPUTexture.h"


namespace SE
{
	TextureBase::TextureBase() : BinaryAsset(), _texture(this), _customData(nullptr), _parent(this)
	{
	}

	TextureBase::TextureBase(const AssetInfo* info) : BinaryAsset(info),
			_texture(this, FileSystem::GetFileName(info->path)), _customData(nullptr), _parent(this)
	{

	}

	Float2 TextureBase::Size() const
	{
		return Float2(static_cast<float>(_texture.TotalWidth()), static_cast<float>(_texture.TotalHeight()));
	}

	int32 TextureBase::GetArraySize() const
	{
		return _texture->ArraySize();
	}

	int32 TextureBase::GetMipLevels() const
	{
		return _texture->MipLevels();
	}

	int32 TextureBase::GetResidentMipLevels() const
	{
		return _texture->ResidentMipLevels();
	}

	uint64 TextureBase::GetCurrentMemoryUsage() const
	{
		return _texture->GetMemoryUsage();
	}

	int32 TextureBase::GetTextureGroup() const
	{
		return _texture._header.TextureGroup;
	}

	void TextureBase::SetTextureGroup(int32 textureGroup)
	{
		if (_texture._header.TextureGroup != textureGroup)
		{
			_texture._header.TextureGroup = textureGroup;
			_texture.RequestStreamingUpdate();
		}
	}


	BytesContainer TextureBase::GetMipData(int32 mipIndex, int32& rowPitch, int32& slicePitch)
	{
		BytesContainer result;
		if (IsVirtual())
		{
			if (_customData == nullptr)
			{
				LOG_ERROR("Render", "Missing virtual texture init data.");
				return result;
			}

			// Get description
			rowPitch = _customData->Mips[mipIndex].RowPitch;
			slicePitch = _customData->Mips[mipIndex].SlicePitch;
		}
		else
		{
			if (WaitForLoaded())
				return result;

			// Get description
			uint32 rowPitch1, slicePitch1;
			const int32 mipWidth = Math::Max(1, Width() >> mipIndex);
			const int32 mipHeight = Math::Max(1, Height() >> mipIndex);
			GPUUtils::ComputePitch(Format(), mipWidth, mipHeight, rowPitch1, slicePitch1);
			rowPitch = rowPitch1;
			slicePitch = slicePitch1;

			// Ensure to have chunk loaded
			if (LoadChunk(CalculateChunkIndex(mipIndex)))
				return result;
		}

		// Get data
		GetMipData(mipIndex, result);
		return result;
	}

	bool TextureBase::GetTextureData(TextureData& result, bool copyData)
	{
		PROFILE_CPU_NAMED("Texture.GetTextureData");
		if (!IsVirtual() && WaitForLoaded())
		{
			LOG_ERROR("Render", "Asset load failed.");
			return true;
		}
		auto dataLock = LockData();

		// Setup description
		result.Width = _texture.TotalWidth();
		result.Height = _texture.TotalHeight();
		result.Depth = 1;
		result.Format = _texture.GetHeader()->Format;
		result.Items.Resize(_texture.TotalArraySize());

		// Setup mips data
		for (int32 arraySlice = 0; arraySlice < result.Items.Count(); arraySlice++)
		{
			auto& slice = result.Items[arraySlice];
			slice.Mips.Resize(_texture.TotalMipLevels());
			for (int32 mipIndex = 0; mipIndex < slice.Mips.Count(); mipIndex++)
			{
				auto& mip = slice.Mips[mipIndex];
				int32 rowPitch, slicePitch;
				BytesContainer mipData = GetMipData(mipIndex, rowPitch, slicePitch);
				if (mipData.IsInvalid())
				{
					LOG_ERROR("Render", "Failed to get texture mip data.");
					return true;
				}
				if (mipData.Length() != slicePitch * _texture.TotalArraySize())
				{
					LOG_ERROR("Render", "Invalid custom texture data (slice pitch * array size is different than data bytes count).");
					return true;
				}
				mip.RowPitch = rowPitch;
				mip.DepthPitch = slicePitch;
				mip.Lines = Math::Max(1, Height() >> mipIndex);
				if (copyData)
					mip.Data.Copy(mipData.Get() + (arraySlice * slicePitch), slicePitch);
				else
					mip.Data.Link(mipData.Get() + (arraySlice * slicePitch), slicePitch);
			}
		}

		return false;
	}

	bool TextureBase::GetTextureMipData(TextureMipData& result, int32 mipIndex, int32 arrayIndex, bool copyData)
	{
		PROFILE_CPU_NAMED("Texture.GetTextureMipData");
		if (!IsVirtual() && WaitForLoaded())
		{
			LOG_ERROR("Render", "Asset load failed.");
			return true;
		}
		if (mipIndex < 0 || mipIndex > GetMipLevels() || arrayIndex < 0 || arrayIndex > GetArraySize())
		{
			Log::ArgumentOutOfRangeException();
			return true;
		}

		// Get raw texture data
		int32 rowPitch, slicePitch;
		BytesContainer mipData = GetMipData(mipIndex, rowPitch, slicePitch);
		if (mipData.IsInvalid())
		{
			LOG_ERROR("Render", "Failed to get texture mip data.");
			return true;
		}
		if (mipData.Length() != slicePitch)
		{
			LOG_ERROR("Render", "Invalid custom texture data (slice pitch * array size is different than data bytes count).");
			return true;
		}

		// Fill result
		result.RowPitch = rowPitch;
		result.DepthPitch = slicePitch;
		result.Lines = Math::Max(1, Height() >> mipIndex);
		if (copyData)
			result.Data.Copy(mipData.Get() + (arrayIndex * slicePitch), slicePitch);
		else
			result.Data.Link(mipData.Get() + (arrayIndex * slicePitch), slicePitch);
		return false;
	}

	bool TextureBase::GetPixels(List<Color32>& pixels, int32 mipIndex, int32 arrayIndex)
	{
		PROFILE_CPU_NAMED("Texture.GetPixels");
		Threading::ScopeLock lock(Locker);

		// Get mip data
//		auto dataLock = LockData();
		TextureMipData mipData;
		if (GetTextureMipData(mipData, mipIndex, arrayIndex, false))
			return true;
		const int32 mipWidth = Math::Max(1, Width() >> mipIndex);
		const int32 mipHeight = Math::Max(1, Height() >> mipIndex);

		// Convert into pixels
		return mipData.GetPixels(pixels, mipWidth, mipHeight, Format());
	}

	bool TextureBase::GetPixels(List<Color>& pixels, int32 mipIndex, int32 arrayIndex)
	{
		PROFILE_CPU_NAMED("Texture.GetPixels");

		// Get mip data
//		auto dataLock = LockData();
		TextureMipData mipData;
		if (!GetTextureMipData(mipData, mipIndex, arrayIndex, false))
			return false;
		const int32 mipWidth = Math::Max(1, Width() >> mipIndex);
		const int32 mipHeight = Math::Max(1, Height() >> mipIndex);

		// Convert into pixels
		return mipData.GetPixels(pixels, mipWidth, mipHeight, Format());
	}

	bool TextureBase::SetPixels(const Span<Color32>& pixels, int32 mipIndex, int32 arrayIndex, bool generateMips)
	{
		PROFILE_CPU_NAMED("Texture.SetPixels");
		if (!IsVirtual())
		{
			LOG_ERROR("Render", "Texture must be virtual.");
			return false;
		}
		Threading::ScopeLock lock(Locker);
		if (_customData == nullptr || Width() == 0)
		{
			LOG_ERROR("Render", "Texture must be initialized.");
			return false;
		}
		const PixelFormat format = Format();
		const int32 width = Math::Max(1, Width() >> mipIndex);
		const int32 height = Math::Max(1, Height() >> mipIndex);
		auto& mipData = _customData->Mips[mipIndex];
		const int32 rowPitch = mipData.RowPitch;
		const int32 sliceSize = mipData.SlicePitch;
		if (pixels.Length() != width * height)
		{
			Log::ArgumentOutOfRangeException();
			return true;
		}

		// Convert pixels to the texture format
		ENGINE_ASSERT(mipData.Data.IsAllocated());
		byte* dst = mipData.Data.Get() + sliceSize * arrayIndex;
		bool error = true;
		switch (format)
		{
		case PixelFormat::R8G8B8A8_SInt:
		case PixelFormat::R8G8B8A8_SNorm:
		case PixelFormat::R8G8B8A8_UInt:
		case PixelFormat::R8G8B8A8_UNorm:
		case PixelFormat::R8G8B8A8_UNorm_SRGB:
		case PixelFormat::B8G8R8A8_UNorm:
		case PixelFormat::B8G8R8A8_UNorm_SRGB:
			if (rowPitch == width * sizeof(Color32))
			{
				Platform::MemoryCopy(dst, pixels.Get(), sliceSize);
				error = false;
			}
			break;
		}
		if (error)
		{
			// Try to use texture sampler utility
			auto sampler = TextureUtils::GetSampler(format);
			if (sampler)
			{
				for (int32 y = 0; y < height; y++)
				{
					for (int32 x = 0; x < width; x++)
					{
						Color c(pixels.Get()[x + y * width]);
						TextureUtils::Store(sampler, x, y, dst, rowPitch, c);
					}
				}
				error = false;
			}
		}
		if (error)
		{
			LOG_ERROR("Render", "Unsupported texture data format {0}.", Types::GetEnumString(format));
			return false;
		}

		// Generate mips optionally
		if (generateMips && mipIndex + 1 < _customData->Mips.Count())
		{
			for (int32 i = mipIndex + 1; i < _customData->Mips.Count(); i++)
				_customData->GenerateMip(i);
		}

		// Request texture data streaming to GPU
		_texture->SetResidentMipLevels(0);
		//	_texture.RequestStreamingUpdate();

		return true;
	}

	bool TextureBase::SetPixels(const Span<Color>& pixels, int32 mipIndex, int32 arrayIndex, bool generateMips)
	{
		PROFILE_CPU_NAMED("Texture.SetPixels");
		if (!IsVirtual())
		{
			LOG_ERROR("Render", "Texture must be virtual.");
			return false;
		}
		Threading::ScopeLock lock(Locker);
		if (_customData == nullptr || Width() == 0)
		{
			LOG_ERROR("Render", "Texture must be initialized.");
			return false;
		}
		const PixelFormat format = Format();
		const int32 width = Math::Max(1, Width() >> mipIndex);
		const int32 height = Math::Max(1, Height() >> mipIndex);
		auto& mipData = _customData->Mips[mipIndex];
		const int32 rowPitch = mipData.RowPitch;
		const int32 sliceSize = mipData.SlicePitch;
		if (pixels.Length() != width * height)
		{
			Log::ArgumentOutOfRangeException();
			return false;
		}

		// Convert pixels to the texture format
		ASSERT(mipData.Data.IsAllocated());
		byte* dst = mipData.Data.Get() + sliceSize * arrayIndex;
		bool error = true;
		switch (format)
		{
		case PixelFormat::R32G32B32A32_Float:
			if (rowPitch == width * sizeof(Color))
			{
				Platform::MemoryCopy(dst, pixels.Get(), sliceSize);
				error = false;
			}
			break;
		}
		if (error)
		{
			// Try to use texture sampler utility
			auto sampler = TextureUtils::GetSampler(format);
			if (sampler)
			{
				for (int32 y = 0; y < height; y++)
				{
					for (int32 x = 0; x < width; x++)
					{
						Color c(pixels.Get()[x + y * width]);
						TextureUtils::Store(sampler, x, y, dst, rowPitch, c);
					}
				}
				error = false;
			}
		}
		if (error)
		{
			LOG_ERROR("Render", "Unsupported texture data format {0}.", Types::GetEnumString(format));
			return false;
		}

		// Generate mips optionally
		if (generateMips && mipIndex + 1 < _customData->Mips.Count())
		{
			for (int32 i = mipIndex + 1; i < _customData->Mips.Count(); i++)
				_customData->GenerateMip(i);
		}

		// Request texture data streaming to GPU
		_texture->SetResidentMipLevels(0);
		//	_texture.RequestStreamingUpdate();

		return true;
	}

	bool TextureBase::Init(TextureInitData* initData)
	{
		// Validate state
		if (!IsVirtual())
		{
			LOG_ERROR("Render", "Texture must be virtual.");
			Delete(initData);
			return false;
		}
		if (initData->Format == PixelFormat::Undefined ||
			Math::RangeExclusive(initData->Width, 1, GPU_MAX_TEXTURE_SIZE) ||
			Math::RangeExclusive(initData->Height, 1, GPU_MAX_TEXTURE_SIZE) ||
			Math::RangeExclusive(initData->ArraySize, 1, GPU_MAX_TEXTURE_ARRAY_SIZE) ||
			Math::RangeExclusive(initData->Mips.Count(), 1, GPU_MAX_TEXTURE_MIP_LEVELS))
		{
			Log::ArgumentOutOfRangeException();
			Delete(initData);
			return false;
		}

		Threading::ScopeLock lock(Locker);

		// Release texture
		_texture.UnloadTexture();

		// Prepare descriptor
		if (_customData != nullptr)
			Delete(_customData);
		_customData = initData;

		// Create texture
		TextureHeader textureHeader;
		textureHeader.Format = _customData->Format;
		textureHeader.Width = _customData->Width;
		textureHeader.Height = _customData->Height;
		textureHeader.IsCubeMap = _customData->ArraySize == 6;
		textureHeader.MipLevels = Math::Max(1, _customData->Mips.Count());
		textureHeader.Type = TextureFormatType::ColorRGBA;
		textureHeader.NeverStream = true;
		if (!_texture.Create(textureHeader))
		{
			LOG_WARNING("Renderer", "Cannot initialize texture.");
			return false;
		}

		return true;
	}

/*#if !COMPILE_WITHOUT_CSHARP

	bool TextureBase::InitCSharp(void* ptr)
	{
		PROFILE_CPU_NAMED("Texture.Init");
		struct InternalInitData
		{
			PixelFormat Format;
			int32 Width;
			int32 Height;
			int32 ArraySize;
			int32 MipLevels;
			int32 GenerateMips;
			int32 DataRowPitch[14];
			int32 DataSlicePitch[14];
			byte* Data[14];
		};
		auto initDataObj = (InternalInitData*)ptr;
		auto initData = New<InitData>();

		initData->Format = initDataObj->Format;
		initData->Width = initDataObj->Width;
		initData->Height = initDataObj->Height;
		initData->ArraySize = initDataObj->ArraySize;
		initData->Mips.Resize(initDataObj->GenerateMips ? MipLevelsCount(initDataObj->Width, initDataObj->Height) : initDataObj->MipLevels);

		// Copy source mips data
		for (int32 mipIndex = 0; mipIndex < initDataObj->MipLevels; mipIndex++)
		{
			auto& mip = initData->Mips[mipIndex];
			mip.RowPitch = initDataObj->DataRowPitch[mipIndex];
			mip.SlicePitch = initDataObj->DataSlicePitch[mipIndex];
			mip.Data.Copy(initDataObj->Data[mipIndex], mip.SlicePitch * initData->ArraySize);
		}

		// Generate mips
		for (int32 mipIndex = initDataObj->MipLevels; mipIndex < initData->Mips.Count(); mipIndex++)
		{
			initData->GenerateMip(mipIndex, initDataObj->GenerateMips & 2);
		}

		return Init(initData);
	}

#endif*/

	uint64 TextureBase::GetMemoryUsage() const
	{
		Locker.Lock();
		uint64 result = BinaryAsset::GetMemoryUsage();
		result += sizeof(TextureBase) - sizeof(BinaryAsset);
		if (_customData)
		{
			result += sizeof(TextureInitData);
			for (auto& mip : _customData->Mips)
				result += mip.Data.Length();
		}
		Locker.Unlock();
		return result;
	}

	void TextureBase::CancelStreaming()
	{
		Asset::CancelStreaming();
		_texture.CancelStreamingTasks();
	}

	int32 TextureBase::CalculateChunkIndex(int32 mipIndex) const
	{
		// Mips are in 0-13 chunks
		return mipIndex;
	}

	CriticalSection& TextureBase::GetOwnerLocker() const
	{
		return _parent->Locker;
	}

	void TextureBase::Unload(bool isReloading)
	{
		if (!isReloading)
		{
			// Release texture
			_texture->ReleaseGPU();
			Delete(_customData);
		}
	}

	Threading::Task* TextureBase::RequestMipDataAsync(int32 mipIndex)
	{
		if (_customData)
			return nullptr;

		auto chunkIndex = CalculateChunkIndex(mipIndex);
		return (Threading::Task*)_parent->RequestChunkDataAsync(chunkIndex);
	}

	Storage::LockData TextureBase::LockData()
	{
		return _parent->storage ? _parent->storage->Lock() : Storage::LockData::Invalid;
	}

	void TextureBase::GetMipData(int32 mipIndex, BytesContainer& data) const
	{
		if (_customData)
		{
			data.Link(_customData->Mips[mipIndex].Data);
			return;
		}

		auto chunkIndex = CalculateChunkIndex(mipIndex);
		_parent->GetChunkData(chunkIndex, data);
	}

	void TextureBase::GetMipDataWithLoading(int32 mipIndex, BytesContainer& data) const
	{
		if (_customData)
		{
			data.Link(_customData->Mips[mipIndex].Data);
			return;
		}

		const auto chunkIndex = CalculateChunkIndex(mipIndex);
		_parent->LoadChunk(chunkIndex);
		_parent->GetChunkData(chunkIndex, data);
	}

	bool TextureBase::GetMipDataCustomPitch(int32 mipIndex, uint32& rowPitch, uint32& slicePitch) const
	{
		bool result = _customData != nullptr;
		if (result)
		{
			rowPitch = _customData->Mips[mipIndex].RowPitch;
			slicePitch = _customData->Mips[mipIndex].SlicePitch;
		}

		return
			result;
	}

	bool TextureBase::OnInit(AssetInitData& initData)
	{
		if (IsVirtual())
			return true;

		if (initData.SerializedVersion != GetSerializedVersion())
		{
			LOG_ERROR("Renderer", "Invalid serialized texture version.");
			return false;
		}

		// Get texture header for asset custom data (fast access)
		TextureHeader textureHeader;
		if (initData.CustomData.Length() == sizeof(TextureHeader))
		{
			Platform::MemoryCopy(&textureHeader, initData.CustomData.Get(), sizeof(textureHeader));
		}
		else
		{
			LOG_ERROR("Renderer", "Missing texture header.");
			return false;
		}

		return _texture.Create(textureHeader);
	}

	Asset::LoadResult TextureBase::load()
	{
		// Loading textures is very fast xD
		return LoadResult::Ok;
	}

	TextureInitData::MipData::MipData(MipData&& other) noexcept
		: Data(MoveTemp(other.Data)), RowPitch(other.RowPitch), SlicePitch(other.SlicePitch)
	{
	}

	TextureInitData::TextureInitData(TextureInitData&& other) noexcept
		: Format(other.Format), Width(other.Width),
		Height(other.Height), ArraySize(other.ArraySize), Mips(MoveTemp(other.Mips))
	{
	}

	bool TextureInitData::GenerateMip(int32 mipIndex, bool linear)
	{
		// Validate input
		if (mipIndex < 1 || mipIndex >= Mips.Count())
		{
			LOG_WARNING("Renderer", "Invalid mip map to generate.");
			return false;
		}
		if (ArraySize < 1)
		{
			LOG_WARNING("Renderer", "Invalid array size.");
			return false;
		}
		if (PixelFormatIsCompressed(Format))
		{
			LOG_WARNING("Renderer", "Cannot generate mip map for compressed format data.");
			return false;
		}
		const auto& srcMip = Mips[mipIndex - 1];
		auto& dstMip = Mips[mipIndex];
		if (srcMip.RowPitch == 0 || srcMip.SlicePitch == 0 || srcMip.Data.IsInvalid())
		{
			LOG_WARNING("Renderer", "Missing data for source mip map.");
			return false;
		}

		PROFILE_CPU_NAMED("Texture.GenerateMip");

		// Allocate data
		const int32 dstMipWidth = Math::Max(1, Width >> mipIndex);
		const int32 dstMipHeight = Math::Max(1, Height >> mipIndex);
		const int32 pixelStride = PixelFormatGetSizeInBytes(Format);
		dstMip.RowPitch = dstMipWidth * pixelStride;
		dstMip.SlicePitch = dstMip.RowPitch * dstMipHeight;
		dstMip.Data.Allocate(dstMip.SlicePitch * ArraySize);

		// Perform filtering
		if (linear)
		{
			switch (Format)
			{
			// 4 component, 32 bit with 8 bits per component - use Color32 type
			case PixelFormat::R8G8B8A8_SInt:
			case PixelFormat::R8G8B8A8_SNorm:
			case PixelFormat::R8G8B8A8_UInt:
			case PixelFormat::R8G8B8A8_UNorm:
			case PixelFormat::R8G8B8A8_UNorm_SRGB:
			case PixelFormat::B8G8R8A8_UNorm:
			case PixelFormat::B8G8R8A8_UNorm_SRGB:
			{
				// Linear downscale filter
				for (int32 arrayIndex = 0; arrayIndex < ArraySize; arrayIndex++)
				{
					const byte* dstData = dstMip.Data.Get() + arrayIndex * dstMip.SlicePitch;
					const byte* srcData = srcMip.Data.Get() + arrayIndex * srcMip.SlicePitch;
					for (int32 y = 0; y < dstMipHeight; y++)
					{
						for (int32 x = 0; x < dstMipWidth;x++)
						{
							const auto dstIndex = y * dstMip.RowPitch + x * pixelStride;

#define SAMPLE(var, dx, dy) const auto var = Color(*(Color32*)(srcData + ((y * 2 + (dy)) * srcMip.RowPitch + x * pixelStride * 2 + (pixelStride * (dx)))))
							SAMPLE(v00, 0, 0);
							SAMPLE(v01, 0, 1);
							SAMPLE(v10, 1, 0);
							SAMPLE(v11, 1, 1);
#undef SAMPLE

							*(Color32*)(dstData + dstIndex) = Color32((v00 + v01 + v10 + v11) * 0.25f);
						}
					}
				}
				break;
			}
			default:
				LOG_ERROR("Renderer", "Unsupported texture data format {0}.", Types::GetEnumString(Format));
				return false;
			}
		}
		else
		{
			// Point downscale filter
			for (int32 arrayIndex = 0; arrayIndex < ArraySize; arrayIndex++)
			{
				byte* dstData = dstMip.Data.Get() + arrayIndex * dstMip.SlicePitch;
				const byte* srcData = srcMip.Data.Get() + arrayIndex * srcMip.SlicePitch;
				for (int32 y = 0; y < dstMipHeight; y++)
				{
					for (int32 x = 0; x < dstMipWidth; x++)
					{
						const auto dstIndex = y * dstMip.RowPitch + x * pixelStride;
						const auto srcIndex = y * 2 * srcMip.RowPitch + x * pixelStride * 2;

						Platform::MemoryCopy(dstData + dstIndex, srcData + srcIndex, pixelStride);
					}
				}
			}

			// Fix right and bottom edges to preserve the original values
			for (int32 arrayIndex = 0; arrayIndex < ArraySize; arrayIndex++)
			{
				byte* dstData = dstMip.Data.Get() + arrayIndex * dstMip.SlicePitch;
				const byte* srcData = srcMip.Data.Get() + arrayIndex * srcMip.SlicePitch;
				for (int32 y = 0; y < dstMipHeight - 1; y++)
				{
					const int32 x = dstMipWidth - 1;
					const auto dstIndex = y * dstMip.RowPitch + x * pixelStride;
					const auto srcIndex = y * 2 * srcMip.RowPitch + x * pixelStride * 2 + pixelStride;

					Platform::MemoryCopy(dstData + dstIndex, srcData + srcIndex, pixelStride);
				}
				for (int32 x = 0; x < dstMipWidth - 1; x++)
				{
					const int32 y = dstMipHeight - 1;
					const auto dstIndex = y * dstMip.RowPitch + x * pixelStride;
					const auto srcIndex = (y * 2 + 1) * srcMip.RowPitch + x * pixelStride * 2;

					Platform::MemoryCopy(dstData + dstIndex, srcData + srcIndex, pixelStride);
				}
				{
					const int32 x = dstMipWidth - 1;
					const int32 y = dstMipHeight - 1;
					const auto dstIndex = y * dstMip.RowPitch + x * pixelStride;
					const auto srcIndex = (y * 2 + 1) * srcMip.RowPitch + x * pixelStride * 2 + pixelStride;

					Platform::MemoryCopy(dstData + dstIndex, srcData + srcIndex, pixelStride);
				}
			}
		}

		return true;
	}

	void TextureInitData::FromTextureData(const TextureData& textureData, bool generateMips)
	{
		Format = textureData.Format;
		Width = textureData.Width;
		Height = textureData.Height;
		ArraySize = textureData.GetArraySize();
		if (generateMips)
			Mips.Resize(GPUUtils::MipLevelsCount(textureData.Width, textureData.Height));
		else
			Mips.Resize(textureData.GetMipLevels());

		for (int32 mipIndex = 0; mipIndex < textureData.GetMipLevels(); mipIndex++)
		{
			auto& mip = Mips[mipIndex];
			auto& data = *textureData.GetData(0, mipIndex);
			mip.Data.Allocate(data.Data.Length() * ArraySize);
			mip.RowPitch = data.RowPitch;
			mip.SlicePitch = data.Data.Length();

			byte* dst = mip.Data.Get();
			for (int32 arrayIndex = 0; arrayIndex < ArraySize; arrayIndex++)
			{
				auto& d = *textureData.GetData(arrayIndex, mipIndex);
				ASSERT(data.RowPitch == d.RowPitch);
				ASSERT(data.Data.Length() == d.Data.Length());
				Platform::MemoryCopy(dst, d.Data.Get(), d.Data.Length());
				dst += data.Data.Length();
				ASSERT((int32)(dst - mip.Data.Get()) <= mip.Data.Length());
			}
		}

		if (generateMips)
		{
			for (int32 mipIndex = textureData.GetMipLevels(); mipIndex < Mips.Count(); mipIndex++)
			{
				GenerateMip(mipIndex, true);
			}
		}
	}

	TextureInitData* TextureInitData::Create(int32 width, int32 height, int32 arraySize, PixelFormat format)
	{
		TextureInitData* data = New<TextureInitData>();
		data->Width = width;
		data->Height = height;
		data->ArraySize = arraySize,
		data->Format = format;

		MipData& mipData = data->Mips.AddOne();
		GPUUtils::ComputePitch(format, width, height, mipData.RowPitch, mipData.SlicePitch);
		mipData.Data.Allocate(mipData.SlicePitch * arraySize);
		Platform::MemoryClear(mipData.Data.Get(), sizeof(byte) * mipData.Data.Length());

		return data;
	}
}

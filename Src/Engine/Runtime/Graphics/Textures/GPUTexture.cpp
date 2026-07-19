
#include "GPUTexture.h"
#include "GPUTextureDescription.h"

#include "Runtime/Core/Math/Color.h"
#include "Runtime/Core/Math/Math.h"
#include "Runtime/Core/Math/Vector2.h"
#include "Runtime/Core/Math/Vector3.h"
#include "Runtime/Core/Profiler/Profiler.h"
#include "Runtime/Core/Thread/ThreadPool.h"
#include "Runtime/Core/Types/BitFlagsUtility.h"

#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/Async/Tasks/GPUCopyResourceTask.h"
#include "Runtime/Graphics/Async/Tasks/GPUUploadTextureMipTask.h"
#include "Runtime/Graphics/Base/GPUUtils.h"

namespace SE
{
	class GPUUploadTextureMipTask;
	class TextureDownloadDataTask;

	namespace
	{
		int32 CalculateMipMapCount(int32 requestedLevel, int32 width)
		{
			//int32 size = width;
			//int32 maxMipMap = 1 + Math::CeilToInt(Math::Log(size) / Math::Log(2.0));
			//return requestedLevel == 0 ? maxMipMap : Math::Min(requestedLevel, maxMipMap);
			if (requestedLevel == 0)
				requestedLevel = GPU_MAX_TEXTURE_MIP_LEVELS;
			int32 maxMipMap = 1;
			while (width > 1)
			{
				width >>= 1;
				maxMipMap++;
			}
			return Math::Min(requestedLevel, maxMipMap);
		}
	}

#pragma region GPUTextureDescription

	const String ToString(TextureDimensions value)
	{
		String result;
		switch (value)
		{
		case TextureDimensions::Texture:
			result = SE_TEXT("Texture");
			break;
		case TextureDimensions::VolumeTexture:
			result = SE_TEXT("VolumeTexture");
			break;
		case TextureDimensions::CubeTexture:
			result = SE_TEXT("CubeTexture");
			break;
		default:
			result = SE_TEXT("");
		}
		return result;
	}

	GPUTextureDescription GPUTextureDescription::New1D(int32 width,
		PixelFormat format,
		GPUTextureBitFlags textureFlags,
		int32 mipCount,
		int32 arraySize)
	{
		GPUTextureDescription desc;
		desc.Dimensions = TextureDimensions::Texture;
		desc.Width = width;
		desc.Height = 1;
		desc.Depth = 1;
		desc.ArraySize = arraySize;
		desc.MipLevels = CalculateMipMapCount(mipCount, width);
		desc.Format = format;
		desc.MultiSampleLevel = MSAALevel::None;
		desc.Flags = textureFlags;
		desc.Usage = GPUResourceUsage::Default;
		desc.DefaultClearColor = Colors::Black;
		return desc;
	}

	GPUTextureDescription GPUTextureDescription::New2D(int32 width,
		int32 height,
		PixelFormat format,
		GPUTextureBitFlags textureFlags,
		int32 mipCount,
		int32 arraySize,
		MSAALevel msaaLevel)
	{
		GPUTextureDescription desc;
		desc.Dimensions = TextureDimensions::Texture;
		desc.Width = width;
		desc.Height = height;
		desc.Depth = 1;
		desc.ArraySize = arraySize;
		desc.MipLevels = CalculateMipMapCount(mipCount, Math::Max(width, height));
		desc.Format = format;
		desc.MultiSampleLevel = msaaLevel;
		desc.Flags = textureFlags;
		desc.DefaultClearColor = Colors::Black;
		desc.Usage = GPUResourceUsage::Default;
		return desc;
	}

	GPUTextureDescription GPUTextureDescription::New3D(const Float3& size,
		PixelFormat format,
		GPUTextureBitFlags textureFlags)
	{
		return New3D((int32)size.x, (int32)size.y, (int32)size.z, 1, format, textureFlags);
	}

	GPUTextureDescription GPUTextureDescription::New3D(int32 width,
		int32 height,
		int32 depth,
		PixelFormat format,
		GPUTextureBitFlags textureFlags,
		int32 mipCount)
	{
		GPUTextureDescription desc;
		desc.Dimensions = TextureDimensions::VolumeTexture;
		desc.Width = width;
		desc.Height = height;
		desc.Depth = depth;
		desc.ArraySize = 1;
		desc.MipLevels = CalculateMipMapCount(mipCount, Math::Max(width, height, depth));
		desc.Format = format;
		desc.MultiSampleLevel = MSAALevel::None;
		desc.Flags = textureFlags;
		desc.DefaultClearColor = Colors::Black;
		desc.Usage = GPUResourceUsage::Default;
		return desc;
	}

	GPUTextureDescription GPUTextureDescription::NewCube(int32 size,
		PixelFormat format,
		GPUTextureBitFlags textureFlags,
		int32 mipCount)
	{
		auto desc = New2D(size, size, format, textureFlags, mipCount, 6, MSAALevel::None);
		desc.Dimensions = TextureDimensions::CubeTexture;
		return desc;
	}

	void GPUTextureDescription::Clear()
	{
		Platform::MemoryClear(this, sizeof(GPUTextureDescription));
		MultiSampleLevel = MSAALevel::None;
	}

	GPUTextureDescription GPUTextureDescription::ToStagingUpload() const
	{
		auto copy = *this;
		copy.Flags = GPUTextureFlags::None;
		copy.Usage = GPUResourceUsage::StagingUpload;
		return copy;
	}

	GPUTextureDescription GPUTextureDescription::ToStagingReadback() const
	{
		auto copy = *this;
		copy.Flags = GPUTextureFlags::None;
		copy.Usage = GPUResourceUsage::StagingReadback;
		return copy;
	}

	bool GPUTextureDescription::Equals(const GPUTextureDescription& other) const
	{
		return Dimensions == other.Dimensions
			&& Width == other.Width
			&& Height == other.Height
			&& Depth == other.Depth
			&& ArraySize == other.ArraySize
			&& MipLevels == other.MipLevels
			&& Format == other.Format
			&& MultiSampleLevel == other.MultiSampleLevel
			&& Flags == other.Flags
			&& Usage == other.Usage
			&& DefaultClearColor == other.DefaultClearColor;
	}

	String GPUTextureDescription::ToString() const
	{
		return String::Format(SE_TEXT(
				"Size: {0}x{1}x{2}[{3}], Type: {4}, Mips: {5}, CharsFormat: {6}, MSAA: {7}, Flags: {8}, Usage: {9}"),
			Width,
			Height,
			Depth,
			ArraySize,
			Types::GetEnumString<TextureDimensions>(Dimensions),
			MipLevels,
			Types::GetEnumString<PixelFormat>(Format),
			Types::GetEnumString<MSAALevel>(MultiSampleLevel),
			TBFlagsUtility::ToString(Flags),
			(int32)Usage);
	}

	uint32 GetHash(const GPUTextureDescription& key)
	{
		uint32 hashCode = key.Width;
		hashCode = HashCombine(hashCode, key.Height);
		hashCode = HashCombine(hashCode, key.Depth);
		hashCode = HashCombine(hashCode, key.ArraySize);
		hashCode = HashCombine(hashCode, (uint32)key.Dimensions);
		hashCode = HashCombine(hashCode, key.MipLevels);
		hashCode = HashCombine(hashCode, (uint32)key.Format);
		hashCode = HashCombine(hashCode, (uint32)key.MultiSampleLevel);
		hashCode = HashCombine(hashCode, (uint32)key.Flags.Get());
		hashCode = HashCombine(hashCode, (uint32)key.Usage);
		hashCode = HashCombine(hashCode, GetHash(key.DefaultClearColor));
		return hashCode;
	}

#pragma endregion

#pragma region GPUTexture

	void GPUTextureView::Init(GPUResource* parent, PixelFormat format, MSAALevel msaa)
	{
		m_Parent = parent;
		m_Format = format;
		m_Msaa = msaa;
		if (parent)
		{
			LastRenderTime = &parent->lastRenderTime;
		}
	}

	GPUTexture* GPUTexture::Spawn(const SpawnParams& params)
	{
		return GPUDevice::instance->CreateTexture();
	}

	GPUTexture* GPUTexture::New()
	{
		return GPUDevice::instance->CreateTexture();
	}

	GPUTexture::GPUTexture()
		: GPUResource(SpawnParams(UID::New(), TypeInitializer))
		, m_ResidentMipLevels(0)
		, m_SRGB(false)
		, m_IsBlockCompressed(false)
	{
		// Keep description data clear (we use _desc.MipLevels to check if it's has been initated)
		m_Desc.Clear();
	}

	Float2 GPUTexture::Size() const
	{
		return Float2(static_cast<float>(m_Desc.Width), static_cast<float>(m_Desc.Height));
	}

	Float3 GPUTexture::Size3() const
	{
		return Float3(static_cast<float>(m_Desc.Width),
			static_cast<float>(m_Desc.Height),
			static_cast<float>(m_Desc.Depth));
	}

	bool GPUTexture::IsPowerOfTwo() const
	{
		return Math::IsPowerOf2(m_Desc.Width) && Math::IsPowerOf2(m_Desc.Height);
	}

	void GPUTexture::GetMipSize(int32 mipLevelIndex, int32& mipWidth, int32& mipHeight) const
	{
		ENGINE_ASSERT(mipLevelIndex >= 0 && mipLevelIndex < MipLevels());
		mipWidth = Math::Max(1, Width() >> mipLevelIndex);
		mipHeight = Math::Max(1, Height() >> mipLevelIndex);
	}

	void GPUTexture::GetMipSize(int32 mipLevelIndex, int32& mipWidth, int32& mipHeight, int32& mipDepth) const
	{
		ENGINE_ASSERT(mipLevelIndex >= 0 && mipLevelIndex < MipLevels());
		mipWidth = Math::Max(1, Width() >> mipLevelIndex);
		mipHeight = Math::Max(1, Height() >> mipLevelIndex);
		mipDepth = Math::Max(1, Depth() >> mipLevelIndex);
	}

	void GPUTexture::GetResidentSize(int32& width, int32& height) const
	{
		// Check if texture isn't loaded
		if (m_ResidentMipLevels <= 0)
		{
			width = height = 0;
			return;
		}

		const int32 mipIndex = m_Desc.MipLevels - m_ResidentMipLevels;
		width = Width() >> mipIndex;
		height = Height() >> mipIndex;
	}

	void GPUTexture::GetResidentSize(int32& width, int32& height, int32& depth) const
	{
		// Check if texture isn't loaded
		if (m_ResidentMipLevels <= 0)
		{
			width = height = depth = 0;
			return;
		}

		const int32 mipIndex = m_Desc.MipLevels - m_ResidentMipLevels;
		width = Width() >> mipIndex;
		height = Height() >> mipIndex;
		depth = Depth() >> mipIndex;
	}

	uint32 GPUTexture::RowPitch(int32 mipIndex) const
	{
		uint32 rowPitch, slicePitch;
		ComputePitch(mipIndex, rowPitch, slicePitch);
		return rowPitch;
	}

	uint32 GPUTexture::SlicePitch(int32 mipIndex) const
	{
		uint32 rowPitch, slicePitch;
		ComputePitch(mipIndex, rowPitch, slicePitch);
		return slicePitch;
	}

	void GPUTexture::ComputePitch(int32 mipIndex, uint32& rowPitch, uint32& slicePitch) const
	{
		int32 mipWidth, mipHeight;
		GetMipSize(mipIndex, mipWidth, mipHeight);
		GPUUtils::ComputePitch(Format(), mipWidth, mipHeight, rowPitch, slicePitch);
	}

	int32 GPUTexture::CalculateMipSize(int32 size, int32 mipLevel) const
	{
		mipLevel = Math::Min(mipLevel, MipLevels());
		return Math::Max(1, size >> mipLevel);
	}

	int32 GPUTexture::ComputeSubresourceSize(int32 subresource, int32 rowAlign, int32 sliceAlign) const
	{
		const int32 mipLevel = subresource % MipLevels();
		const int32 slicePitch = ComputeSlicePitch(mipLevel, rowAlign);
		const int32 depth = CalculateMipSize(Depth(), mipLevel);
		return Math::AlignUp<int32>(slicePitch * depth, sliceAlign);
	}

	int32 GPUTexture::ComputeBufferOffset(int32 subresource, int32 rowAlign, int32 sliceAlign) const
	{
		int32 offset = 0;
		for (int32 i = 0; i < subresource; i++)
		{
			offset += ComputeSubresourceSize(i, rowAlign, sliceAlign);
		}
		return offset;
	}

	int32 GPUTexture::ComputeBufferTotalSize(int32 rowAlign, int32 sliceAlign) const
	{
		int32 result = 0;
		for (int32 mipLevel = 0; mipLevel < MipLevels(); mipLevel++)
		{
			const int32 slicePitch = ComputeSlicePitch(mipLevel, rowAlign);
			const int32 depth = CalculateMipSize(Depth(), mipLevel);
			result += Math::AlignUp<int32>(slicePitch * depth, sliceAlign);
		}
		return result * ArraySize();
	}

	int32 GPUTexture::ComputeSlicePitch(int32 mipLevel, int32 rowAlign) const
	{
		return ComputeRowPitch(mipLevel, rowAlign) * CalculateMipSize(Height(), mipLevel);
	}

	int32 GPUTexture::ComputeRowPitch(int32 mipLevel, int32 rowAlign) const
	{
		int32 mipWidth = CalculateMipSize(Width(), mipLevel);
		int32 mipHeight = CalculateMipSize(Height(), mipLevel);
		uint32 rowPitch, slicePitch;
		GPUUtils::ComputePitch(Format(), mipWidth, mipHeight, rowPitch, slicePitch);
		return Math::AlignUp<int32>(rowPitch, rowAlign);
	}

	bool GPUTexture::Init(const GPUTextureDescription& desc)
	{
		// Validate description
		const auto device = GPUDevice::instance;
		const GPULimits& gpuLimits = device->GetGPULimits();
		
		if (desc.Usage == GPUResourceUsage::Dynamic)
		{
			LOG_WARNING("Graphic", "GPUTexture Cannot create texture. Dynamic textures are not supported. Description: {0}",
				desc.ToString());
			return false;
		}
		if (desc.MipLevels < 0 || desc.MipLevels > GPU_MAX_TEXTURE_MIP_LEVELS)
		{
			LOG_WARNING("Graphic", "GPUTexture Cannot create texture. Invalid amount of mip levels. Description: {0}", desc.ToString());
			return false;
		}
		if (desc.IsDepthStencil())
		{
			if (desc.MipLevels > 1)
			{
				LOG_WARNING("Graphic", "GPUTexture Cannot create texture. Depth Stencil texture cannot have mip maps. Description: {0}",
					desc.ToString());
				return false;
			}
			if (desc.IsRenderTarget())
			{
				LOG_WARNING("Graphic", "GPUTexture Cannot create texture. Depth Stencil texture cannot be used as a Render Target. Description: {0}",
					desc.ToString());
				return false;
			}
			if (desc.Flags.IsFlag(GPUTextureFlags::ReadOnlyDepthView) && !gpuLimits.HasReadOnlyDepth)
			{
				LOG_WARNING("Graphic", "GPUTexture Cannot create texture. The current graphics platform does not support read-only Depth Stencil texture. Description: {0}",
					desc.ToString());
				return false;
			}
		}
		else
		{
			if (desc.Flags.IsFlag(GPUTextureFlags::ReadOnlyDepthView))
			{
				LOG_WARNING("Graphic", "GPUTexture Cannot create texture. Cannot create read-only Depth Stencil texture that is not a Depth Stencil texture. Add DepthStencil flag. Description: {0}",
					desc.ToString());
				return false;
			}
		}
		if (desc.HasPerMipViews() && !(desc.IsShaderResource() || desc.IsRenderTarget()))
		{
			LOG_WARNING("Graphic", "GPUTexture Cannot create texture. Depth Stencil texture cannot have mip maps. Description: {0}",
				desc.ToString());
			return false;
		}

		switch (desc.Dimensions)
		{
		case TextureDimensions::Texture:
		{
			if (desc.HasPerSliceViews())
			{
				LOG_WARNING("Graphic", "GPUTexture Cannot create texture. Texture cannot have per slice views. Description: {0}",
					desc.ToString());
				return false;
			}
			if (desc.Width <= 0 || desc.Height <= 0 || desc.ArraySize <= 0
				|| desc.Width > gpuLimits.MaximumTexture2DSize
				|| desc.Height > gpuLimits.MaximumTexture2DSize
				|| desc.ArraySize > gpuLimits.MaximumTexture2DArraySize)
			{
				LOG_WARNING("Graphic", "GPUTexture Cannot create texture. Invalid dimensions. Description: {0}", desc.ToString());
				return false;
			}

			break;
		}
		case TextureDimensions::VolumeTexture:
		{
			if (desc.IsDepthStencil())
			{
				LOG_WARNING("Graphic", "GPUTexture Cannot create texture. Only 2D Texture can be used as a Depth Stencil. Description: {0}",
					desc.ToString());
				return false;
			}
			if (desc.ArraySize != 1)
			{
				LOG_WARNING("Graphic", "GPUTexture Cannot create texture. Volume texture cannot create array of volume textures. Description: {0}",
					desc.ToString());
				return false;
			}
			if (desc.MultiSampleLevel != MSAALevel::None)
			{
				LOG_WARNING("Graphic", "GPUTexture Cannot create texture. Volume texture cannot use multi-sampling. Description: {0}",
					desc.ToString());
				return false;
			}
			if (desc.HasPerMipViews())
			{
				LOG_WARNING("Graphic", "GPUTexture Cannot create texture. Volume texture cannot have per mip map views. Description: {0}",
					desc.ToString());
				return false;
			}
			if (desc.HasPerMipViews() && !desc.IsRenderTarget())
			{
				LOG_WARNING("Graphic", "GPUTexture Cannot create texture. Volume texture cannot have per slice map views if is not a render target. Description: {0}",
					desc.ToString());
				return false;
			}
			if (desc.Width <= 0 || desc.Height <= 0 || desc.Depth <= 0
				|| desc.Width > gpuLimits.MaximumTexture3DSize
				|| desc.Height > gpuLimits.MaximumTexture3DSize
				|| desc.Depth > gpuLimits.MaximumTexture3DSize)
			{
				LOG_WARNING("Graphic", "GPUTexture Cannot create texture. Invalid dimensions. Description: {0}", desc.ToString());
				return false;
			}

			break;
		}
		case TextureDimensions::CubeTexture:
		{
			if (desc.HasPerSliceViews())
			{
				LOG_WARNING("Graphic", "GPUTexture Cannot create texture. Cube texture cannot have per slice views. Description: {0}",
					desc.ToString());
				return false;
			}
			if (desc.Width <= 0 || desc.ArraySize <= 0
				|| desc.Width > gpuLimits.MaximumTextureCubeSize
				|| desc.Height > gpuLimits.MaximumTextureCubeSize
				|| desc.ArraySize * 6 > gpuLimits.MaximumTexture2DArraySize
				|| desc.Width != desc.Height)
			{
				LOG_WARNING("Graphic", "GPUTexture Cannot create texture. Invalid dimensions. Description: {0}", desc.ToString());
				return false;
			}

			break;
		}
		}

		const bool isCompressed = IsFormatBlockCompressed(desc.Format);
		if (isCompressed)
		{
			const int32 blockSize = PixelFormatGetBlockSize(desc.Format);
			if (desc.Width < blockSize || desc.Height < blockSize)
			{
				LOG_WARNING("Graphic", "GPUTexture Cannot create texture. Invalid dimensions. Description: {0}", desc.ToString());
				return false;
			}
		}

		// Release previous data
		ReleaseGPU();

		// Initialize
		m_Desc = desc;
		m_SRGB = PixelFormatIsSRGB(desc.Format);
		m_IsBlockCompressed = isCompressed;
		if (!OnInit())
		{
			ReleaseGPU();
			m_Desc.Clear();
			m_ResidentMipLevels = 0;
			LOG_WARNING("Graphic", "GPUTexture Cannot initialize texture. Description: {0}", desc.ToString());
			return false;
		}

		// Render targets and depth buffers doesn't support normal textures streaming and are considered to be always resident
		if (!IsRegularTexture())
		{
			m_ResidentMipLevels = MipLevels();
		}

		return true;
	}

	GPUTexture* GPUTexture::ToStagingReadback() const
	{
		const auto desc = m_Desc.ToStagingReadback();
		Threading::ScopeLock gpuLock(GPUDevice::instance->locker);
		auto* staging = GPUDevice::instance->CreateTexture(SE_TEXT("Staging.ReadBack"));
		if (!staging->Init(desc))
		{
			Delete(staging);
			return nullptr;
		}
		return staging;
	}

	GPUTexture* GPUTexture::ToStagingUpload() const
	{
		const auto desc = m_Desc.ToStagingUpload();
		Threading::ScopeLock gpuLock(GPUDevice::instance->locker);
		auto* staging = GPUDevice::instance->CreateTexture(SE_TEXT("Staging.Upload"));
		if (staging->Init(desc))
		{
			Delete(staging);
			return nullptr;
		}
		return staging;
	}

	bool GPUTexture::Resize(int32 width, int32 height, int32 depth, PixelFormat format)
	{
		PROFILE_CPU();
		if (!IsAllocated())
		{
			LOG_WARNING("Graphic", "GPUTexture Cannot resize not created textures.");
			return false;
		}

		auto desc = GetDescription();
		if (format == PixelFormat::Undefined)
			format = desc.Format;

		// Skip if size won't change
		if (desc.Width == width && desc.Height == height && desc.Depth == depth && desc.Format == format)
			return false;

		desc.Format = format;
		desc.Width = width;
		desc.Height = height;
		desc.Depth = depth;
		if (desc.MipLevels > 1)
			desc.MipLevels = CalculateMipMapCount(0, Math::Max(width, height));

		// Recreate
		return Init(desc);
	}

	GPUTask* GPUTexture::UploadMipMapAsync(const BytesContainer& data, int32 mipIndex, bool copyData)
	{
		uint32 rowPitch, slicePitch;
		ComputePitch(mipIndex, rowPitch, slicePitch);
		return UploadMipMapAsync(data, mipIndex, rowPitch, slicePitch, copyData);
	}

	GPUTask* GPUTexture::UploadMipMapAsync(const BytesContainer& data, int32 mipIndex, int32 rowPitch, int32 slicePitch, bool copyData)
	{
		PROFILE_CPU();
		ENGINE_ASSERT(IsAllocated());
		ENGINE_ASSERT(mipIndex < MipLevels() && data.IsValid());
		ENGINE_ASSERT(data.Length() >= slicePitch);

		// Optimize texture upload invoked during rendering
		if (Threading::IsMainThread() && GPUDevice::instance->IsRendering())
		{
			// Update all array slices
			const byte* dataSource = data.Get();
			for (int32 arrayIndex = 0; arrayIndex < m_Desc.ArraySize; arrayIndex++)
			{
				GPUDevice::instance->GetMainContext()->UpdateTexture(this, arrayIndex, mipIndex, dataSource, rowPitch, slicePitch);
				dataSource += slicePitch;
			}
			if (mipIndex == HighestResidentMipIndex() - 1)
			{
				// Mark as mip loaded
				SetResidentMipLevels(ResidentMipLevels() + 1);
			}
			return nullptr;
		}

		GPUUploadTextureMipTask* task = ::SE::New<GPUUploadTextureMipTask>(this, mipIndex, data, rowPitch, slicePitch, copyData);
		ENGINE_ASSERT(task && task->HasReference(this));
		return task;
	}

	uint64 GPUTexture::CalculateMemoryUsage() const
	{
		return GPUUtils::CalculateTextureMemoryUsage(Format(), Width(), Height(), Depth(), MipLevels()) * ArraySize();
	}

	String GPUTexture::ToString() const
	{
#if GPU_ENABLE_RESOURCE_NAMING
		return String::Format(SE_TEXT("Texture {0}, Residency: {1}, Name: {2}"),
			m_Desc.ToString(),
			m_ResidentMipLevels,
			String(" "));
//			GetName());
#else
		return TEXT("Texture");
#endif
	}

	GPUResourceType GPUTexture::GetResType() const
	{
		if (IsVolume())
			return GPUResourceType::VolumeTexture;
		if (IsCubeMap())
			return GPUResourceType::CubeTexture;
		return IsRegularTexture() ? GPUResourceType::Texture : GPUResourceType::RenderTarget;
	}

	void GPUTexture::OnReleaseGPU()
	{
		m_Desc.Clear();
		m_ResidentMipLevels = 0;
	}

	/*GPUTask* GPUTexture::UploadMipMapAsync(const BytesContainer& data, int32 mipIndex, bool copyData)
	{
		uint32 rowPitch, slicePitch;
		ComputePitch(mipIndex, rowPitch, slicePitch);
		return UploadMipMapAsync(data, mipIndex, rowPitch, slicePitch, copyData);
	}

	GPUTask* GPUTexture::UploadMipMapAsync(const BytesContainer& data,
		int32 mipIndex,
		int32 rowPitch,
		int32 slicePitch,
		bool copyData)
	{
		PROFILE_CPU();
		ENGINE_ASSERT(IsAllocated());
		ENGINE_ASSERT(mipIndex < MipLevels() && data.IsValid());
		ENGINE_ASSERT(data.Length() >= slicePitch);

		// Optimize texture upload invoked during rendering
		if (IsInMainThread() && GPUDevice::Instance->IsRendering())
		{
			// Update all array slices
			const byte* dataSource = data.Get();
			for (int32 arrayIndex = 0; arrayIndex < _desc.ArraySize; arrayIndex++)
			{
				GPUDevice::Instance->GetMainContext()->UpdateTexture(this,
					arrayIndex,
					mipIndex,
					dataSource,
					rowPitch,
					slicePitch);
				dataSource += slicePitch;
			}
			if (mipIndex == HighestResidentMipIndex() - 1)
			{
				// Mark as mip loaded
				SetResidentMipLevels(ResidentMipLevels() + 1);
			}
			return nullptr;
		}

		auto task = ::New<GPUUploadTextureMipTask > (this, mipIndex, data, rowPitch, slicePitch, copyData);
		ASSERT_LOW_LAYER(task && task->HasReference(this));
		return task;
	}*/

	class TextureDownloadDataTask : public Threading::ThreadPoolTask
	{
	private:
		GPUTextureReference _texture;
		GPUTexture* _staging;
		TextureData* _data;
		bool _deleteStaging;

	public:
		TextureDownloadDataTask(GPUTexture* texture, GPUTexture* staging, TextureData& data)
			: _texture(texture), _staging(staging), _data(&data)
		{
			_deleteStaging = texture != staging;
		}

		~TextureDownloadDataTask() override
		{
			if (_deleteStaging && _staging)
				_staging->DeleteObjectNow();
		}

	public:
		// [ThreadPoolTask]
		bool HasReference(void* resource) const override
		{
			return _texture == resource || _staging == resource;
		}

	protected:
		// [ThreadPoolTask]
		bool Run() override
		{
			auto texture = _texture.Get();
			if (texture == nullptr || _staging == nullptr || _data == nullptr)
			{
				LOG_WARNING("Graphic", "GPUTexture Cannot download texture data. Missing objects.");
				return false;
			}
			return _staging->DownloadData(*_data);
		}

		void OnEnd() override
		{
			_texture.Unlink();

			// Base
			ThreadPoolTask::OnEnd();
		}
	};

	bool GPUTexture::DownloadData(TextureData& result)
	{
		// Skip for empty ones
		if (MipLevels() == 0)
		{
			LOG_WARNING("Graphic", "Cannot download GPU texture data from an empty texture.");
			return false;
		}
		if (Depth() != 1)
		{
			LOG_WARNING("Graphic", "not support volume texture data downloading.");
			return false;
		}
		PROFILE_CPU();

		// Use faster path for staging resources
		if (IsStaging())
		{
			const auto arraySize = ArraySize();
			const auto mipLevels = MipLevels();

			// Set texture info
			result.Width = Width();
			result.Height = Height();
			result.Depth = Depth();
			result.Format = Format();

			// Get all mip maps for each array slice
			auto& rawResultData = result.Items;
			rawResultData.Resize(arraySize, false);
			for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
			{
				auto& arraySlice = rawResultData[arrayIndex];
				arraySlice.Mips.Resize(mipLevels);

				for (int32 mipMapIndex = 0; mipMapIndex < mipLevels; mipMapIndex++)
				{
					auto& mip = arraySlice.Mips[mipMapIndex];
					const int32 mipWidth = result.Width >> mipMapIndex;
					const int32 mipHeight = result.Height >> mipMapIndex;
					uint32 mipRowPitch, mipSlicePitch;
					GPUUtils::ComputePitch(result.Format, mipWidth, mipHeight, mipRowPitch, mipSlicePitch);

					// Gather data
					if (GetData(arrayIndex, mipMapIndex, mip, mipRowPitch))
					{
						LOG_WARNING("Graphic", "GPUTextureStaging resource of \'{0}\' get data failed.", ToString());
						return false;
					}
				}
			}

			return true;
		}

		const auto name = ToString();

		// Ensure not running on main thread - we support DownloadData from textures only on a worker threads (Thread Pool Workers or Content Loaders)
		if (Threading::IsMainThread())
		{
			LOG_WARNING("Graphic", "GPUTexture Downloading GPU texture data from the main thread is not supported.");
			return false;
		}

		// Create async task
		auto task = DownloadDataAsync(result);
		if (task == nullptr)
		{
			LOG_WARNING("Graphic", "GPUTexture Cannot create async download task for resource {0}.", name);
			return false;
		}

		// Wait for work to be done
		task->Start();
		if (!task->Wait())
		{
			LOG_WARNING("Graphic", "GPUTexture Resource \'{0}\' copy failed.", name);
			return false;
		}

		return true;
	}

	Threading::Task* GPUTexture::DownloadDataAsync(TextureData& result)
	{
		// Skip for empty ones
		if (MipLevels() == 0)
		{
			LOG_WARNING("Graphic", "GPUTexture Cannot download texture data. It has not ben created yet.");
			return nullptr;
		}
		if (Depth() != 1)
		{
			LOG_WARNING("Graphic", "not support volume texture data downloading.");
			return nullptr;
		}
		PROFILE_CPU();

		// Use faster path for staging resources
		if (IsStaging())
		{
			// Create task to copy downloaded data to TextureData container
			auto getDataTask = ::SE::New<TextureDownloadDataTask>(this, this, result);
			ENGINE_ASSERT(getDataTask->HasReference(this));
			return getDataTask;
		}

		// Create the staging resource
		const auto staging = ToStagingReadback();
		if (staging == nullptr)
		{
			LOG_ERROR("Graphic", "Cannot create staging resource from {0}.", ToString());
			return nullptr;
		}

		// Create async resource copy task
		auto copyTask = ::SE::New<GPUCopyResourceTask>(this, staging);
		ENGINE_ASSERT(copyTask->HasReference(this) && copyTask->HasReference(staging));

		// Create task to copy downloaded data to TextureData container
		auto getDataTask = ::SE::New<TextureDownloadDataTask>(this, staging, result);
		ENGINE_ASSERT(getDataTask->HasReference(this) && getDataTask->HasReference(staging));

		// Set continuation
		copyTask->ContinueWith(getDataTask);

		return copyTask;
	}

	void GPUTexture::SetResidentMipLevels(int32 count)
	{
		count = Math::Clamp(count, 0, MipLevels());
		if (m_ResidentMipLevels == count || !IsRegularTexture())
			return;
		m_ResidentMipLevels = count;
		OnResidentMipsChanged();
		ResidentMipsChanged(this);
	}

#pragma endregion


	TextureHeader::TextureHeader()
	{
		Platform::MemoryClear(this, sizeof(*this));
		TextureGroup = -1;
	}

	static_assert(sizeof(TextureHeader) == 36, "Invalid TextureHeader size.");

} // SE

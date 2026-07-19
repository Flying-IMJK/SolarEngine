
#include "StreamingTexture.h"
#include "TextureBase.h"
#include "Runtime/Core/Logging/Logging.h"
#include "Runtime/Core/Thread/Threading.h"
#include "Runtime/Core/TypeSystem/Types.h"
#include "Runtime/Resource/Streaming/StreamingGroup.h"
#include "Runtime/Resource/Loading/AssetLoading.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/Base/GPUUtils.h"
#include "Runtime/Graphics/Textures/GPUTexture.h"
#include "Runtime/Graphics/Async/GPUTask.h"
#include "Runtime/Graphics/Async/Tasks/GPUUploadTextureMipTask.h"

namespace SE
{
	StreamingTexture::StreamingTexture()
	{
	}

	StreamingTexture::StreamingTexture(TextureBase* parent)
		: StreamableResource(StreamingGroups::Instance().Textures()), _owner(parent), m_Texture(nullptr), _isBlockCompressed(false)
	{
		ENGINE_ASSERT(parent != nullptr);
		_header.MipLevels = 0;
	}

	StreamingTexture::StreamingTexture(TextureBase* parent, String name)
		: StreamableResource(StreamingGroups::Instance().Textures()), _owner(parent), m_Texture(nullptr), _isBlockCompressed(false)
	{
		ENGINE_ASSERT(parent != nullptr);

		// Always have created texture object
		ENGINE_ASSERT(GPUDevice::instance);
		m_Texture = GPUDevice::instance->CreateTexture(name);
		ENGINE_ASSERT(m_Texture != nullptr);

		_header.MipLevels = 0;
	}

	StreamingTexture::~StreamingTexture()
	{
		UnloadTexture();
		SAFE_DELETE(m_Texture);
	}

	Float2 StreamingTexture::Size() const
	{
		return m_Texture->Size();
	}

	int32 StreamingTexture::TextureMipIndexToTotalIndex(int32 textureMipIndex) const
	{
		const int32 missingMips = TotalMipLevels() - m_Texture->MipLevels();
		return textureMipIndex + missingMips;
	}

	int32 StreamingTexture::TotalIndexToTextureMipIndex(int32 mipIndex) const
	{
		const int32 missingMips = TotalMipLevels() - m_Texture->MipLevels();
		return mipIndex - missingMips;
	}

	bool StreamingTexture::Create(const TextureHeader& header)
	{
		// Validate header (further validation is performed by the Texture.Init)
		if (header.MipLevels > GPU_MAX_TEXTURE_MIP_LEVELS
			|| Math::RangeExclusive(header.Width, 1, GPU_MAX_TEXTURE_SIZE)
			|| Math::RangeExclusive(header.Height, 1, GPU_MAX_TEXTURE_SIZE))
		{
			LOG_WARNING("Renderer", "Invalid texture header.");
			return false;
		}

		ENGINE_ASSERT(m_Texture);

		Threading::ScopeLock lock(_owner->GetOwnerLocker());

		if (IsInitialized())
		{
			m_Texture->ReleaseGPU();
		}

		// Cache header
		// Note: by caching header we assume that streaming texture has been initialized.
		// Then we can start it's streaming so it may be allocated later (using texture.Init)
		// But this may not happen because resources may be created and loaded but not always allocated.
		// That's one of the main advantages of the current resources streaming system.
		_header = header;
		_isBlockCompressed = PixelFormatIsCompressed(_header.Format);
		if (_isBlockCompressed)
		{
			// Ensure that streaming doesn't go too low because the hardware expects the texture to be min in size of compressed texture block
			const int32 blockSize = PixelFormatGetBlockSize(_header.Format);
			int32 lastMip = header.MipLevels - 1;
			while ((header.Width >> lastMip) < blockSize && (header.Height >> lastMip) < blockSize && lastMip > 0)
			{
				lastMip--;
			}
			_minMipCountBlockCompressed = Math::Min(header.MipLevels - lastMip + 1, header.MipLevels);
		}

		// Request resource streaming
#if GPU_ENABLE_TEXTURES_STREAMING
		bool isDynamic = !_header.NeverStream;
#else
		bool isDynamic = false;
#endif
		StartStreaming(isDynamic);

		return true;
	}

	void StreamingTexture::UnloadTexture()
	{
		Threading::ScopeLock lock(_owner->GetOwnerLocker());
		CancelStreamingTasks();
		m_Texture->ReleaseGPU();
		_header.MipLevels = 0;
		ENGINE_ASSERT(_streamingTasks.Count() == 0);
	}

	uint64 StreamingTexture::GetTotalMemoryUsage() const
	{
		const uint64 arraySize = _header.IsCubeMap ? 6 : 1;
		return GPUUtils::CalculateTextureMemoryUsage(_header.Format, _header.Width, _header.Height, _header.MipLevels) * arraySize;
	}

	String StreamingTexture::ToString() const
	{
		return m_Texture->ToString();
	}

	int32 StreamingTexture::GetMaxResidency() const
	{
		return _header.MipLevels;
	}

	int32 StreamingTexture::GetCurrentResidency() const
	{
		return m_Texture->ResidentMipLevels();
	}

	int32 StreamingTexture::GetAllocatedResidency() const
	{
		return m_Texture->MipLevels();
	}

	bool StreamingTexture::CanBeUpdated() const
	{
		// Streaming Texture cannot be updated if:
		// - is not initialized
		// - mip data uploading job running
		// - resize texture job running
		if (IsInitialized())
		{
			Threading::ScopeLock lock(_owner->GetOwnerLocker());
			return _streamingTasks.Count() == 0;
		}
		return false;
	}



	class StreamTextureResizeTask : public GPUTask
	{
	private:
		StreamingTexture* _streamingTexture;
		GPUTexture* _newTexture;
		int32 _uploadedMipCount;

	public:
		StreamTextureResizeTask(StreamingTexture* texture, GPUTexture* newTexture)
			: GPUTask(Type::CopyResource), _streamingTexture(texture), _newTexture(newTexture)
		{
			_streamingTexture->_streamingTasks.Add(this);
		}

		~StreamTextureResizeTask() override
		{
			_newTexture->DeleteObjectNow();
			_newTexture = nullptr;
		}

	protected:
		// [GPUTask]
		Result run(GPUTasksContext* context) override
		{
			if (_streamingTexture == nullptr)
				return Result::MissingResources;

			// Copy all shared mips from previous texture to the new one
			GPUTexture* dstTexture = _newTexture;
			const int32 dstMips = dstTexture->MipLevels();
			GPUTexture* srcTexture = _streamingTexture->GetTexture();
			const int32 srcMips = srcTexture->MipLevels();
			const int32 mipCount = Math::Min(dstMips, srcMips);
			ENGINE_ASSERT(mipCount > 0);
			for (int32 mipIndex = 0; mipIndex < mipCount; mipIndex++)
			{
				context->GPU->CopySubresource(dstTexture, dstMips - mipIndex - 1, srcTexture, srcMips - mipIndex - 1);
			}
			_uploadedMipCount = mipCount;

			return Result::Ok;
		}

		void OnEnd() override
		{
			if (_streamingTexture)
			{
				Threading::ScopeLock lock(_streamingTexture->GetOwner()->GetOwnerLocker());
				_streamingTexture->_streamingTasks.Remove(this);
			}

			// Base
			GPUTask::OnEnd();
		}

		void OnSync() override
		{
			Swap(_streamingTexture->m_Texture, _newTexture);
			_streamingTexture->GetTexture()->SetResidentMipLevels(_uploadedMipCount);
			_streamingTexture->ResidencyChanged();
			_newTexture->DeleteObjectNow();
			_newTexture = nullptr;
		}
	};

	Threading::Task* StreamingTexture::UpdateAllocation(int32 residency)
	{
		Threading::ScopeLock lock(_owner->GetOwnerLocker());

		ENGINE_ASSERT(m_Texture && IsInitialized() && Math::RangeInclusive(residency, 0, TotalMipLevels()));
		Threading::Task* result = nullptr;

		const int32 allocatedResidency = GetAllocatedResidency();
		ENGINE_ASSERT(allocatedResidency >= 0);
		if (residency == allocatedResidency)
		{
			// Residency won't change
		}
		else if (residency == 0)
		{
			// Release texture memory
			m_Texture->ReleaseGPU();
		}
		else
		{
			// Use new texture object for resizing task
			GPUTexture* texture = m_Texture;
			if (allocatedResidency != 0)
			{
#if GPU_ENABLE_RESOURCE_NAMING
				texture = GPUDevice::instance->CreateTexture(m_Texture->GetName());
#else
				texture = GPUDevice::instance->CreateTexture();
#endif
			}

			// Create texture description
			const int32 mip = TotalMipLevels() - residency;
			const int32 width = Math::Max(TotalWidth() >> mip, 1);
			const int32 height = Math::Max(TotalHeight() >> mip, 1);
			GPUTextureDescription desc;
			if (IsCubeMap())
			{
				ENGINE_ASSERT(width == height);
				desc = GPUTextureDescription::NewCube(width, residency, _header.Format, GPUTextureFlags::ShaderResource);
			}
			else
			{
				desc = GPUTextureDescription::New2D(width, height, residency, _header.Format, GPUTextureFlags::ShaderResource);
			}

			// Setup texture
			if (!texture->Init(desc))
			{
				Streaming.Error = true;
#if GPU_ENABLE_RESOURCE_NAMING
				LOG_ERROR("Renderer", "Cannot allocate texture {0}", texture->GetName());
#else
				LOG_ERROR("Renderer", "Cannot allocate texture");
#endif
			}
			if (allocatedResidency != 0)
			{
				// Copy data from the previous texture
				result = New<StreamTextureResizeTask>(this, texture);
			}
			else
			{
				// Use the new texture
				m_Texture = texture;
			}
		}

		return result;
	}

	class StreamTextureMipTask : public GPUUploadTextureMipTask
	{
	private:
		StreamingTexture* _streamingTexture;
		Storage::LockData _dataLock;

	public:
		StreamTextureMipTask(StreamingTexture* texture, int32 mipIndex)
			: GPUUploadTextureMipTask(texture->GetTexture(), mipIndex, Span<byte>(nullptr, 0), 0, 0, false), _streamingTexture(texture),
			  _dataLock(_streamingTexture->GetOwner()->LockData())
		{
			_streamingTexture->_streamingTasks.Add(this);
			m_Texture.Released.Bind<StreamTextureMipTask, &StreamTextureMipTask::OnResourceReleased2>(this);
		}

	private:
		void OnResourceReleased2()
		{
			// Unlink texture
			if (_streamingTexture)
			{
				Threading::ScopeLock lock(_streamingTexture->GetOwner()->GetOwnerLocker());
				_streamingTexture->_streamingTasks.Remove(this);
				_streamingTexture = nullptr;
			}
		}

	protected:
		// [GPUTask]
		Result run(GPUTasksContext* context) override
		{
			const auto texture = m_Texture.Get();
			if (texture == nullptr)
				return Result::MissingResources;

			// Ensure that texture has been allocated before this task and has proper format
			if (!texture->IsAllocated() || texture->Format() != _streamingTexture->GetHeader()->Format)
			{
				LOG_ERROR("Graphic", "Cannot stream texture {0} (streaming format: {1})", texture->ToString(), Types::GetEnumString(_streamingTexture->GetHeader()->Format));
				return Result::Failed;
			}

			// Get asset data
			BytesContainer data;
			const auto absoluteMipIndex = _streamingTexture->TextureMipIndexToTotalIndex(_mipIndex);
			_streamingTexture->GetOwner()->GetMipData(absoluteMipIndex, data);
			if (data.IsInvalid())
				return Result::MissingData;

			// Cache data
			const int32 arraySize = texture->ArraySize();
			uint32 rowPitch, slicePitch;
			if (!_streamingTexture->GetOwner()->GetMipDataCustomPitch(absoluteMipIndex, rowPitch, slicePitch))
				texture->ComputePitch(_mipIndex, rowPitch, slicePitch);
			_data.Link(data);
			ENGINE_ASSERT(data.Length() >= (int32)slicePitch * arraySize);

			// Update all array slices
			const byte* dataSource = data.Get();
			for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
			{
				context->GPU->UpdateTexture(texture, arrayIndex, _mipIndex, dataSource, rowPitch, slicePitch);
				dataSource += slicePitch;
			}

			return Result::Ok;
		}

		void OnEnd() override
		{
			_dataLock.Release();
			if (_streamingTexture)
			{
				Threading::ScopeLock lock(_streamingTexture->GetOwner()->GetOwnerLocker());
				_streamingTexture->_streamingTasks.Remove(this);
				_streamingTexture = nullptr;
			}

			// Base
			GPUUploadTextureMipTask::OnEnd();
		}

		void OnFail() override
		{
			if (_streamingTexture)
			{
				// Stop streaming on fail
				_streamingTexture->ResetStreaming();
			}

			GPUUploadTextureMipTask::OnFail();
		}
	};

	Threading::Task* StreamingTexture::CreateStreamingTask(int32 residency)
	{
		Threading::ScopeLock lock(_owner->GetOwnerLocker());

		ENGINE_ASSERT(m_Texture && IsInitialized() && Math::RangeInclusive(residency, 0, TotalMipLevels()));
		Threading::Task* result = nullptr;

		// Switch if go up or down with residency
		const int32 mipsCount = residency - GetCurrentResidency();
		if (mipsCount > 0)
		{
			// Create tasks collection
			const auto startMipIndex = TotalMipLevels() - m_Texture->ResidentMipLevels() - 1;
			const auto endMipIndex = startMipIndex - mipsCount;
			for (int32 mipIndex = startMipIndex; mipIndex > endMipIndex; mipIndex--)
			{
				ENGINE_ASSERT(mipIndex >= 0 && mipIndex < _header.MipLevels);

				// Request texture mip map data
				auto task = _owner->RequestMipDataAsync(mipIndex);
				if (task)
				{
					if (result)
						result->ContinueWith(task);
					else
						result = task;
				}

				// Add upload data task
				const int32 allocatedMipIndex = TotalIndexToTextureMipIndex(mipIndex);
				task = New<StreamTextureMipTask>(this, allocatedMipIndex);
				if (result)
					result->ContinueWith(task);
				else
					result = task;
			}

			ENGINE_ASSERT(result);
		}
		else
		{
			// Check if trim the mips down to 0 (full texture release)
			if (residency == 0)
			{
				// Do the quick data release
				m_Texture->ReleaseGPU();
				ResidencyChanged();
			}
			else
			{
				// TODO: create task for reducing texture quality, or update SRV now
				// add support for streaming quality down
				ENGINE_UNREACHABLE_CODE();
			}
		}

		return result;
	}

	void StreamingTexture::CancelStreamingTasks()
	{
		Threading::ScopeLock lock(_owner->GetOwnerLocker());
		auto tasks = _streamingTasks;
		for (auto task : tasks)
		{
			task->Cancel();
		}
	}
}
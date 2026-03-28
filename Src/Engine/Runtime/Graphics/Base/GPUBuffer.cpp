
#include "GPUBuffer.h"
#include "Core/Memory/Memory.h"
#include "Core/Profiler/ProfilerCPU.h"
#include "Core/Thread/ThreadPool.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/GPUResourceProperty.h"
#include "Runtime/Graphics/Async/Tasks/GPUCopyResourceTask.h"

namespace SE
{
	GPUBufferDescription GPUBufferDescription::Buffer(uint32 size, EnumFlags<GPUBufferFlags> flags, PixelFormat format, const void* initData, uint32 stride, GPUResourceUsage usage)
	{
		GPUBufferDescription desc;
		desc.Size = size;
		desc.Stride = stride;
		desc.Flags = flags;
		desc.Format = format;
		desc.InitData = initData;
		desc.Usage = usage;
		return desc;
	}

	GPUBufferDescription GPUBufferDescription::Typed(int32 count, PixelFormat viewFormat, bool isUnorderedAccess, GPUResourceUsage usage)
	{
		EnumFlags<GPUBufferFlags> bufferFlags = GPUBufferFlags::ShaderResource;
		if (isUnorderedAccess)
		{
			bufferFlags.SetFlag(GPUBufferFlags::UnorderedAccess);
		}
		const auto stride = PixelFormatGetSizeInBits(viewFormat);
		return Buffer(count * stride, bufferFlags, viewFormat, nullptr, stride, usage);
	}

	GPUBufferDescription GPUBufferDescription::Typed(const void* data, int32 count, PixelFormat viewFormat, bool isUnorderedAccess, GPUResourceUsage usage)
	{
		EnumFlags<GPUBufferFlags> bufferFlags = GPUBufferFlags::ShaderResource;
		if (isUnorderedAccess)
		{
			bufferFlags.IsFlag(GPUBufferFlags::UnorderedAccess);
		}
		const auto stride = PixelFormatGetSizeInBits(viewFormat);
		return Buffer(count * stride, bufferFlags, viewFormat, data, stride, usage);
	}

	void GPUBufferDescription::Clear()
	{
		Platform::MemoryClear(this, sizeof(GPUBufferDescription));
	}

	GPUBufferDescription GPUBufferDescription::ToStagingUpload() const
	{
		auto desc = *this;
		desc.Usage = GPUResourceUsage::StagingUpload;
		desc.Flags = GPUBufferFlags::None;
		desc.InitData = nullptr;
		return desc;
	}

	GPUBufferDescription GPUBufferDescription::ToStagingReadBack() const
	{
		auto desc = *this;
		desc.Usage = GPUResourceUsage::StagingReadback;
		desc.Flags = GPUBufferFlags::None;
		desc.InitData = nullptr;
		return desc;
	}

	bool GPUBufferDescription::Equals(const GPUBufferDescription& other) const
	{
		return Size == other.Size
			&& Stride == other.Stride
			&& Flags == other.Flags
			&& Format == other.Format
			&& Usage == other.Usage
			&& InitData == other.InitData;
	}

	String GPUBufferDescription::ToString() const
	{
		return String::Format(SE_TEXT("Size: {0}, Stride: {1}, Flags: {2}, CharsFormat: {3}, Usage: {4}"),
			Size,
			Stride,
			String(""),// ScriptingEnum::ToStringFlags(Flags),
			PixelFormatGetString(Format),
			(int32)Usage);
	}

	uint32 GetHash(const GPUBufferDescription& key)
	{
		uint32 hashCode = key.Size;
		hashCode = (hashCode * 397) ^ key.Stride;
		hashCode = (hashCode * 397) ^ key.Flags.Get();
		hashCode = (hashCode * 397) ^ (uint32)key.Format;
		hashCode = (hashCode * 397) ^ (uint32)key.Usage;
		return hashCode;
	}

	// GPUBufferView
	GPUBufferView::GPUBufferView()
	{

	}

	// GPUBuffer

	class BufferDownloadDataTask : public Threading::ThreadPoolTask
	{
	private:
		BufferReference m_Buffer;
		GPUBuffer* m_Staging;
		BytesContainer& m_Data;

	public:
		BufferDownloadDataTask(GPUBuffer* buffer, GPUBuffer* staging, BytesContainer& data)
			: m_Buffer(buffer)
			, m_Staging(staging)
			, m_Data(data)
		{
		}

		~BufferDownloadDataTask() override
		{
			DeleteObjectSafe(m_Staging);
		}

	public:
		// [ThreadPoolTask]
		bool HasReference(void
		* resource) const override
		{
			return m_Buffer == resource || m_Staging == resource;
		}

	protected:
		// [ThreadPoolTask]
		bool Run() override
		{
			// Check resources
			const auto buffer = m_Buffer.Get();
			if (buffer == nullptr || m_Staging == nullptr)
			{
				LOG_WARNING("Graphic", "Cannot download buffer data. Missing objects.");
				return true;
			}

			// Gather data
			if (m_Staging->GetData(m_Data))
			{
				LOG_WARNING("Graphic", "Staging resource of \'{0}\' get data failed.", buffer->ToString());
				return true;
			}

			return false;
		}

		void OnEnd() override
		{
			m_Buffer.Unlink();

			// Base
			ThreadPoolTask::OnEnd();
		}
	};



	void GPUBuffer::SetData(const void* data, uint32 size)
	{
		PROFILE_CPU();
		if (size == 0 || data == nullptr)
		{
			LOG_ERROR("Graphic", "Buffer.SetData OutOfRange");
			return;
		}
		if (size > GetSize())
		{
			LOG_ERROR("Graphic", "Buffer.SetData OutOfRange");
			return;
		}
		void* mapped = Map(GPUResourceMapMode::Write);
		if (!mapped)
		{
			return;
		}
		Platform::MemoryCopy(mapped, data, size);
		Unmap();
	}

	bool GPUBuffer::Init(const GPUBufferDescription& desc)
	{
		ENGINE_ASSERT(Math::RangeInclusive<uint32>(desc.Size, 1, Max_int32)
			&& Math::RangeInclusive<uint32>(desc.Stride, 0, 1024));

		// Validate description
		if (desc.Flags.IsFlag(GPUBufferFlags::Structured))
		{
			if (desc.Stride <= 0)
			{
				LOG_WARNING("Graphic", "GPUBuffer Cannot create buffer. Element size cannot be less or equal 0 for structured buffer.");
				return false;
			}
		}
		if (desc.Flags.IsFlag(GPUBufferFlags::RawBuffer))
		{
			if (desc.Format != PixelFormat::Undefined)
			{
				LOG_WARNING("Graphic", "GPUBuffer Cannot create buffer. Raw buffers must use format R32_Typeless.");
				return false;
			}
		}

		// Release previous data
		ReleaseGPU();

		// Initialize
		m_Desc = desc;
		if (!OnInit())
		{
			ReleaseGPU();
			LOG_WARNING("Graphic", "GPUBuffer Cannot initialize buffer. Description: {0}", desc.ToString());
			return false;
		}

		return true;
	}


	GPUBuffer* GPUBuffer::ToStagingReadback() const
	{
		const auto desc = m_Desc.ToStagingReadBack();
		Threading::ScopeLock gpuLock(GPUDevice::instance->locker);
		auto* staging = GPUDevice::instance->CreateBuffer(SE_TEXT("Staging.Readback"));
		if (staging->Init(desc))
		{
			staging->ReleaseGPU();
			Delete(staging);
			return nullptr;
		}
		return staging;
	}

	GPUBuffer* GPUBuffer::ToStagingUpload() const
	{
		const auto desc = m_Desc.ToStagingUpload();
		Threading::ScopeLock gpuLock(GPUDevice::instance->locker);
		auto* staging = GPUDevice::instance->CreateBuffer(SE_TEXT("Staging.Upload"));
		if (staging->Init(desc))
		{
			staging->ReleaseGPU();
			Delete(staging);
			return nullptr;
		}
		return staging;
	}


	bool GPUBuffer::Resize(uint32 newSize)
	{
		PROFILE_CPU();
		if (!IsAllocated())
		{
			LOG_ERROR("Graphic", "Resize buffer{0} is not allocate", GetName());
			return false;
		}

		// Setup new description
		auto desc = m_Desc;
		desc.Size = newSize;
		desc.InitData = nullptr;

		// Recreate
		return Init(desc);
	}

	bool GPUBuffer::GetData(BytesContainer& output)
	{
		PROFILE_CPU();
		void* mapped = Map(GPUResourceMapMode::Read);
		if (!mapped)
			return true;
		output.Copy((byte*)mapped, GetSize());
		Unmap();
		return false;
	}

	bool GPUBuffer::DownloadData(BytesContainer& result)
	{
		// Skip for empty ones
		if (GetSize() == 0)
		{
			LOG_WARNING("Graphic", "Cannot download GPU buffer data from an empty buffer.");
			return true;
		}
		if (m_Desc.Usage == GPUResourceUsage::StagingReadback || m_Desc.Usage == GPUResourceUsage::Dynamic)
		{
			// Use faster path for staging resources
			return GetData(result);
		}
		PROFILE_CPU();

		// Ensure not running on main thread
		if (Threading::IsMainThread())
		{
			// TODO: support mesh data download from GPU on a main thread during rendering
			LOG_WARNING("Graphic", "Cannot download GPU buffer data on a main thread. Use staging readback buffer or invoke this function from another thread.");
			return false;
		}

		// Create async task
		auto task = DownloadDataAsync(result);
		if (task == nullptr)
		{
			LOG_WARNING("Graphic", "Cannot create async download task for resource {0}.", ToString());
			return true;
		}

		// Wait for work to be done
		task->Start();
		if (task->Wait())
		{
			LOG_WARNING("Graphic", "Resource \'{0}\' copy failed.", ToString());
			return true;
		}

		return false;
	}

	Threading::Task* GPUBuffer::DownloadDataAsync(BytesContainer& result)
	{
		// Skip for empty ones
		if (GetSize() == 0)
			return nullptr;

		// Create the staging resource
		const auto staging = ToStagingReadback();
		if (staging == nullptr)
		{
			LOG_WARNING("Graphic", "Cannot create staging resource from {0}.", ToString());
			return nullptr;
		}

		// Create async resource copy task
		auto copyTask = New<GPUCopyResourceTask>(this, staging);
		ENGINE_ASSERT(copyTask->HasReference(this) && copyTask->HasReference(staging));

		// Create task to copy downloaded data to BytesContainer
		const auto getDataTask = New<BufferDownloadDataTask>(this, staging, result);
		ENGINE_ASSERT(getDataTask->HasReference(this) && getDataTask->HasReference(staging));

		// Set continuation
		copyTask->ContinueWith(getDataTask);

		return copyTask;
	}

	GPUResourceType GPUBuffer::GetResType() const
	{
		return GPUResourceType::Buffer;
	}

}

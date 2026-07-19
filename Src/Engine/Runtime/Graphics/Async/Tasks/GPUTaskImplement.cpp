
#include "GPUUploadTextureMipTask.h"
#include "GPUUploadBufferTask.h"
#include "Runtime/Graphics/Textures/GPUTexture.h"
#include "Runtime/Graphics/Base/GPUBuffer.h"
#include "Runtime/Core/Types/Object.h"

namespace SE
{
	GPUUploadTextureMipTask::GPUUploadTextureMipTask(GPUTexture* texture, int32 mipIndex, Span<byte> data, int32 rowPitch, int32 slicePitch, bool copyData)
		: GPUTask(Type::UploadTexture)
		, m_Texture(texture)
		, _mipIndex(mipIndex)
		, _rowPitch(rowPitch)
		, _slicePitch(slicePitch)
	{
		m_Texture.Released.Bind<GPUUploadTextureMipTask, &GPUUploadTextureMipTask::OnResourceReleased>(this);

		if (copyData)
			_data.Copy(data);
		else
			_data.Link(data);
	}

	GPUTask::Result GPUUploadTextureMipTask::run(GPUTasksContext* context)
	{
		const auto texture = m_Texture.Get();
		if (texture == nullptr)
			return Result::MissingResources;
		ENGINE_ASSERT(texture->IsAllocated());

		// Update all array slices
		const byte* dataSource = _data.Get();
		const int32 arraySize = texture->ArraySize();
		ENGINE_ASSERT(_data.Length() >= _slicePitch * arraySize);
		for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
		{
			context->GPU->UpdateTexture(texture, arrayIndex, _mipIndex, dataSource, _rowPitch, _slicePitch);
			dataSource += _slicePitch;
		}

		return Result::Ok;
	}

	void GPUUploadTextureMipTask::OnSync()
	{
		auto texture = m_Texture.Get();
		if (texture)
		{
			// Check if the new mips has been just uploaded
			if (_mipIndex == texture->HighestResidentMipIndex() - 1)
			{
				// Mark as mip loaded
				texture->SetResidentMipLevels(texture->ResidentMipLevels() + 1);
			}
		}

		// Base
		GPUTask::OnSync();
	}

	void GPUUploadTextureMipTask::OnEnd()
	{
		m_Texture.Unlink();

		// Base
		GPUTask::OnEnd();
	}

	bool GPUUploadTextureMipTask::HasReference(void* resource) const
	{
		return m_Texture == static_cast<Object*>(resource);
	}



	GPUUploadBufferTask::GPUUploadBufferTask(GPUBuffer* buffer, int32 offset, Span<byte> data, bool copyData)
		: GPUTask(Type::UploadBuffer)
		, _buffer(buffer)
		, _offset(offset)
	{
		_buffer.Released.Bind<GPUUploadBufferTask, &GPUUploadBufferTask::OnResourceReleased>(this);

		if (copyData)
			_data.Copy(data);
		else
			_data.Link(data);
	}

	void GPUUploadBufferTask::OnResourceReleased()
	{
		Cancel();
	}

	GPUTask::Result GPUUploadBufferTask::run(GPUTasksContext* context)
	{
		if (!_buffer)
			return Result::MissingResources;
		context->GPU->UpdateBuffer(_buffer, _data.Get(), _data.Length(), _offset);
		return Result::Ok;
	}

	void GPUUploadBufferTask::OnEnd()
	{
		_buffer.Unlink();

		// Base
		GPUTask::OnEnd();
	}

	bool GPUUploadBufferTask::HasReference(void* resource) const
	{
		return _buffer == reinterpret_cast<Object*>(resource);
	}
}

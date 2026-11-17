#include "MeshBase.h"

#include "Runtime/Core/Profiler/ProfilerCPU.h"
#include "Runtime/Core/Serialization/MemoryReadStream.h"
#include "Runtime/Core/Thread/Threading.h"

#include "Runtime/Graphics/Base/GPUBuffer.h"

#include "MeshDataLayout.h"
#include "ModelBase.h"
#include "ModelData.h"

namespace SE
{
	void MeshBase::SetMaterialSlotIndex(int32 value)
	{
		if (value < 0 || value >= _model->MaterialSlots.Count())
		{
			LOG_WARNING("Render", "Cannot set mesh material slot to {0} while model has {1} slots.", value, _model->MaterialSlots.Count());
			return;
		}

		_materialSlotIndex = value;
	}

	void MeshBase::SetBounds(const BoundingBox& box)
	{
		_box = box;
		BoundingSphere::FromBox(box, _sphere);
	}

	bool MeshBase::DownloadDataGPU(MeshBufferType type, BytesContainer& result) const
    {
        GPUBuffer* buffer = nullptr;
        switch (type)
        {
        case MeshBufferType::Index:
            buffer = _indexBuffer;
            break;
        case MeshBufferType::Vertex0:
            buffer = _vertexBuffers[0];
            break;
        case MeshBufferType::Vertex1:
            buffer = _vertexBuffers[1];
            break;
        case MeshBufferType::Vertex2:
            buffer = _vertexBuffers[2];
            break;
        }
        return buffer && buffer->DownloadData(result);
    }

    Threading::Task* MeshBase::DownloadDataGPUAsync(MeshBufferType type, BytesContainer& result) const
    {
        GPUBuffer* buffer = nullptr;
        switch (type)
        {
        case MeshBufferType::Index:
            buffer = _indexBuffer;
            break;
        case MeshBufferType::Vertex0:
            buffer = _vertexBuffers[0];
            break;
        case MeshBufferType::Vertex1:
            buffer = _vertexBuffers[1];
            break;
        case MeshBufferType::Vertex2:
            buffer = _vertexBuffers[2];
            break;
        }
        return buffer ? buffer->DownloadDataAsync(result) : nullptr;
    }

    bool MeshBase::DownloadDataCPU(MeshBufferType type, BytesContainer& result, int32& count) const
    {
        result.Release();
        count = 0;
        ModelBase* model = GetModelBase();
        if (model == nullptr)
        {
            return false;
        }
        Threading::ScopeLock lock(model->Locker);
        if (_cachedVertexBuffer[0].IsInvalid())
        {
            PROFILE_CPU();
            if (model->IsVirtual())
            {
                LOG_ERROR("Render", "Cannot access CPU data of virtual models. Use GPU data download.");
                return false;
            }

            // Fetch chunk with data from drive/memory
            const auto chunkIndex = MODEL_LOD_TO_CHUNK_INDEX(GetLODIndex());
            if (!model->LoadChunk(chunkIndex))
            {
                return false;
            }
            const auto chunk = model->GetChunk(chunkIndex);
            if (!chunk)
            {
                LOG_ERROR("Render", "Missing chunk.");
                return false;
            }

            MemoryReadStream stream(chunk->Get(), chunk->Size());

            // Seek to find mesh location
            for (int32 meshIndex = 0; meshIndex <= _index; meshIndex++)
            {
                // #MODEL_DATA_FORMAT_USAGE
                if (stream.GetLength() - stream.GetPosition() < sizeof(uint32) * 2)
                {
                    return false;
                }
                uint32 vertices;
                stream.ReadUint32(&vertices);
                uint32 triangles;
                stream.ReadUint32(&triangles);
                if (vertices == 0 || triangles == 0 ||
                    vertices > Max_int32 / sizeof(VB1ElementType) || triangles > Max_int32 / (3 * sizeof(uint32)))
                {
                    LOG_ERROR("Render", "Invalid mesh data.");
                    return false;
                }
                uint32 indicesCount = triangles * 3;
                bool use16BitIndexBuffer = indicesCount <= Max_uint16;
                uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
                uint64 vertexBytes = static_cast<uint64>(vertices) * (sizeof(VB0ElementType) + sizeof(VB1ElementType));
                if (vertexBytes + sizeof(bool) > stream.GetLength() - stream.GetPosition())
                {
                    return false;
                }
                auto vb0 = stream.Move<VB0ElementType>(vertices);
                auto vb1 = stream.Move<VB1ElementType>(vertices);
                bool hasColors = stream.ReadBool();
                uint64 remainingBytes = static_cast<uint64>(indicesCount) * ibStride;
                if (hasColors)
                {
                    remainingBytes += static_cast<uint64>(vertices) * sizeof(VB2ElementType);
                }
                if (remainingBytes > stream.GetLength() - stream.GetPosition())
                {
                    return false;
                }
                VB2ElementType* vb2 = nullptr;
                if (hasColors)
                {
                    vb2 = stream.Move<VB2ElementType>(vertices);
                }
                auto ib = stream.Move<byte>(indicesCount * ibStride);

                if (meshIndex != _index)
                    continue;

                // Own cached bytes: saving or replacing the source chunk must not invalidate future reads.
                _cachedIndexBufferCount = indicesCount;
                _cachedVertexBufferCount = vertices;
                _cachedIndexBuffer.Copy(ib, indicesCount * ibStride);
                _cachedVertexBuffer[0].Copy((const byte*)vb0, vertices * sizeof(VB0ElementType));
                _cachedVertexBuffer[1].Copy((const byte*)vb1, vertices * sizeof(VB1ElementType));
                if (hasColors)
                {
                    _cachedVertexBuffer[2].Copy((const byte*)vb2, vertices * sizeof(VB2ElementType));
                }
                break;
            }
        }

        switch (type)
        {
        case MeshBufferType::Index:
            result.Copy(_cachedIndexBuffer);
            count = _cachedIndexBufferCount;
            break;
        case MeshBufferType::Vertex0:
            result.Copy(_cachedVertexBuffer[0]);
            count = _cachedVertexBufferCount;
            break;
        case MeshBufferType::Vertex1:
            result.Copy(_cachedVertexBuffer[1]);
            count = _cachedVertexBufferCount;
            break;
        case MeshBufferType::Vertex2:
            result.Copy(_cachedVertexBuffer[2]);
            count = _cachedVertexBuffer[2].Length() / sizeof(VB2ElementType);
            break;
        default:
            return false;
        }
        return true;
    }

}

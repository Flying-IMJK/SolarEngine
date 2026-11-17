#include <Runtime/Render/Assets/Geometry/MeshAccessor.h>

#include <Runtime/Core/Math/Half.h>
#include <Runtime/Core/Thread/Threading.h>
#include <Runtime/Graphics/Base/GPUBuffer.h>
#include <Runtime/Render/Assets/Geometry/Mesh.h>
#include <Runtime/Render/Assets/Geometry/Model.h>
#include <Runtime/Render/Assets/Geometry/StaticMeshVertexFactory.h>

namespace SE
{
    bool MeshAccessor::LoadMesh(Mesh* pMesh, bool forceGpu)
    {
        m_Layout = nullptr;
        m_Indices.Clear();
        m_VertexCount = 0;
        m_LightmapUVsIndex = -1;
        for (BytesContainer& buffer : m_VertexBuffers)
        {
            buffer.Release();
        }
        if (Platform::AtomicRead(&m_CancelRequested) != 0)
        {
            return false;
        }
        if (pMesh == nullptr || Threading::IsMainThread())
        {
            LOG_WARNING("Render", "Mesh data snapshots must be loaded from a valid mesh on a worker thread.");
            return false;
        }
        Model* model = pMesh->GetModel();
        if (model == nullptr || !model->WaitForLoaded())
        {
            return false;
        }

        const VertexFactoryLayout& layout = StaticMeshVertexFactory::GetLayout();
        bool requiredStreams[3] = {};
        for (int32 channel = 0; channel < 4; channel++)
        {
            const VertexFactoryInputElement* element = layout.FindElement(SE_TEXT("TEXCOORD"), channel);
            if (element == nullptr)
            {
                continue;
            }
            const VertexFactoryBufferBinding* binding = layout.FindBinding(element->Slot);
            if (element->Slot >= 3 || binding == nullptr || binding->InputRate != VertexFactoryInputRate::PerVertex ||
                element->Format != PixelFormat::R16G16_Float || element->Offset + sizeof(Half2) > binding->Stride)
            {
                LOG_WARNING("Render", "Unsupported mesh UV layout for channel {0}.", channel);
                return false;
            }
            requiredStreams[element->Slot] = true;
        }

        BytesContainer indexData;
        int32 indexCount = 0;
        int32 indexStride = 0;
        // Hold the model lock while acquiring source buffers; GPU tasks own their resource references after submission.
        model->Locker.Lock();
        m_LightmapUVsIndex = pMesh->GetLightmapUVsIndex();
        forceGpu |= model->IsVirtual();
        if (forceGpu)
        {
            GPUBuffer* indexBuffer = pMesh->GetIndexBuffer();
            bool success = indexBuffer != nullptr;
            for (uint32 slot = 0; slot < 3; slot++)
            {
                if (requiredStreams[slot])
                {
                    GPUBuffer* buffer = pMesh->GetVertexBuffer(slot);
                    const VertexFactoryBufferBinding* binding = layout.FindBinding(slot);
                    success &= buffer != nullptr && buffer->GetStride() == binding->Stride;
                }
            }
            if (!success)
            {
                model->Locker.Unlock();
                LOG_WARNING("Render", "Mesh GPU buffers are not ready for UV readback.");
                return false;
            }
            indexCount = static_cast<int32>(indexBuffer->GetElementsCount());
            indexStride = static_cast<int32>(indexBuffer->GetStride());
            m_LoadLock.Lock();
            success = Platform::AtomicRead(&m_CancelRequested) == 0;
            for (int32 bufferIndex = 0; bufferIndex < 4; bufferIndex++)
            {
                if (!success)
                {
                    break;
                }
                if (bufferIndex != 0 && !requiredStreams[bufferIndex - 1])
                {
                    continue;
                }
                BytesContainer& target = bufferIndex == 0 ? indexData : m_VertexBuffers[bufferIndex - 1];
                Threading::Task* task = pMesh->DownloadDataGPUAsync(static_cast<MeshBufferType>(bufferIndex), target);
                if (task == nullptr)
                {
                    success = false;
                    break;
                }
                task->Start();
                m_Tasks.Add(task);
            }
            m_LoadLock.Unlock();
            model->Locker.Unlock();
            // SolarEngine Task::Wait returns true on success. Drain every submitted task even after a failure.
            for (Threading::Task* task : m_Tasks)
            {
                success &= task->Wait();
            }
            m_LoadLock.Lock();
            m_Tasks.Clear();
            m_LoadLock.Unlock();
            success &= Platform::AtomicRead(&m_CancelRequested) == 0;
            if (!success)
            {
                if (Platform::AtomicRead(&m_CancelRequested) == 0)
                {
                    LOG_WARNING("Render", "Failed to read mesh UV buffers from the GPU.");
                }
                return false;
            }
            for (uint32 slot = 0; slot < 3; slot++)
            {
                if (requiredStreams[slot])
                {
                    const VertexFactoryBufferBinding* binding = layout.FindBinding(slot);
                    if (m_VertexBuffers[slot].Length() % binding->Stride != 0)
                    {
                        return false;
                    }
                    int32 count = static_cast<int32>(m_VertexBuffers[slot].Length() / binding->Stride);
                    if (m_VertexCount != 0 && m_VertexCount != count)
                    {
                        return false;
                    }
                    m_VertexCount = count;
                }
            }
        }
        else
        {
            bool success = pMesh->DownloadDataCPU(MeshBufferType::Index, indexData, indexCount);
            for (uint32 slot = 0; slot < 3 && success; slot++)
            {
                if (requiredStreams[slot])
                {
                    int32 count = 0;
                    success = pMesh->DownloadDataCPU(static_cast<MeshBufferType>(slot + 1), m_VertexBuffers[slot], count);
                    success &= m_VertexCount == 0 || m_VertexCount == count;
                    m_VertexCount = count;
                }
            }
            model->Locker.Unlock();
            if (!success)
            {
                LOG_WARNING("Render", "Failed to read mesh UV buffers from the CPU.");
                return false;
            }
            indexStride = indexCount > 0 ? indexData.Length() / indexCount : 0;
        }

        if (indexCount <= 0 || indexCount % 3 != 0 || m_VertexCount <= 0 ||
            (indexStride != sizeof(uint16) && indexStride != sizeof(uint32)) ||
            static_cast<int64>(indexCount) * indexStride != indexData.Length())
        {
            LOG_WARNING("Render", "Invalid mesh index or UV buffer size.");
            return false;
        }
        m_Indices.Resize(indexCount);
        for (int32 i = 0; i < indexCount; i++)
        {
            uint32 index;
            if (indexStride == sizeof(uint16))
            {
                uint16 value;
                Platform::MemoryCopy(&value, indexData.Get() + i * indexStride, sizeof(value));
                index = value;
            }
            else
            {
                Platform::MemoryCopy(&index, indexData.Get() + i * indexStride, sizeof(index));
            }
            if (index >= static_cast<uint32>(m_VertexCount))
            {
                m_Indices.Clear();
                LOG_WARNING("Render", "Mesh index exceeds its UV vertex buffer.");
                return false;
            }
            m_Indices[i] = index;
        }
        if (Platform::AtomicRead(&m_CancelRequested) != 0)
        {
            return false;
        }
        m_Layout = &layout;
        return true;
    }

    void MeshAccessor::CancelLoad()
    {
        Platform::AtomicStore(&m_CancelRequested, 1);
        Threading::ScopeLock lock(m_LoadLock);
        for (Threading::Task* task : m_Tasks)
        {
            // Once a copy has finished, its CPU continuation can complete without another render frame.
            if (!task->IsEnded())
            {
                ENGINE_ASSERT(Threading::IsMainThread());
                task->Cancel();
            }
        }
    }

    bool MeshAccessor::GetTexCoords(int32 channel, List<Float2>& texCoords) const
    {
        texCoords.Clear();
        if (m_Layout == nullptr || channel < 0 || channel >= 4 || (channel == 1 && m_LightmapUVsIndex == -1))
        {
            return false;
        }
        const VertexFactoryInputElement* element = m_Layout->FindElement(SE_TEXT("TEXCOORD"), channel);
        if (element == nullptr || element->Slot >= 3)
        {
            return false;
        }
        const VertexFactoryBufferBinding* binding = m_Layout->FindBinding(element->Slot);
        const BytesContainer& buffer = m_VertexBuffers[element->Slot];
        if (binding == nullptr || element->Format != PixelFormat::R16G16_Float ||
            static_cast<int64>(m_VertexCount) * binding->Stride != buffer.Length())
        {
            return false;
        }
        texCoords.Resize(m_VertexCount);
        for (int32 i = 0; i < m_VertexCount; i++)
        {
            Half2 value;
            Platform::MemoryCopy(&value, buffer.Get() + i * binding->Stride + element->Offset, sizeof(value));
            texCoords[i] = value.ToFloat2();
        }
        return true;
    }
}

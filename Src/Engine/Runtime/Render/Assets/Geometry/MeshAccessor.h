#pragma once

#include <Runtime/Core/Scripting/ScriptingObject.h>
#include <Runtime/Core/Platform/CriticalSection.h>
#include <Runtime/Core/Types/Collections/DataContainer.h>
#include <Runtime/Core/Types/Collections/List.h>
#include <Runtime/Core/Math/Vector2.h>

namespace SE
{
    namespace Threading
    {
        class Task;
    }

    class Mesh;
    class VertexFactoryLayout;

    /// <summary>
    /// Owns a read-only snapshot of static mesh indices and UV vertex streams.
    /// The caller owns this object and must finish loading before reading or destroying it.
    /// </summary>
    SE_CLASS(API())
    class SE_API_RUNTIME MeshAccessor : public ScriptingObject
    {
        SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(MeshAccessor, ScriptingObject)

    private:
        BytesContainer m_VertexBuffers[3];
        List<uint32> m_Indices;
        const VertexFactoryLayout* m_Layout = nullptr;
        int32 m_VertexCount = 0;
        int32 m_LightmapUVsIndex = -1;
        CriticalSection m_LoadLock;
        List<Threading::Task*> m_Tasks;
        volatile int64 m_CancelRequested = 0;

    public:
        /// <summary>
        /// Loads owned mesh data on a worker thread. Ordinary assets use CPU data unless forceGpu is true;
        /// virtual assets always use GPU readback. Returns true on success. Do not reload the model until this call ends.
        /// </summary>
        SE_FUNCTION(API())
        bool LoadMesh(Mesh* pMesh, bool forceGpu = false);

        /// <summary>
        /// Cancels pending GPU copies from the main thread before waiting for LoadMesh to finish.
        /// Before loading starts this may also be called by a canceled worker registration.
        /// A canceled accessor cannot be loaded again. The caller must still wait before destroying it.
        /// </summary>
        SE_FUNCTION(API())
        void CancelLoad();

        /// <summary>Gets the snapshot indices, converted from either 16-bit or 32-bit source data.</summary>
        SE_FUNCTION(API(Prop))
        const List<uint32>& GetIndexBuffer() const { return m_Indices; }

        /// <summary>Gets the lightmap UV channel of this snapshot, or -1 if absent.</summary>
        SE_FUNCTION(API(Prop))
        int32 GetLightmapUVsIndex() const { return m_LightmapUVsIndex; }

        /// <summary>
        /// Copies one UV channel into caller-owned values. Returns false if the channel is absent or unsupported.
        /// Uses the captured VertexFactory layout and never reads the source mesh again.
        /// </summary>
        SE_FUNCTION(API())
        bool GetTexCoords(int32 channel, SE_PARAM(API(Out)) List<Float2>& texCoords) const;
    };
}

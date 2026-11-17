using System;
using System.Threading;
using System.Threading.Tasks;

namespace SE.Editor
{
    /// <summary>Owns asynchronous mesh snapshots; complete reads before the native model can reload.</summary>
    internal sealed class MeshDataCache : IDisposable
    {
        internal sealed class MeshData
        {
            public MeshBase Mesh;
            public MeshAccessor VertexAccessor;
            public Float2[][] TexCoords;
            public uint[] Indices;
            public int LightmapUVsIndex;
        }

        private Task<MeshData[][]> m_Request;
        private CancellationTokenSource m_Cancellation;
        private ModelBase m_Model;
        private bool m_ForceGpu;

        public MeshData[][] MeshDatas { get; private set; }
        public string Error { get; private set; }

        public bool RequestMeshData(ModelBase model, bool forceGpu = false)
        {
            if (model == null)
            {
                throw new ArgumentNullException(nameof(model));
            }
            if (m_Model != model || m_ForceGpu != forceGpu)
            {
                Dispose();
                m_Model = model;
                m_ForceGpu = forceGpu;
            }
            if (m_Request == null && MeshDatas == null && Error == null)
            {
                m_Cancellation = new CancellationTokenSource();
                CancellationToken cancellation = m_Cancellation.Token;
                m_Request = Task.Run(() =>
                {
                    MeshData[][] data = null;
                    try
                    {
                        cancellation.ThrowIfCancellationRequested();
                        if (!model.WaitForLoaded())
                        {
                            throw new InvalidOperationException("Model loading failed.");
                        }
                        data = new MeshData[model.GetLODsCount()][];
                        for (int lod = 0; lod < data.Length; lod++)
                        {
                            cancellation.ThrowIfCancellationRequested();
                            model.GetMeshes(out MeshBase[] meshes, lod);
                            data[lod] = new MeshData[meshes.Length];
                            for (int meshIndex = 0; meshIndex < meshes.Length; meshIndex++)
                            {
                                cancellation.ThrowIfCancellationRequested();
                                if (meshes[meshIndex] is not Mesh mesh)
                                {
                                    throw new InvalidOperationException("Unsupported model mesh type.");
                                }
                                MeshData entry = data[lod][meshIndex] = new MeshData
                                {
                                    Mesh = mesh,
                                    VertexAccessor = Object.New<MeshAccessor>(),
                                    TexCoords = new Float2[4][],
                                };
                                if (entry.VertexAccessor == null)
                                {
                                    throw new InvalidOperationException($"Failed to read LOD {lod}, mesh {meshIndex}.");
                                }
                                using (CancellationTokenRegistration registration = cancellation.Register(entry.VertexAccessor.CancelLoad))
                                {
                                    bool loaded = entry.VertexAccessor.LoadMesh(mesh, forceGpu);
                                    cancellation.ThrowIfCancellationRequested();
                                    if (!loaded)
                                    {
                                        throw new InvalidOperationException($"Failed to read LOD {lod}, mesh {meshIndex}.");
                                    }
                                }
                                entry.Indices = entry.VertexAccessor.IndexBuffer;
                                entry.LightmapUVsIndex = entry.VertexAccessor.LightmapUVsIndex;
                                // Download buffers once. Decode each available channel in bulk on this worker.
                                for (int channel = 0; channel < entry.TexCoords.Length; channel++)
                                {
                                    cancellation.ThrowIfCancellationRequested();
                                    if (entry.VertexAccessor.GetTexCoords(channel, out Float2[] uvs))
                                    {
                                        entry.TexCoords[channel] = uvs;
                                    }
                                }
                            }
                        }
                        cancellation.ThrowIfCancellationRequested();
                        return data;
                    }
                    catch
                    {
                        DisposeMeshData(data);
                        throw;
                    }
                }, cancellation);
            }
            if (m_Request != null && m_Request.IsCompleted)
            {
                try
                {
                    MeshDatas = m_Request.GetAwaiter().GetResult();
                }
                catch (Exception ex)
                {
                    Error = "Failed to read mesh data: " + ex.Message;
                    Debug.LogWarning(ex);
                }
                m_Request = null;
                m_Cancellation.Dispose();
                m_Cancellation = null;
            }
            return MeshDatas != null;
        }

        public void WaitForMeshDataRequestEnd()
        {
            if (m_Request != null)
            {
                m_Cancellation.Cancel();
                try
                {
                    // A completed request may not have been published by RequestMeshData yet.
                    DisposeMeshData(m_Request.GetAwaiter().GetResult());
                }
                catch (OperationCanceledException)
                {
                }
                catch (Exception ex)
                {
                    Debug.LogWarning(ex);
                }
                m_Request = null;
                m_Cancellation.Dispose();
                m_Cancellation = null;
            }
        }

        public void Dispose()
        {
            WaitForMeshDataRequestEnd();
            DisposeMeshData(MeshDatas);
            MeshDatas = null;
            Error = null;
            m_Model = null;
            m_ForceGpu = false;
        }

        private static void DisposeMeshData(MeshData[][] data)
        {
            if (data == null)
            {
                return;
            }
            foreach (MeshData[] lod in data)
            {
                if (lod == null)
                {
                    continue;
                }
                foreach (MeshData entry in lod)
                {
                    if (entry != null)
                    {
                        Object.DestroyNow(entry.VertexAccessor);
                        entry.VertexAccessor = null;
                        entry.Mesh = null;
                    }
                }
            }
        }
    }
}

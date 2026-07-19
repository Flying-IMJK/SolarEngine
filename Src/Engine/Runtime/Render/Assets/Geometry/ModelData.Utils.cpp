
#include "ModelData.h"
#include "Runtime/Core/Profiler/ProfilerCPU.h"
#include "Runtime/Core/Types/Collections/BitArray.h"
#include "Runtime/Utilities/Model/VertexTriangleAdjacency.h"

#include "assimp/SpatialSort.h"

namespace SE
{
    int32 FindVertex(const MeshData& mesh, int32 vertexIndex, int32 startIndex, int32 searchRange,
        const List<int32>& mapping, const Assimp::SpatialSort& spatialSort, std::vector<unsigned int>& sparialSortCache)
    {
        const float uvEpsSqr = (1.0f / 250.0f) * (1.0f / 250.0f);

        const Float3 vPosition = mesh.Positions[vertexIndex];
        spatialSort.FindPositions(*(aiVector3D*)&vPosition, 1e-4f, sparialSortCache);
        if (sparialSortCache.empty())
            return INVALID_INDEX;

        const Float2 vUV = mesh.UVs.HasItems() ? mesh.UVs[vertexIndex] : Float2::Zero;
        const Float3 vNormal = mesh.Normals.HasItems() ? mesh.Normals[vertexIndex] : Float3::Zero;
        const Float3 vTangent = mesh.Tangents.HasItems() ? mesh.Tangents[vertexIndex] : Float3::Zero;
        const Float2 vLightmapUV = mesh.LightmapUVs.HasItems() ? mesh.LightmapUVs[vertexIndex] : Float2::Zero;
        const Color vColor = mesh.Colors.HasItems() ? mesh.Colors[vertexIndex] : Colors::Black; // Assuming Color::Black as a default color

        const int32 end = startIndex + searchRange;

        for (size_t i = 0; i < sparialSortCache.size(); i++)
        {
            const int32 v = sparialSortCache[i];
            if (v < startIndex || v >= end)
                continue;

            if (mapping[v] == INVALID_INDEX)
                continue;
            if (mesh.UVs.HasItems() && (vUV - mesh.UVs[v]).LengthSquared() > uvEpsSqr)
                continue;
            if (mesh.Normals.HasItems() && Float3::Dot(vNormal, mesh.Normals[v]) < 0.98f)
                continue;
            if (mesh.Tangents.HasItems() && Float3::Dot(vTangent, mesh.Tangents[v]) < 0.98f)
                continue;
            if (mesh.LightmapUVs.HasItems() && (vLightmapUV - mesh.LightmapUVs[v]).LengthSquared() > uvEpsSqr)
                continue;
            if (mesh.Colors.HasItems() && vColor != mesh.Colors[v])
                continue;
            // TODO: check more components?

            return v;
        }

        return INVALID_INDEX;
    }

    template<typename T>
    void RemapBuffer(List<T>& src, List<T>& dst, const List<int32>& mapping, int32 newSize)
    {
        if (src.IsEmpty())
            return;

        dst.Resize(newSize, false);

        for (int32 i = 0, j = 0; i < src.Count(); i++)
        {
            const auto idx = mapping[i];
            if (idx != INVALID_INDEX)
            {
                dst[j++] = src[i];
            }
        }
    }


    void MeshData::BuildIndexBuffer()
    {
        PROFILE_CPU();
        const auto startTime = Platform::GetTimeSeconds();

        const int32 vertexCount = Positions.Count();
        MeshData newMesh;
        newMesh.Indices.EnsureCapacity(vertexCount);
        List<int32> mapping;
        mapping.Resize(vertexCount);
        int32 newVertexCounter = 0;


        // Set up a SpatialSort to quickly find all vertices close to a given position
        Assimp::SpatialSort vertexFinder;
        vertexFinder.Fill((const aiVector3D*)Positions.Get(), vertexCount, sizeof(Float3));
        std::vector<unsigned int> sparialSortCache;

        // Build index buffer
        for (int32 vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++)
        {
            // Find duplicated vertex before the current one
            const int32 reuseVertexIndex = FindVertex(*this, vertexIndex, 0, vertexIndex, mapping, vertexFinder, sparialSortCache);
            if (reuseVertexIndex == INVALID_INDEX)
            {
                newMesh.Indices.Add(newVertexCounter);
                mapping[vertexIndex] = newVertexCounter;
                newVertexCounter++;
            }
            else
            {
                // Remove vertex
                const int32 mapped = mapping[reuseVertexIndex];
                ENGINE_ASSERT(mapped != INVALID_INDEX && mapped < vertexIndex);
                newMesh.Indices.Add(mapped);
                mapping[vertexIndex] = INVALID_INDEX;
            }
        }

        // Skip if no change
        if (vertexCount == newVertexCounter)
            return;

        // Remove unused vertices
        newMesh.SwapBuffers(*this);
#define REMAP_BUFFER(name) RemapBuffer(newMesh.name, name, mapping, newVertexCounter)
        REMAP_BUFFER(Positions);
        REMAP_BUFFER(UVs);
        REMAP_BUFFER(Normals);
        REMAP_BUFFER(Tangents);
        REMAP_BUFFER(BitangentSigns);
        REMAP_BUFFER(LightmapUVs);
        REMAP_BUFFER(Colors);
        REMAP_BUFFER(BlendIndices);
        REMAP_BUFFER(BlendWeights);
#undef REMAP_BUFFER
        BlendShapes.Resize(newMesh.BlendShapes.Count());
        for (int32 blendShapeIndex = 0; blendShapeIndex < newMesh.BlendShapes.Count(); blendShapeIndex++)
        {
            auto& srcBlendShape = newMesh.BlendShapes[blendShapeIndex];
            auto& dstBlendShape = BlendShapes[blendShapeIndex];

            dstBlendShape.Name = srcBlendShape.Name;
            dstBlendShape.Weight = srcBlendShape.Weight;
            dstBlendShape.Vertices.Resize(newVertexCounter);
            for (int32 i = 0, j = 0; i < srcBlendShape.Vertices.Count(); i++)
            {
                const auto idx = mapping[i];
                if (idx != INVALID_INDEX)
                {
                    auto& v = srcBlendShape.Vertices[i];
                    ASSERT_LOW_LAYER(v.VertexIndex < (uint32)vertexCount);
                    ASSERT_LOW_LAYER(mapping[v.VertexIndex] != INVALID_INDEX);
                    v.VertexIndex = mapping[v.VertexIndex];
                    ASSERT_LOW_LAYER(v.VertexIndex < (uint32)newVertexCounter);
                    dstBlendShape.Vertices[j++] = v;
                }
            }
        }

        const auto endTime = Platform::GetTimeSeconds();
        const double time = Math::RoundTo2DecimalPlaces(endTime - startTime);
        if (time > 0.5f) // Don't log if generation was fast enough
            LOG_INFO("Render", "Generated {3} for mesh in {0}s ({1} vertices, {2} indices)", time, vertexCount, Indices.Count(), SE_TEXT("indices"));
    }

    void MeshData::FindPositions(const Float3& position, float epsilon, List<int32>& result)
    {
        result.Clear();

        for (int32 i = 0; i < Positions.Count(); i++)
        {
            if (Float3::NearEqual(position, Positions[i], epsilon))
                result.Add(i);
        }
    }

    bool MeshData::GenerateNormals(float smoothingAngle)
    {
        if (Positions.IsEmpty() || Indices.IsEmpty())
        {
            LOG_WARNING("Render", "Missing vertex or index data to generate normals.");
            return false;
        }
        PROFILE_CPU();

        const auto startTime = Platform::GetTimeSeconds();

        const int32 vertexCount = Positions.Count();
        const int32 indexCount = Indices.Count();
        Normals.Resize(vertexCount, false);
        smoothingAngle = Math::Clamp(smoothingAngle, 0.0f, 175.0f);

        // Compute per-face normals but store them per-vertex
        Float3 min, max;
        min = max = Positions[0];
        for (int32 i = 0; i < indexCount; i += 3)
        {
            const Float3 v1 = Positions[Indices[i + 0]];
            const Float3 v2 = Positions[Indices[i + 1]];
            const Float3 v3 = Positions[Indices[i + 2]];
            const Float3 n = ((v2 - v1) ^ (v3 - v1));

            Normals[Indices[i + 0]] = n;
            Normals[Indices[i + 1]] = n;
            Normals[Indices[i + 2]] = n;

            Float3::Min(min, v1, min);
            Float3::Min(min, v2, min);
            Float3::Min(min, v3, min);

            Float3::Max(max, v1, max);
            Float3::Max(max, v2, max);
            Float3::Max(max, v3, max);
        }

        // Set up a SpatialSort to quickly find all vertices close to a given position
        Assimp::SpatialSort vertexFinder;
        vertexFinder.Fill((const aiVector3D*)Positions.Get(), vertexCount, sizeof(Float3));
        std::vector<uint32> verticesFound(16);

        const float posEpsilon = (max - min).Length() * 1e-4f;

        // Check if use the angle limit (then use the faster path)
        if (smoothingAngle >= 175.0f)
        {
            BitArray<> used;
            used.Resize(vertexCount);
            used.SetAll(false);

            for (int32 i = 0; i < vertexCount; i++)
            {
                if (used[i])
                    continue;

                // Get all vertices that share this one
                vertexFinder.FindPositions(*(aiVector3D*)&Positions[i], posEpsilon, verticesFound);
                const int32 verticesFoundCount = (int32)verticesFound.size();

                // Get the smooth normal
                Float3 n;
                for (int32 a = 0; a < verticesFoundCount; a++)
                    n += Normals[verticesFound[a]];
                n.Normalize();

                // Write the smoothed normal back to all affected normals
                for (int32 a = 0; a < verticesFoundCount; a++)
                {
                    const auto vtx = verticesFound[a];
                    Normals[vtx] = n;
                    used.Set(vtx, true);
                }
            }
        }
        else
        {
            const float limit = cosf(smoothingAngle * Math::DegreesToRadians);

            for (int32 i = 0; i < vertexCount; i++)
            {
                // Get all vertices that share this one
                vertexFinder.FindPositions(*(aiVector3D*)&Positions[i], posEpsilon, verticesFound);
                const int32 verticesFoundCount = (int32)verticesFound.size();


                // Get the smooth normal
                Float3 vr = Normals[i];
                const float vrlen = vr.Length();
                Float3 n;
                for (int32 a = 0; a < verticesFoundCount; a++)
                {
                    Float3 v = Normals[verticesFound[a]];

                    // Check whether the angle between the two normals is not too large
                    // TODO: use length squared?
                    if (v * vr >= limit * vrlen * v.Length())
                        n += v;
                }
                Normals[i] = Float3::Normalize(n);
            }
        }

        const auto endTime = Platform::GetTimeSeconds();
        const double time = Math::RoundTo2DecimalPlaces(endTime - startTime);
        if (time > 0.5f) // Don't log if generation was fast enough
            LOG_INFO("Render", "Generated {3} for mesh in {0}s ({1} vertices, {2} indices)", time, vertexCount, indexCount, SE_TEXT("normals"));

        return true;
    }

    bool MeshData::GenerateTangents(float smoothingAngle)
    {
        if (Positions.IsEmpty() || Indices.IsEmpty())
        {
            LOG_WARNING("Render", "Missing vertex or index data to generate tangents.");
            return false;
        }
        if (Normals.IsEmpty() || UVs.IsEmpty())
        {
            LOG_WARNING("Render", "Missing normals or texcoors data to generate tangents.");
            return false;
        }
        PROFILE_CPU();

        const auto startTime = Platform::GetTimeSeconds();
        const int32 vertexCount = Positions.Count();
        const int32 indexCount = Indices.Count();
        Tangents.Resize(vertexCount, false);
        smoothingAngle = Math::Clamp(smoothingAngle, 0.0f, 45.0f);

#if USE_MIKKTSPACE
        SMikkTSpaceInterface callbacks = {
            GetNumFaces,
            GetNumVerticesOfFace,
            GetPosition,
            GetNormal,
            GetTexCoord,
            SetTSpaceBasic,
            nullptr
        };
        const SMikkTSpaceContext context = { &callbacks, this };
        BitangentSigns.Resize(vertexCount, false);
        genTangSpace(&context, 180.0f - smoothingAngle);
#else
        const float angleEpsilon = 0.9999f;
        BitArray<> vertexDone;
        vertexDone.Resize(vertexCount);
        vertexDone.SetAll(false);

        const Float3* meshNorm = Normals.Get();
        const Float2* meshTex = UVs.Get();
        Float3* meshTang = Tangents.Get();

        // Calculate the tangent per-triangle
        Float3 min, max;
        min = max = Positions[Indices[0]];
        for (int32 i = 0; i < indexCount; i += 3)
        {
            const int32 p0 = Indices[i + 0], p1 = Indices[i + 1], p2 = Indices[i + 2];

            const Float3 v0 = Positions[p0];
            const Float3 v1 = Positions[p1];
            const Float3 v2 = Positions[p2];

            Float3::Min(min, v0, min);
            Float3::Min(min, v1, min);
            Float3::Min(min, v2, min);

            Float3::Max(max, v0, max);
            Float3::Max(max, v1, max);
            Float3::Max(max, v2, max);

            // Position differences p1->p2 and p1->p3
            Float3 v = v1 - v0, w = v2 - v0;

            // Texture offset p1->p2 and p1->p3
            float sx = meshTex[p1].x - meshTex[p0].x, sy = meshTex[p1].y - meshTex[p0].y;
            float tx = meshTex[p2].x - meshTex[p0].x, ty = meshTex[p2].y - meshTex[p0].y;
            const float dir = (tx * sy - ty * sx) < 0.0f ? -1.0f : 1.0f;
            if (sx * ty == sy * tx)
            {
                // Use default UV direction for invalid case
                sx = 0.0;
                sy = 1.0;
                tx = 1.0;
                ty = 0.0;
            }

            // Tangent points in the direction where to positive X axis of the texture coord's would point in model space
            // Bitangent's points along the positive Y axis of the texture coord's, respectively
            Float3 tangent, bitangent;
            tangent.x = (w.x * sy - v.x * ty) * dir;
            tangent.y = (w.y * sy - v.y * ty) * dir;
            tangent.z = (w.z * sy - v.z * ty) * dir;
            bitangent.x = (w.x * sx - v.x * tx) * dir;
            bitangent.y = (w.y * sx - v.y * tx) * dir;
            bitangent.z = (w.z * sx - v.z * tx) * dir;

            // Set tangent frame for every vertex in that triangle
            for (int32 b = 0; b < 3; b++)
            {
                const int32 p = Indices[i + b];

                // Project tangent and bitangent into the plane formed by the normal
                Float3 localTangent = tangent - meshNorm[p] * (tangent * meshNorm[p]);
                Float3 localBitangent = bitangent - meshNorm[p] * (bitangent * meshNorm[p]);
                localTangent.Normalize();
                localBitangent.Normalize();

                // Reconstruct tangent according to normal and bitangent when it's infinite or NaN
                if (localTangent.IsNanOrInfinity())
                {
                    localTangent = meshNorm[p] ^ localBitangent;
                    localTangent.Normalize();
                }

                // Write data into the mesh
                meshTang[p] = localTangent;
            }
        }


        // Set up a SpatialSort to quickly find all vertices close to a given position
        Assimp::SpatialSort vertexFinder;
        vertexFinder.Fill((const aiVector3D*)Positions.Get(), vertexCount, sizeof(Float3));
        std::vector<uint32> verticesFound(16);

        const float posEpsilon = (max - min).Length() * 1e-4f;
        const float limit = cosf(smoothingAngle * Math::DegreesToRadians);
        List<int32> closeVertices;

        // In the second pass we now smooth out all tangents at the same local position if they are not too far off
        for (int32 a = 0; a < vertexCount; a++)
        {
            if (vertexDone[a])
                continue;

            const Float3 origPos = Positions[a];
            const Float3 origNorm = Normals[a];
            const Float3 origTang = Tangents[a];
            closeVertices.Clear();

            // Find all vertices close to that position
            vertexFinder.FindPositions(*(aiVector3D*)&origPos, posEpsilon, verticesFound);
            const int32 verticesFoundCount = (int32)verticesFound.size();

            closeVertices.EnsureCapacity(verticesFoundCount + 5);
            closeVertices.Add(a);

            // Look among them for other vertices sharing the same normal and a close-enough tangent
            for (int32 b = 0; b < verticesFoundCount; b++)
            {
                const int32 idx = verticesFound[b];

                if (vertexDone[idx])
                    continue;
                if (meshNorm[idx] * origNorm < angleEpsilon)
                    continue;
                if (meshTang[idx] * origTang < limit)
                    continue;

                // It's similar enough -> add it to the smoothing group
                closeVertices.Add(idx);
                vertexDone.Set(idx, true);
            }

            // Smooth the tangents of all vertices that were found to be close enough
            Float3 smoothTangent(0, 0, 0), smoothBitangent(0, 0, 0);
            for (int32 b = 0; b < closeVertices.Count(); b++)
            {
                auto p = closeVertices[b];
                smoothTangent += meshTang[p];
            }
            smoothTangent.Normalize();

            // Write it back into all affected tangents
            for (int32 b = 0; b < closeVertices.Count(); b++)
            {
                meshTang[closeVertices[b]] = smoothTangent;
            }
        }
#endif

        const auto endTime = Platform::GetTimeSeconds();
        const double time = Math::RoundTo2DecimalPlaces(endTime - startTime);
        if (time > 0.5f) // Don't log if generation was fast enough
            LOG_INFO("Render", "Generated {3} for mesh in {0}s ({1} vertices, {2} indices)", time, vertexCount, indexCount, SE_TEXT("tangents"));

        return true;
    }

    void MeshData::ImproveCacheLocality()
    {
        // The algorithm is roughly basing on this paper (except without overdraw reduction):
        // http://www.cs.princeton.edu/gfx/pubs/Sander_2007_%3ETR/tipsy.pdf

        // Configured size of the cache for the vertex buffer used by the algorithm (size is in vertices count, it's not a stride)
        const int32 VertexCacheSize = 12;

        if (Positions.IsEmpty() || Indices.IsEmpty() || Positions.Count() <= VertexCacheSize)
            return;
        PROFILE_CPU();

        const auto startTime = Platform::GetTimeSeconds();

        const auto vertexCount = Positions.Count();
        const auto indexCount = Indices.Count();
        const uint32* const pcEnd = Indices.Get() + Indices.Count();

        // First we need to build a vertex-triangle adjacency list
        VertexTriangleAdjacency adj(Indices.Get(), indexCount, vertexCount, true);

        // Build a list to store per-vertex caching time stamps
        uint32* const piCachingStamps = NewArray<uint32>(vertexCount);
        Platform::MemoryClear(piCachingStamps, vertexCount * sizeof(uint32));

        // Allocate an empty output index buffer. We store the output indices in one large array.
        // Since the number of triangles won't change the input triangles can be reused. This is how
        const uint32 iIdxCnt = indexCount;
        uint32* const piIBOutput = NewArray<uint32>(iIdxCnt);
        uint32* piCSIter = piIBOutput;

        // Allocate the flag array to hold the information
        // Whether a triangle has already been emitted or not
        std::vector<bool> abEmitted(indexCount / 3, false);

        // Dead-end vertex index stack
        List<uint32> sDeadEndVStack;

        // Create a copy of the piNumTriPtr buffer
        uint32* const piNumTriPtr = adj.LiveTriangles;
        const List<uint32> piNumTriPtrNoModify(piNumTriPtr, vertexCount);

        // Get the largest number of referenced triangles and allocate the "candidate buffer"
        uint32 iMaxRefTris = 0;
        {
            const uint32* piCur = adj.LiveTriangles;
            const uint32* const piCurEnd = adj.LiveTriangles + vertexCount;
            for (; piCur != piCurEnd; piCur++)
            {
                iMaxRefTris = Math::Max(iMaxRefTris, *piCur);
            }
        }
        ENGINE_ASSERT(iMaxRefTris > 0);
        uint32* piCandidates = NewArray<uint32>(iMaxRefTris * 3);
        uint32 iCacheMisses = 0;

        int ivdx = 0;
        int ics = 1;
        int iStampCnt = VertexCacheSize + 1;
        while (ivdx >= 0)
        {
            uint32 icnt = piNumTriPtrNoModify[ivdx];
            uint32* piList = adj.GetAdjacentTriangles(ivdx);
            uint32* piCurCandidate = piCandidates;

            // Get all triangles in the neighborhood
            for (uint32 tri = 0; tri < icnt; tri++)
            {
                // If they have not yet been emitted, add them to the output IB
                const uint32 fidx = *piList++;
                if (!abEmitted[fidx])
                {
                    // So iterate through all vertices of the current triangle
                    uint32* pcFace = &Indices[fidx * 3];
                    for (uint32 *p = pcFace, *p2 = pcFace + 3; p != p2; p++)
                    {
                        const uint32 dp = *p;

                        // The current vertex won't have any free triangles after this step
                        if (ivdx != (int)dp)
                        {
                            // Append the vertex to the dead-end stack
                            sDeadEndVStack.Push(dp);

                            // Register as candidate for the next step
                            *piCurCandidate++ = dp;

                            // Decrease the per-vertex triangle counts
                            piNumTriPtr[dp]--;
                        }

                        // Append the vertex to the output index buffer
                        *piCSIter++ = dp;

                        // If the vertex is not yet in cache, set its cache count
                        if (iStampCnt - piCachingStamps[dp] > VertexCacheSize)
                        {
                            piCachingStamps[dp] = iStampCnt++;
                            iCacheMisses++;
                        }
                    }

                    // Flag triangle as emitted
                    abEmitted[fidx] = true;
                }
            }

            // The vertex has now no living adjacent triangles anymore
            piNumTriPtr[ivdx] = 0;

            // Get next fanning vertex
            ivdx = -1;
            int max_priority = -1;
            for (uint32* piCur = piCandidates; piCur != piCurCandidate; ++piCur)
            {
                const uint32 dp = *piCur;

                // Must have live triangles
                if (piNumTriPtr[dp] > 0)
                {
                    int priority = 0;

                    // Will the vertex be in cache, even after fanning occurs?
                    uint32 tmp;
                    if ((tmp = iStampCnt - piCachingStamps[dp]) + 2 * piNumTriPtr[dp] <= VertexCacheSize)
                    {
                        priority = tmp;
                    }

                    // Keep best candidate
                    if (priority > max_priority)
                    {
                        max_priority = priority;
                        ivdx = dp;
                    }
                }
            }

            // Did we reach a dead end?
            if (-1 == ivdx)
            {
                // Need to get a non-local vertex for which we have a good chance that it is still in the cache
                while (!sDeadEndVStack.IsEmpty())
                {
                    uint32 iCachedIdx = sDeadEndVStack.Peek();
                    sDeadEndVStack.Pop();
                    if (piNumTriPtr[iCachedIdx] > 0)
                    {
                        ivdx = iCachedIdx;
                        break;
                    }
                }

                if (-1 == ivdx)
                {
                    // Well, there isn't such a vertex. Simply get the next vertex in input order and hope it is not too bad
                    while (ics < (int)vertexCount)
                    {
                        ics++;
                        if (piNumTriPtr[ics] > 0)
                        {
                            ivdx = ics;
                            break;
                        }
                    }
                }
            }
        }

        // Sort the output index buffer back to the input array
        piCSIter = piIBOutput;
        for (uint32* pcFace = Indices.Get(); pcFace != pcEnd; pcFace += 3)
        {
            pcFace[0] = *piCSIter++;
            pcFace[1] = *piCSIter++;
            pcFace[2] = *piCSIter++;
        }

        // Delete temporary storage
        PlatformAllocator::Free(piCachingStamps);
        PlatformAllocator::Free(piIBOutput);
        PlatformAllocator::Free(piCandidates);

        const auto endTime = Platform::GetTimeSeconds();
        const double time = Math::RoundTo2DecimalPlaces(endTime - startTime);
        if (time > 0.5f) // Don't log if generation was fast enough
            LOG_INFO("Render", "Generated {3} for mesh in {0}s ({1} vertices, {2} indices)", time, vertexCount, indexCount, SE_TEXT("optimized indices"));
    }

    float MeshData::CalculateTrianglesArea() const
    {
        PROFILE_CPU();
        float sum = 0;
        // TODO: use SIMD
        for (int32 i = 0; i + 2 < Indices.Count(); i += 3)
        {
            const Float3 v1 = Positions[Indices[i + 0]];
            const Float3 v2 = Positions[Indices[i + 1]];
            const Float3 v3 = Positions[Indices[i + 2]];
            sum += Float3::TriangleArea(v1, v2, v3);
        }
        return sum;
    }
}


#include "RenderList.h"

#include "RenderContext.h"
#include "RenderTask.h"
#include "Assets/Material/IMaterial.h"
#include "Runtime/Core/Systems.h"
#include "Runtime/Core/Math/Half.h"
#include "Runtime/Core/Platform/Win32/Application_Win32.h"
#include "Runtime/Core/Profiler/Profiler.h"
#include "Runtime/Core/Profiler/ProfilerCPU.h"
#include "Runtime/Core/Types/Collections/Sorting.h"
#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Resource/Assets/Materials/MaterialBase.h"
#include "Utils/RenderUtils.h"

namespace SE
{
    // Minimum roughness value used for shading (prevent 0 roughness which causes NaNs in Vis_SmithJointApprox)
#define MIN_ROUGHNESS 0.04f

    struct RenderListData
    {
        // Cached data for the draw calls sorting
        List<uint64> SortingKeys[2];
        List<int32> SortingIndices;
        List<DrawBatch> SortingBatches;
        List<RenderList*> FreeRenderList;

        struct MemPoolEntry
        {
            void* Ptr;
            uintptr Size;
        };

        List<MemPoolEntry> MemPool;
        CriticalSection MemPoolLocker;
    } *rendererListData;

    class RendererListSystem : public ISystem
    {
        ENGINE_SYSTEM(RendererListSystem)
    public:
        RendererListSystem() : ISystem(SE_TEXT("Renderer List"), 21)
        {
        }

        bool OnInit() override;
        void OnDispose() override;
    };

    ENGINE_SYSTEM_REGISTER(RendererListSystem)

    bool RendererListSystem::OnInit()
    {
        rendererListData = New<RenderListData>();
		
        return ISystem::OnInit();
    }

    void RendererListSystem::OnDispose()
    {
        Delete(rendererListData);
        ISystem::OnDispose();
    }


    void* RendererAllocation::Allocate(uintptr size)
    {
        void* result = nullptr;
        rendererListData->MemPoolLocker.Lock();
        for (int32 i = 0; i < rendererListData->MemPool.Count(); i++)
        {
            if (rendererListData->MemPool[i].Size == size)
            {
                result = rendererListData->MemPool[i].Ptr;
                rendererListData->MemPool.RemoveAt(i);
                break;
            }
        }
        rendererListData->MemPoolLocker.Unlock();
        if (!result)
        {
            result = Platform::Allocate(size, 16);
        }
        return result;
    }

    void RendererAllocation::Free(void* ptr, uintptr size)
    {
        rendererListData->MemPoolLocker.Lock();
        rendererListData->MemPool.Add({ ptr, size });
        rendererListData->MemPoolLocker.Unlock();
    }

    
    
    void RendererDirectionalLightData::SetupLightData(LightData* data, bool useShadow) const
    {
        data->SpotAngles.x = -2.0f;
        data->SpotAngles.y = 1.0f;
        data->SourceRadius = 0;
        data->SourceLength = 0;
        data->Color = Color;
        data->MinRoughness = Math::Max(MinRoughness, MIN_ROUGHNESS);
        data->Position = Float3::Zero;
        data->CastShadows = useShadow ? 1.0f : 0.0f;
        data->Direction = -Direction;
        data->Radius = 0;
        data->FalloffExponent = 0;
        data->InverseSquared = 0;
        data->RadiusInv = 0;
    }

    void RendererSpotLightData::SetupLightData(LightData* data, bool useShadow) const
    {
        data->SpotAngles.x = CosOuterCone;
        data->SpotAngles.y = InvCosConeDifference;
        data->SourceRadius = SourceRadius;
        data->SourceLength = 0.0f;
        data->Color = Color;
        data->MinRoughness = Math::Max(MinRoughness, MIN_ROUGHNESS);
        data->Position = Position;
        data->CastShadows = useShadow ? 1.0f : 0.0f;
        data->Direction = Direction;
        data->Radius = Radius;
        data->FalloffExponent = FallOffExponent;
        data->InverseSquared = UseInverseSquaredFalloff ? 1.0f : 0.0f;
        data->RadiusInv = 1.0f / Radius;
    }

    void RendererPointLightData::SetupLightData(LightData* data, bool useShadow) const
    {
        data->SpotAngles.x = -2.0f;
        data->SpotAngles.y = 1.0f;
        data->SourceRadius = SourceRadius;
        data->SourceLength = SourceLength;
        data->Color = Color;
        data->MinRoughness = Math::Max(MinRoughness, MIN_ROUGHNESS);
        data->Position = Position;
        data->CastShadows = useShadow ? 1.0f : 0.0f;
        data->Direction = Direction;
        data->Radius = Radius;
        data->FalloffExponent = FallOffExponent;
        data->InverseSquared = UseInverseSquaredFalloff ? 1.0f : 0.0f;
        data->RadiusInv = 1.0f / Radius;
    }

    void RendererSkyLightData::SetupLightData(LightData* data, bool useShadow) const
    {
        data->SpotAngles.x = AdditiveColor.x;
        data->SpotAngles.y = AdditiveColor.y;
        data->SourceRadius = AdditiveColor.z;
        // data->SourceLength = Image ? Image->StreamingTexture()->TotalMipLevels() - 2.0f : 0.0f;
        data->Color = Color;
        data->MinRoughness = MIN_ROUGHNESS;
        data->Position = Position;
        data->CastShadows = useShadow ? 1.0f : 0.0f;
        data->Direction = Float3::Forward;
        data->Radius = Radius;
        data->FalloffExponent = 0;
        data->InverseSquared = 0;
        data->RadiusInv = 1.0f / Radius;
    }


    void DrawCallsList::Clear()
    {
        Indices.Clear();
        PreBatchedDrawCalls.Clear();
        Batches.Clear();
        CanUseInstancing = true;
    }

    bool DrawCallsList::IsEmpty() const
    {
        return Indices.Count() + PreBatchedDrawCalls.Count() == 0;
    }



    RenderList* RenderList::GetFromPool()
    {
        if (rendererListData->FreeRenderList.HasItems())
        {
            const auto result = rendererListData->FreeRenderList.Last();
            rendererListData->FreeRenderList.RemoveLast();
            return result;
        }

        return New<RenderList>();
    }

    void RenderList::ReturnToPool(RenderList* cache)
    {
        if (!cache)
            return;

        ENGINE_ASSERT(!rendererListData->FreeRenderList.Contains(cache));
        rendererListData->FreeRenderList.Add(cache);
        cache->Clear();
    }

    void RenderList::CleanupCache()
    {
        // Don't call it during rendering (data may be already in use)
        ENGINE_ASSERT(RenderTask::CurrentTask == nullptr);

        rendererListData->SortingKeys[0].Resize(0);
        rendererListData->SortingKeys[1].Resize(0);
        rendererListData->SortingIndices.Resize(0);
        rendererListData->FreeRenderList.ClearDelete();
        for (auto& e : rendererListData->MemPool)
        {
            Platform::Free(e.Ptr);
        }
        rendererListData->MemPool.Clear();
    }

    
    RenderList::RenderList() : Object()
        , DirectionalLights(4)
        , PointLights(32)
        , SpotLights(32)
        , SkyLights(4)
        // , EnvironmentProbes(32)
        // , Decals(64)
        , Sky(nullptr)
        // , AtmosphericFog(nullptr)
        // , Fog(nullptr)
        // , Blendable(32)
        , _instanceBuffer(1024 * sizeof(InstanceData), sizeof(InstanceData), SE_TEXT("Instance Buffer"))
    {
    }

    void RenderList::Init(RenderContext& renderContext)
    {
        renderContext.view.frustum.GetCorners(FrustumCornersWs);
        for (int32 i = 0; i < 8; i++)
        {
            Float3::Transform(FrustumCornersWs[i], renderContext.view.View, FrustumCornersVs[i]);
        }
    }

    void RenderList::Clear()
    {
        // Scenes.Clear();
        DrawCalls.Clear();
        BatchedDrawCalls.Clear();
        for (auto& list : DrawCallsLists)
            list.Clear();
        ShadowDepthDrawCallsList.Clear();
        PointLights.Clear();
        SpotLights.Clear();
        SkyLights.Clear();
        DirectionalLights.Clear();
        // EnvironmentProbes.Clear();
        // Decals.Clear();
        VolumetricFogParticles.Clear();
        // Sky = nullptr;
        // AtmosphericFog = nullptr;
        // Fog = nullptr;
        // PostFx.Clear();
        // Settings = PostProcessSettings();
        // Blendable.Clear();
        _instanceBuffer.Clear();
    }

    struct PackedSortKey
    {
        union
        {
            uint64 Data;

            struct
            {
                uint32 DistanceKey;
                uint16 BatchKey;
                uint16 SortKey;
            };
        };
    };

    void CalculateSortKey(const RenderContext& renderContext, DrawCall& drawCall, int16 sortOrder)
    {
        const Float3 planeNormal = renderContext.view.Direction;
        const float planePoint = -Float3::Dot(planeNormal, renderContext.view.Position);
        const float distance = Float3::Dot(planeNormal, drawCall.ObjectPosition) - planePoint;
        PackedSortKey key;
        key.DistanceKey = RenderUtils::ComputeDistanceSortKey(distance);
        uint32 batchKey = GetHash(drawCall.Material);
        batchKey = (batchKey * 397) ^ GetHash(drawCall.Geometry.VertexBuffers[0]);
        batchKey = (batchKey * 397) ^ GetHash(drawCall.Geometry.VertexBuffers[1]);
        batchKey = (batchKey * 397) ^ GetHash(drawCall.Geometry.VertexBuffers[2]);
        batchKey = (batchKey * 397) ^ GetHash(drawCall.Geometry.IndexBuffer);
        IMaterial::InstancingHandler handler;
        if (drawCall.Material->CanUseInstancing(handler))
            handler.GetHash(drawCall, batchKey);
        batchKey += (int32)(471 * drawCall.WorldDeterminantSign);
        key.SortKey = (uint16)(sortOrder - Min_int16);
        key.BatchKey = (uint16)batchKey;
        drawCall.SortKey = key.Data;
    }

    void RenderList::AddDrawCall(const RenderContext& renderContext, EnumFlags<DrawPass> drawModes, EnumFlags<StaticMask> staticFlags, DrawCall& drawCall, bool receivesDecals, int16 sortOrder)
    {
#if ENABLE_ASSERTION_LOW_LAYERS
        // Ensure that draw modes are non-empty and in conjunction with material draw modes
        auto materialDrawModes = drawCall.Material->GetDrawModes();
        ASSERT_LOW_LAYER(drawModes != DrawPass::None && ((uint32)drawModes & ~(uint32)materialDrawModes) == 0);
#endif

        // Append draw call data
        CalculateSortKey(renderContext, drawCall, sortOrder);
        const int32 index = DrawCalls.Add(drawCall);

        // Add draw call to proper draw lists
        if (drawModes.IsFlag(DrawPass::Depth))
        {
            DrawCallsLists[(int32)DrawCallsListType::Depth].Indices.Add(index);
        }
        if (drawModes.IsFlag(DrawPass::GBuffer) && drawModes.IsFlag(DrawPass::GlobalSurfaceAtlas))
        {
            if (receivesDecals)
                DrawCallsLists[(int32)DrawCallsListType::GBuffer].Indices.Add(index);
            else
                DrawCallsLists[(int32)DrawCallsListType::GBufferNoDecals].Indices.Add(index);
        }
        if (drawModes.IsFlag(DrawPass::Forward))
        {
            DrawCallsLists[(int32)DrawCallsListType::Forward].Indices.Add(index);
        }
        if (drawModes.IsFlag(DrawPass::Distortion))
        {
            DrawCallsLists[(int32)DrawCallsListType::Distortion].Indices.Add(index);
        }
        if (drawModes.IsFlag(DrawPass::MotionVectors) && !staticFlags.IsFlag(StaticMask::Transform))
        {
            DrawCallsLists[(int32)DrawCallsListType::MotionVectors].Indices.Add(index);
        }
    }

    void RenderList::AddDrawCall(const RenderContextBatch& renderContextBatch, EnumFlags<DrawPass> drawModes, EnumFlags<StaticMask> staticFlags,
        EnumFlags<ShadowsCastingMode> shadowsMode, const BoundingSphere& bounds, DrawCall& drawCall, bool receivesDecals, int16 sortOrder)
    {
#if ENABLE_ASSERTION_LOW_LAYERS
        // Ensure that draw modes are non-empty and in conjunction with material draw modes
        auto materialDrawModes = drawCall.Material->GetDrawModes();
        ASSERT_LOW_LAYER(drawModes != DrawPass::None && ((uint32)drawModes & ~(uint32)materialDrawModes) == 0);
#endif
        const RenderContext& mainRenderContext = renderContextBatch.Contexts.Get()[0];

        // Append draw call data
        CalculateSortKey(mainRenderContext, drawCall, sortOrder);
        const int32 index = DrawCalls.Add(drawCall);

        // Add draw call to proper draw lists
        EnumFlags<DrawPass> modes = drawModes;
        modes.Marge(mainRenderContext.view.GetShadowsDrawPassMask(shadowsMode));

        drawModes = modes;
        drawModes.Marge(mainRenderContext.view.Pass);
        if (!drawModes.Is(DrawPass::None) && mainRenderContext.view.cullingFrustum.Intersects(bounds))
        {
            if (drawModes.IsFlag(DrawPass::Depth))
            {
                DrawCallsLists[(int32)DrawCallsListType::Depth].Indices.Add(index);
            }
            if (drawModes.IsFlag(DrawPass::GBuffer) && drawModes.IsFlag(DrawPass::GlobalSurfaceAtlas))
            {
                if (receivesDecals)
                    DrawCallsLists[(int32)DrawCallsListType::GBuffer].Indices.Add(index);
                else
                    DrawCallsLists[(int32)DrawCallsListType::GBufferNoDecals].Indices.Add(index);
            }
            if (drawModes.IsFlag(DrawPass::Forward))
            {
                DrawCallsLists[(int32)DrawCallsListType::Forward].Indices.Add(index);
            }
            if (drawModes.IsFlag(DrawPass::Distortion))
            {
                DrawCallsLists[(int32)DrawCallsListType::Distortion].Indices.Add(index);
            }
            if (drawModes.IsFlag(DrawPass::MotionVectors) && !staticFlags.IsFlag(StaticMask::Transform))
            {
                DrawCallsLists[(int32)DrawCallsListType::MotionVectors].Indices.Add(index);
            }
        }
        for (int32 i = 1; i < renderContextBatch.Contexts.Count(); i++)
        {
            const RenderContext& renderContext = renderContextBatch.Contexts.Get()[i];
            ASSERT_LOW_LAYER(renderContext.view.Pass == DrawPass::Depth);
            drawModes = modes;
            drawModes.Marge(renderContext.view.Pass);
            if (!drawModes.Is(DrawPass::None) && renderContext.view.cullingFrustum.Intersects(bounds))
            {
                renderContext.list->ShadowDepthDrawCallsList.Indices.Add(index);
            }
        }
    }

    namespace
    {
        /// <summary>
        /// Checks if this draw call be batched together with the other one.
        /// </summary>
        /// <param name="a">The first draw call.</param>
        /// <param name="b">The second draw call.</param>
        /// <returns>True if can merge them, otherwise false.</returns>
        FORCE_INLINE bool CanBatchWith(const DrawCall& a, const DrawCall& b)
        {
            IMaterial::InstancingHandler handler;
            return a.Material == b.Material &&
                    a.Material->CanUseInstancing(handler) &&
                    Platform::MemoryCompare(&a.Geometry, &b.Geometry, sizeof(a.Geometry)) == 0 &&
                    a.InstanceCount != 0 &&
                    b.InstanceCount != 0 &&
                    handler.CanBatch(a, b) &&
                    a.WorldDeterminantSign * b.WorldDeterminantSign > 0;
        }
    }

    void RenderList::SortDrawCalls(const RenderContext& renderContext, bool reverseDistance, DrawCallsList& list, const RenderListBuffer<DrawCall>& drawCalls, bool stable)
    {
        PROFILE_CPU();
        const auto* drawCallsData = drawCalls.Get();
        const auto* listData = list.Indices.Get();
        const int32 listSize = list.Indices.Count();
        ZoneValue(listSize);

        // Peek shared memory
#define PREPARE_CACHE(list) (list).Clear(); (list).Resize(listSize)
        PREPARE_CACHE(rendererListData->SortingKeys[0]);
        PREPARE_CACHE(rendererListData->SortingKeys[1]);
        PREPARE_CACHE(rendererListData->SortingIndices);
#undef PREPARE_CACHE
        uint64* sortedKeys = rendererListData->SortingKeys[0].Get();

        // Setup sort keys
        if (reverseDistance)
        {
            for (int32 i = 0; i < listSize; i++)
            {
                const DrawCall& drawCall = drawCallsData[listData[i]];
                PackedSortKey key;
                key.Data = drawCall.SortKey;
                key.DistanceKey ^= Max_uint32; // Reverse depth
                key.SortKey ^= Max_uint16; // Reverse sort order
                sortedKeys[i] = key.Data;
            }
        }
        else
        {
            for (int32 i = 0; i < listSize; i++)
            {
                const DrawCall& drawCall = drawCallsData[listData[i]];
                sortedKeys[i] = drawCall.SortKey;
            }
        }

        // Sort draw calls indices
        int32* resultIndices = list.Indices.Get();
        Sorting::RadixSort(sortedKeys, resultIndices, rendererListData->SortingKeys[1].Get(), rendererListData->SortingIndices.Get(), listSize);
        if (resultIndices != list.Indices.Get())
            Platform::MemoryCopy(list.Indices.Get(), resultIndices, sizeof(int32) * listSize);

        // Perform draw calls batching
        list.Batches.Clear();
        for (int32 i = 0; i < listSize;)
        {
            const DrawCall& drawCall = drawCallsData[listData[i]];
            int32 batchSize = 1;
            int32 instanceCount = drawCall.InstanceCount;

            // Check the following draw calls to merge them (using instancing)
            for (int32 j = i + 1; j < listSize; j++)
            {
                const DrawCall& other = drawCallsData[listData[j]];
                if (!CanBatchWith(drawCall, other))
                    break;
                batchSize++;
                instanceCount += other.InstanceCount;
            }

            DrawBatch batch;
            static_assert(sizeof(DrawBatch) == sizeof(uint64) * 2, "Fix the size of draw batch to optimize memory access.");
            batch.SortKey = sortedKeys[i];
            batch.StartIndex = i;
            batch.BatchSize = batchSize;
            batch.InstanceCount = instanceCount;
            list.Batches.Add(batch);

            i += batchSize;
        }

        // When using depth buffer draw calls are already almost ideally sorted by Radix Sort but transparency needs more stability to prevent flickering
        if (stable)
        {
            // Sort draw calls batches by depth
            Sorting::MergeSort(list.Batches, &rendererListData->SortingBatches);
        }
    }

    FORCE_INLINE bool CanUseInstancing(EnumFlags<DrawPass> pass)
    {
        return pass.Is(DrawPass::GBuffer) || pass.Is(DrawPass::Depth);
    }

    void RenderList::ExecuteDrawCalls(const RenderContext& renderContext, DrawCallsList& list, const RenderListBuffer<DrawCall>& drawCalls, GPUTextureView* input)
    {
        if (list.IsEmpty())
            return;
        PROFILE_GPU_CPU("Drawing");
        const auto* drawCallsData = drawCalls.Get();
        const auto* listData = list.Indices.Get();
        const auto* batchesData = list.Batches.Get();
        const auto context = renderContext.gpuContext;;
        bool useInstancing = list.CanUseInstancing && CanUseInstancing(renderContext.view.Pass) && GPUDevice::instance->GetGPULimits().HasInstancing;
        // TaaJitterRemoveContext taaJitterRemove(renderContext.view);

        // Clear SR slots to prevent any resources binding issues (leftovers from the previous passes)
        context->ResetSR();

        // Prepare instance buffer
        if (useInstancing)
        {
            // Prepare buffer memory
            int32 instancedBatchesCount = 0;
            for (int32 i = 0; i < list.Batches.Count(); i++)
            {
                auto& batch = batchesData[i];
                if (batch.BatchSize > 1)
                    instancedBatchesCount += batch.BatchSize;
            }
            for (int32 i = 0; i < list.PreBatchedDrawCalls.Count(); i++)
            {
                auto& batch = BatchedDrawCalls.Get()[list.PreBatchedDrawCalls.Get()[i]];
                if (batch.Instances.Count() > 1)
                    instancedBatchesCount += batch.Instances.Count();
            }
            if (instancedBatchesCount == 0)
            {
                // Faster path if none of the draw batches requires instancing
                useInstancing = false;
                goto DRAW;
            }
            _instanceBuffer.Clear();
            _instanceBuffer.data.Resize(instancedBatchesCount * sizeof(InstanceData));
            auto instanceData = (InstanceData*)_instanceBuffer.data.Get();

            // Write to instance buffer
            for (int32 i = 0; i < list.Batches.Count(); i++)
            {
                auto& batch = batchesData[i];
                if (batch.BatchSize > 1)
                {
                    IMaterial::InstancingHandler handler;
                    drawCallsData[listData[batch.StartIndex]].Material->CanUseInstancing(handler);
                    for (int32 j = 0; j < batch.BatchSize; j++)
                    {
                        auto& drawCall = drawCallsData[listData[batch.StartIndex + j]];
                        handler.WriteDrawCall(instanceData, drawCall);
                        instanceData++;
                    }
                }
            }
            for (int32 i = 0; i < list.PreBatchedDrawCalls.Count(); i++)
            {
                auto& batch = BatchedDrawCalls.Get()[list.PreBatchedDrawCalls.Get()[i]];
                if (batch.Instances.Count() > 1)
                {
                    Platform::MemoryCopy(instanceData, batch.Instances.Get(), batch.Instances.Count() * sizeof(InstanceData));
                    instanceData += batch.Instances.Count();
                }
            }

            // Upload data
            _instanceBuffer.Flush(context);
        }

        DRAW:

            // Execute draw calls
            MaterialBase::BindParameters bindParams(context, renderContext);
            bindParams.Input = input;
            bindParams.BindViewData();
            if (useInstancing)
            {
                int32 instanceBufferOffset = 0;
                GPUBuffer* vb[4];
                uint32 vbOffsets[4];
                for (int32 i = 0; i < list.Batches.Count(); i++)
                {
                    auto& batch = batchesData[i];
                    const DrawCall& drawCall = drawCallsData[listData[batch.StartIndex]];

                    int32 vbCount = 0;
                    while (vbCount < ARRAY_SIZE(drawCall.Geometry.VertexBuffers) && drawCall.Geometry.VertexBuffers[vbCount])
                    {
                        vb[vbCount] = drawCall.Geometry.VertexBuffers[vbCount];
                        vbOffsets[vbCount] = drawCall.Geometry.VertexBuffersOffsets[vbCount];
                        vbCount++;
                    }
                    for (int32 j = vbCount; j < ARRAY_SIZE(drawCall.Geometry.VertexBuffers); j++)
                    {
                        vb[vbCount] = nullptr;
                        vbOffsets[vbCount] = 0;
                    }

                    bindParams.firstDrawCall = &drawCall;
                    bindParams.drawCallsCount = batch.BatchSize;
                    drawCall.Material->Bind(bindParams);

                    context->BindIB(drawCall.Geometry.IndexBuffer);

                    if (drawCall.InstanceCount == 0)
                    {
                        // No support for batching indirect draw calls
                        ASSERT_LOW_LAYER(batch.BatchSize == 1);

                        context->BindVB(ToSpan(vb, vbCount), vbOffsets);
                        context->DrawIndexedInstancedIndirect(drawCall.Draw.IndirectArgsBuffer, drawCall.Draw.IndirectArgsOffset);
                    }
                    else
                    {
                        if (batch.BatchSize == 1)
                        {
                            context->BindVB(ToSpan(vb, vbCount), vbOffsets);
                            context->DrawIndexedInstanced(drawCall.Draw.IndicesCount, batch.InstanceCount, 0, 0, drawCall.Draw.StartIndex);
                        }
                        else
                        {
                            vbCount = 3;
                            vb[vbCount] = _instanceBuffer.GetBuffer();
                            vbOffsets[vbCount] = 0;
                            vbCount++;
                            context->BindVB(ToSpan(vb, vbCount), vbOffsets);
                            context->DrawIndexedInstanced(drawCall.Draw.IndicesCount, batch.InstanceCount, instanceBufferOffset, 0, drawCall.Draw.StartIndex);
                            instanceBufferOffset += batch.BatchSize;
                        }
                    }
                }
                for (int32 i = 0; i < list.PreBatchedDrawCalls.Count(); i++)
                {
                    auto& batch = BatchedDrawCalls.Get()[list.PreBatchedDrawCalls.Get()[i]];
                    auto& drawCall = batch.DrawCall;

                    int32 vbCount = 0;
                    while (vbCount < ARRAY_SIZE(drawCall.Geometry.VertexBuffers) && drawCall.Geometry.VertexBuffers[vbCount])
                    {
                        vb[vbCount] = drawCall.Geometry.VertexBuffers[vbCount];
                        vbOffsets[vbCount] = drawCall.Geometry.VertexBuffersOffsets[vbCount];
                        vbCount++;
                    }
                    for (int32 j = vbCount; j < ARRAY_SIZE(drawCall.Geometry.VertexBuffers); j++)
                    {
                        vb[vbCount] = nullptr;
                        vbOffsets[vbCount] = 0;
                    }

                    bindParams.firstDrawCall = &drawCall;
                    bindParams.drawCallsCount = batch.Instances.Count();
                    drawCall.Material->Bind(bindParams);

                    context->BindIB(drawCall.Geometry.IndexBuffer);

                    if (drawCall.InstanceCount == 0)
                    {
                        ASSERT_LOW_LAYER(batch.Instances.Count() == 1);
                        context->BindVB(ToSpan(vb, vbCount), vbOffsets);
                        context->DrawIndexedInstancedIndirect(drawCall.Draw.IndirectArgsBuffer, drawCall.Draw.IndirectArgsOffset);
                    }
                    else
                    {
                        if (batch.Instances.Count() == 1)
                        {
                            context->BindVB(ToSpan(vb, vbCount), vbOffsets);
                            context->DrawIndexedInstanced(drawCall.Draw.IndicesCount, batch.Instances.Count(), 0, 0, drawCall.Draw.StartIndex);
                        }
                        else
                        {
                            vbCount = 3;
                            vb[vbCount] = _instanceBuffer.GetBuffer();
                            vbOffsets[vbCount] = 0;
                            vbCount++;
                            context->BindVB(ToSpan(vb, vbCount), vbOffsets);
                            context->DrawIndexedInstanced(drawCall.Draw.IndicesCount, batch.Instances.Count(), instanceBufferOffset, 0, drawCall.Draw.StartIndex);
                            instanceBufferOffset += batch.Instances.Count();
                        }
                    }
                }
            }
            else
            {
                bindParams.drawCallsCount = 1;
                for (int32 i = 0; i < list.Batches.Count(); i++)
                {
                    auto& batch = batchesData[i];

                    for (int32 j = 0; j < batch.BatchSize; j++)
                    {
                        const DrawCall& drawCall = drawCalls[listData[batch.StartIndex + j]];
                        bindParams.firstDrawCall = &drawCall;
                        drawCall.Material->Bind(bindParams);

                        context->BindIB(drawCall.Geometry.IndexBuffer);
                        context->BindVB(ToSpan(drawCall.Geometry.VertexBuffers, 3), drawCall.Geometry.VertexBuffersOffsets);

                        if (drawCall.InstanceCount == 0)
                        {
                            context->DrawIndexedInstancedIndirect(drawCall.Draw.IndirectArgsBuffer, drawCall.Draw.IndirectArgsOffset);
                        }
                        else
                        {
                            context->DrawIndexedInstanced(drawCall.Draw.IndicesCount, drawCall.InstanceCount, 0, 0, drawCall.Draw.StartIndex);
                        }
                    }
                }
                for (int32 i = 0; i < list.PreBatchedDrawCalls.Count(); i++)
                {
                    auto& batch = BatchedDrawCalls.Get()[list.PreBatchedDrawCalls.Get()[i]];
                    auto drawCall = batch.DrawCall;
                    drawCall.ObjectRadius = 0.0f;
                    bindParams.firstDrawCall = &drawCall;
                    const auto* instancesData = batch.Instances.Get();

                    for (int32 j = 0; j < batch.Instances.Count(); j++)
                    {
                        auto& instance = instancesData[j];
                        drawCall.ObjectPosition = instance.InstanceOrigin;
                        drawCall.PerInstanceRandom = instance.PerInstanceRandom;
                        auto lightmapArea = instance.InstanceLightmapArea.ToFloat4();
                        drawCall.Surface.LightmapUVsArea = *(Rectangle*)&lightmapArea;
                        drawCall.Surface.LODDitherFactor = instance.LODDitherFactor;
                        drawCall.World.SetRow(1, Float4(instance.InstanceTransform1, 0.0f));
                        drawCall.World.SetRow(2, Float4(instance.InstanceTransform2, 0.0f));
                        drawCall.World.SetRow(3, Float4(instance.InstanceTransform3, 0.0f));
                        drawCall.World.SetRow(4, Float4(instance.InstanceOrigin, 1.0f));
                        drawCall.Material->Bind(bindParams);

                        context->BindIB(drawCall.Geometry.IndexBuffer);
                        context->BindVB(ToSpan(drawCall.Geometry.VertexBuffers, 3), drawCall.Geometry.VertexBuffersOffsets);
                        context->DrawIndexedInstanced(drawCall.Draw.IndicesCount, drawCall.InstanceCount, 0, 0, drawCall.Draw.StartIndex);
                    }
                }
                if (list.Batches.IsEmpty() && list.Indices.Count() != 0)
                {
                    // Draw calls list has nto been batched so execute draw calls separately
                    for (int32 j = 0; j < list.Indices.Count(); j++)
                    {
                        const DrawCall& drawCall = drawCalls[listData[j]];
                        bindParams.firstDrawCall = &drawCall;
                        drawCall.Material->Bind(bindParams);

                        context->BindIB(drawCall.Geometry.IndexBuffer);
                        context->BindVB(ToSpan(drawCall.Geometry.VertexBuffers, 3), drawCall.Geometry.VertexBuffersOffsets);

                        if (drawCall.InstanceCount == 0)
                        {
                            context->DrawIndexedInstancedIndirect(drawCall.Draw.IndirectArgsBuffer, drawCall.Draw.IndirectArgsOffset);
                        }
                        else
                        {
                            context->DrawIndexedInstanced(drawCall.Draw.IndicesCount, drawCall.InstanceCount, 0, 0, drawCall.Draw.StartIndex);
                        }
                    }
                }
            }
    }

    void SurfaceDrawCallHandler::GetHash(const DrawCall& drawCall, uint32& batchKey)
    {
        batchKey = (batchKey * 397);// ^ GetHash(drawCall.Surface.Lightmap);
    }

    bool SurfaceDrawCallHandler::CanBatch(const DrawCall& a, const DrawCall& b)
    {
        return true;
        /*// TODO: find reason why batching static meshes with lightmap causes problems with sampling in shader (flickering when meshes in batch order gets changes due to async draw calls collection)
        return a.Surface.Lightmap == nullptr && b.Surface.Lightmap == nullptr &&
                //return a.Surface.Lightmap == b.Surface.Lightmap &&
                a.Surface.Skinning == nullptr &&
                b.Surface.Skinning == nullptr;*/
    }

    void SurfaceDrawCallHandler::WriteDrawCall(InstanceData* instanceData, const DrawCall& drawCall)
    {
        instanceData->InstanceOrigin = Float3(drawCall.World.M41, drawCall.World.M42, drawCall.World.M43);
        instanceData->PerInstanceRandom = drawCall.PerInstanceRandom;
        instanceData->InstanceTransform1 = Float3(drawCall.World.M11, drawCall.World.M12, drawCall.World.M13);
        instanceData->LODDitherFactor = drawCall.Surface.LODDitherFactor;
        instanceData->InstanceTransform2 = Float3(drawCall.World.M21, drawCall.World.M22, drawCall.World.M23);
        instanceData->InstanceTransform3 = Float3(drawCall.World.M31, drawCall.World.M32, drawCall.World.M33);
        instanceData->InstanceLightmapArea = Half4(drawCall.Surface.LightmapUVsArea);
    }
} // SE
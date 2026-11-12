
#include <Runtime/Render/RenderList.h>

#include <Runtime/Core/Math/BoundingVolumes.h>
#include <Runtime/Core/Math/CollisionsHelper.h>
#include <Runtime/Core/Math/Half.h>
#include <Runtime/Core/Platform/Win32/Application_Win32.h>
#include <Runtime/Core/Profiler/Profiler.h>
#include <Runtime/Core/Profiler/ProfilerCPU.h>
#include <Runtime/Core/Systems.h>
#include <Runtime/Core/Types/Collections/Sorting.h>
#include <Runtime/Graphics/GPUContext.h>
#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Graphics/Shaders/ShaderProgramInstance.h>
#include <Runtime/Render/Assets/Material/IMaterial.h>
#include <Runtime/Render/RenderContext.h>
#include <Runtime/Render/RenderTask.h>
#include <Runtime/Render/Utils/RenderUtils.h>

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
        data->SpotAngles.X = -2.0f;
        data->SpotAngles.Y = 1.0f;
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
        data->SpotAngles.X = CosOuterCone;
        data->SpotAngles.Y = InvCosConeDifference;
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
        data->SpotAngles.X = -2.0f;
        data->SpotAngles.Y = 1.0f;
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
        data->SpotAngles.X = AdditiveColor.X;
        data->SpotAngles.Y = AdditiveColor.Y;
        data->SourceRadius = AdditiveColor.Z;
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
        , m_InstanceBuffer(1024 * sizeof(StaticMeshInstanceData), sizeof(StaticMeshInstanceData), SE_TEXT("Instance Buffer"))
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
        {
            list.Clear();
        }
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
        m_InstanceBuffer.Clear();
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
        {
            handler.GetHash(drawCall, batchKey);
        }
        batchKey += (int32)(471 * drawCall.WorldDeterminantSign);
        key.SortKey = (uint16)(sortOrder - Min_int16);
        key.BatchKey = (uint16)batchKey;
        drawCall.SortKey = key.Data;
    }

    void RenderList::AddDrawCall(const RenderContext& renderContext, DrawPass drawModes, StaticMask staticFlags, DrawCall& drawCall, bool receivesDecals, int16 sortOrder)
    {
#if ENABLE_ASSERTION_LOW_LAYERS
        // Ensure that draw modes are non-empty and in conjunction with material draw modes
        auto materialDrawModes = drawCall.SLC2 != nullptr ? drawModes : drawCall.Material->GetDrawModes();
        ASSERT_LOW_LAYER(drawModes != DrawPass::None && ((uint32)drawModes & ~(uint32)materialDrawModes) == 0);
#endif

        // Append draw call data
        CalculateSortKey(renderContext, drawCall, sortOrder);
        const int32 index = DrawCalls.Add(drawCall);

        // Add draw call to proper draw lists
        if (EnumHasAllFlags(drawModes, DrawPass::Depth))
        {
            DrawCallsLists[(int32)DrawCallsListType::Depth].Indices.Add(index);
        }
        if (EnumHasAllFlags(drawModes, DrawPass::GBuffer) && EnumHasAllFlags(drawModes, DrawPass::GlobalSurfaceAtlas))
        {
            if (receivesDecals)
                DrawCallsLists[(int32)DrawCallsListType::GBuffer].Indices.Add(index);
            else
                DrawCallsLists[(int32)DrawCallsListType::GBufferNoDecals].Indices.Add(index);
        }
        if (EnumHasAllFlags(drawModes, DrawPass::Forward))
        {
            DrawCallsLists[(int32)DrawCallsListType::Forward].Indices.Add(index);
        }
        if (EnumHasAllFlags(drawModes, DrawPass::Distortion))
        {
            DrawCallsLists[(int32)DrawCallsListType::Distortion].Indices.Add(index);
        }
        if (EnumHasAllFlags(drawModes, DrawPass::MotionVectors) && !EnumHasAllFlags(staticFlags, StaticMask::Transform))
        {
            DrawCallsLists[(int32)DrawCallsListType::MotionVectors].Indices.Add(index);
        }
    }


    void RenderList::AddDrawCall(const RenderContextBatch& renderContextBatch, DrawPass drawModes, StaticMask staticFlags,
        ShadowsCastingMode shadowsMode, const BoundingSphere& bounds, DrawCall& drawCall, bool receivesDecals, int16 sortOrder)
    {
#if ENABLE_ASSERTION_LOW_LAYERS
        // Ensure that draw modes are non-empty and in conjunction with material draw modes
        auto materialDrawModes = drawCall.SLC2 != nullptr ? drawModes : drawCall.Material->GetDrawModes();
        ASSERT_LOW_LAYER(drawModes != DrawPass::None && ((uint32)drawModes & ~(uint32)materialDrawModes) == 0);
#endif
        const RenderContext& mainRenderContext = renderContextBatch.Contexts.Get()[0];

        // Append draw call data
        CalculateSortKey(mainRenderContext, drawCall, sortOrder);
        const int32 index = DrawCalls.Add(drawCall);

        // Add draw call to proper draw lists
        DrawPass modes = drawModes;
        modes = EnumAddFlags(modes, mainRenderContext.view.GetShadowsDrawPassMask(shadowsMode));

        drawModes = modes;
        modes = EnumAddFlags(drawModes, mainRenderContext.view.Pass);
        if (drawModes != DrawPass::None && mainRenderContext.view.cullingFrustum.Intersects(bounds))
        {
            if (EnumHasAllFlags(drawModes, DrawPass::Depth))
            {
                DrawCallsLists[(int32)DrawCallsListType::Depth].Indices.Add(index);
            }
            if (EnumHasAllFlags(drawModes, DrawPass::GBuffer) && EnumHasAllFlags(drawModes, DrawPass::GlobalSurfaceAtlas))
            {
                if (receivesDecals)
                    DrawCallsLists[(int32)DrawCallsListType::GBuffer].Indices.Add(index);
                else
                    DrawCallsLists[(int32)DrawCallsListType::GBufferNoDecals].Indices.Add(index);
            }
            if (EnumHasAllFlags(drawModes, DrawPass::Forward))
            {
                DrawCallsLists[(int32)DrawCallsListType::Forward].Indices.Add(index);
            }
            if (EnumHasAllFlags(drawModes, DrawPass::Distortion))
            {
                DrawCallsLists[(int32)DrawCallsListType::Distortion].Indices.Add(index);
            }
            if (EnumHasAllFlags(drawModes, DrawPass::MotionVectors) && !EnumHasAllFlags(staticFlags, StaticMask::Transform))
            {
                DrawCallsLists[(int32)DrawCallsListType::MotionVectors].Indices.Add(index);
            }
        }
        for (int32 i = 1; i < renderContextBatch.Contexts.Count(); i++)
        {
            const RenderContext& renderContext = renderContextBatch.Contexts.Get()[i];
            ASSERT_LOW_LAYER(renderContext.view.Pass == DrawPass::Depth);
            drawModes = modes;
            drawModes = EnumAddFlags(drawModes, renderContext.view.Pass);
            if (drawModes != DrawPass::None && renderContext.view.cullingFrustum.Intersects(bounds))
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
            // 自动合批中的每个 DrawCall 只写入一条实例记录。
            return a.Material == b.Material &&
                    a.Material->CanUseInstancing(handler) &&
                    Platform::MemoryCompare(&a.Geometry, &b.Geometry, sizeof(a.Geometry)) == 0 &&
                    a.InstanceCount == 1 &&
                    b.InstanceCount == 1 &&
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

    FORCE_INLINE bool CanUseInstancing(DrawPass pass)
    {
        return pass == DrawPass::GBuffer || pass == DrawPass::Depth;
    }


    /**
     * 
     * @param renderContext 
     * @param list 
     * @param drawCalls 
     * @param prepareDraw 
     * @param input 
     */
    void RenderList::ExecuteDrawCalls(const RenderContext&              renderContext,
                                      DrawCallsList&                    list,
                                      const RenderListBuffer<DrawCall>& drawCalls, 
                                      Function<void(IMaterial*)>        prepareDraw,
                                      GPUTextureView*                   input)
    {
        if (list.IsEmpty())
        {
            return;
        }

        PROFILE_GPU_CPU("Drawing");
        GPUContext*  context       = renderContext.gpuContext;
        const int32* indices       = list.Indices.Get();
        bool         useInstancing = list.CanUseInstancing && CanUseInstancing(renderContext.view.Pass) &&
                                     GPUDevice::instance->GetGPULimits().HasInstancing;
        if (useInstancing)
        {
            int32 count = 0;
            for (const auto& batch : list.Batches)
            {
                if (batch.BatchSize > 1)
                {
                    count += batch.BatchSize;
                }
            }
            for (int32 index : list.PreBatchedDrawCalls)
            {
                const auto& batch = BatchedDrawCalls.Get()[index];
                if (batch.Instances.Count() > 1)
                {
                    count += batch.Instances.Count();
                }
            }
            useInstancing = count != 0;

            if (useInstancing)
            {
                m_InstanceBuffer.Clear();
                m_InstanceBuffer.data.Resize(count * sizeof(StaticMeshInstanceData));
                StaticMeshInstanceData* data = reinterpret_cast<StaticMeshInstanceData*>(m_InstanceBuffer.data.Get());
                for (const auto& batch : list.Batches)
                {
                    if (batch.BatchSize <= 1)
                    {
                        continue;
                    }
                    IMaterial::InstancingHandler handler;
                    const DrawCall& first = drawCalls[indices[batch.StartIndex]];
                    if (!first.Material || !first.Material->CanUseInstancing(handler))
                    {
                        return;
                    }
                    for (int32 i = 0; i < batch.BatchSize; ++i)
                    {
                        handler.WriteDrawCall(data++, drawCalls[indices[batch.StartIndex + i]]);
                    }
                }

                for (int32 index : list.PreBatchedDrawCalls)
                {
                    const auto& batch = BatchedDrawCalls.Get()[index];
                    if (batch.Instances.Count() > 1)
                    {
                        Platform::MemoryCopy(data, batch.Instances.Get(), batch.Instances.Count() * sizeof(StaticMeshInstanceData));
                        data += batch.Instances.Count();
                    }
                }
                m_InstanceBuffer.Flush(context);
            }
        }

        IMaterial::BindParameters params(context, renderContext);
        params.Input      = input;


        DrawSubmitContext submitContext = {};

        submitContext.renderContext  = &renderContext;
        submitContext.bindParameters = &params;
        submitContext.PrepareDraw    = &prepareDraw;

        uint32 instanceOffset = 0;
        for (const auto& batch : list.Batches)
        {
            if (useInstancing)
            {
                const bool instanced = batch.BatchSize > 1;

                submitContext.draw          = &drawCalls[indices[batch.StartIndex]];
                submitContext.instanceCount = batch.InstanceCount;
                submitContext.offset        = instanceOffset;
                if (instanced)
                {
                    submitContext.pInstances = m_InstanceBuffer.GetBuffer();
                }
                else
                {
                    submitContext.pInstances = nullptr;
                }

                DrawSubmit(submitContext);
                if (instanced)
                {
                    instanceOffset += batch.BatchSize * sizeof(StaticMeshInstanceData);
                }
            }
            else
            {
                for (int32 i = 0; i < batch.BatchSize; ++i)
                {
                    const DrawCall& draw = drawCalls[indices[batch.StartIndex + i]];

                    submitContext.draw          = &draw;
                    submitContext.instanceCount = draw.InstanceCount;
                    submitContext.offset        = 0;
                    submitContext.pInstances    = nullptr;

                    DrawSubmit(submitContext);
                }
            }
        }

        for (int32 index : list.PreBatchedDrawCalls)
        {
            const auto& batch = BatchedDrawCalls.Get()[index];
            if (useInstancing)
            {
                const bool instanced = batch.Instances.Count() > 1;

                submitContext.draw = &batch.DrawCall;
                submitContext.instanceCount = batch.Instances.Count();
                submitContext.offset        = instanceOffset;
                if (instanced)
                {
                    submitContext.pInstances = m_InstanceBuffer.GetBuffer();
                }
                else
                {
                    submitContext.pInstances = nullptr;
                }

                DrawSubmit(submitContext);
                if (instanced)
                {
                    instanceOffset += batch.Instances.Count() * sizeof(StaticMeshInstanceData);
                }
            }
            else
            {
                DrawCall draw     = batch.DrawCall;
                draw.ObjectRadius = 0.0f;
                for (const StaticMeshInstanceData& instance : batch.Instances)
                {
                    draw.ObjectPosition          = instance.InstanceOrigin;
                    draw.PerInstanceRandom       = instance.PerInstanceRandom;
                    const Float4 lightmap        = instance.InstanceLightmapArea.ToFloat4();
                    draw.Surface.LightmapUVsArea = *reinterpret_cast<const Rectangle*>(&lightmap);
                    draw.Surface.LODDitherFactor = instance.LODDitherFactor;
                    draw.World.SetRow(1, Float4(instance.InstanceTransform1, 0.0f));
                    draw.World.SetRow(2, Float4(instance.InstanceTransform2, 0.0f));
                    draw.World.SetRow(3, Float4(instance.InstanceTransform3, 0.0f));
                    draw.World.SetRow(4, Float4(instance.InstanceOrigin, 1.0f));

                    submitContext.draw          = &draw;
                    submitContext.instanceCount = draw.InstanceCount;
                    submitContext.offset        = 0;
                    submitContext.pInstances    = nullptr;

                    DrawSubmit(submitContext);
                }
            }
        }

        if (list.Batches.IsEmpty())
        {
            
            submitContext.pInstances        = nullptr;
            submitContext.offset            = 0;

            for (int32 index : list.Indices)
            {
                const DrawCall& draw = drawCalls[index];

                submitContext.draw = &draw;
                submitContext.instanceCount = draw.InstanceCount;

                DrawSubmit(submitContext);
            }
        }
    }

    void RenderList::DrawSubmit(DrawSubmitContext& submitContext)
    {
        const DrawCall& draw = *submitContext.draw;
        if (!draw.Material || !draw.Material->IsReady())
        {
            return;
        }

        const RenderContext&       renderContext  = *submitContext.renderContext;
        uint32                     instanceCount  = submitContext.instanceCount;
        GPUBuffer*                 pInstances     = submitContext.pInstances;
        uint32                     offset         = submitContext.offset;
        IMaterial::BindParameters& bindParameters = *submitContext.bindParameters;


        bindParameters.firstDrawCall = &draw;
        bindParameters.drawCallsCount = instanceCount;
        bindParameters.UseInstancing  = pInstances != nullptr;
        draw.Material->Bind(bindParameters);

        ShaderProgramInstance* program = draw.Material->GetProgram();
        if (!program)
        {
            return;
        }

        // Bind after the material has reset its instance and applied material parameter overrides.
        const auto& view = renderContext.view;
        const Matrix viewMatrix               = view.View;
        const Matrix viewProjectionMatrix     = view.frustum.GetMatrix();
        const Matrix prevViewProjectionMatrix = view.PrevViewProjection;
        const Matrix mainViewProjectionMatrix = view.MainViewProjection;
        const Matrix worldMatrix              = draw.World;
        const Matrix prevWorldMatrix          = draw.Surface.PrevWorld;
        const struct
        {
            const Char* Path;
            const void* Value;
            uint32      Size;
        } parameters[] = {
            {SE_TEXT("ViewData.ViewMatrix"), &viewMatrix, sizeof(viewMatrix)},
            {SE_TEXT("ViewData.ViewProjectionMatrix"), &viewProjectionMatrix, sizeof(viewProjectionMatrix)},
            {SE_TEXT("ViewData.PrevViewProjectionMatrix"), &prevViewProjectionMatrix, sizeof(prevViewProjectionMatrix)},
            {SE_TEXT("ViewData.MainViewProjectionMatrix"), &mainViewProjectionMatrix, sizeof(mainViewProjectionMatrix)},
            {SE_TEXT("ViewData.MainScreenSize"), &view.MainScreenSize, sizeof(view.MainScreenSize)},
            {SE_TEXT("ViewData.ViewPos"), &view.Position, sizeof(view.Position)},
            {SE_TEXT("ViewData.ViewFar"), &view.Far, sizeof(view.Far)},
            {SE_TEXT("ViewData.ViewDir"), &view.Direction, sizeof(view.Direction)},
            {SE_TEXT("ViewData.TimeParam"), &bindParameters.timeParam, sizeof(bindParameters.timeParam)},
            {SE_TEXT("ViewData.ViewInfo"), &view.ViewInfo, sizeof(view.ViewInfo)},
            {SE_TEXT("ViewData.ScreenSize"), &view.ScreenSize, sizeof(view.ScreenSize)},
            {SE_TEXT("ViewData.TemporalAAJitter"), &view.TemporalAAJitter, sizeof(view.TemporalAAJitter)},
            {SE_TEXT("Data.WorldMatrix"), &worldMatrix, sizeof(worldMatrix)},
            {SE_TEXT("Data.PrevWorldMatrix"), &prevWorldMatrix, sizeof(prevWorldMatrix)},
            {SE_TEXT("Data.WorldDeterminantSign"), &draw.WorldDeterminantSign, sizeof(draw.WorldDeterminantSign)},
            {SE_TEXT("Data.LODDitherFactor"), &draw.Surface.LODDitherFactor, sizeof(draw.Surface.LODDitherFactor)},
            {SE_TEXT("Data.PerInstanceRandom"), &draw.PerInstanceRandom, sizeof(draw.PerInstanceRandom)},
            {SE_TEXT("Data.GeometrySize"), &draw.Surface.GeometrySize, sizeof(draw.Surface.GeometrySize)},
        };

        for (const auto& parameter : parameters)
        {
            ShaderVar variable;
            // Preserve optimized-out fields; any present field must match the value's ABI.
            if (program->FindVar(parameter.Path, variable))
            {
                variable.SetUniform(parameter.Value, parameter.Size);
            }
        }

        if (draw.Material->GetInfo().BlendMode != MaterialBlendMode::Opaque && renderContext.list)
        {
            const auto& sceneLights = *renderContext.list;
            LightData   directional = {};
            if (sceneLights.DirectionalLights.HasItems())
            {
                sceneLights.DirectionalLights.First().SetupLightData(&directional, false);
            }
            LightData            localLights[4]   = {};
            uint32               localLightsCount = 0;
            const BoundingSphere bounds(draw.ObjectPosition, draw.ObjectRadius);
            for (const auto& light : sceneLights.PointLights)
            {
                if (localLightsCount == ARRAY_SIZE(localLights))
                {
                    break;
                }
                if (CollisionsHelper::SphereIntersectsSphere(bounds, BoundingSphere(light.Position, light.Radius)))
                {
                    light.SetupLightData(&localLights[localLightsCount++], false);
                }
            }
            for (const auto& light : sceneLights.SpotLights)
            {
                if (localLightsCount == ARRAY_SIZE(localLights))
                {
                    break;
                }
                if (CollisionsHelper::SphereIntersectsSphere(bounds, BoundingSphere(light.Position, light.Radius)))
                {
                    light.SetupLightData(&localLights[localLightsCount++], false);
                }
            }
            for (uint32 lightIndex = 0; lightIndex <= localLightsCount; ++lightIndex)
            {
                const LightData& light = lightIndex == 0 ? directional : localLights[lightIndex - 1];
                const String     path  = lightIndex == 0 ? SE_TEXT("Data.DirectionalLight.") : String::Format(SE_TEXT("Data.LocalLights[{0}]."), lightIndex - 1);
                const struct
                {
                    const Char* Name;
                    const void* Value;
                    uint32      Size;
                } lightParameters[] = {
                    {SE_TEXT("SpotAngles"), &light.SpotAngles, sizeof(light.SpotAngles)},
                    {SE_TEXT("SourceRadius"), &light.SourceRadius, sizeof(light.SourceRadius)},
                    {SE_TEXT("SourceLength"), &light.SourceLength, sizeof(light.SourceLength)},
                    {SE_TEXT("Color"), &light.Color, sizeof(light.Color)},
                    {SE_TEXT("MinRoughness"), &light.MinRoughness, sizeof(light.MinRoughness)},
                    {SE_TEXT("Position"), &light.Position, sizeof(light.Position)},
                    {SE_TEXT("CastShadows"), &light.CastShadows, sizeof(light.CastShadows)},
                    {SE_TEXT("Direction"), &light.Direction, sizeof(light.Direction)},
                    {SE_TEXT("Radius"), &light.Radius, sizeof(light.Radius)},
                    {SE_TEXT("FalloffExponent"), &light.FalloffExponent, sizeof(light.FalloffExponent)},
                    {SE_TEXT("InverseSquared"), &light.InverseSquared, sizeof(light.InverseSquared)},
                    {SE_TEXT("RadiusInv"), &light.RadiusInv, sizeof(light.RadiusInv)},
                };
                for (const auto& parameter : lightParameters)
                {
                    ShaderVar variable;
                    if (program->FindVar(path + parameter.Name, variable) && (variable.GetLocation().UniformByteSize != parameter.Size || !variable.SetUniform(parameter.Value, parameter.Size)))
                    {
                        return;
                    }
                }
            }
            ShaderVar variable;
            if (program->FindVar(SE_TEXT("Data.LocalLightsCount"), variable))
            {
                variable.SetUniform(&localLightsCount, sizeof(localLightsCount));
            }
        }

        if (submitContext.PrepareDraw->IsBinded())
        {
            submitContext.PrepareDraw->operator()(draw.Material);
        }

        RenderGeometry geometry;
        geometry.Layout      = draw.Geometry.vertexLayout;
        geometry.IndexBuffer = draw.Geometry.IndexBuffer;
        geometry.IndexFormat = draw.Geometry.IndexFormat;
        geometry.IndexOffset = draw.Geometry.IndexOffset;
        for (uint32 slot = 0; slot < ARRAY_SIZE(draw.Geometry.VertexBuffers); ++slot)
        {
            if (draw.Geometry.VertexBuffers[slot])
            {
                geometry.VertexBuffers.Add({slot, draw.Geometry.VertexBuffers[slot], draw.Geometry.VertexBuffersOffsets[slot]});
            }
        }

        if (pInstances)
        {
            RenderGeometry instanced;
            if (!StaticMeshVertexFactory::CreateRenderGeometryWithInstanceBuffer(geometry, pInstances, offset, instanced))
            {
                return;
            }
            geometry = MoveTemp(instanced);
        }

        if (!renderContext.gpuContext->BindRenderGeometry(geometry))
        {
            return;
        }

        // The backend freezes the ProgramInstance once for this submission. Byte offsets already select
        // the instance stream range, so startInstance stays zero for both batched and prebatched draws.
        if (draw.InstanceCount == 0)
        {
            if (geometry.IndexBuffer)
            {
                renderContext.gpuContext->DrawIndexedInstancedIndirect(*program, draw.Draw.IndirectArgsBuffer, draw.Draw.IndirectArgsOffset);
            }
            else
            {
                renderContext.gpuContext->DrawInstancedIndirect(*program, draw.Draw.IndirectArgsBuffer, draw.Draw.IndirectArgsOffset);
            }
        }
        else if (geometry.IndexBuffer)
        {
            renderContext.gpuContext->DrawIndexedInstanced(program, draw.Draw.IndicesCount, instanceCount, 0, 0, draw.Draw.StartIndex);
        }
        else
        {
            renderContext.gpuContext->DrawInstanced(program, draw.Draw.IndicesCount, instanceCount);
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

    void SurfaceDrawCallHandler::WriteDrawCall(StaticMeshInstanceData* instanceData, const DrawCall& drawCall)
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

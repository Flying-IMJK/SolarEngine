#pragma once

#include <Runtime/Render/Assets/Material/MaterialParams.h>
#include <Runtime/Resource/AssetRef.h>
#include <Runtime/Resource/Assets/SLC2Materials/GraphicShader.h>
#include <Runtime/Resource/Assets/SLC2Materials/SLC2MaterialBinding.h>
#include <Runtime/Resource/Assets/SLC2Materials/SLC2MaterialDrawProgram.h>

namespace SE
{
    class SLC2MaterialInstance;

    SE_CLASS(Reflect, API(NoSpawn))
    class SE_API_RUNTIME SLC2Material final : public BinaryAsset
    {
        SE_DEFINE_CLASS(SLC2Material, BinaryAsset)
        ASSET_HEADER(SLC2Material);

    public:
        // Read operations return caller-owned snapshots; no mutable internal container or reference escapes.
        bool GetParameters(MaterialParams& output) const;
        // Mutations read inputs synchronously and publish only a completely validated copy. False leaves published
        // state unchanged; SetGraphicShader retains only an AssetRef to pShader.
        bool                    SetParameters(MaterialParams& parameters);
        bool                    SetParameterValue(const UID& parameterId, const Variant& value);
        void                    GetParameterBindings(List<SLC2MaterialParameterBinding>& output) const;
        bool                    SetParameterBinding(const SLC2MaterialParameterBinding& binding);
        bool                    RemoveParameterBinding(const UID& parameterId);
        void                    GetPipelines(List<SLC2MaterialPipelineState>& output) const;
        bool                    SetPipeline(const SLC2MaterialPipelineState& pipeline);
        bool                    RemovePipeline(const String& programId);
        bool                    SetGraphicShader(GraphicShader* pShader);
        AssetRef<GraphicShader> GetGraphicShader() const;
        // May wait for Shader and parameter content. Call on the GPU/render-resource thread. On failure output is
        // empty; on success it owns the selected Program, Pipeline, and Generation but not provider resources. Keep
        // output and those resources alive through Prepare and backend descriptor/draw submission. No GPU work is
        // submitted or completed here.
        bool PrepareDraw(const String&                             passName,
                         SLC2MaterialGeometryMode                  geometryMode,
                         const ShaderCompileTarget&                target,
                         const List<String>&                       defines,
                         const SLC2MaterialDrawContext&            context,
                         std::unique_ptr<SLC2MaterialDrawProgram>& output) const;

#if SE_EDITOR
        // Reads all inputs synchronously without retaining them and returns false for invalid state or save failure.
        static bool SaveAssetData(const StringView&                         path,
                                  GraphicShader*                            pShader,
                                  MaterialParams&                           parameters,
                                  const List<SLC2MaterialParameterBinding>& bindings,
                                  const List<SLC2MaterialPipelineState>&    pipelines);
        void        GetReferences(List<UID>& output) const override;
#endif

        uint32 GetSerializedVersion() const override;

    protected:
        bool            OnInit(AssetInitData& initData) override;
        AssetChunksFlag GetChunksToPreload() const override;
        LoadResult      load() override;
        void            Unload(bool isReloading) override;

    private:
        friend class SLC2MaterialInstance;

        bool SnapshotParameters(MaterialParams& output, int32& version, uint64& revision) const;
        bool PrepareDrawWithParameters(const String&                             passName,
                                       SLC2MaterialGeometryMode                  geometryMode,
                                       const ShaderCompileTarget&                target,
                                       const List<String>&                       defines,
                                       const MaterialParams&                     parameters,
                                       uint64                                    expectedRevision,
                                       const SLC2MaterialDrawContext&            context,
                                       std::unique_ptr<SLC2MaterialDrawProgram>& output) const;

        AssetRef<GraphicShader>                       m_Shader;
        MaterialParams                                m_Parameters;
        List<SLC2MaterialParameterBinding>            m_Bindings;
        List<SLC2MaterialPipelineState>               m_Pipelines;
        mutable List<SharedPtr<SLC2MaterialPipeline>> m_CachedPipelines;
        uint64                                        m_Revision = 0;
    };
} // namespace SE

#pragma once

#include <Runtime/Resource/Assets/SLC2Materials/SLC2Material.h>

namespace SE
{
    SE_CLASS(Reflect, API(NoSpawn))
    class SE_API_RUNTIME SLC2MaterialInstance final : public BinaryAsset
    {
        SE_DEFINE_CLASS(SLC2MaterialInstance, BinaryAsset)
        ASSET_HEADER(SLC2MaterialInstance);

    public:
        // Returns a caller-owned resolved clone. Resolution may wait for the base or parent and fails for an invalid
        // relation, cycle, depth limit, or synchronization error.
        bool GetParameters(MaterialParams& output) const;
        // Override changes are atomic; they may resolve ancestry and false leaves the published override set unchanged.
        bool SetParameterOverride(const UID& parameterId, const Variant& value);
        bool ClearParameterOverride(const UID& parameterId);
        // Stores an AssetRef rather than raw ownership. Calls may wait for loading and reject null, self, cyclic, or
        // unsynchronizable relations without changing the published relation.
        bool SetBaseMaterial(SLC2Material* pMaterial);
        bool SetParentInstance(SLC2MaterialInstance* pInstance);
        // Return owning AssetRef snapshots.
        AssetRef<SLC2Material>         GetBaseMaterial() const;
        AssetRef<SLC2MaterialInstance> GetParentInstance() const;
        // May wait while resolving ancestry and Shader content. Call on the GPU/render-resource thread. On failure
        // output is empty; on success it owns the selected Program, Pipeline, and Generation but not provider
        // resources. Keep output and those resources alive through backend descriptor/draw submission.
        bool PrepareDraw(const String&                             passName,
                         SLC2MaterialGeometryMode                  geometryMode,
                         const ShaderCompileTarget&                target,
                         const List<String>&                       defines,
                         const SLC2MaterialDrawContext&            context,
                         std::unique_ptr<SLC2MaterialDrawProgram>& output) const;

#if SE_EDITOR
        // Exactly one of pMaterial and pParent is required. Inputs are read synchronously and not retained; invalid
        // state or save failure returns false.
        static bool SaveAssetData(const StringView&     path,
                                  SLC2Material*         pMaterial,
                                  SLC2MaterialInstance* pParent,
                                  MaterialParams&       parameters);
        void        GetReferences(List<UID>& output) const override;
#endif

        uint32 GetSerializedVersion() const override;

    protected:
        bool            OnInit(AssetInitData& initData) override;
        AssetChunksFlag GetChunksToPreload() const override;
        LoadResult      load() override;
        void            Unload(bool isReloading) override;

    private:
        bool ResolveParameters(MaterialParams&                    output,
                               AssetRef<SLC2Material>&            baseMaterial,
                               uint64&                            baseRevision,
                               List<const SLC2MaterialInstance*>& stack) const;

        AssetRef<SLC2Material>         m_Base;
        AssetRef<SLC2MaterialInstance> m_Parent;
        mutable MaterialParams         m_Parameters;
        mutable int32                  m_SourceVersion = 0;
        mutable bool                   m_Synchronized  = false;
        mutable uint64                 m_Revision      = 0;
    };
} // namespace SE

#pragma once

#include "Runtime/Resource/Assets/Materials/MaterialBase.h"

namespace SE
{
    /// <summary>
    /// Instance of the <seealso cref="Material" /> with custom set of material parameter values.
    /// </summary>
    class SE_API_RUNTIME MaterialInstance : public MaterialBase
    {
        SE_CLASS_DEFAULT(MaterialInstance, MaterialBase);
    private:
        MaterialBase* _baseMaterial = nullptr;

    public:
        MaterialInstance(const AssetInfo* info);

        /// <summary>
        /// Gets the base material. If value gets changed parameters collection is restored to the default values of the new material.
        /// </summary>
        FORCE_INLINE MaterialBase* GetBaseMaterial() const
        {
            return _baseMaterial;
        }

        /// <summary>
        /// Sets the base material. If value gets changed parameters collection is restored to the default values of the new material.
        /// </summary>
        /// <param name="baseMaterial">The base material.</param>
        void SetBaseMaterial(MaterialBase* baseMaterial);

#if SE_EDITOR

        /// <summary>
        /// Saves this asset to the file. Supported only in Editor.
        /// </summary>
        /// <remarks>If you use saving with the GPU mesh data then the call has to be provided from the thread other than the main game thread.</remarks>
        /// <param name="path">The custom asset path to use for the saving. Use empty value to save this asset to its own storage location. Can be used to duplicate asset. Must be specified when saving virtual asset.</param>
        /// <returns>True if cannot save data, otherwise false.</returns>
        bool Save(const StringView& path = StringView::Empty);

#endif

    private:
        void OnBaseSet();
        void OnBaseUnset();
        void OnBaseUnloaded(Asset* p);
        void OnBaseParamsChanged();

    public:
        // [MaterialBase]
        bool IsMaterialInstance() const override;
#if SE_EDITOR
        void GetReferences(List<UID>& output) const override;
#endif

        // [IMaterial]
        const MaterialInfo& GetInfo() const override;
        GPUShader* GetShader() const override;
        bool IsReady() const override;
        EnumFlags<DrawPass> GetDrawModes() const override;
        bool CanUseLightmap() const override;
        bool CanUseInstancing(InstancingHandler& handler) const override;
        void Bind(BindParameters& params) override;

    protected:
        // [MaterialBase]
        LoadResult load() override;
        void Unload(bool isReloading) override;
        AssetChunksFlag GetChunksToPreload() const override;
    };
} // SE

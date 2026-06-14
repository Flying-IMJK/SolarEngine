#pragma once

#include "MaterialBase.h"
#include "IShaderAsset.h"

namespace SE
{
	class MaterialShader;

	#define MATERIAL_VERSION 1

	// <summary>
	/// Material asset that contains shader for rendering models on the GPU.
	/// </summary>
	SE_CLASS(Reflect, API, NoSpawn)
	class SE_API_RUNTIME Material final : public MaterialBase, public IShaderAsset
	{
		SE_DEFINE_CLASS_DEFAULT(Material, MaterialBase);
		ASSET_HEADER(Material);
	private:
		MaterialShader* m_MaterialShader = nullptr;
		ShaderStorage::Header _shaderHeader;

	public:
		explicit Material(const AssetInfo* info);

		/// <summary>
		/// Tries to load surface graph from the asset.
		/// </summary>
		/// <param name="createDefaultIfMissing">True if create default surface if missing.</param>
		/// <returns>The output surface data, or empty if failed to load.</returns>
		BytesContainer LoadSurface(bool createDefaultIfMissing);

#if SE_EDITOR
		/// <summary>
		/// Updates the material surface (save new one, discard cached data, reload asset).
		/// </summary>
		/// <param name="data">The surface graph data.</param>
		/// <param name="info">The material info structure.</param>
		/// <returns>True if cannot save it, otherwise false.</returns>
		bool SaveSurface(const BytesContainer& data, const MaterialInfo& info);

		// [ShaderAssetBase]
		void InitCompilationOptions(ShaderCompilationOptions& options) override;
#endif

	public:
		// [MaterialBase]
		bool IsMaterialInstance() const override;

		// [IMaterial]
		const MaterialInfo& GetInfo() const override;
		GPUShader* GetShader() const override;
		bool IsReady() const override;
		EnumFlags<DrawPass> GetDrawModes() const override;
		bool CanUseLightmap() const override;
		bool CanUseInstancing(InstancingHandler& handler) const override;
		void Bind(BindParameters& params) override;

	protected:
		// [BinaryAsset]
		bool OnInit(AssetInitData& initData) override;

		// [ShaderAssetBase]
		BinaryAsset* GetShaderAsset() const override
		{
			return (BinaryAsset*)this;
		}

		// [MaterialBase]
		LoadResult load() override;
		void Unload(bool isReloading) override;
		AssetChunksFlag GetChunksToPreload() const override;

#if SE_EDITOR
		void OnDependencyModified(BinaryAsset* asset) override;

		/// <summary>
		/// Saves this shader asset to the storage container.
		/// </summary>
		/// <returns>True if failed, otherwise false.</returns>
		bool Save() override;
#endif
	};
} // SE

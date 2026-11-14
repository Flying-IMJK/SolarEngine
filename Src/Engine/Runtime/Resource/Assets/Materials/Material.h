#pragma once

#include <Runtime/Resource/Assets/Materials/IShaderAsset.h>
#include <Runtime/Resource/Assets/Materials/MaterialBase.h>

namespace SE
{
	class MaterialShader;

	#define MATERIAL_VERSION 1
    #define MATERIAL_FILE_CHUNK_SOURCE_FILES 8

    // <summary>
    /// Material asset that contains shader for rendering models on the GPU.
    /// </summary>
    SE_CLASS(Reflect, API(NoSpawn))
    class SE_API_RUNTIME Material final : public MaterialBase, public IShaderAsset
	{
    private:
        SE_DEFINE_CLASS(Material, MaterialBase);
		ASSET_HEADER(Material);
	private:
		MaterialShader* m_MaterialShader = nullptr;
		ShaderStorage::Header _shaderHeader;
#if SE_EDITOR
        List<String> m_SourceFiles;
        bool         LoadMaterialSourceFiles();
        bool         SaveMaterialSourceFiles();
#endif

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
        void InitCompilationRequest(ShaderCompileRequest& request, StringAnsi& preamble) override;
        void GetAdditionalSourceFiles(List<String>& files) const override { files = m_SourceFiles; }
#endif

	public:
		// [MaterialBase]
		bool IsMaterialInstance() const override;
		MaterialShader* GetMaterialShaderObject() const override;

		// [IMaterial]
		const MaterialInfo& GetInfo() const override;
        GPUShader*      GetShader() const override;
        bool                IsReady() const override;
        DrawPass            GetDrawModes() const override;
        bool CanUseLightmap() const override;
		bool CanUseInstancing(InstancingHandler& handler) const override;
		void Bind(BindParameters& params) override;

        ShaderProgramInstance* GetProgram() override;

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

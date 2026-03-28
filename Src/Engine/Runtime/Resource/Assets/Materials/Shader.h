#pragma once
#include "IShaderAsset.h"

namespace SE
{
	class GPUShader;

	// <summary>
	/// The shader asset. Contains a program that runs on the GPU and is able to perform rendering calculation using textures, vertices and other resources.
	/// </summary>
	class SE_API_RUNTIME Shader final : public BinaryAsset, public IShaderAsset
	{
		SE_CLASS(Shader, BinaryAsset)
	private:
		/// <summary>
		/// The GPU shader object (not null).
		/// </summary>
		GPUShader* m_GpuShader = nullptr;
		ShaderStorage::Header _shaderHeader;

	public:
		explicit Shader(const AssetInfo* info);

		/// <summary>
		/// Finalizes an instance of the <see cref="Shader"/> class.
		/// </summary>
		~Shader() override;

	public:
		/// <summary>
		/// Gets the GPU shader object.
		/// </summary>
		GPUShader* GetShader() const
		{
			return m_GpuShader;
		}

#if SE_EDITOR
		bool Save() override;
#endif

	protected:
		// [BinaryAsset]
		LoadResult load() override;
		void Unload(bool isReloading) override;

		bool OnInit(AssetInitData& initData) override;

		AssetChunksFlag GetChunksToPreload() const override
		{
			AssetChunksFlag result = 0;
			const auto cachingMode = ShaderStorage::GetCachingMode();
			if (cachingMode == ShaderStorage::CachingMode::AssetInternal && !IsNullRenderer())
				result |= GET_CHUNK_FLAG(GetCacheChunkIndex());
			return result;
		}

		BinaryAsset* GetShaderAsset() const override
		{
			return (BinaryAsset*)this;
		}

	public:
		uint32 GetSerializedVersion() const override;
 	};

} // SE

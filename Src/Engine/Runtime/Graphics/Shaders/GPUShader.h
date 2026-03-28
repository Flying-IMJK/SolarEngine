#pragma once

#include "Runtime/API.h"
#include "Core/Types/Collections/Dictionary.h"
#include "Core/Types/Strings/StringView.h"
#include "Core/Serialization/MemoryReadStream.h"

#include "Runtime/Graphics/Base/GPUResource.h"

#include "GPUShaderProgram.h"

namespace SE
{
	class GPUConstantBuffer;

	#define GPU_SHADER_CACHE_VERSION 1

	/// <summary>
	/// 着色程序的集合
	/// </summary>
	class GPUShaderProgramsContainer
	{
	private:
		Dictionary<int32, GPUShaderProgram*> m_Shaders;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="GPUShaderProgramsContainer"/> class.
		/// </summary>
		GPUShaderProgramsContainer();

		/// <summary>
		/// Finalizes an instance of the <see cref="GPUShaderProgramsContainer"/> class.
		/// </summary>
		~GPUShaderProgramsContainer();

	public:
		/// <summary>
		/// Adds a new shader program to the collection.
		/// </summary>
		/// <param name="shader">The shader to store.</param>
		/// <param name="permutationIndex">The shader permutation index.</param>
		void Add(GPUShaderProgram* shader, int32 permutationIndex);

		/// <summary>
		/// Gets a shader of given name and permutation index.
		/// </summary>
		/// <param name="name">The shader program name.</param>
		/// <param name="permutationIndex">The shader permutation index.</param>
		/// <returns>Stored shader program or null if cannot find it.</returns>
		GPUShaderProgram* Get(const StringView& name, int32 permutationIndex) const;

		/// <summary>
		/// Clears collection (deletes all shaders).
		/// </summary>
		void Clear();

	public:
		/// <summary>
		/// Calculates unique hash for given shader program name and its permutation index.
		/// </summary>
		/// <param name="name">The shader program name.</param>
		/// <param name="permutationIndex">The shader program permutation index.</param>
		/// <returns>Calculated hash value.</returns>
		static uint32 CalculateHash(const StringView& name, int32 permutationIndex);
	};


	class GPUShader : public GPUResource
	{
	protected:
		GPUShaderProgramsContainer m_Shaders;
		GPUConstantBuffer* m_ConstantBuffers[MAX_CONSTANT_BUFFER_SLOTS];

		GPUShader();

	public:
		/// <summary>
		/// Creates the shader resource and loads its data from the bytes.
		/// <param name="stream">The stream with compiled shader data.</param>
		/// <returns>True if cannot create state, otherwise false.</returns>
		virtual bool Create(MemoryReadStream & archive);

	public:
		/// <summary>
		/// Gets the vertex shader.
		/// </summary>
		/// <param name="name">The shader program name.</param>
		/// <param name="permutationIndex">The shader permutation index.</param>
		/// <returns>The shader object.</returns>
		inline GPUShaderProgramVS* GetVS(const StringView& name, int32 permutationIndex = 0) const
		{
			return static_cast<GPUShaderProgramVS*>(GetShader(ShaderStage::Vertex, name, permutationIndex));
		}

		/// <summary>
		/// Gets the hull shader.
		/// </summary>
		/// <param name="name">The shader program name.</param>
		/// <param name="permutationIndex">The shader permutation index.</param>
		/// <returns>The shader object.</returns>
		inline GPUShaderProgramHS* GetHS(const StringView& name, int32 permutationIndex = 0) const
		{
			return static_cast<GPUShaderProgramHS*>(GetShader(ShaderStage::Hull, name, permutationIndex));
		}

		/// <summary>
		/// Gets domain shader.
		/// </summary>
		/// <param name="name">The shader program name.</param>
		/// <param name="permutationIndex">The shader permutation index.</param>
		/// <returns>The shader object.</returns>
		inline GPUShaderProgramDS* GetDS(const StringView& name, int32 permutationIndex = 0) const
		{
			return static_cast<GPUShaderProgramDS*>(GetShader(ShaderStage::Domain, name, permutationIndex));
		}

		/// <summary>
		/// Gets the geometry shader.
		/// </summary>
		/// <param name="name">The shader program name.</param>
		/// <param name="permutationIndex">The shader permutation index.</param>
		/// <returns>The shader object.</returns>
		inline GPUShaderProgramGS* GetGS(const StringView& name, int32 permutationIndex = 0) const
		{
			return static_cast<GPUShaderProgramGS*>(GetShader(ShaderStage::Geometry, name, permutationIndex));
		}

		/// <summary>
		/// Gets the pixel shader.
		/// </summary>
		/// <param name="name">The shader program name.</param>
		/// <param name="permutationIndex">The shader permutation index.</param>
		/// <returns>The shader object.</returns>
		inline GPUShaderProgramPS* GetPS(const StringView& name, int32 permutationIndex = 0) const
		{
			return static_cast<GPUShaderProgramPS*>(GetShader(ShaderStage::Pixel, name, permutationIndex));
		}

		/// <summary>
		/// Gets the compute shader.
		/// </summary>
		/// <param name="name">The shader program name.</param>
		/// <param name="permutationIndex">The shader permutation index.</param>
		/// <returns>The shader object.</returns>
		inline GPUShaderProgramCS* GetCS(const StringView& name, int32 permutationIndex = 0) const
		{
			return static_cast<GPUShaderProgramCS*>(GetShader(ShaderStage::Compute, name, permutationIndex));
		}

		/// <summary>
		/// Gets the constant buffer.
		/// </summary>
		/// <param name="slot">The buffer slot index.</param>
		/// <returns>The Constant Buffer object.</returns>
		inline GPUConstantBuffer* GetCB(int32 slot) const
		{
			ENGINE_ASSERT(slot >= 0 && slot < ARRAY_SIZE(m_ConstantBuffers));
			return m_ConstantBuffers[slot];
		}

	public:
		/// <summary>
		/// Determines whether the specified shader program is in the shader.
		/// </summary>
		/// <param name="name">The shader program name.</param>
		/// <param name="permutationIndex">The shader permutation index.</param>
		/// <returns><c>true</c> if the shader is valid; otherwise, <c>false</c>.</returns>
		inline bool HasShader(const StringView& name, int32 permutationIndex = 0) const
		{
			return m_Shaders.Get(name, permutationIndex) != nullptr;
		}

	protected:
		GPUShaderProgram* GetShader(ShaderStage stage, const StringView& name, int32 permutationIndex) const;

		virtual GPUShaderProgram* CreateGPUShaderProgram(ShaderStage type,
			const GPUShaderProgramInitializer& initializer,
			byte* cacheBytes,
			uint32 cacheSize,
			MemoryReadStream& stream) = 0;

	public:
		// [GPUResource]
		GPUResourceType GetResType() const final override;

	protected:
		// [GPUResource]
		void OnReleaseGPU() override;
	};

}

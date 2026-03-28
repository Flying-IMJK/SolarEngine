
#include "GPUShader.h"
#include "Runtime/Graphics/GPUDevice.h"

namespace SE
{
	GPUShaderProgramsContainer::GPUShaderProgramsContainer()
		: m_Shaders(64)
	{
		// TODO: test different values for m_Shaders capacity, test performance impact (less hash collisions but more memory?)
	}

	GPUShaderProgramsContainer::~GPUShaderProgramsContainer()
	{
		// Remember to delete all programs
		m_Shaders.ClearDelete();
	}

	void GPUShaderProgramsContainer::Add(GPUShaderProgram* shader, int32 permutationIndex)
	{
		// Validate input
		ENGINE_ASSERT(shader && Math::RangeInclusive(permutationIndex, 0, SHADER_PERMUTATIONS_MAX_COUNT - 1));

		if ((Get(shader->GetName(), permutationIndex) != nullptr))
		{
			ENGINE_UNREACHABLE_CODE();
		}


		// Store shader
		const int32 hash = CalculateHash(shader->GetName(), permutationIndex);
		m_Shaders.Add(hash, shader);
	}

	GPUShaderProgram* GPUShaderProgramsContainer::Get(const StringView& name, int32 permutationIndex) const
	{
		// Validate input
		ENGINE_ASSERT(name.Length() > 0 && Math::RangeInclusive(permutationIndex, 0, SHADER_PERMUTATIONS_MAX_COUNT - 1));

		// Find shader
		GPUShaderProgram* result = nullptr;
		const int32 hash = CalculateHash(name, permutationIndex);
		m_Shaders.TryGet(hash, result);

		return result;
	}

	void GPUShaderProgramsContainer::Clear()
	{
		m_Shaders.ClearDelete();
	}

	uint32 GPUShaderProgramsContainer::CalculateHash(const StringView& name, int32 permutationIndex)
	{
		return GetHash(name) * 37 + permutationIndex;
	}

	GPUShader::GPUShader() : GPUResource()
	{
		Platform::MemoryClear(m_ConstantBuffers, sizeof(m_ConstantBuffers));
	}

	bool GPUShader::Create(MemoryReadStream& archive)
	{
		ReleaseGPU();

		// Version
		int32 version;
		archive.ReadInt32(&version);

		if (version != GPU_SHADER_CACHE_VERSION)
		{
			LOG_WARNING("Graphic", "Shader Unsupported shader version {0}. The supported version is {1}.", version, GPU_SHADER_CACHE_VERSION);
			return false;
		}

		// Additional data start
		int32 additionalDataStart;
		archive.ReadInt32(&additionalDataStart);

		// Shaders count
		int32 shadersCount;
		archive.ReadInt32(&shadersCount);
		GPUShaderProgramInitializer initializer;

#if !BUILD_RELEASE
		initializer.Owner = this;
		StringView name = GetName();
#else
		const StringView name;
#endif
		GPULimits gpuLimits = GPUDevice::instance->GetGPULimits();
		const bool hasCompute = gpuLimits.HasCompute;
		for (int32 i = 0; i < shadersCount; i++)
		{
			ShaderStage type = static_cast<ShaderStage>(archive.ReadByte());
			int32 permutationsCount = archive.ReadByte();

			ENGINE_ASSERT(Math::RangeInclusive(permutationsCount, 1, SHADER_PERMUTATIONS_MAX_COUNT));

			// Load shader name
			StringAnsi nameTemp;
			archive.ReadStringAnsi(&nameTemp, 11);
			initializer.Name = nameTemp.ToString();

			ENGINE_ASSERT(initializer.Name.HasChars());

			// Load shader flags
			archive.ReadUint32((uint32*)&initializer.Flags);

			for (int32 permutationIndex = 0; permutationIndex < permutationsCount; permutationIndex++)
			{
				// Load cache
				uint32 cacheSize;
				archive.ReadUint32(&cacheSize);
				if (cacheSize > archive.GetLength() - archive.GetPosition())
				{
					LOG_WARNING("Graphic", "Shader Invalid shader cache size.");
					return false;
				}
				byte* cache = archive.Move<byte>(cacheSize);

				// Read bindings
				archive.ReadBytes(&initializer.Bindings, sizeof(ShaderBindings));

				// Create shader program
				if (type == ShaderStage::Compute && !hasCompute)
				{
					LOG_WARNING("Graphic", "Shader Failed to create {} Shader program '{}' ({}).", (int32)type, initializer.Name, name);
					continue;
				}
				GPUShaderProgram* shader = CreateGPUShaderProgram(type, initializer, cache, cacheSize, archive);
				if (shader == nullptr)
				{
					// 不支持曲面细分
					if (!gpuLimits.HasTessellation && type == ShaderStage::Hull || type == ShaderStage::Domain)
						continue;

					// 不支持几何着色器
					if (!gpuLimits.HasGeometryShaders && type == ShaderStage::Geometry)
						continue;

					LOG_ERROR("Graphic", "Shader Failed to create {} Shader program '{}' ({}).", (int32)type, initializer.Name, name);
					return false;
				}

				// Add to collection
				m_Shaders.Add(shader, permutationIndex);
			}
		}

		// Constant Buffers
		const byte constantBuffersCount = archive.ReadByte();
		const byte maximumConstantBufferSlot = archive.ReadByte();
		if (constantBuffersCount > 0)
		{
			ENGINE_ASSERT(maximumConstantBufferSlot < MAX_CONSTANT_BUFFER_SLOTS);

			for (int32 i = 0; i < constantBuffersCount; i++)
			{
				// Load info
				const byte slotIndex = archive.ReadByte();
				uint32 size;
				archive.ReadUint32(&size);

				// Create CB
#if GPU_ENABLE_RESOURCE_NAMING
				String name = String::Format(SE_TEXT("{}.CB{}"), GetName(), i);
#else
				String name;
#endif
				ENGINE_ASSERT(m_ConstantBuffers[slotIndex] == nullptr);
				const auto cb = GPUDevice::instance->CreateConstantBuffer(size, name);
				if (cb == nullptr)
				{
					LOG_WARNING("Graphic", "Shader Failed to create shader constant buffer.");
					return false;
				}
				m_ConstantBuffers[slotIndex] = cb;
			}
		}

		// Don't read additional data

		m_MemoryUsage = 1;
		return true;
	}

	GPUShaderProgram* GPUShader::GetShader(ShaderStage stage, const StringView& name, int32 permutationIndex) const
	{
		const auto shader = m_Shaders.Get(name, permutationIndex);

#if BUILD_RELEASE
		// Release build is more critical on that
		ENGINE_ASSERT(shader != nullptr && shader->GetStage() == stage);
#else

		if (shader == nullptr)
		{
			LOG_ERROR("Graphic", "Shader Missing {0} shader \'{1}\'[{2}]. Object: {3}.",
				(int32)stage, name, permutationIndex, ToString());
		}
		else if (shader->GetStage() != stage)
		{
			LOG_ERROR("Graphic", "Shader Invalid shader stage \'{1}\'[{2}]. Expected: {0}. Actual: {4}. Object: {3}.",
				(int32)stage, name, permutationIndex, ToString(), (int32)shader->GetStage());
		}

#endif

		return shader;
	}

	GPUResourceType GPUShader::GetResType() const
	{
		return GPUResourceType::Shader;
	}

	void GPUShader::OnReleaseGPU()
	{
		for (GPUConstantBuffer*& cb : m_ConstantBuffers)
		{
			if (cb)
			{
//				SAFE_DELETE_GPU_RESOURCE(cb);
				cb = nullptr;
			}
		}
		m_MemoryUsage = 0;
		m_Shaders.Clear();
	}

}
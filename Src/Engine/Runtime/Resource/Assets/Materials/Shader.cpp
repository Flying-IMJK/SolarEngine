
#include "Shader.h"

#include "Core/Serialization/MemoryReadStream.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/Shaders/GPUShader.h"
#include "Runtime/Resource/Factories/BinaryAssetFactory.h"

namespace SE
{
	BINARY_ASSET_FACTORY(Shader, false);

	Shader::Shader(const AssetInfo* info) : BinaryAsset(info)
	{
		ENGINE_ASSERT(GPUDevice::instance);
		m_GpuShader = GPUDevice::instance->CreateShader(info->path);
		ENGINE_ASSERT(m_GpuShader);
	}

	Shader::~Shader()
	{
		Delete(m_GpuShader);
	}

	bool Shader::Save()
	{
		auto parent = GetShaderAsset();
		AssetInitData data;
		data.SerializedVersion = ShaderStorage::Header::Version;
		data.CustomData.Link(&_shaderHeader);
		parent->metadata.Release();
		return parent->SaveAsset(data);
	}

	Asset::LoadResult Shader::load()
	{
		// Special case for Null renderer that doesn't need shaders
		if (IsNullRenderer())
		{
			return LoadResult::Ok;
		}

		// Load shader cache (it may call compilation or gather cached data)
		ShaderCacheResult shaderCache;
		if (!LoadShaderCache(shaderCache))
		{
			LOG_ERROR("Resource", "Cannot load \'{0}\' shader cache.", ToString());
			return LoadResult::Failed;
		}
		MemoryReadStream shaderCacheStream(shaderCache.Data.Get(), shaderCache.Data.Length());

		// Create shader from cache
		if (!m_GpuShader->Create(shaderCacheStream))
		{
			LOG_ERROR("Resource", "Cannot load shader \'{0}\'", ToString());
			return LoadResult::Failed;
		}

		RegisterForShaderReloads(this, shaderCache);

		return LoadResult::Ok;
	}

	void Shader::Unload(bool isReloading)
	{
		UnregisterForShaderReloads(this);
		m_GpuShader->ReleaseGPU();
	}

	bool Shader::OnInit(AssetInitData& initData)
	{
		// Validate version
		if (initData.SerializedVersion != ShaderStorage::Header::Version)
		{
			LOG_WARNING("Resource", "Invalid shader serialized version.");
			return false;
		}

		// Validate data
		if (initData.CustomData.Length() != sizeof(_shaderHeader))
		{
			LOG_WARNING("Resource", "Invalid shader header.");
			return false;
		}

		// Load header 'as-is'
		Platform::MemoryCopy(&_shaderHeader, initData.CustomData.Get(), sizeof(_shaderHeader));

		return true;
	}

	uint32 Shader::GetSerializedVersion() const
	{
		return 1;
	}
} // SE
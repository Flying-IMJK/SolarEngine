#include "ShaderBindingSnapshot.h"

#include "Runtime/Core/Memory/Memory.h"

namespace SE
{
	bool ShaderBindingSnapshot::IsFrozen() const
	{
		return _frozen;
	}

	const List<ShaderBindingResource>& ShaderBindingSnapshot::GetResources() const
	{
		return _resources;
	}

	const List<ShaderBindingUniformData>& ShaderBindingSnapshot::GetUniformData() const
	{
		return _uniformData;
	}

	void ShaderBindingSnapshot::Clear()
	{
		_frozen = false;
		_resources.Clear();
		_uniformData.Clear();
	}

	void ShaderBindingSnapshot::AddResource(const ShaderBindingResource& resource)
	{
		_resources.Add(resource);
	}

	void ShaderBindingSnapshot::AddUniformData(const uint32 blockId, const List<byte>& data)
	{
		ShaderBindingUniformData uniformData;
		uniformData.BlockId = blockId;
		uniformData.Data.Resize(data.Count());
		if (data.HasItems())
		{
			// 快照保存 uniform 数据副本，避免后端写 descriptor 时依赖仍可修改的 ParameterBlock 内存。
			Platform::MemoryCopy(uniformData.Data.Get(), data.Get(), data.Count());
		}
		_uniformData.Add(MoveTemp(uniformData));
	}

	void ShaderBindingSnapshot::Freeze()
	{
		_frozen = true;
	}
}

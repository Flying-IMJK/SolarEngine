#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Collections/List.h"

namespace SE
{
	class ShaderParameterBlock;
	class ShaderProgramInstance;

	enum class ShaderRuntimeResourceType : byte
	{
		Texture,
		Buffer,
		Sampler,
	};

	struct SE_API_RUNTIME ShaderBindingResource
	{
		uint32 BlockId = 0;
		uint32 RangeIndex = 0;
		uint32 ResourceIndex = 0;
		ShaderRuntimeResourceType Type = ShaderRuntimeResourceType::Texture;
		void* Value = nullptr;
	};

	struct SE_API_RUNTIME ShaderBindingUniformData
	{
		uint32 BlockId = 0;
		List<byte> Data;
	};

	// P4 的平台无关提交边界。P5 只能读取此快照，不能读取或回写可变 ParameterBlock。
	// Freeze 后代表本次 dispatch 的绑定内容已经确定，后端可安全按快照写 descriptor。
	class SE_API_RUNTIME ShaderBindingSnapshot
	{
	public:
		bool IsFrozen() const;
		const List<ShaderBindingResource>& GetResources() const;
		const List<ShaderBindingUniformData>& GetUniformData() const;

	private:
		friend class ShaderParameterBlock;
		friend class ShaderProgramInstance;

		void Clear();
		void AddResource(const ShaderBindingResource& resource);
		void AddUniformData(uint32 blockId, const List<byte>& data);
		void Freeze();

		bool _frozen = false;
		List<ShaderBindingResource> _resources;
		List<ShaderBindingUniformData> _uniformData;
	};
}

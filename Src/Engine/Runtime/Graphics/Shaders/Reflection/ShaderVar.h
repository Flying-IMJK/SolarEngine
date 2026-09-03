#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Strings/String.h"

namespace SE
{
	class ShaderParameterBlock;
	class ShaderProgramReflection;

	// ShaderVar 在反射树中的定位结果；既能表示普通 uniform 偏移，也能表示资源 range/index。
	struct SE_API_RUNTIME ShaderVarLocation
	{
		uint32 BlockId = 0;
		int32 UniformByteOffset = -1;
		int32 ResourceRangeIndex = -1;
		int32 ResourceIndex = -1;
		uint32 TypeId = 0;
	};

	// 面向上层的 shader 参数访问句柄。
	// 它不拥有数据，只把反射定位结果转发到对应 ShaderParameterBlock。
	class SE_API_RUNTIME ShaderVar
	{
	public:
		ShaderVar() = default;
		ShaderVar(ShaderParameterBlock* parameterBlock, const ShaderProgramReflection* reflection, const ShaderVarLocation& location)
			: m_ParameterBlock(parameterBlock)
			, _reflection(reflection)
			, _location(location)
		{
		}

		bool FindMember(const String& name, ShaderVar& result) const;
		bool GetElement(uint32 index, ShaderVar& result) const;
		ShaderVar operator[](uint32 index) const;
		bool SetTexture(void* value) const;
		bool SetBuffer(void* value) const;
		bool SetSampler(void* value) const;
		bool SetUniform(const void* data, uint32 size) const;

		const ShaderVarLocation& GetLocation() const
		{
			return _location;
		}

		bool IsValid() const
		{
			return m_ParameterBlock != nullptr && _reflection != nullptr;
		}

	private:
		ShaderParameterBlock* m_ParameterBlock = nullptr;
		const ShaderProgramReflection* _reflection = nullptr;
		ShaderVarLocation _location;
	};
}

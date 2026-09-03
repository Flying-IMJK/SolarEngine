#pragma once

#include "Runtime/API.h"
#include "Reflection/ShaderVar.h"
#include "ShaderBindingSnapshot.h"
#include "ShaderParameterBlock.h"

namespace SE
{
	class SLC2ShaderProgram;
	class ShaderProgramReflection;

	// 一次 Shader 使用的可变参数状态。
	// 它引用不可变 SLC2ShaderProgram，同时持有 root ParameterBlock 中的资源绑定和普通 uniform 数据。
	class SE_API_RUNTIME ShaderProgramInstance
	{
	public:
		bool Initialize(SLC2ShaderProgram* program);
		ShaderVar GetRootVar();
		bool FindVar(const String& path, ShaderVar& result);
		bool SetTexture(const String& path, void* value);
		bool SetBuffer(const String& path, void* value);
		bool SetSampler(const String& path, void* value);
		bool SetUniform(const String& path, const void* data, uint32 size);
		// 将当前可变参数状态冻结为后端只读快照，后端只消费快照，不再回读 ParameterBlock。
		bool ValidateAllBindings(ShaderBindingSnapshot& snapshot) const;

		SLC2ShaderProgram* GetProgram() const;

	private:
		// 路径解析只依赖不可变 reflection；实际写入由解析出的 ShaderVarLocation 再落到 ParameterBlock。
		bool Resolve(const String& path, ShaderVarLocation& location) const;

		SLC2ShaderProgram* m_Program = nullptr;
		ShaderParameterBlock m_RootParameters;
	};
}

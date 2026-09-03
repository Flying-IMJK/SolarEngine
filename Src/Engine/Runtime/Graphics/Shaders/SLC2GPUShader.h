#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Graphics/Base/GPUResource.h"
#include "Runtime/ShaderCompilation/ShaderCompileTypes.h"
#include "Runtime/ShaderCompilation/Slang/SLC2/SLC2Artifact.h"
#include "Reflection/ShaderProgramReflection.h"

namespace SE
{
	class ShaderProgramInstance;

	// 运行时选择一个已编译 SLC2 变体的最小输入：
	// ProgramId 定位 SHADER_PROGRAM，Target 定位编译目标，Defines 经变体规划器归一化后定位 Variant。
	struct SE_API_RUNTIME ShaderProgramSelection
	{
		String ProgramId;
		ShaderCompileTarget Target;
		List<String> Defines;
	};

	// 已选中 Program/Target/Variant 后生成的不可变 ShaderProgram 缓存对象。
	// 平台无关层只保存 SLC2 指针和反射；具体 GPU 后端在派生类中持有 shader module / pipeline layout 等对象。
	class SE_API_RUNTIME SLC2ShaderProgram
	{
	public:
		virtual ~SLC2ShaderProgram()
		{
		}

		virtual bool Initialize(const SLC2ProgramRecord* program, const SLC2TargetRecord* target, const SLC2VariantRecord* variant);

		const SLC2VariantRecord* GetVariant() const;
		const ShaderProgramReflection& GetReflection() const;

	protected:
		const SLC2ProgramRecord* _program = nullptr;
		const SLC2TargetRecord* _target = nullptr;
		const SLC2VariantRecord* _variant = nullptr;
		ShaderProgramReflection _reflection;
	};

	class SE_API_RUNTIME SLC2GPUShader : public GPUResource
	{
	protected:
		SLC2GPUShader();

	public:
		~SLC2GPUShader() override;

		bool Load(const List<byte>& data);
		// 根据 selection 复用或创建不可变 ShaderProgram，并初始化一次可变的 ShaderProgramInstance 绑定状态。
		bool CreateProgramInstance(const ShaderProgramSelection& selection, ShaderProgramInstance& instance);

	private:
		bool SelectProgram(const ShaderProgramSelection& selection, const SLC2ProgramRecord*& program, const SLC2TargetRecord*& target, const SLC2VariantRecord*& variant) const;
		bool GetOrCreateProgram(const SLC2ProgramRecord* program, const SLC2TargetRecord* target, const SLC2VariantRecord* variant, SLC2ShaderProgram*& shaderProgram);
		void ReleasePrograms();

	protected:
		virtual SLC2ShaderProgram* CreateSLC2ShaderProgram(const SLC2ProgramRecord* program, const SLC2TargetRecord* target, const SLC2VariantRecord* variant) = 0;

	public:
		GPUResourceType GetResType() const final override;

	protected:
		void OnReleaseGPU() override;

	private:
		// 反序列化后的 SLC2 Artifact 是 GPUShader 的资源内容；_programs 只缓存被实际选择过的运行时后端 Program。
		SLC2Artifact _artifact;
		List<SLC2ShaderProgram*> _programs;
	};
}

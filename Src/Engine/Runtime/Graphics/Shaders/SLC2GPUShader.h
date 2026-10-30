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
    class SLC2GPUShaderProgram;

	class SE_API_RUNTIME SLC2GPUShader : public GPUResource
	{
	protected:
		SLC2GPUShader();

	public:
		~SLC2GPUShader() override;

		bool Load(const byte* data, int dataLength);

		// 根据 ProgramName 和 变体列表 复用或创建不可变 ShaderProgram。
        bool GetOrCreateProgram(const String&                   programName,
                                const List<String>&             defines, 
                                SLC2GPUShaderProgram*& program);

		bool GetOrCreateProgram(const String& programName, SLC2GPUShaderProgram*& program);

        GPUResourceType GetResType() const final override;

	private:
        static bool ResolveProgramSelection(const SLC2Artifact&       artifact,
                                            const String&             programName,
                                            const List<String>&       Defines,
                                            const SLC2ProgramRecord*& program,
                                            const SLC2TargetRecord*&  target,
                                            const SLC2VariantRecord*& variant);

        bool SelectProgram(const String&             programName,
                           const List<String>&       defines,
                           const SLC2ProgramRecord*& program,
                           const SLC2TargetRecord*&  target,
                           const SLC2VariantRecord*& variant) const;

		bool CreateProgram(const SLC2ProgramRecord* program, const SLC2TargetRecord* target, const SLC2VariantRecord* variant, SLC2GPUShaderProgram*& shaderProgram);
		void ReleasePrograms();

	protected:
		virtual SLC2GPUShaderProgram* CreateSLC2ShaderProgram(const SLC2ProgramRecord* program, const SLC2TargetRecord* target, const SLC2VariantRecord* variant) = 0;

		void OnReleaseGPU() override;

	private:
		SLC2Artifact m_Artifact;
		List<SLC2GPUShaderProgram*> m_Programs;
	};
}

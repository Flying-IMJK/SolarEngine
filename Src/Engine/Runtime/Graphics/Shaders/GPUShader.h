#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Graphics/Base/GPUResource.h"
#include "Runtime/ShaderCompilation/ShaderCompileTypes.h"
#include "Runtime/ShaderCompilation/Slang/Artifact/ShaderArtifact.h"
#include "Reflection/ShaderProgramReflection.h"

namespace SE
{
	class ShaderProgramInstance;
    class GPUShaderProgram;

	class SE_API_RUNTIME GPUShader : public GPUResource
	{
	protected:
		GPUShader();

	public:
		~GPUShader() override;

		bool Load(const byte* data, int dataLength);
        bool HasProgram(const String& programName) const;
        /// Read-only selection for the active target; does not create or cache a GPU Program.
        /// The owner must keep the loaded artifact unchanged for the duration of this query.
        bool IsGraphicsProgram(const String& programName, const List<String>& defines) const;

        // 根据 ProgramName 和 变体列表 复用或创建不可变 ShaderProgram。
        bool GetOrCreateProgram(const String&                   programName,
                                const List<String>&             defines, 
                                GPUShaderProgram*& program);

		bool GetOrCreateProgram(const String& programName, GPUShaderProgram*& program);

        GPUResourceType GetResType() const final override;

	private:
        static bool ResolveProgramSelection(const ShaderArtifact&       artifact,
                                            const String&             programName,
                                            const List<String>&       Defines,
                                            const ShaderProgramRecord*& program,
                                            const ShaderTargetRecord*&  target,
                                            const ShaderVariantRecord*& variant);

        bool SelectProgram(const String&             programName,
                           const List<String>&       defines,
                           const ShaderProgramRecord*& program,
                           const ShaderTargetRecord*&  target,
                           const ShaderVariantRecord*& variant) const;

		bool CreateProgram(const ShaderProgramRecord* program, const ShaderTargetRecord* target, const ShaderVariantRecord* variant, GPUShaderProgram*& shaderProgram);
		void ReleasePrograms();

	protected:
		virtual GPUShaderProgram* CreateShaderProgram(const ShaderProgramRecord* program, const ShaderTargetRecord* target, const ShaderVariantRecord* variant) = 0;

		void OnReleaseGPU() override;

	private:
		ShaderArtifact m_Artifact;
		List<GPUShaderProgram*> m_Programs;
	};
}

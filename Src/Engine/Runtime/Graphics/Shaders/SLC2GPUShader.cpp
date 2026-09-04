#include "SLC2GPUShader.h"

#include "ShaderProgramInstance.h"
#include "SLC2GPUShaderProgram.h"
#include "Runtime/Core/Memory/Memory.h"
#include "Runtime/ShaderCompilation/Slang/SLC2/SLC2Reader.h"
#include "Runtime/ShaderCompilation/Slang/ShaderVariantPlanner.h"

namespace SE
{
	SLC2GPUShader::SLC2GPUShader()
		: GPUResource()
	{
	}

	SLC2GPUShader::~SLC2GPUShader()
	{
		ReleasePrograms();
	}

	bool SLC2GPUShader::Load(const List<byte>& data)
	{
		SLC2Artifact artifact;
        String       error;
		if (!SLC2Reader::Read(data, artifact, error))
		{
            LOG_ERROR("Graphics", "Failed to read SLC2 artifact: {0}", error);
			return false;
		}
		ReleasePrograms();
		_artifact = MoveTemp(artifact);
		int32 programCapacity = 0;
		for (int32 programIndex = 0; programIndex < _artifact.Programs.Count(); programIndex++)
		{
			for (int32 targetIndex = 0; targetIndex < _artifact.Programs[programIndex].Targets.Count(); targetIndex++)
			{
				programCapacity += _artifact.Programs[programIndex].Targets[targetIndex].Variants.Count();
			}
		}
		m_Programs.SetCapacity(programCapacity, false);
		m_MemoryUsage = data.Count();
		return true;
	}

	bool SLC2GPUShader::SelectProgram(const ShaderProgramSelection& selection, const SLC2ProgramRecord*& program, const SLC2TargetRecord*& target, const SLC2VariantRecord*& variant) const
	{
		program = nullptr;
		target = nullptr;
		variant = nullptr;
		for (int32 index = 0; index < _artifact.Programs.Count(); index++)
		{
			if (_artifact.Programs[index].ProgramId == selection.ProgramId)
			{
				program = &_artifact.Programs[index];
				break;
			}
		}
		if (program == nullptr)
		{
			LOG_ERROR("Graphics", "ProgramId was not found.");
			return false;
		}
		const String targetKey = BuildTargetKey(selection.Target);
		for (int32 index = 0; index < program->Targets.Count(); index++)
		{
			if (program->Targets[index].TargetKey == targetKey)
			{
				target = &program->Targets[index];
				break;
			}
		}
		if (target == nullptr)
		{
			LOG_ERROR("Graphics", "exact target was not found.");
			return false;
		}
		ShaderVariantPlan normalized;
        String            error;
		if (!ShaderVariantPlanner::Normalize(program->VariantGroups, selection.Defines, normalized, error))
		{
			LOG_ERROR("Graphics", "Failed to normalize shader variant: {0}", error);
			return false;
		}
		// 运行时输入的 Defines 不直接比较字符串，而是先归一化到编译端同一套 Variant key。
		for (int32 index = 0; index < target->Variants.Count(); index++)
		{
			if (target->Variants[index].Variant == normalized.Variant)
			{
				variant = &target->Variants[index];
				break;
			}
		}
		if (variant == nullptr)
		{
			LOG_ERROR("Graphics", "SLC2 requested variant was not compiled.");
			return false;
		}
		return true;
	}

	bool SLC2GPUShader::GetOrCreateProgram(const SLC2ProgramRecord* program, const SLC2TargetRecord* target, const SLC2VariantRecord* variant, SLC2GPUShaderProgram*& shaderProgram)
	{
		// Variant 记录来自 _artifact，指针在下一次 Load/Release 前稳定，可作为当前 GPUShader 内部缓存 key。
		for (int32 index = 0; index < m_Programs.Count(); index++)
		{
			if (m_Programs[index]->GetVariant() == variant)
			{
				shaderProgram = m_Programs[index];
				return true;
			}
		}
		SLC2GPUShaderProgram* created = CreateSLC2ShaderProgram(program, target, variant);
		if (created == nullptr)
		{
			return false;
		}
		m_Programs.Add(created);
		shaderProgram = created;
		return true;
	}

	bool SLC2GPUShader::CreateProgramInstance(const ShaderProgramSelection& selection, ShaderProgramInstance& instance)
	{
		const SLC2ProgramRecord* program;
		const SLC2TargetRecord* target;
		const SLC2VariantRecord* variant;
		if (!SelectProgram(selection, program, target, variant))
		{
			return false;
		}
		SLC2GPUShaderProgram* shaderProgram = nullptr;
		if (!GetOrCreateProgram(program, target, variant, shaderProgram))
		{
			return false;
		}
		return instance.Initialize(shaderProgram);
	}

	void SLC2GPUShader::ReleasePrograms()
	{
		m_Programs.ClearDelete();
	}

	GPUResourceType SLC2GPUShader::GetResType() const
	{
		return GPUResourceType::Shader;
	}

	void SLC2GPUShader::OnReleaseGPU()
	{
		ReleasePrograms();
		m_MemoryUsage = 0;
	}
}

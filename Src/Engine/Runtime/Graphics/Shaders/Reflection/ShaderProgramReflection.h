#pragma once

#include "Runtime/API.h"
#include "Runtime/ShaderCompilation/Slang/ShaderReflectionIR.h"

namespace SE
{
	class SE_API_RUNTIME ShaderProgramReflection
	{
	public:
		bool Initialize(const ShaderReflectionIR& layout);

		const ShaderReflectionIR& GetLayout() const;
		const ShaderIRTypeRecord* GetType(uint32 typeId) const;
		const ShaderIRParameterBlock* GetBlock(uint32 blockId) const;
		const ShaderIRMember* FindMember(uint32 typeId, const String& name) const;
		const ShaderIRRangeBinding* FindRangeBinding(uint32 blockId, uint32 rangeIndex) const;

	private:
		const ShaderReflectionIR* m_Layout = nullptr;
	};
}

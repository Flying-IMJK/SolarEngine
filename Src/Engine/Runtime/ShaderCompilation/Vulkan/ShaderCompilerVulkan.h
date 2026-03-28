#pragma once

#include "Runtime/ShaderCompilation/ShaderCompiler.h"

namespace SE
{
	/// <summary>
	/// Implementation of shaders compiler for Vulkan rendering backend.
	/// </summary>
	class SE_API_RUNTIME ShaderCompilerVulkan : public ShaderCompiler
	{
	private:

		List<char> _funcNameDefineBuffer;

	public:

		/// <summary>
		/// Initializes a new instance of the <see cref="ShaderCompilerVulkan"/> class.
		/// </summary>
		/// <param name="profile">The profile.</param>
		ShaderCompilerVulkan(ShaderProfile profile);

		/// <summary>
		/// Finalizes an instance of the <see cref="ShaderCompilerVulkan"/> class.
		/// </summary>
		~ShaderCompilerVulkan();

	protected:

		// [ShaderCompiler]
		bool CompileShader(ShaderFunctionMeta& meta, WritePermutationData customDataWrite = nullptr) override;
		bool OnCompileBegin() override;
	};

}

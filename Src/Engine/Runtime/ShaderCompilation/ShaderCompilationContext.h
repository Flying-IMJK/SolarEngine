#pragma once

#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/Core/Types/Collections/HashSet.h"
#include "Runtime/Core/Types/UID.h"

#include "Runtime/API.h"
#include "Runtime/Graphics/Shaders/Config.h"
#include "Runtime/Graphics/Base/GPUEnums.h"


namespace SE
{
	class ShaderMeta;
	class ShaderFunctionMeta;
	class MemoryWriteStream;

	/// <summary>
	/// Shader compilation options container
	/// </summary>
	struct SE_API_RUNTIME ShaderCompilationOptions
	{
		public:

		/// <summary>
		/// Name of the target object (name of the shader or material for better logging readability)
		/// </summary>
		String TargetName;

		/// <summary>
		/// Unique ID of the target object
		/// </summary>
		UID TargetID = UID::Empty;

		/// <summary>
		/// Shader source code (null terminated)
		/// </summary>
		const char* Source = nullptr;

		/// <summary>
		/// Shader source code length
		/// </summary>
		uint32 SourceLength = 0;

		public:

		/// <summary>
		/// Target shader profile
		/// </summary>
		ShaderProfile Profile = ShaderProfile::Unknown;


		bool debugInfo;

		/// <summary>
		/// Disables shaders compiler optimizations. Can be used to debug shaders on a target platform or to speed up the shaders compilation time.
		/// </summary>
		bool NoOptimize = false;

		/// <summary>
		/// Enables shader debug data generation (depends on the target platform rendering backend).
		/// </summary>
		bool GenerateDebugData = false;

		/// <summary>
		/// Enable/disable promoting warnings to compilation errors
		/// </summary>
		bool TreatWarningsAsErrors = false;

		/// <summary>
		/// 强制输出编译代码
		/// </summary>
		bool ForceStoreCompilationSource = false;

		/// <summary>
		/// Custom macros for the shader compilation
		/// </summary>
		List<ShaderMacro> Macros;

		public:

		/// <summary>
		/// Output stream to write compiled shader cache to
		/// </summary>
		MemoryWriteStream* Output = nullptr;
	};

	/// <summary>
	/// Shader compilation context container
	/// </summary>
	class ShaderCompilationContext
	{
	public:

		/// <summary>
		/// The compilation options.
		/// </summary>
		const ShaderCompilationOptions* Options;

		/// <summary>
		/// The shader metadata container.
		/// </summary>
		ShaderMeta* Meta;

	public:

		/// <summary>
		/// Output stream to write compiled shader cache to.
		/// </summary>
		MemoryWriteStream* Output;

		/// <summary>
		/// All source files included by this file (absolute paths). Generated during shader compilation.
		/// </summary>
		HashSet<String> Includes;

	public:

		/// <summary>
		/// Name of the target object (in ASCII)
		/// </summary>
		char TargetNameAnsi[64];

	public:

		#define ShaderCompilationError(context, message) context->OnError(message, __FILE__, __LINE__);

		/// <summary>
		/// Event called on compilation error
		/// </summary>
		/// <param name="message">Error message</param>
		void OnError(const char* message, const char* file, int line);

		/// <summary>
		/// Event called on compilation debug data collecting
		/// </summary>
		/// <param name="meta">Target function meta</param>
		/// <param name="permutationIndex">Permutation index</param>
		/// <param name="data">Data pointer</param>
		/// <param name="dataLength">Data size in bytes</param>
		void OnCollectDebugInfo(ShaderFunctionMeta& meta, int32 permutationIndex, const char* data, const int32 dataLength);

	public:

		/// <summary>
		/// Init
		/// </summary>
		/// <param name="options">Options</param>
		/// <param name="meta">Metadata</param>
		ShaderCompilationContext(const ShaderCompilationOptions* options, ShaderMeta* meta);
	};

}

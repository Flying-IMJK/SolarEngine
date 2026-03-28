#pragma once

#include "ShaderCompilationContext.h"
#include "Parser/ShaderMeta.h"
#include "Runtime/Graphics/Shaders/GPUShaderProgram.h"

namespace SE
{
	/// <summary>
	/// Base class for the objects that can compile shaders source code.
	/// </summary>
	class ShaderCompiler
	{
	public:
		struct ShaderResourceBuffer
		{
			byte Slot;
			bool IsUsed;
			uint32 Size;
		};

	private:
		List<char> m_FuncNameDefineBuffer;

	protected:

		ShaderProfile m_Profile;
		ShaderCompilationContext* m_Context = nullptr;
		List<ShaderMacro> m_GlobalMacros;
		List<ShaderMacro> m_Macros;
		List<ShaderResourceBuffer> m_ConstantBuffers;

	public:

		/// <summary>
		/// Initializes a new instance of the <see cref="ShaderCompiler"/> class.
		/// </summary>
		/// <param name="profile">The profile.</param>
		ShaderCompiler(ShaderProfile profile)
			: m_Profile(profile)
		{
		}

		/// <summary>
		/// Finalizes an instance of the <see cref="ShaderCompiler"/> class.
		/// </summary>
		virtual ~ShaderCompiler() = default;

	public:

		/// <summary>
		/// Gets shader profile supported by this compiler.
		/// </summary>
		/// <returns>The shader profile.</returns>
		FORCE_INLINE ShaderProfile GetProfile() const
		{
			return m_Profile;
		}

		/// <summary>
		/// Performs the shader compilation.
		/// </summary>
		/// <param name="context">The compilation context.</param>
		/// <returns>True if failed, otherwise false.</returns>
		bool Compile(ShaderCompilationContext* context);

		/// <summary>
		/// Gets the included file source code. Handles system includes and absolute includes. Method is thread-safe.
		/// </summary>
		/// <param name="context">The compilation context.</param>
		/// <param name="sourceFile">The source file that is being compiled.</param>
		/// <param name="includedFile">The included file name (absolute or relative).</param>
		/// <param name="source">The output source code of the file (null-terminated), null if failed to load.</param>
		/// <param name="sourceLength">The output source code length of the file (characters count), 0 if failed to load.</param>
		/// <returns>True if failed, otherwise false.</returns>
		static bool GetIncludedFileSource(ShaderCompilationContext* context, const char* sourceFile, const char* includedFile, const char*& source, int32& sourceLength);

		/// <summary>
		/// Clears the cache used by the shader includes.
		/// </summary>
		static void DisposeIncludedFilesCache();

	protected:

		typedef bool (*WritePermutationData)(ShaderCompilationContext*, ShaderFunctionMeta&, int32, const List<ShaderMacro>&);

		virtual bool CompileShader(ShaderFunctionMeta& meta, WritePermutationData customDataWrite = nullptr) = 0;

		bool CompileShaders();

		virtual bool OnCompileBegin();
		virtual bool OnCompileEnd();

		static bool WriteShaderFunctionBegin(ShaderCompilationContext* context, ShaderFunctionMeta& meta);
		static bool WriteShaderFunctionPermutation(ShaderCompilationContext* context, ShaderFunctionMeta& meta, int32 permutationIndex, const ShaderBindings& bindings, const void* header, int32 headerSize, const void* cache, int32 cacheSize);
		static bool WriteShaderFunctionPermutation(ShaderCompilationContext* context, ShaderFunctionMeta& meta, int32 permutationIndex, const ShaderBindings& bindings, const void* cache, int32 cacheSize);
		static bool WriteShaderFunctionEnd(ShaderCompilationContext* context, ShaderFunctionMeta& meta);
		static bool WriteCustomDataVS(ShaderCompilationContext* context, ShaderFunctionMeta& meta, int32 permutationIndex, const List<ShaderMacro>& macros);
		static bool WriteCustomDataHS(ShaderCompilationContext* context, ShaderFunctionMeta& meta, int32 permutationIndex, const List<ShaderMacro>& macros);
		void GetDefineForFunction(ShaderFunctionMeta& meta, List<ShaderMacro>& macros);
	};

} // SE


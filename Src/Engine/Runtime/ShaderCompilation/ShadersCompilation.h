#pragma once

#include "Runtime/API.h"
#include "ShaderCompiler.h"
#include "ShaderCompileTypes.h"

namespace SE
{
	class Asset;

	/// <summary>
	/// Shaders compilation service allows to compile shader source code for a desire platform. Supports multi-threading.
	/// </summary>
	class SE_API_RUNTIME ShadersCompilation
	{
	public:
		/// <summary>
		/// Compiles the shader.
		/// </summary>
		/// <param name="options">Compilation options</param>
		/// <returns>True if failed, otherwise false</returns>
		static bool Compile(ShaderCompilationOptions& options);

		/// <summary>
		/// Compiles a Slang-only shader resource into SLC2 text data. This path is offline-only and does not touch GPU runtime objects.
		/// </summary>
		/// <param name="request">Compilation request.</param>
		/// <returns>Compilation status, diagnostics and SLC2 data.</returns>
		static ShaderCompileResult CompileSlang(const ShaderCompileRequest& request);

		/// <summary>
		/// Runs a minimal offline Slang compilation self-test and returns the generated SLC2 data.
		/// </summary>
		/// <remarks>
		/// This helper is intentionally offline-only. It allows editor commands or tests to verify
		/// the compiler pipeline before runtime shader parameter binding exists.
		/// </remarks>
		static ShaderCompileResult CompileSlangOfflineSelfTest();

		/// <summary>
		/// Registers shader asset for the automated reloads on source includes changes.
		/// </summary
		/// <param name="asset">The asset.</param>
		/// <param name="includedPath">The included file path.</param>
		static void RegisterForShaderReloads(Asset* asset, const String& includedPath);

		/// <summary>
		/// Unregisters shader asset from the automated reloads on source includes changes.
		/// </summary>
		/// <param name="asset">The asset.</param>
		static void UnregisterForShaderReloads(Asset* asset);

		/// <summary>
		/// Reads the included shader files stored in the shader cache data.
		/// </summary>
		/// <param name="shaderCache">The shader cache data.</param>
		/// <param name="shaderCacheLength">The shader cache data length (in bytes).</param>
		/// <param name="includes">The output included.</param>
		static void ExtractShaderIncludes(byte* shaderCache, int32 shaderCacheLength, List<String>& includes);

		// Resolves shader path name into absolute file path. Resolves './<ProjectName>/ShaderFile.hlsl' cases into a full path.
		static String ResolveShaderPath(StringView path);
		// Compacts the full shader file path into portable format with project name prefix such as './<ProjectName>/ShaderFile.hlsl'.
		static String CompactShaderPath(StringView path);

	private:

		static ShaderCompiler* CreateCompiler(ShaderProfile profile);
		static ShaderCompiler* RequestCompiler(ShaderProfile profile);
		static void FreeCompiler(ShaderCompiler* compiler);
	};
}

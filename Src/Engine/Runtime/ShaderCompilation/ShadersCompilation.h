#pragma once

#include <Runtime/API.h>
#include <Runtime/ShaderCompilation/ShaderCompileTypes.h>

namespace SE
{
	class Asset;

	/// <summary>
	/// Compiles Slang shader sources to ShaderArtifact and tracks source includes for automatic reloads.
	/// </summary>
	class SE_API_RUNTIME ShadersCompilation
	{
	public:
		/// <summary>
		/// Compiles a Slang-only shader resource into ShaderArtifact text data. This path is offline-only and does not touch GPU runtime objects.
		/// </summary>
		/// <param name="request">Compilation request.</param>
		/// <returns>Compilation status, diagnostics and ShaderArtifact data.</returns>
		static ShaderCompileResult CompileSlang(const ShaderCompileRequest& request);

		/// <summary>
		/// Runs a minimal offline Slang compilation self-test and returns the generated ShaderArtifact data.
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
	};
}

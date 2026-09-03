#pragma once

#include "Runtime/Core/Types/Strings/String.h"

#include <slang.h>

namespace SE
{
	/// <summary>
	/// Compile-side Slang file system adapter.
	/// </summary>
	/// <remarks>
	/// ShaderCompileRequest intentionally does not expose search paths. Import/include resolution is
	/// owned by this service so later engine virtual shader paths can be added without changing the
	/// public compile request identity.
	/// </remarks>
	class SlangShaderFileSystem final : public ISlangFileSystemExt
	{
	public:
        explicit SlangShaderFileSystem(const List<String>& rootSourcePath);

		ISlangFileSystem* GetSlangFileSystem();

		SLANG_NO_THROW SlangResult SLANG_MCALL queryInterface(SlangUUID const& uuid, void** outObject) override;
		SLANG_NO_THROW uint32_t SLANG_MCALL addRef() override;
		SLANG_NO_THROW uint32_t SLANG_MCALL release() override;
		SLANG_NO_THROW void* SLANG_MCALL castAs(const SlangUUID& guid) override;

		SLANG_NO_THROW SlangResult SLANG_MCALL loadFile(char const* path, ISlangBlob** outBlob) override;
		SLANG_NO_THROW SlangResult SLANG_MCALL getFileUniqueIdentity(const char* path, ISlangBlob** outUniqueIdentity) override;
		SLANG_NO_THROW SlangResult SLANG_MCALL calcCombinedPath(SlangPathType fromPathType, const char* fromPath, const char* path, ISlangBlob** pathOut) override;
		SLANG_NO_THROW SlangResult SLANG_MCALL getPathType(const char* path, SlangPathType* pathTypeOut) override;
		SLANG_NO_THROW SlangResult SLANG_MCALL getPath(PathKind kind, const char* path, ISlangBlob** outPath) override;
		SLANG_NO_THROW void SLANG_MCALL clearCache() override;
		SLANG_NO_THROW SlangResult SLANG_MCALL enumeratePathContents(const char* path, FileSystemContentsCallBack callback, void* userData) override;
		SLANG_NO_THROW OSPathKind SLANG_MCALL getOSPathKind() override;

	private:
		String ResolvePath(const char* path);

	private:
		int64 volatile _refCount;
		List<String> _searchRoots;
	};
}

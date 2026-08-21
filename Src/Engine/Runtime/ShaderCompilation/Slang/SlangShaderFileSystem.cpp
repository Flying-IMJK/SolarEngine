#include "SlangShaderFileSystem.h"

#include "Runtime/Core/Platform/File.h"
#include "Runtime/Core/Platform/FileSystem.h"
#include "Runtime/Core/Platform/Platform.h"
#include "Runtime/Core/Types/Collections/DataContainer.h"

#include <slang-com-helper.h>

namespace SE
{
	namespace
	{
		class SlangShaderBlob final : public ISlangBlob
		{
		public:
            explicit SlangShaderBlob(DataContainer<byte>&& data)
				: _refCount(1)
				, _data(MoveTemp(data))
			{
			}

			static SlangShaderBlob* FromString(const String& text)
			{
				const StringAnsi textAnsi(text);
				List<byte, HeapAllocation> data;
				data.Resize(textAnsi.Length() + 1);
				for (int32 i = 0; i < textAnsi.Length(); i++)
				{
					data[i] = (byte)textAnsi.Get()[i];
				}
				data[textAnsi.Length()] = 0;
				return new SlangShaderBlob(MoveTemp(data));
			}

			SLANG_NO_THROW SlangResult SLANG_MCALL queryInterface(SlangUUID const& uuid, void** outObject) override
			{
				if (outObject == nullptr)
				{
					return SLANG_FAIL;
				}
				*outObject = nullptr;
				if (uuid == ISlangBlob::getTypeGuid() || uuid == ISlangUnknown::getTypeGuid())
				{
					addRef();
					*outObject = static_cast<ISlangBlob*>(this);
					return SLANG_OK;
				}
				return SLANG_E_NO_INTERFACE;
			}

			SLANG_NO_THROW uint32_t SLANG_MCALL addRef() override
			{
				return (uint32)Platform::AtomicIncrement(&_refCount);
			}

			SLANG_NO_THROW uint32_t SLANG_MCALL release() override
			{
				const int64 refCount = Platform::AtomicDecrement(&_refCount);
				if (refCount == 0)
				{
					delete this;
				}
				return (uint32)refCount;
			}

			SLANG_NO_THROW void const* SLANG_MCALL getBufferPointer() override
            {
                return _data.Length() == 0 ? nullptr : _data.Get();
            }

			SLANG_NO_THROW size_t SLANG_MCALL getBufferSize() override { return (size_t)_data.Length(); }

		private:
			int64 volatile _refCount;
            DataContainer<byte> _data;
		};

		String NormalizePath(String path)
		{
			if (path.IsEmpty())
			{
				return path;
			}
			FileSystem::NormalizePath(path);
			FileSystem::PathRemoveRelativeParts(path);
			return path;
		}

		String GetDirectoryName(const String& path)
		{
			return String(FileSystem::GetParentDirectory(path));
		}

		bool IsAbsolutePath(const String& path)
		{
			return !path.IsEmpty() && !FileSystem::IsRelative(path);
		}

		String CombinePath(const String& directory, const String& path)
		{
			if (path.IsEmpty())
			{
				return NormalizePath(directory);
			}
			if (IsAbsolutePath(path))
			{
				return NormalizePath(path);
			}
			if (directory.IsEmpty() || directory == SE_TEXT("."))
			{
				return NormalizePath(path);
			}
			return NormalizePath(FileSystem::CombinePaths(directory, path));
		}

		bool PathExistsAsFileOrDirectory(const String& path)
		{
			return FileSystem::FileExists(path) || FileSystem::DirectoryExists(path);
		}

		bool ReadFileBytes(const String& path, DataContainer<byte>& bytes)
		{
			return File::ReadAllBytes(path, bytes);
		}

		SlangResult MakeStringBlob(const String& text, ISlangBlob** outBlob)
		{
			if (outBlob == nullptr)
			{
				return SLANG_FAIL;
			}
			*outBlob = SlangShaderBlob::FromString(text);
			return SLANG_OK;
		}
	}

	SlangShaderFileSystem::SlangShaderFileSystem(const String& rootSourcePath)
		: _refCount(1)
		, _rootSourcePath(NormalizePath(rootSourcePath))
		, _rootSourceDirectory(GetDirectoryName(_rootSourcePath))
	{
		if (!_rootSourceDirectory.IsEmpty())
		{
			_searchRoots.Add(_rootSourceDirectory);
		}
	}

	ISlangFileSystem* SlangShaderFileSystem::GetSlangFileSystem()
	{
		return static_cast<ISlangFileSystemExt*>(this);
	}

	SLANG_NO_THROW SlangResult SLANG_MCALL SlangShaderFileSystem::queryInterface(SlangUUID const& uuid, void** outObject)
	{
		if (outObject == nullptr)
		{
			return SLANG_FAIL;
		}
		*outObject = nullptr;
		if (void* interfaceObject = castAs(uuid))
		{
			addRef();
			*outObject = interfaceObject;
			return SLANG_OK;
		}
		return SLANG_E_NO_INTERFACE;
	}

	SLANG_NO_THROW uint32_t SLANG_MCALL SlangShaderFileSystem::addRef()
	{
		return (uint32)Platform::AtomicIncrement(&_refCount);
	}

	SLANG_NO_THROW uint32_t SLANG_MCALL SlangShaderFileSystem::release()
	{
		const int64 refCount = Platform::AtomicDecrement(&_refCount);
		if (refCount == 0)
		{
			delete this;
		}
		return (uint32)refCount;
	}

	SLANG_NO_THROW void* SLANG_MCALL SlangShaderFileSystem::castAs(const SlangUUID& guid)
	{
		if (guid == ISlangFileSystemExt::getTypeGuid())
		{
			return static_cast<ISlangFileSystemExt*>(this);
		}
		if (guid == ISlangFileSystem::getTypeGuid())
		{
			return static_cast<ISlangFileSystem*>(this);
		}
		if (guid == ISlangCastable::getTypeGuid())
		{
			return static_cast<ISlangCastable*>(this);
		}
		if (guid == ISlangUnknown::getTypeGuid())
		{
			return static_cast<ISlangUnknown*>(this);
		}
		return nullptr;
	}

	SLANG_NO_THROW SlangResult SLANG_MCALL SlangShaderFileSystem::loadFile(char const* path, ISlangBlob** outBlob)
	{
		if (outBlob == nullptr)
		{
			return SLANG_FAIL;
		}
		*outBlob = nullptr;
        
		DataContainer<byte> bytes;
		const String resolvedPath = ResolvePath(path);
		if (resolvedPath.IsEmpty() || !ReadFileBytes(resolvedPath, bytes))
		{
			return SLANG_E_NOT_FOUND;
		}

		*outBlob = new SlangShaderBlob(MoveTemp(bytes));
		return SLANG_OK;
	}

	SLANG_NO_THROW SlangResult SLANG_MCALL SlangShaderFileSystem::getFileUniqueIdentity(const char* path, ISlangBlob** outUniqueIdentity)
	{
		const String resolvedPath = ResolvePath(path);
		if (resolvedPath.IsEmpty())
		{
			return SLANG_E_NOT_FOUND;
		}

		return MakeStringBlob(NormalizePath(resolvedPath), outUniqueIdentity);
	}

	SLANG_NO_THROW SlangResult SLANG_MCALL SlangShaderFileSystem::calcCombinedPath(SlangPathType fromPathType, const char* fromPath, const char* path, ISlangBlob** pathOut)
	{
		if (pathOut == nullptr)
		{
			return SLANG_FAIL;
		}
		*pathOut = nullptr;

		String requestedPath = String(path);
        requestedPath = NormalizePath(requestedPath);
		if (requestedPath.IsEmpty())
		{
			return SLANG_E_NOT_FOUND;
		}

		if (IsAbsolutePath(requestedPath))
		{
			return MakeStringBlob(requestedPath, pathOut);
		}

		String basePath = String(fromPath);
        basePath = NormalizePath(basePath);
		if (basePath.IsEmpty())
		{
			basePath = _rootSourceDirectory;
		}
		else if (fromPathType == SLANG_PATH_TYPE_FILE)
		{
			basePath = GetDirectoryName(basePath);
		}

		const String combinedPath = CombinePath(basePath, requestedPath);
		if (PathExistsAsFileOrDirectory(combinedPath))
		{
			return MakeStringBlob(combinedPath, pathOut);
		}

		const StringAnsi requestedPathAnsi(requestedPath);
		const String resolvedPath = ResolvePath(requestedPathAnsi.Get());
		return MakeStringBlob(resolvedPath.IsEmpty() ? combinedPath : resolvedPath, pathOut);
	}

	SLANG_NO_THROW SlangResult SLANG_MCALL SlangShaderFileSystem::getPathType(const char* path, SlangPathType* pathTypeOut)
	{
		if (pathTypeOut == nullptr)
		{
			return SLANG_FAIL;
		}

		const String resolvedPath = ResolvePath(path);
		if (resolvedPath.IsEmpty())
		{
			return SLANG_E_NOT_FOUND;
		}

		if (FileSystem::FileExists(resolvedPath))
		{
			*pathTypeOut = SLANG_PATH_TYPE_FILE;
			return SLANG_OK;
		}
		if (FileSystem::DirectoryExists(resolvedPath))
		{
			*pathTypeOut = SLANG_PATH_TYPE_DIRECTORY;
			return SLANG_OK;
		}
		return SLANG_E_NOT_FOUND;
	}

	SLANG_NO_THROW SlangResult SLANG_MCALL SlangShaderFileSystem::getPath(PathKind, const char* path, ISlangBlob** outPath)
	{
		const String resolvedPath = ResolvePath(path);
		return MakeStringBlob(resolvedPath.IsEmpty() ? NormalizePath(String(path)) : resolvedPath, outPath);
	}

	SLANG_NO_THROW void SLANG_MCALL SlangShaderFileSystem::clearCache()
	{
	}

	SLANG_NO_THROW SlangResult SLANG_MCALL SlangShaderFileSystem::enumeratePathContents(const char*, FileSystemContentsCallBack, void*)
	{
		return SLANG_E_NOT_IMPLEMENTED;
	}

	SLANG_NO_THROW OSPathKind SLANG_MCALL SlangShaderFileSystem::getOSPathKind()
	{
		return OSPathKind::Direct;
	}

	String SlangShaderFileSystem::ResolvePath(const char* path)
	{
        String requestedPath = String(path);
        requestedPath = NormalizePath(requestedPath);
		if (requestedPath.IsEmpty())
		{
			return String::Empty;
		}

		if (IsAbsolutePath(requestedPath) && PathExistsAsFileOrDirectory(requestedPath))
		{
			return requestedPath;
		}

		if (requestedPath == _rootSourcePath)
		{
			return requestedPath;
		}

		for (int32 searchRootIndex = 0; searchRootIndex < _searchRoots.Count(); searchRootIndex++)
		{
			const String candidatePath = CombinePath(_searchRoots[searchRootIndex], requestedPath);
			if (PathExistsAsFileOrDirectory(candidatePath))
			{
				return candidatePath;
			}
		}

		if (PathExistsAsFileOrDirectory(requestedPath))
		{
			return requestedPath;
		}

		return String::Empty;
	}
}

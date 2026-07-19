
#include "Runtime/Core/Platform/FileSystem.h"
#include "Runtime/Core/Platform/File.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/Core/Types/Strings/StringView.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Core/Math/Math.h"
#include "FileSystemBase.h"

namespace SE
{
	constexpr Char DirectorySeparatorChar = '\\';
	constexpr Char AltDirectorySeparatorChar = '/';
	constexpr char VolumeSeparatorChar = ':';

	bool FileSystemBase::ShowOpenFileDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& filter, bool multiSelect, const StringView& title, List<String, HeapAllocation>& filenames)
	{
		// No supported
		return true;
	}

	bool FileSystemBase::ShowSaveFileDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& filter, bool multiSelect, const StringView& title, List<String, HeapAllocation>& filenames)
	{
		// No supported
		return true;
	}

	bool FileSystemBase::ShowBrowseFolderDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& title, String& path)
	{
		// No supported
		return true;
	}

	bool FileSystemBase::ShowFileExplorer(const StringView& path)
	{
		// No supported
		return true;
	}

	void FileSystemBase::SaveBitmapToFile(byte* data, uint32 width, uint32 height, uint32 bitsPerPixel, const uint32 padding, const String& path)
	{
		// Try to open file
		auto file = File::Open(path, FileMode::CreateAlways, FileAccess::Write, FileShare::None);
		if (file == nullptr)
			return;

		const unsigned long headers_size = 54;
		const unsigned long pixel_data_size = height * ((width * bitsPerPixel / 8) + padding);
		const unsigned int filesize = headers_size + pixel_data_size;
		uint8 bmpfileheader[14] = { 'B', 'M', 0, 0, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0 };
		uint8 bmpinfoheader[40] = { 40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 24, 0 };
		uint8 bmppad[3] = { 0, 0, 0 };

		// Init the file header
		bmpfileheader[2] = (uint8)(filesize);
		bmpfileheader[3] = (uint8)(filesize >> 8);
		bmpfileheader[4] = (uint8)(filesize >> 16);
		bmpfileheader[5] = (uint8)(filesize >> 24);

		// Init the bitmap info header
		bmpinfoheader[4] = (uint8)(width);
		bmpinfoheader[5] = (uint8)(width >> 8);
		bmpinfoheader[6] = (uint8)(width >> 16);
		bmpinfoheader[7] = (uint8)(width >> 24);
		bmpinfoheader[8] = (uint8)(height);
		bmpinfoheader[9] = (uint8)(height >> 8);
		bmpinfoheader[10] = (uint8)(height >> 16);
		bmpinfoheader[11] = (uint8)(height >> 24);

		// Write the file header
		file->Write(bmpfileheader, sizeof(bmpfileheader));

		// Write the bitmap info header
		file->Write(bmpinfoheader, sizeof(bmpinfoheader));

		// Write the RGB Data
		file->Write(data, pixel_data_size);

		// Close the file
		Delete(file);
	}

	bool FileSystemBase::AreFilePathsEqual(const StringView& path1, const StringView& path2)
	{
		if (path1.Compare(path2, StringSearchCase::CaseSensitive) == 0)
			return true;

		// Normalize file paths
		String filename1(path1);
		String filename2(path2);
		NormalizePath(filename1);
		NormalizePath(filename2);

		return filename1.Compare(filename2, StringSearchCase::IgnoreCase) == 0;
	}

	void FileSystemBase::NormalizePath(String& path)
	{
		path.Replace(DirectorySeparatorChar, AltDirectorySeparatorChar);
		if (path.Length() > 2 && StringUtils::IsAlpha(path[0]) && path[1] == ':')
		{
			path[2] = AltDirectorySeparatorChar;
		}
	}

	void FileSystemBase::NormalizePath(StringAnsi& path)
	{
		path.Replace(DirectorySeparatorChar, AltDirectorySeparatorChar);
		if (path.Length() > 2 && StringUtils::IsAlpha(path[0]) && path[1] == ':')
		{
			path[2] = AltDirectorySeparatorChar;
		}
	}

	String FileSystemBase::CombinePaths(StringView left, StringView right)
	{
		String result = left;
		int cnt = left.Length();
		if (cnt > 1 && left[cnt - 1] != AltDirectorySeparatorChar && left[cnt - 1] != DirectorySeparatorChar
			&& (right.Length() == 0 || (right[0] != AltDirectorySeparatorChar && right[0] != DirectorySeparatorChar)))
		{
			result += AltDirectorySeparatorChar;
		}
		result += right;
		return result;
	}

	String FileSystemBase::CombinePaths(StringView left, StringView middle, StringView right)
	{
		return CombinePaths(CombinePaths(left, middle), right);
	}

	bool FileSystemBase::IsRelative(const StringView& path)
	{
		const bool isRooted =
			(path.Length() >= 2 && StringUtils::IsAlpha(path[0]) && path[1] == ':') ||
				path.StartsWith(StringView(SE_TEXT("\\\\"), 2), StringSearchCase::CaseSensitive) ||
				path.StartsWith(&DirectorySeparatorChar) ||
				path.StartsWith(&AltDirectorySeparatorChar);
		return !isRooted;
	}

	String FileSystemBase::GetExtension(const StringView& path)
	{
		Char chr;
		int32 length = path.Length();
		int32 num = length;
		do
		{
			num--;
			if (num < 0)
			{
				break;
			}
			chr = path[num];
			if (chr != '.')
			{
				continue;
			}
			if (num == length - 1)
			{
				return String::Empty;
			}
			num++;
			return path.Substring(num, length - num);
		} while (chr != DirectorySeparatorChar && chr != AltDirectorySeparatorChar && chr != SE_TEXT(':'));

		return String::Empty;
	}

	String& FileSystemBase::NormalizeExtension(String& extension)
	{
		if (extension.Length() != 0 && extension[0] == '.')
		{
			extension.Remove(0, 1);
		}
		extension.ToLower();
		return extension;
	}

	bool FileSystemBase::MatchesExtension(const StringView& path, const StringView& extension)
	{
		Char chr;
		int32 length = path.Length();
		int32 num = length;
		do
		{
			num--;
			if (num < 0)
			{
				break;
			}
			chr = path[num];
			if (chr != '.')
			{
				continue;
			}
			if (num == length - 1)
			{
				return false;
			}
			num++;
			return StringUtils::Compare(path.Get() + num, length - num, extension.Get(), extension.Length()) == 0;
		} while (chr != DirectorySeparatorChar && chr != AltDirectorySeparatorChar && chr != SE_TEXT(':'));

		return false;
	}

	void FileSystemBase::ReplaceExtension(String& path, const StringView& newExtension)
	{
		Char chr;
		int32 length = path.Length();
		int32 num = length;
		do
		{
			num--;
			if (num < 0)
			{
				break;
			}
			chr = path[num];
			if (chr != '.')
			{
				continue;
			}
			if (num == length - 1)
			{
				return;
			}
			num++;
			path.Replace(num, length - num, newExtension.Get(), newExtension.Length());
			return;
		} while (chr != DirectorySeparatorChar && chr != AltDirectorySeparatorChar && chr != SE_TEXT(':'));
	}

	int32 FileSystemBase::FindExtensionStartIdx(const StringView& path)
	{
		Char chr;
		int32 length = path.Length();
		int32 num = length;
		do
		{
			num--;
			if (num < 0)
			{
				break;
			}
			chr = path[num];
			if (chr != '.')
			{
				continue;
			}
			if (num == length - 1)
			{
				return INVALID_INDEX;
			}
			return num;
		} while (chr != DirectorySeparatorChar && chr != AltDirectorySeparatorChar && chr != SE_TEXT(':'));

		return INVALID_INDEX;
	}

	int32 FileSystemBase::FindExtensionStartIdx(const StringAnsiView& path)
	{
		char chr;
		int32 length = path.Length();
		int32 num = length;
		do
		{
			num--;
			if (num < 0)
			{
				break;
			}
			chr = path[num];
			if (chr != '.')
			{
				continue;
			}
			if (num == length - 1)
			{
				return INVALID_INDEX;
			}
			return num;
		} while (chr != DirectorySeparatorChar && chr != AltDirectorySeparatorChar && chr != ':');

		return INVALID_INDEX;
	}

	bool FileSystemBase::IsDirectory(const StringView& path)
	{
		return false;
	}

	bool FileSystemBase::IsUnderDirectory(const StringView& path, const StringView& parentDirectory)
	{
		if (path.Length() < parentDirectory.Length())
		{
			return false;
		}

		int32 length = parentDirectory.Length();
		int32 result = StringUtils::Compare(*path, *parentDirectory, length);

		return result == 0;
	}

	StringView FileSystemBase::GetParentDirectory(const StringView& path)
	{
		ENGINE_ASSERT( !path.IsEmpty());

		int32 lastDelimiterIdx = path.FindLast(DirectorySeparatorChar);
		if (lastDelimiterIdx == INVALID_INDEX)
		{
			lastDelimiterIdx = path.FindLast(AltDirectorySeparatorChar);
		}

		// Handle directory paths
		if ( lastDelimiterIdx == path.Length() - 1 )
		{
			lastDelimiterIdx = path.FindLast(DirectorySeparatorChar, lastDelimiterIdx - 1);
			if (lastDelimiterIdx == INVALID_INDEX)
			{
				lastDelimiterIdx = path.FindLast(AltDirectorySeparatorChar, lastDelimiterIdx - 1);
			}
		}

		//-------------------------------------------------------------------------
		StringView parentPath = StringView::Empty;
		// If we found a parent, create the substring for it
		if ( lastDelimiterIdx != INVALID_INDEX)
		{
			parentPath = path.Substring( 0, lastDelimiterIdx);
		}
		return parentPath;
	}

	void PathRemoveRelativeParts(String& path)
	{
		FileSystem::NormalizePath(path);

		List<String> components;
		path.Split(AltDirectorySeparatorChar, components);

		List<String, InlinedAllocation<16>> stack;
		for (String& bit : components)
		{
			if (bit == SE_TEXT(".."))
			{
				if (stack.HasItems())
				{
					String popped = stack.Pop();
					const int32 windowsDriveStart = popped.Find(DirectorySeparatorChar);
					if (popped == SE_TEXT(".."))
					{
						stack.Push(popped);
						stack.Push(bit);
					}
					else if (windowsDriveStart != -1)
					{
						stack.Add(MoveTemp(popped.Left(windowsDriveStart + 1)));
					}
				}
				else
				{
					stack.Push(bit);
				}
			}
			else if (bit == SE_TEXT("."))
			{
				// Skip /./
			}
			else
			{
				stack.Add(MoveTemp(bit));
			}
		}

		const bool isRooted = path.StartsWith(AltDirectorySeparatorChar) || (path.Length() >= 2 && path[0] == '.' && path[1] == AltDirectorySeparatorChar);
		path.Clear();
		for (const String& e : stack)
			path /= e;
		if (isRooted && path.HasChars() && path[0] != AltDirectorySeparatorChar)
			path.Insert(0, SE_TEXT("/"));
	}

	void RemoveLongPathPrefix(const String &path, String &result)
	{
		if (!path.StartsWith(SE_TEXT("\\\\?\\"), StringSearchCase::CaseSensitive))
		{
			result = path;
		}
		if (!path.StartsWith(SE_TEXT("\\\\?\\UNC\\"), StringSearchCase::IgnoreCase))
		{
			result = path.Substring(4);
		}
		result = path;
		result.Remove(2, 6);
	}

	StringView FileSystemBase::GetDirectoryName(const StringView &path)
	{
		int offset = 0;
		if (path.EndsWith(DirectorySeparatorChar) || path.EndsWith(AltDirectorySeparatorChar))
		{
			offset = 1;
		}

		const int32 lastFrontSlash = path.FindLast(DirectorySeparatorChar, offset);
		const int32 lastBackSlash = path.FindLast(AltDirectorySeparatorChar, offset);
		const int32 splitIndex = Math::Max(lastBackSlash, lastFrontSlash);
		return splitIndex != INVALID_INDEX ? path.Substring(splitIndex + 1) : StringView::Empty;
	}

	StringView FileSystemBase::GetFileName(const StringView &path)
	{
		Char chr;
		const int32 length = path.Length();
		int32 num = length;
		do
		{
			num--;
			if (num < 0)
				return path;
			chr = path[num];
		} while (chr != DirectorySeparatorChar && chr != AltDirectorySeparatorChar && chr != VolumeSeparatorChar);
		return path.Substring(num + 1, length - num - 1);
	}

	StringView FileSystemBase::GetFileNameWithoutExtension(const StringView &path)
	{
		StringView filename = GetFileName(path);
		const int32 num = filename.FindLast('.');
		if (num != -1)
			return filename.Substring(0, num);
		return filename;
	}

	StringView FileSystemBase::GetPathWithoutExtension(const StringView &path)
	{
		const int32 num = path.FindLast('.');
		if (num != -1)
			return path.Substring(0, num);
		return path;
	}

	bool FileSystemBase::IsInvalidPathChar(Char c)
	{
		char illegalChars[] =
		{
			'?',
			'\\',
			'/',
			'\"',
			'<',
			'>',
			'|',
			':',
			'*',
			'\u0001',
			'\u0002',
			'\u0003',
			'\u0004',
			'\u0005',
			'\u0006',
			'\a',
			'\b',
			'\t',
			'\n',
			'\v',
			'\f',
			'\r',
			'\u000E',
			'\u000F',
			'\u0010',
			'\u0011',
			'\u0012',
			'\u0013',
			'\u0014',
			'\u0015',
			'\u0016',
			'\u0017',
			'\u0018',
			'\u0019',
			'\u001A',
			'\u001B',
			'\u001C',
			'\u001D',
			'\u001E',
			'\u001F'
		};
		for (auto i : illegalChars)
		{
			if (c == i)
				return true;
		}
		return false;
	}

	void FileSystemBase::PathRemoveRelativeParts(String &path)
	{
        FileSystem::NormalizePath(path);

		List <String> components;
		path.Split(AltDirectorySeparatorChar, components);

		List<String, InlinedAllocation<16>> stack;
		for (String &bit: components)
		{
			if (bit == SE_TEXT(".."))
			{
				if (stack.HasItems())
				{
					String popped = stack.Pop();
					const int32 windowsDriveStart = popped.Find(DirectorySeparatorChar);
					if (popped == SE_TEXT(".."))
					{
						stack.Push(popped);
						stack.Push(bit);
					}
					else if (windowsDriveStart != -1)
					{
						stack.Add(MoveTemp(popped.Left(windowsDriveStart + 1)));
					}
				}
				else
				{
					stack.Push(bit);
				}
			}
			else if (bit == SE_TEXT("."))
			{
				// Skip /./
			}
			else
			{
				stack.Add(MoveTemp(bit));
			}
		}

		const bool isRooted = path.StartsWith(AltDirectorySeparatorChar) || (path.Length() >= 2 && path[0] == '.' && path[1] == AltDirectorySeparatorChar);
		path.Clear();
		for (const String &e: stack)
		{
			path /= e;
		}
		if (isRooted && path.HasChars() && path[0] != AltDirectorySeparatorChar)
		{
			path.Insert(0, SE_TEXT("/"));
		}
	}

	int32 FileSystemBase::GetDirectoryDepth(String &path)
	{
		int32 dirDepth = -1;

		NormalizePath(path);

		int32 delimiterIdx = -1;
		do
		{
			int32 index = path.Find(&AltDirectorySeparatorChar, delimiterIdx + 1);
			if (index != INVALID_INDEX)
			{
				dirDepth++;
			}
			else if (delimiterIdx < path.Length() - 1)
			{
				dirDepth++;
			}
			delimiterIdx = index;
		} while (delimiterIdx != INVALID_INDEX);

		return dirDepth;
	}

	void FileSystemBase::SplitPath(const String& path, List<String, InlinedAllocation<10>>& splitPath)
	{
		int32 start = 0;
		int32 separatorPos;

		do
		{
			separatorPos = path.FindFirstOf(SE_TEXT("\\/"), start);
			if (separatorPos == -1)
				splitPath.Add(path.Substring(start));
			else
				splitPath.Add(path.Substring(start, separatorPos - start));
			start = separatorPos + 1;
		} while (separatorPos != -1);
	}

	String FileSystemBase::ConvertAbsolutePathToRelative(const String& basePath, const String& path)
	{
		List<String, InlinedAllocation<10>> toDirs;
		List<String, InlinedAllocation<10>> fromDirs;

		SplitPath(path, toDirs);
		SplitPath(basePath, fromDirs);

		String output;

		List<String, InlinedAllocation<10>>::Iterator toIt = toDirs.begin(), fromIt = fromDirs.begin();
		const List<String, InlinedAllocation<10>>::Iterator toEnd = toDirs.end(), fromEnd = fromDirs.end();

		while (toIt != toEnd && fromIt != fromEnd && *toIt == *fromIt)
		{
			++toIt;
			++fromIt;
		}

		while (fromIt != fromEnd)
		{
			output += SE_TEXT("../");
			++fromIt;
		}

		while (toIt != toEnd)
		{
			output += *toIt;
			++toIt;

			if (toIt != toEnd)
				output += AltDirectorySeparatorChar;
		}

		return output;
	}

	bool FileSystemBase::CopyFile(const String& dst, const String& src)
	{
		// Open and create files
		const auto srcFile = File::Open(src, FileMode::OpenExisting, FileAccess::Read);
		if (!srcFile)
		{
			return true;
		}
		const auto dstFile = File::Open(dst, FileMode::CreateAlways, FileAccess::Write);
		if (!dstFile)
		{
			Delete(srcFile);
			return true;
		}

		// Skip for empty file
		uint32 size = srcFile->GetSize();
		if (size == 0)
		{
			Delete(srcFile);
			Delete(dstFile);
			return false;
		}

		// Copy data
		const uint32 bufferSize = Math::Min<uint32>(1024 * 1024, size);
		byte* buffer = (byte*)PlatformAllocator::Allocate(bufferSize);
		while (size)
		{
			const uint32 readSize = Math::Min<uint32>(bufferSize, size);
			srcFile->Read(buffer, readSize);
			dstFile->Write(buffer, readSize);
			size -= readSize;
		}
		PlatformAllocator::Free(buffer);

		// Cleanup
		Delete(srcFile);
		Delete(dstFile);

		return false;
	}

	bool FileSystemBase::CopyDirectory(const String& dst, const String& src, bool withSubDirectories)
	{
		return !FileSystem::DirectoryExists(*src) || FileSystemBase::DirectoryCopyHelper(dst, src, withSubDirectories);
	}

	uint64 FileSystemBase::GetDirectorySize(const StringView& path)
	{
		uint64 result = 0;
		List<String> files;
		FileSystem::DirectoryGetFiles(files, path, SE_TEXT("*"), DirectorySearchOption::All);
		for (const String& file : files)
			result += FileSystem::GetFileSize(file);
		return result;
	}

	String FileSystemBase::ConvertRelativePathToAbsolute(const String& basePath, const String& path)
	{
		String fullyPathed;
		if (IsRelative(path))
		{
			fullyPathed = basePath;
		}

		fullyPathed /= path;

		NormalizePath(fullyPathed);
		return fullyPathed;
	}

	bool FileSystemBase::DirectoryCopyHelper(const String& dst, const String& src, bool withSubDirectories)
	{
		// Create dst directory
		if (!FileSystem::DirectoryExists(dst))
		{
			if (FileSystem::CreateDirectory(dst))
				return true;
		}

		// Copy all files
		List<String> cache(32);
		if (FileSystem::DirectoryGetFiles(cache, src, SE_TEXT("*"), DirectorySearchOption::TopOnly))
			return true;
		for (int32 i = 0; i < cache.Count(); i++)
		{
			String dstFile = dst / GetFileName(cache[i]);
			if (FileSystem::CopyFile(*dstFile, *cache[i]))
				return true;
		}

		// Copy all subdirectories (if need to)
		if (withSubDirectories)
		{
			cache.Clear();
			if (FileSystem::GetChildDirectories(cache, src))
				return true;
			for (int32 i = 0; i < cache.Count(); i++)
			{
				String dstDir = dst / GetFileName(cache[i]);
				if (DirectoryCopyHelper(dstDir, cache[i], true))
					return true;
			}
		}

		return false;
	}
}
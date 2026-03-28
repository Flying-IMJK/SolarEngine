#pragma once

#if PLATFORM_WIN32

#include "Core/Platform/Base/FileSystemBase.h"

namespace SE
{
	/// <summary>
	/// Win32 platform implementation of filesystem service
	/// </summary>
	class SE_API_CORE Win32FileSystem : public FileSystemBase
	{
		// TODO: fix docs

	public:

		// Creates a new directory
		// @param path Directory path
		// @returns True if cannot create directory, otherwise false
		static bool CreateDirectory(const StringView& path);

		// Deletes an existing directory
		// @param path Directory path
		// @param deleteSubdirectories True if delete all subdirectories and files, otherwise false
		// @returns True if cannot delete directory, otherwise false
		static bool DeleteDirectory(const String& path, bool deleteContents = true);

		// Check if directory exists
		// @param path Directory path to check
		// @returns True if directory exists, otherwise false
		static bool DirectoryExists(const StringView& path);

		// 在指定目录中查找与指定搜索模式匹配的文件名（包括其路径），并使参数来确定是否搜索子目录
		// @param results When this method completes, this list contains list of all filenames that match the specified search pattern
		// @param path Path of the directory to search in it
		// @param searchPattern Custom search pattern to use during that operation
		// @param option Additional search options
		// @returns False if an error occurred, otherwise True
		static bool DirectoryGetFiles(List<String, HeapAllocation>& results,
			const String& path,
			const Char* searchPattern,
			DirectorySearchOption option = DirectorySearchOption::All);

		// 查找指定目录内的目录 （包括其路径）和 文件的名称
		// @param results 此列表包含与指定搜索模式匹配的所有文件名的列表
		// @param directory 搜索的目录路径
		// @returns False if an error occurred, otherwise True
		static bool GetChildDirectories(List<String, HeapAllocation>& results, const String& directory);

		// 查找指定目录内的目录 （包括其路径） 的名称，并使参数来确定是否搜索子目录
		// @param results 此列表包含与指定搜索模式匹配的所有文件名的列表
		// @param directory 搜索的目录路径
		// @returns False if an error occurred, otherwise True
		static bool GetDirectoryAndFiles(List<String, HeapAllocation>& results, const String& path, DirectorySearchOption option = DirectorySearchOption::All);

	public:

		// Check if file exists
		// @param path File path to check
		// @returns True if file exists, otherwise false
		static bool FileExists(const StringView& path);

		// Deletes an existing file
		// @param path File path
		// @returns False if cannot delete file, otherwise false
		static bool DeleteFile(const StringView& path);

		// Tries to get size of the file
		// @param path File path to check
		// @returns Amount of bytes in file
		static uint64 GetFileSize(const StringView& path);

		// Check if file is read-only
		// @param path File path to check
		// @returns True if file is read-only
		static bool IsReadOnly(const StringView& path);

		// Sets file read-only flag
		// @param path File path
		// @returns False if cannot update file
		static bool SetReadOnly(const StringView& path, bool isReadOnly);

		// Move file
		// @param dst Destination path
		// @param src Source path
		// @returns False if cannot move file
		static bool MoveFile(const StringView& dst, const StringView& src, bool overwrite = false);

		// Clone file
		// @param dst Destination path
		// @param src Source path
		// @returns False if cannot copy file
		static bool CopyFile(const StringView& dst, const StringView& src);

		/***
		 * 路径是否为文件夹
		 * @param path
		 * @return
		 */
		static bool IsDirectory(const StringView& path);

	public:

		/// <summary>
		/// Converts the UNIX style line endings into DOS style (from \n into \r\n).
		/// </summary>
		/// <param name="text">The input text data.</param>
		/// <param name="output">The output result.</param>
		static void ConvertLineEndingsToDos(const StringView& text, List<Char, HeapAllocation>& output);

	public:

		/// <summary>
		/// Gets last time when file has been modified (in UTC).
		/// </summary>
		/// <param name="path">The file path to check.</param>
		/// <returns>The last write time or DateTime::MinValue() if cannot get data.</returns>
		static DateTime GetFileLastEditTime(const StringView& path);
	public:
		/**
		 * 获取当前程序路径
		 * @return
		 */
		static String GetCurrentModulePath();

		/**
		 * 获取当前程序文件夹路径
		 * @return
		 */
		static String GetCurrentProcessDirectory();

	private:

		static bool getFilesFromDirectoryTop(List<String, HeapAllocation>& results, const String& directory, const Char* searchPattern);
		static bool getFilesFromDirectoryAll(List<String, HeapAllocation>& results, const String& directory, const Char* searchPattern);
	};

}
#endif

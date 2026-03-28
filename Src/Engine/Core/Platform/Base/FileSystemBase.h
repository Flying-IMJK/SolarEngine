#pragma once

#include "Core/API.h"
#include "Core/Types/Variable.h"
#include "Core/Platform/Types.h"

namespace SE
{
	/// <summary>
	/// 指定是搜索当前目录，还是搜索当前目录和所有子目录。
	/// </summary>
	enum class DirectorySearchOption
	{
		/// <summary>
		/// 在搜索操作中包括当前目录及其所有子目录。此选项包括搜索中的重新分析点，例如挂载的驱动器和符号链接。
		/// </summary>
		All,

		/// <summary>
		/// 在搜索操作中仅包含当前目录。
		/// </summary>
		TopOnly,
	};

	/// <summary>
	/// Special system folder types.
	/// </summary>
	enum class SpecialFolder
	{
		Desktop,
		Documents,
		Pictures,
		AppData,
		LocalAppData,
		ProgramData,
		Temporary,
	};

	/// <summary>
	/// Platform implementation of filesystem service.
	/// </summary>
	class SE_API_CORE FileSystemBase
	{

		/// <summary>
		/// Displays a standard dialog box that prompts the user to open a file(s).
		/// </summary>
		/// <param name="initialDirectory">The initial directory.</param>
		/// <param name="filter">The file filter string as null-terminated pairs of name and list of extensions. Multiple file extensions must be separated with semicolon.</param>
		/// <param name="multiSelect">True if allow multiple files to be selected, otherwise use single-file mode.</param>
		/// <param name="title">The dialog title.</param>
		/// <param name="filenames">The output names of the files picked by the user.</param>
		/// <returns>True if failed, otherwise false.</returns>
		/// <remarks>
		/// Example file filters:
		///    "All Files\0*.*"
		///    "All Files\0*.*\0Image Files\0*.png;*.jpg"
		/// </remarks>
		static bool ShowOpenFileDialog(const StringView& initialDirectory,
			const StringView& filter,
			bool multiSelect,
			const StringView& title,
			List <String, HeapAllocation>& filenames);

		/// <summary>
		/// Displays a standard dialog box that prompts the user to open a file(s).
		/// </summary>
		/// <param name="parentWindow">The parent window or null.</param>
		/// <param name="initialDirectory">The initial directory.</param>
		/// <param name="filter">The file filter string as null-terminated pairs of name and list of extensions. Multiple file extensions must be separated with semicolon.</param>
		/// <param name="multiSelect">True if allow multiple files to be selected, otherwise use single-file mode.</param>
		/// <param name="title">The dialog title.</param>
		/// <param name="filenames">The output names of the files picked by the user.</param>
		/// <returns>True if failed, otherwise false.</returns>
		/// <remarks>
		/// Example file filters:
		///    "All Files\0*.*"
		///    "All Files\0*.*\0Image Files\0*.png;*.jpg"
		/// </remarks>
		static bool ShowOpenFileDialog(Window* parentWindow,
			const StringView& initialDirectory,
			const StringView& filter,
			bool multiSelect,
			const StringView& title,
			List <String, HeapAllocation>& filenames);

		/// <summary>
		/// Displays a standard dialog box that prompts the user to save a file(s).
		/// </summary>
		/// <param name="parentWindow">The parent window.</param>
		/// <param name="initialDirectory">The initial directory.</param>
		/// <param name="filter">The file filter string as null-terminated pairs of name and list of extensions. Multiple file extensions must be separated with semicolon.</param>
		/// <param name="multiSelect">True if allow multiple files to be selected, otherwise use single-file mode.</param>
		/// <param name="title">The title.</param>
		/// <param name="filenames">The output names of the files picked by the user.</param>
		/// <returns>True if failed, otherwise false.</returns>
		/// <remarks>
		/// Example file filters:
		///    "All Files\0*.*"
		///    "All Files\0*.*\0Image Files\0*.png;*.jpg"
		/// </remarks>
		static bool ShowSaveFileDialog(Window* parentWindow,
			const StringView& initialDirectory,
			const StringView& filter,
			bool multiSelect,
			const StringView& title,
			List <String, HeapAllocation>& filenames);

		/// <summary>
		/// Displays a standard dialog box that prompts the user to select a folder.
		/// </summary>
		/// <param name="parentWindow">The parent window.</param>
		/// <param name="initialDirectory">The initial directory.</param>
		/// <param name="title">The dialog title.</param>
		/// <param name="path">The output path.</param>
		/// <returns>True if failed, otherwise false.</returns>
		static bool ShowBrowseFolderDialog(Window* parentWindow, const StringView& initialDirectory, const StringView& title, String& path);

		/// <summary>
		/// Opens a standard file explorer application and navigates to the given directory.
		/// </summary>
		/// <param name="path">The path.</param>
		/// <returns>True if failed, otherwise false.</returns>
		static bool ShowFileExplorer(const StringView& path);

	public:

		static void SaveBitmapToFile(byte* data, uint32 width, uint32 height, uint32 bitsPerPixel, const uint32 padding, const String& path);

	public:

		static bool AreFilePathsEqual(const StringView& path1, const StringView& path2);


		/// <summary>
		/// 将路径规范化为当前平台文件系统的有效路径
		/// Normalize input path for valid path name for current platform file system
		/// </summary>
		/// <param name="path">Path to normalize</param>
		static void NormalizePath(String& path);

		static void NormalizePath(StringAnsi& path);

		static String CombinePaths(StringView left, StringView right);

		static String CombinePaths(StringView left, StringView middle, StringView right);

		/// <summary>
		/// 检查是否为相对路径
		/// </summary>
		/// <param name="path">Input path to check</param>
		/// <returns>True if input path is relative one, otherwise false</returns>
		static bool IsRelative(const StringView& path);

		/// <summary>
		/// 获取文件扩展名（不带点）
		/// </summary>
		/// <param name="path">Input path to process</param>
		/// <returns>File extension</returns>
		static String GetExtension(const StringView& path);

		static String& NormalizeExtension(String& extension);

		/***
		 * 匹配扩展名
		 * @param path
		 * @return
		 */
		static bool MatchesExtension(const StringView& path, const StringView& extension);

		/***
		 * 替换文件扩展名
		 * @param path
		 * @return
		 */
		static void ReplaceExtension(String& path, const StringView& newExtension);

		/***
		 * 路径是否为文件夹
		 * @param path
		 * @return
		 */
		static bool IsDirectory(const StringView& path);

		/***
		 * 获取路径扩展名字符起始位置
		 * @param path
		 * @return
		 */
		static int32 FindExtensionStartIdx(const StringView& path);


		/***
		 * 是否为子路径
		 * @param path 路径
		 * @param parentDirectory 父路径
		 * @return
		 */
		static bool IsUnderDirectory(const StringView& path, const StringView& parentDirectory);

		/**
		 * 获取父文件加路径
		 * @param path
		 * @param parentDirectory
		 */
		static StringView GetParentDirectory(const StringView& path);

		static void PathRemoveRelativeParts(String& path);

		static void SplitPath(const String& path, List<String, InlinedAllocation<10>>& splitPath);

		static int32 GetDirectoryDepth(String &path);

		// Returns the directory name of the specified path string
		// @param path The path string from which to obtain the directory name
		// @returns Directory name
		static StringView GetDirectoryName(const StringView &path);

		/**
		 * 获取文件路径的文件名
		 * @param path
		 * @return
		 */
		static StringView GetFileName(const StringView &path);

		/**
		 * 获取不带扩展名的文件名
		 * @return
		 */
		static StringView GetFileNameWithoutExtension(const StringView &path);

		/**
		 * 获取不带文件后缀的路径
		 * @param path
		 * @return
		 */
		static StringView GetPathWithoutExtension(const StringView &path);

		/*
		 * 是否为无效路径字符
		 */
		static bool IsInvalidPathChar(Char c);

	public:


		static int32 FindExtensionStartIdx(const StringAnsiView& path);

	public:

		static bool CopyFile(const String& dst, const String& src);
		static bool CopyDirectory(const String& dst, const String& src, bool withSubDirectories);
		static uint64 GetDirectorySize(const StringView& path);

	public:
		/**
		 * 获取当前程序路径
		 * @return
		 */
		static String GetCurrentModulePath() = delete;

		/**
		 * 获取当前程序文件夹路径
		 * @return
		 */
		static String GetCurrentProcessDirectory() = delete;

	public:

		/// <summary>
		/// 将相对于 basePath 的路径转换为绝对路径
		/// </summary>
		/// <param name="basePath">Base path</param>
		/// <param name="path">Path relative to basePath</param>
		/// <returns>Absolute path</returns>
		static String ConvertRelativePathToAbsolute(const String& basePath, const String& path);

		/// <summary>
		/// 将绝对路径转换为 basePath 的相对路径
		/// </summary>
		/// <param name="basePath">Base path</param>
		/// <param name="path">Absolute path</param>
		/// <returns>Relative path</returns>
		static String ConvertAbsolutePathToRelative(const String& basePath, const String& path);

	private:

		static bool DirectoryCopyHelper(const String& dst, const String& src, bool withSubDirectories);
	};

}
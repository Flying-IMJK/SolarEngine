
using System.IO;

namespace SE
{
    public partial class FileSystem
    {

        /// <summary>
        /// Copies the directory. Supports subdirectories copy with files override option.
        /// </summary>
        /// <param name="srcDirectoryPath">The source directory path.</param>
        /// <param name="dstDirectoryPath">The destination directory path.</param>
        /// <param name="copySubDirs">If set to <c>true</c> copy subdirectories.</param>
        /// <param name="overrideFiles">if set to <c>true</c> override existing files.</param>
        public static void DirectoryCopy(string srcDirectoryPath, string dstDirectoryPath, bool copySubDirs = true,
            bool overrideFiles = false)
        {
            DirectoryInfo dir = new DirectoryInfo(srcDirectoryPath);
            if (!dir.Exists)
            {
                throw new DirectoryNotFoundException("Missing source directory to copy. " + srcDirectoryPath);
            }
            DirectoryCopy(dir, dstDirectoryPath, copySubDirs, overrideFiles);
        }

        private static void DirectoryCopy(DirectoryInfo dir, string dstDirectoryPath, bool copySubDirs = true,
            bool overrideFiles = false)
        {
            if (!Directory.Exists(dstDirectoryPath))
            {
                Directory.CreateDirectory(dstDirectoryPath);
            }

            FileInfo[] files = dir.GetFiles();
            for (int i = 0; i < files.Length; i++)
            {
                string tmp = Path.Combine(dstDirectoryPath, files[i].Name);
                files[i].CopyTo(tmp, overrideFiles);
            }

            if (copySubDirs)
            {
                DirectoryInfo[] dirs = dir.GetDirectories();
                for (int i = 0; i < dirs.Length; i++)
                {
                    string tmp = Path.Combine(dstDirectoryPath, dirs[i].Name);
                    DirectoryCopy(dirs[i], tmp, true, overrideFiles);
                }
            }
        }

        /// <summary>
        /// Removes extension from the file path.
        /// </summary>
        /// <param name="path">The path.</param>
        /// <returns>Path without extension.</returns>
        public static string GetPathWithoutExtension(string path)
        {
            int num = path.LastIndexOf('.');
            if (num != -1)
            {
                return path.Substring(0, num);
            }
            return path;
        }

        /// <summary>
        /// Normalizes path separators to '/' to match the native FileSystem::NormalizePath.
        /// </summary>
        /// <param name="path">The path.</param>
        /// <returns>The normalized path.</returns>
        public static string NormalizePath(string path)
        {
            if (string.IsNullOrEmpty(path))
            {
                return path;
            }
            char[] chars = path.ToCharArray();

            // Convert all '\' to '/'
            for (int i = 0; i < chars.Length; i++)
            {
                if (chars[i] == '\\')
                {
                    chars[i] = '/';
                }
            }

            // Normalize the separator after a drive letter, as the native implementation does.
            if (chars.Length > 2 && char.IsLetter(chars[0]) && chars[1] == ':')
            {
                chars[2] = '/';
            }

            return new string(chars);
        }

        /// <summary>
        /// Normalizes the file extension to common format: no leading dot and all lowercase.
        /// For example: '.TxT' will return 'txt'.
        /// </summary>
        /// <param name="extension">The extension.</param>
        /// <returns>The normalized extension.</returns>
        public static string NormalizeExtension(string extension)
        {
            if (extension.Length != 0 && extension[0] == '.')
            {
                extension = extension.Remove(0, 1);
            }
            return extension.ToLower();
        }

        /// <summary>
        /// Combines the paths.
        /// </summary>
        /// <param name="left">The left.</param>
        /// <param name="right">The right.</param>
        /// <returns>The combined path</returns>
        public static string CombinePaths(string left, string right)
        {
            int cnt = left.Length;
            if (cnt > 1 && left[cnt - 1] != '/' && left[cnt - 1] != '\\'
                && (right.Length == 0 || (right[0] != '/' && right[0] != '\\')))
            {
                left += '/';
            }
            return left + right;
        }

        /// <summary>
        /// Combines the paths.
        /// </summary>
        /// <param name="left">The left.</param>
        /// <param name="middle">The middle.</param>
        /// <param name="right">The right.</param>
        /// <returns>The combined path</returns>
        public static string CombinePaths(string left, string middle, string right)
        {
            return CombinePaths(CombinePaths(left, middle), right);
        }

        /// <summary>
        /// Determines whether the specified path is relative or is absolute.
        /// </summary>
        /// <param name="path">The input path.</param>
        /// <returns><c>true</c> if the specified path is relative; otherwise, <c>false</c> if is relative.</returns>
        public static bool IsRelative(string path)
        {
            bool isRooted = (path.Length >= 2 && char.IsLetterOrDigit(path[0]) && path[1] == ':') ||
                            path.StartsWith("\\\\") ||
                            path.StartsWith("\\") ||
                            path.StartsWith("/");
            return !isRooted;
        }
    }
}

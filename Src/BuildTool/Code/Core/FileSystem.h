#pragma once

#include <filesystem>

#include "String.h"
#include "Utils.h"

namespace SE::BuildTool
{
    enum class DirectorySearchOption
    {
        TopOnly,
        All,
    };

    namespace FileSystem
    {
        using Path = std::string;

        inline void NormalizePath(std::string& path)
        {
            Utils::String::ReplaceAll(path, "\\", "/");
            while (Utils::String::Contains(path, "//"))
                Utils::String::ReplaceAll(path, "//", "/");
        }

        inline std::string PathCombine(const std::string& a, const std::string& b)
        {
            std::string result = a;
            if (!result.empty() && !Utils::String::EndsWith(result, '/') && !Utils::String::EndsWith(result, '\\'))
                result += "/";
            result += b;
            NormalizePath(result);
            return result;
        }

        inline bool FileExists(const std::string& path)
        {
            return std::filesystem::is_regular_file(path);
        }

        inline bool DirectoryExists(const std::string& path)
        {
            return std::filesystem::is_directory(path);
        }

        inline bool CreateDirectory(const std::string& path)
        {
            std::error_code ec;
            return std::filesystem::create_directories(path, ec) || std::filesystem::exists(path);
        }

        inline bool DeleteFile(const std::string& path)
        {
            std::error_code ec;
            return std::filesystem::remove(path, ec) || !std::filesystem::exists(path);
        }

        inline bool DeleteDirectory(const std::string& path)
        {
            std::error_code ec;
            std::filesystem::remove_all(path, ec);
            return !ec;
        }

        inline std::string GetParentDirectory(const std::string& path)
        {
            std::string result(std::filesystem::path(path).parent_path().generic_string());
            NormalizePath(result);
            if (!result.empty() && !Utils::String::EndsWith(result, '/'))
                result += "/";
            return result;
        }

        inline std::string GetFileNameWithoutExtension(const std::string& path)
        {
            return std::filesystem::path(path).stem().string();
        }

        inline std::string GetCurrentProcessDirectory()
        {
            std::string result(std::filesystem::current_path().generic_string());
            NormalizePath(result);
            if (!result.empty() && !Utils::String::EndsWith(result, '/'))
                result += "/";
            return result;
        }

        inline std::string GetCurrentProcessPath()
        {
            return GetCurrentProcessDirectory();
        }

        inline bool MatchesExtension(const std::string& path, const std::string& extension)
        {
            std::string ext(std::filesystem::path(path).extension().string());
            if (Utils::String::StartsWith(ext, "."))
                ext.erase(0, 1);
            return Utils::String::ToLowerCopy(ext) == Utils::String::ToLowerCopy(extension);
        }

        inline uint64_t GetFileLastEditTime(const std::string& path)
        {
            std::error_code ec;
            auto time = std::filesystem::last_write_time(path, ec);
            if (ec)
            {
                return 0;
            }
            return (uint64_t)time.time_since_epoch().count();
        }

        inline bool IsUnderDirectory(std::string filePath, std::string directoryPath)
        {
            NormalizePath(filePath);
            NormalizePath(directoryPath);
            std::string lowerFile = Utils::String::ToLowerCopy(filePath);
            std::string lowerDir = Utils::String::ToLowerCopy(directoryPath);
            if (!Utils::String::EndsWith(lowerDir, '/'))
                lowerDir += "/";
            return Utils::String::StartsWith(lowerFile, lowerDir) || lowerFile == lowerDir.substr(0, lowerDir.length() - 1);
        }

        inline void DirectoryGetFiles(std::vector<std::string>& files, const std::string& directory, const Char* pattern,
                                      DirectorySearchOption option)
        {
            files.clear();
            if (!DirectoryExists(directory))
                return;

            std::string wanted = pattern ? pattern : "";
            auto accept = [&](const std::filesystem::path& path) {
                if (wanted.empty())
                    return true;
                if (wanted == "*.h")
                    return path.extension() == ".h";
                return true;
            };

            std::error_code ec;
            if (option == DirectorySearchOption::All)
            {
                for (auto& entry : std::filesystem::recursive_directory_iterator(directory, ec))
                {
                    if (entry.is_regular_file() && accept(entry.path()))
                        files.push_back(std::string(entry.path().generic_string()));
                }
            }
            else
            {
                for (auto& entry : std::filesystem::directory_iterator(directory, ec))
                {
                    if (entry.is_regular_file() && accept(entry.path()))
                        files.push_back(std::string(entry.path().generic_string()));
                }
            }
        }
    }
}

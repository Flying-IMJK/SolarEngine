#include "ClangParser.h"
#include "../Database/TypeDatabase.h"
#include "ClangUtils.h"
#include "Core/FileSystem.h"
#include "Core/Time.h"
#include "Core/Utils.h"
#include "CodeGenerators/CodeGenerator_BindingsTypeMap.h"
#include "CodeGenerators/CodeGenerator_Utils.h"

#include "ClangVisitors_TranslationUnit.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <system_error>

//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    static std::string StripCppKeywordPrefixes(std::string type)
    {
        Utils::String::TrimStart(type);
        Utils::String::TrimEnd(type);

        while (Utils::String::StartsWith(type, "::"))
        {
            type = type.substr(2);
        }
        if (Utils::String::StartsWith(type, "class "))
        {
            type = type.substr(6);
        }
        else if (Utils::String::StartsWith(type, "struct "))
        {
            type = type.substr(7);
        }
        else if (Utils::String::StartsWith(type, "enum "))
        {
            type = type.substr(5);
        }

        Utils::String::TrimStart(type);
        Utils::String::TrimEnd(type);
        return type;
    }

    static std::string GetUnqualifiedCppTypeName(std::string const& cppType)
    {
        std::string result      = StripCppKeywordPrefixes(cppType);
        int         pos         = INVALID_INDEX;
        int         searchStart = 0;
        while (true)
        {
            std::string tail  = result.substr(searchStart);
            int         found = Utils::String::Find(tail, "::");
            if (found == INVALID_INDEX)
            {
                break;
            }
            pos         = searchStart + found;
            searchStart = pos + 2;
        }
        if (pos != INVALID_INDEX)
        {
            result = result.substr(pos + 2);
        }
        Utils::String::TrimStart(result);
        Utils::String::TrimEnd(result);
        return result;
    }

    static TypeMapping const* FindTypeMappingForPod(std::string const& cppType)
    {
        std::string        stripped = StripCppKeywordPrefixes(StripTypeQualifiers(cppType));
        TypeMapping const* mapping  = FindTypeMapping(stripped.c_str());
        if (mapping != nullptr)
        {
            return mapping;
        }

        std::string unqualified = GetUnqualifiedCppTypeName(stripped);
        return FindTypeMapping(unqualified.c_str());
    }

    static bool DoesCppTypeMatch(std::string const& strippedCppType,
                                 std::string const& unqualifiedCppType,
                                 std::string const& candidateCppType)
    {
        if (candidateCppType.empty())
        {
            return false;
        }

        std::string candidate = StripCppKeywordPrefixes(StripTypeQualifiers(candidateCppType));
        return strippedCppType == candidate || unqualifiedCppType == GetUnqualifiedCppTypeName(candidate);
    }

    static bool IsKnownNonPodValueType(std::string const& cppType)
    {
        std::string type = GetUnqualifiedCppTypeName(StripTypeQualifiers(cppType));
        return type == "Array" || type == "String" || type == "StringView" || type == "StringAnsi" ||
               type == "StringAnsiView";
    }

    static TypeInfoBase const* FindApiTypeByCppType(TypeDatabase const& database, std::string const& cppType)
    {
        std::string stripped    = StripCppKeywordPrefixes(StripTypeQualifiers(cppType));
        std::string unqualified = GetUnqualifiedCppTypeName(stripped);

        for (auto const& type : database.GetAllTypes())
        {
            if (!type->isAPI)
            {
                continue;
            }

            std::string fullName =
                CodeGeneratorUtils::GetNativeName(type->namespaceScopeList, type->structScopeList, type->name);
            if (DoesCppTypeMatch(stripped, unqualified, fullName))
            {
                return type;
            }

            if (type->IsFlag(TypeInfoBase::Flag::IsStruct))
            {
                auto const* structType = static_cast<TypeInfoStruct const*>(type);
                if (DoesCppTypeMatch(stripped, unqualified, structType->templateInstantiationTypeName))
                {
                    return type;
                }
            }
        }

        for (auto const& type : database.GetAllTypes())
        {
            if (type->isAPI && DoesCppTypeMatch(stripped, unqualified, type->name))
            {
                return type;
            }
        }

        return nullptr;
    }

    static bool
    CalculateStructureIsPod(TypeDatabase const& database, TypeInfoBase const& type, std::vector<TypeID>& stack);

    static bool
    IsBindingFieldTypePod(TypeDatabase const& database, std::string const& cppType, std::vector<TypeID>& stack)
    {
        CppTypeInfo typeInfo;
        typeInfo.Parse(cppType);

        if (typeInfo.isPointer || typeInfo.isRef || typeInfo.isMoveRef)
        {
            return true;
        }
        if (IsCollectionType(cppType))
        {
            return false;
        }

        std::string        valueType = typeInfo.baseType.empty() ? cppType : typeInfo.baseType;
        TypeMapping const* mapping   = FindTypeMappingForPod(valueType);
        if (mapping != nullptr)
        {
            return mapping->isBlittable;
        }
        if (IsKnownNonPodValueType(valueType))
        {
            return false;
        }

        TypeInfoBase const* apiType = FindApiTypeByCppType(database, valueType);
        if (apiType != nullptr)
        {
            if (apiType->IsFlag(TypeInfoBase::Flag::IsEnum))
            {
                return true;
            }
            if (!apiType->IsFlag(TypeInfoBase::Flag::IsStruct))
            {
                return false;
            }
            return CalculateStructureIsPod(database, *apiType, stack);
        }

        return true;
    }

    static bool
    CalculateStructureIsPod(TypeDatabase const& database, TypeInfoBase const& type, std::vector<TypeID>& stack)
    {
        if (!type.IsFlag(TypeInfoBase::Flag::IsStruct))
        {
            return false;
        }

        auto const& structType = static_cast<TypeInfoStruct const&>(type);
        if (structType.APIIsInterface || !structType.interfaces.empty())
        {
            return false;
        }
        if (Utils::Vector::Contains(stack, type.typeID))
        {
            return true;
        }

        stack.push_back(type.typeID);
        bool isPod = true;

        if (!structType.baseClassName.empty())
        {
            TypeInfoBase const* baseType = FindApiTypeByCppType(database, structType.baseClassName);
            isPod                        = baseType != nullptr && baseType->IsFlag(TypeInfoBase::Flag::IsStruct) &&
                                           CalculateStructureIsPod(database, *baseType, stack);
        }

        for (int i = 0; isPod && i < structType.fields.size(); ++i)
        {
            TypeInfoField const& field = structType.fields[i];
            if (!field.isStatic && !IsBindingFieldTypePod(database, field.type.ToString(), stack))
            {
                isPod = false;
            }
        }

        stack.pop_back();
        return isPod;
    }

    static void UpdateStructPodFlags(TypeDatabase& database)
    {
        std::vector<TypeID> stack;

        for (auto* type : database.GetAllTypes())
        {
            if (!type->IsFlag(TypeInfoBase::Flag::IsStruct))
            {
                continue;
            }

            stack.clear();
            auto* structType  = static_cast<TypeInfoStruct*>(type);
            structType->isPod = structType->isStruct && CalculateStructureIsPod(database, *structType, stack);
        }
    }

    static void RegisterTypeNameAliases(TypeDatabase const& database)
    {
        ClearApiTypeNameAliases();
        for (auto const& type : database.GetAllTypes())
        {
            if (!type->isAPI || type->IsFlag(TypeInfoBase::Flag::IsEnum))
            {
                continue;
            }

            TypeInfoStruct const* structType = static_cast<TypeInfoStruct const*>(type);

            std::string nativeFullName = CodeGeneratorUtils::GetNativeName(
                structType->namespaceScopeList, structType->structScopeList, type->name);
            std::string publicName = structType->APIName.empty() ? structType->name : structType->APIName;
            std::string publicFullName =
                CodeGeneratorUtils::GetFullCSTypeName(structType->namespaceScopeList, publicName);
            if (!structType->APIInBuildMapType.empty())
            {
                // InBuild types are parsed for lookup, but their managed type
                // already exists. Force aliases to the explicit full C# type so
                // stubs and generated signatures never invent a new declaration.
                publicName = structType->APIInBuildMapType;
                publicFullName = structType->APIInBuildMapType;
            }
            RegisterApiTypeNameAlias(structType->name, nativeFullName, publicName, publicFullName);

            std::string const& templateInstantiationName = structType->templateInstantiationTypeName;
            if (!templateInstantiationName.empty())
            {
                RegisterApiTypeNameAlias(GetUnqualifiedCppTypeName(templateInstantiationName),
                                         templateInstantiationName,
                                         publicName,
                                         publicFullName);
            }

            if (!structType->APIInBuildMapType.empty())
            {
                continue;
            }

            if (structType->isScriptingObject)
            {
                RegisterApiScriptingObjectType(structType->name, nativeFullName);
                if (!templateInstantiationName.empty())
                {
                    RegisterApiScriptingObjectType(GetUnqualifiedCppTypeName(templateInstantiationName),
                                                   templateInstantiationName);
                }
            }
            else if (structType->IsFlag(TypeInfoBase::Flag::IsStruct) && !structType->isPod)
            {
                RegisterApiInteropStructType(structType->name, nativeFullName, publicName, publicFullName);
                if (!templateInstantiationName.empty())
                {
                    RegisterApiInteropStructType(GetUnqualifiedCppTypeName(templateInstantiationName),
                                                 nativeFullName,
                                                 publicName,
                                                 publicFullName);
                }
            }
            else if (!structType->IsFlag(TypeInfoBase::Flag::IsStruct) && !structType->APIIsStatic)
            {
                RegisterApiNativeObjectType(structType->name, nativeFullName);
                if (!templateInstantiationName.empty())
                {
                    RegisterApiNativeObjectType(GetUnqualifiedCppTypeName(templateInstantiationName),
                                                templateInstantiationName);
                }
            }
        }
    }

    void ClangParser::LogClangDiagnostics(CXTranslationUnit& translationUnit)
    {
        std::vector<std::string> diagnostics;
        ClangUtils::GetDiagnostics(translationUnit, diagnostics);

        if (diagnostics.empty())
        {
            return;
        }

        std::cout << " * Clang Diagnostics - " << diagnostics.size() << " diagnostic(s)" << std::endl;
        for (std::string const& diagnostic : diagnostics)
        {
            std::cout << "   " << diagnostic << std::endl;
        }
    }

    std::string ClangParser::NormalizeToolchainPath(std::string path)
    {
        Utils::String::TrimStart(path);
        Utils::String::TrimEnd(path);
        if (Utils::String::StartsWith(path, "\"") && Utils::String::EndsWith(path, '"'))
        {
            path = path.substr(1, path.length() - 2);
        }

        FileSystem::NormalizePath(path);
        while (!path.empty() && (Utils::String::EndsWith(path, '/') || Utils::String::EndsWith(path, '\\')))
        {
            path.pop_back();
        }
        return path;
    }

    std::string ClangParser::GetParentPath(std::string const& path)
    {
        std::string result = std::filesystem::path(path).parent_path().generic_string();
        FileSystem::NormalizePath(result);
        return NormalizeToolchainPath(result);
    }

    void ClangParser::AddUniquePath(std::vector<std::string>& paths, std::string path)
    {
        path = NormalizeToolchainPath(path);
        if (path.empty())
        {
            return;
        }
        if (!Utils::Vector::Contains(paths, path))
        {
            paths.push_back(path);
        }
    }

    std::string ClangParser::GetEnvironmentVariableValue(char const* environmentVariableName)
    {
#if defined(_WIN32)
        size_t requiredSize = 0;
        getenv_s(&requiredSize, nullptr, 0, environmentVariableName);
        if (requiredSize == 0)
        {
            return "";
        }

        std::string value(requiredSize, '\0');
        getenv_s(&requiredSize, value.data(), value.size(), environmentVariableName);
        if (!value.empty() && value.back() == '\0')
        {
            value.pop_back();
        }
        return value;
#else
        char const* value = std::getenv(environmentVariableName);
        if (value == nullptr || value[0] == '\0')
        {
            return "";
        }
        return value;
#endif
    }

    void ClangParser::TryAddEnvironmentPath(std::vector<std::string>& paths, char const* environmentVariableName)
    {
        std::string const value = GetEnvironmentVariableValue(environmentVariableName);
        if (value.empty())
        {
            return;
        }

        AddUniquePath(paths, value);
    }

    void ClangParser::TryAddEnvironmentPathList(std::vector<std::string>& paths, char const* environmentVariableName)
    {
        std::string const value = GetEnvironmentVariableValue(environmentVariableName);
        if (value.empty())
        {
            return;
        }

        std::vector<std::string> entries;
        Utils::String::Split(value, ';', entries);
        for (std::string const& entry : entries)
        {
            AddUniquePath(paths, entry);
        }
    }

    bool ClangParser::TryExtractMsvcToolsDirectoryFromPath(std::string const& path,
                                                           std::string& outToolsVersion,
                                                           std::string& outToolsDirectory)
    {
        std::string const normalizedPath = NormalizeToolchainPath(path);
        std::string const lowerPath      = Utils::String::ToLowerCopy(normalizedPath);
        std::string const marker         = "/vc/tools/msvc/";

        int const markerPos = Utils::String::Find(lowerPath, marker);
        if (markerPos == INVALID_INDEX)
        {
            return false;
        }

        int const versionStart = markerPos + (int)marker.length();
        int       versionEnd   = Utils::String::Find(normalizedPath, '/', versionStart);
        if (versionEnd == INVALID_INDEX)
        {
            versionEnd = (int)normalizedPath.length();
        }
        if (versionEnd <= versionStart)
        {
            return false;
        }

        outToolsVersion   = normalizedPath.substr(versionStart, versionEnd - versionStart);
        outToolsDirectory = normalizedPath.substr(0, versionEnd);
        return true;
    }


    bool ClangParser::ReadCommandOutputLines(std::string const& command, std::vector<std::string>& outLines)
    {
#if defined(_WIN32)
        outLines.clear();

        FILE* pipe = _popen(command.c_str(), "r");
        if (pipe == nullptr)
        {
            return false;
        }

        std::array<char, 4096> buffer{};
        while (fgets(buffer.data(), (int)buffer.size(), pipe) != nullptr)
        {
            std::string line = buffer.data();
            Utils::String::TrimStart(line);
            Utils::String::TrimEnd(line);
            if (!line.empty())
            {
                outLines.push_back(line);
            }
        }

        int const exitCode = _pclose(pipe);
        return exitCode == 0 || !outLines.empty();
#else
        (void)command;
        outLines.clear();
        return false;
#endif
    }

    void ClangParser::TryAddMsvcToolsDirectoryFromPath(std::vector<std::string>& exactToolDirectories,
                                                       std::vector<std::string>& toolRoots,
                                                       std::string const& path)
    {
        std::string toolsVersion;
        std::string toolsDirectory;
        if (!TryExtractMsvcToolsDirectoryFromPath(path, toolsVersion, toolsDirectory))
        {
            return;
        }

        AddUniquePath(exactToolDirectories, toolsDirectory);
        AddUniquePath(toolRoots, GetParentPath(toolsDirectory));
    }

    void ClangParser::TryAddMsvcToolsDirectoriesFromPathEnvironment(std::vector<std::string>& exactToolDirectories,
                                                                    std::vector<std::string>& toolRoots)
    {
        std::vector<std::string> pathEntries;
        TryAddEnvironmentPathList(pathEntries, "PATH");
        for (std::string const& pathEntry : pathEntries)
        {
            TryAddMsvcToolsDirectoryFromPath(exactToolDirectories, toolRoots, pathEntry);
        }
    }

    void ClangParser::TryAddVsWherePaths(std::vector<std::string>& vsWherePaths)
    {
        std::vector<std::string> pathEntries;
        TryAddEnvironmentPathList(pathEntries, "PATH");
        for (std::string const& pathEntry : pathEntries)
        {
            AddUniquePath(vsWherePaths, FileSystem::PathCombine(pathEntry, "vswhere.exe"));
        }

        std::vector<std::string> programFilesDirectories;
        TryAddEnvironmentPath(programFilesDirectories, "ProgramFiles(x86)");
        TryAddEnvironmentPath(programFilesDirectories, "ProgramFiles");
        for (std::string const& programFilesDirectory : programFilesDirectories)
        {
            AddUniquePath(vsWherePaths,
                          FileSystem::PathCombine(programFilesDirectory,
                                                  "Microsoft Visual Studio/Installer/vswhere.exe"));
        }
    }

    void ClangParser::TryAddMsvcToolRootsFromVisualStudioInstaller(std::vector<std::string>& toolRoots)
    {
        std::vector<std::string> vsWherePaths;
        TryAddVsWherePaths(vsWherePaths);

        for (std::string & vsWherePath : vsWherePaths)
        {
            if (!FileSystem::FileExists(vsWherePath))
            {
                continue;
            }

            std::vector<std::string> installationPaths;

            Utils::String::ReplaceAll(vsWherePath, "\"", "\\\"");
            std::string command =  "\"" + vsWherePath + "\"" + " -all -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath";

            if (!ReadCommandOutputLines(command, installationPaths))
            {
                continue;
            }

            for (std::string const& installationPath : installationPaths)
            {
                AddUniquePath(toolRoots, FileSystem::PathCombine(installationPath, "VC/Tools/MSVC"));
            }
        }
    }

    bool ClangParser::TryExtractWindowsSdkIncludeRoot(std::string const& includePath,
                                                      std::string& outSdkVersion,
                                                      std::string& outIncludeRoot)
    {
        std::string const normalizedPath = NormalizeToolchainPath(includePath);
        std::string const lowerPath      = Utils::String::ToLowerCopy(normalizedPath);
        std::string const marker         = "/windows kits/10/include/";

        int const markerPos = Utils::String::Find(lowerPath, marker);
        if (markerPos == INVALID_INDEX)
        {
            return false;
        }

        int const versionStart = markerPos + (int)marker.length();
        int       versionEnd   = Utils::String::Find(normalizedPath, '/', versionStart);
        if (versionEnd == INVALID_INDEX)
        {
            versionEnd = (int)normalizedPath.length();
        }
        if (versionEnd <= versionStart)
        {
            return false;
        }

        outSdkVersion = normalizedPath.substr(versionStart, versionEnd - versionStart);
        outIncludeRoot = normalizedPath.substr(0, versionEnd);
        return true;
    }

    void ClangParser::TryAddWindowsSdkRootsFromRegistry(std::vector<std::string>& sdkRoots)
    {
        std::vector<std::string> lines;
        if (!ReadCommandOutputLines("reg query \"HKLM\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots\" /v KitsRoot10",
                                    lines))
        {
            return;
        }

        for (std::string const& line : lines)
        {
            int const valueTypePos = Utils::String::Find(line, "REG_SZ");
            if (valueTypePos == INVALID_INDEX)
            {
                continue;
            }

            std::string sdkRoot = line.substr(valueTypePos + 6);
            Utils::String::TrimStart(sdkRoot);
            Utils::String::TrimEnd(sdkRoot);
            AddUniquePath(sdkRoots, sdkRoot);
        }
    }

    int ClangParser::CompareToolchainVersions(std::string const& lhs, std::string const& rhs)
    {
        std::vector<std::string> lhsParts;
        std::vector<std::string> rhsParts;
        Utils::String::Split(lhs, '.', lhsParts);
        Utils::String::Split(rhs, '.', rhsParts);

        size_t const count = std::max(lhsParts.size(), rhsParts.size());
        for (size_t i = 0; i < count; ++i)
        {
            std::string const lhsPart = i < lhsParts.size() ? lhsParts[i] : "0";
            std::string const rhsPart = i < rhsParts.size() ? rhsParts[i] : "0";

            bool lhsNumeric = !lhsPart.empty();
            bool rhsNumeric = !rhsPart.empty();
            uint64_t lhsValue = 0;
            uint64_t rhsValue = 0;
            for (char const ch : lhsPart)
            {
                if (!std::isdigit(static_cast<unsigned char>(ch)))
                {
                    lhsNumeric = false;
                    break;
                }
                lhsValue = lhsValue * 10 + (uint64_t)(ch - '0');
            }
            for (char const ch : rhsPart)
            {
                if (!std::isdigit(static_cast<unsigned char>(ch)))
                {
                    rhsNumeric = false;
                    break;
                }
                rhsValue = rhsValue * 10 + (uint64_t)(ch - '0');
            }

            if (lhsNumeric && rhsNumeric)
            {
                if (lhsValue < rhsValue)
                {
                    return -1;
                }
                if (lhsValue > rhsValue)
                {
                    return 1;
                }
                continue;
            }

            int const textCompare = lhsPart.compare(rhsPart);
            if (textCompare < 0)
            {
                return -1;
            }
            if (textCompare > 0)
            {
                return 1;
            }
        }

        return 0;
    }

    bool ClangParser::IsToolchainVersionAtMost(std::string const& version, std::string const& maxVersion)
    {
        return CompareToolchainVersions(version, maxVersion) <= 0;
    }

    void ClangParser::AddToolchainVersionCandidate(std::vector<ToolchainVersionCandidate>& candidates,
                                                   std::string const& version,
                                                   std::string const& path)
    {
        std::string normalizedPath = NormalizeToolchainPath(path);
        if (version.empty() || normalizedPath.empty())
        {
            return;
        }
        for (ToolchainVersionCandidate const& candidate : candidates)
        {
            if (candidate.version == version && candidate.path == normalizedPath)
            {
                return;
            }
        }

        ToolchainVersionCandidate& candidate = Utils::Vector::AddOne(candidates);
        candidate.version = version;
        candidate.path = normalizedPath;
    }

    void ClangParser::TryAddMsvcToolsCandidatesFromRoot(std::vector<ToolchainVersionCandidate>& candidates,
                                                        std::string const& toolRoot)
    {
        std::string const normalizedRoot = NormalizeToolchainPath(toolRoot);
        if (!FileSystem::DirectoryExists(normalizedRoot))
        {
            return;
        }

        std::error_code ec;
        for (auto const& entry : std::filesystem::directory_iterator(normalizedRoot, ec))
        {
            if (!entry.is_directory())
            {
                continue;
            }

            std::string const toolsDirectory = entry.path().generic_string();
            if (!FileSystem::DirectoryExists(FileSystem::PathCombine(toolsDirectory, "include")))
            {
                continue;
            }

            AddToolchainVersionCandidate(candidates, entry.path().filename().generic_string(), toolsDirectory);
        }
    }

    void ClangParser::TryAddWindowsSdkIncludeCandidatesFromRoot(std::vector<ToolchainVersionCandidate>& candidates,
                                                                std::string const& sdkRoot)
    {
        std::string const includeRoot = FileSystem::PathCombine(NormalizeToolchainPath(sdkRoot), "Include");
        if (!FileSystem::DirectoryExists(includeRoot))
        {
            return;
        }

        std::error_code ec;
        for (auto const& entry : std::filesystem::directory_iterator(includeRoot, ec))
        {
            if (!entry.is_directory())
            {
                continue;
            }

            std::string const versionIncludeRoot = entry.path().generic_string();
            if (!FileSystem::DirectoryExists(FileSystem::PathCombine(versionIncludeRoot, "ucrt")) ||
                !FileSystem::DirectoryExists(FileSystem::PathCombine(versionIncludeRoot, "shared")) ||
                !FileSystem::DirectoryExists(FileSystem::PathCombine(versionIncludeRoot, "um")))
            {
                continue;
            }

            AddToolchainVersionCandidate(candidates, entry.path().filename().generic_string(), versionIncludeRoot);
        }
    }

    bool ClangParser::TrySelectNewestToolchainCandidate(std::vector<ToolchainVersionCandidate>& candidates,
                                                        std::string const& maxVersion,
                                                        std::string& outVersion,
                                                        std::string& outPath)
    {
        outVersion.clear();
        outPath.clear();

        Utils::Vector::QuickSort(candidates, [this](ToolchainVersionCandidate const& lhs,
                                                    ToolchainVersionCandidate const& rhs) {
            int const versionCompare = CompareToolchainVersions(lhs.version, rhs.version);
            if (versionCompare != 0)
            {
                return versionCompare > 0;
            }
            return lhs.path < rhs.path;
        });

        for (ToolchainVersionCandidate const& candidate : candidates)
        {
            if (!IsToolchainVersionAtMost(candidate.version, maxVersion))
            {
                continue;
            }

            outVersion = candidate.version;
            outPath = candidate.path;
            return true;
        }

        return false;
    }

    bool ClangParser::TryResolveMsvcToolsDirectory(std::string const& maxVersion,
                                                   std::string& outToolsVersion,
                                                   std::string& outToolsDirectory)
    {
        outToolsVersion.clear();
        outToolsDirectory.clear();

        std::vector<ToolchainVersionCandidate> candidates;
        std::vector<std::string> exactToolDirectories;
        std::vector<std::string> toolRoots;

        TryAddEnvironmentPath(exactToolDirectories, "VCToolsInstallDir");
        for (std::string const& toolsDirectory : exactToolDirectories)
        {
            AddUniquePath(toolRoots, GetParentPath(toolsDirectory));
        }

        std::vector<std::string> vcInstallDirectories;
        TryAddEnvironmentPath(vcInstallDirectories, "VCINSTALLDIR");
        for (std::string const& vcInstallDirectory : vcInstallDirectories)
        {
            AddUniquePath(toolRoots, FileSystem::PathCombine(vcInstallDirectory, "Tools/MSVC"));
        }

        std::vector<std::string> vsInstallDirectories;
        TryAddEnvironmentPath(vsInstallDirectories, "VSINSTALLDIR");
        for (std::string const& vsInstallDirectory : vsInstallDirectories)
        {
            AddUniquePath(toolRoots, FileSystem::PathCombine(vsInstallDirectory, "VC/Tools/MSVC"));
        }

        std::vector<std::string> includePaths;
        TryAddEnvironmentPathList(includePaths, "INCLUDE");
        for (std::string const& includePath : includePaths)
        {
            TryAddMsvcToolsDirectoryFromPath(exactToolDirectories, toolRoots, includePath);
        }

        TryAddMsvcToolsDirectoriesFromPathEnvironment(exactToolDirectories, toolRoots);
        TryAddMsvcToolRootsFromVisualStudioInstaller(toolRoots);

        for (std::string const& toolsDirectory : exactToolDirectories)
        {
            std::string toolsVersion;
            std::string normalizedToolsDirectory;
            if (!TryExtractMsvcToolsDirectoryFromPath(
                    FileSystem::PathCombine(toolsDirectory, "include"), toolsVersion, normalizedToolsDirectory))
            {
                continue;
            }
            if (FileSystem::DirectoryExists(FileSystem::PathCombine(normalizedToolsDirectory, "include")))
            {
                AddToolchainVersionCandidate(candidates, toolsVersion, normalizedToolsDirectory);
            }
        }

        for (std::string const& toolRoot : toolRoots)
        {
            TryAddMsvcToolsCandidatesFromRoot(candidates, toolRoot);
        }

        return TrySelectNewestToolchainCandidate(candidates, maxVersion, outToolsVersion, outToolsDirectory);
    }

    bool ClangParser::TryResolveWindowsSdkIncludeRoot(std::string const& maxVersion,
                                                      std::string& outSdkVersion,
                                                      std::string& outIncludeRoot)
    {
        outSdkVersion.clear();
        outIncludeRoot.clear();

        std::vector<ToolchainVersionCandidate> candidates;
        std::vector<std::string> sdkRoots;
        TryAddEnvironmentPath(sdkRoots, "WindowsSdkDir");
        TryAddEnvironmentPath(sdkRoots, "UniversalCRTSdkDir");
        TryAddWindowsSdkRootsFromRegistry(sdkRoots);

        for (std::string const& sdkRoot : sdkRoots)
        {
            TryAddWindowsSdkIncludeCandidatesFromRoot(candidates, sdkRoot);
        }

        std::vector<std::string> includePaths;
        TryAddEnvironmentPathList(includePaths, "INCLUDE");
        for (std::string const& includePath : includePaths)
        {
            std::string sdkVersion;
            std::string includeRoot;
            if (TryExtractWindowsSdkIncludeRoot(includePath, sdkVersion, includeRoot) &&
                FileSystem::DirectoryExists(FileSystem::PathCombine(includeRoot, "ucrt")) &&
                FileSystem::DirectoryExists(FileSystem::PathCombine(includeRoot, "shared")) &&
                FileSystem::DirectoryExists(FileSystem::PathCombine(includeRoot, "um")))
            {
                AddToolchainVersionCandidate(candidates, sdkVersion, includeRoot);
            }
        }

        return TrySelectNewestToolchainCandidate(candidates, maxVersion, outSdkVersion, outIncludeRoot);
    }

    bool ClangParser::AddClangSystemInclude(std::vector<std::string>& argumentStorage,
                                            std::vector<char const*>& clangArgs,
                                            std::string const& includePath)
    {
        if (!FileSystem::DirectoryExists(includePath))
        {
            m_context.LogError("Invalid clang system include path: {0}", includePath);
            return false;
        }

        clangArgs.push_back("/imsvc");
        argumentStorage.push_back(includePath);
        clangArgs.push_back(argumentStorage.back().c_str());
        return true;
    }

    bool ClangParser::AddMsvcToolchainArgs(std::vector<std::string>& argumentStorage,
                                           std::vector<char const*>& clangArgs)
    {
        constexpr char const* maxMsvcToolsVersion = "14.40.33807";
        constexpr char const* maxWindowsSdkVersion = "10.0.22621.0";

        std::string msvcToolsVersion;
        std::string msvcToolsDirectory;
        if (!TryResolveMsvcToolsDirectory(maxMsvcToolsVersion, msvcToolsVersion, msvcToolsDirectory))
        {
            m_context.LogError("Could not find MSVC tools version <= {0}", maxMsvcToolsVersion);
            return false;
        }

        std::string windowsSdkVersion;
        std::string windowsSdkIncludeRoot;
        if (!TryResolveWindowsSdkIncludeRoot(maxWindowsSdkVersion, windowsSdkVersion, windowsSdkIncludeRoot))
        {
            m_context.LogError("Could not find Windows SDK version <= {0}", maxWindowsSdkVersion);
            return false;
        }

        std::string const msvcIncludePath = FileSystem::PathCombine(msvcToolsDirectory, "include");

        std::cout << " * Clang MSVC Toolchain - MSVC " << msvcToolsVersion << " (" << msvcIncludePath
                  << "), Windows SDK " << windowsSdkVersion << " (" << windowsSdkIncludeRoot << ")" << std::endl;

        if (!AddClangSystemInclude(argumentStorage, clangArgs, msvcIncludePath) ||
            !AddClangSystemInclude(argumentStorage, clangArgs, FileSystem::PathCombine(windowsSdkIncludeRoot, "ucrt")) ||
            !AddClangSystemInclude(argumentStorage, clangArgs, FileSystem::PathCombine(windowsSdkIncludeRoot, "shared")) ||
            !AddClangSystemInclude(argumentStorage, clangArgs, FileSystem::PathCombine(windowsSdkIncludeRoot, "um")) ||
            !AddClangSystemInclude(argumentStorage, clangArgs, FileSystem::PathCombine(windowsSdkIncludeRoot, "winrt")) ||
            !AddClangSystemInclude(argumentStorage, clangArgs, FileSystem::PathCombine(windowsSdkIncludeRoot, "cppwinrt")))
        {
            return false;
        }

        return true;
    }

    ClangParser::ClangParser( SolutionInfo* pSolution, TypeDatabase* pDatabase, std::string const& reflectionDataPath )
        : m_context( pSolution, pDatabase )
        , m_totalParsingTime( 0 )
        , m_totalVisitingTime( 0 )
        , m_reflectionDataPath( reflectionDataPath )
    {}

    bool ClangParser::Parse(std::vector<HeaderInfo*> const& headers)
    {
        // Create single amalgamated header file for all headers to parse
        //-------------------------------------------------------------------------

        std::ofstream reflectorFileStream;
		std::string const reflectorHeader = m_reflectionDataPath + "Reflector.h";
        FileSystem::CreateDirectory(FileSystem::GetParentDirectory(reflectorHeader));
        reflectorFileStream.open( reflectorHeader, std::ios::out | std::ios::trunc );
        ENGINE_ASSERT( !reflectorFileStream.fail() );

		std::string includeStr;
        m_context.headersToVisit.clear();
        for ( HeaderInfo const* pHeader : headers )
        {
            m_context.headersToVisit.push_back( ClangParserContext::HeaderToVisit(pHeader->headerId, pHeader) );
            includeStr += "#include \"" + pHeader->filePath + "\"\n";
        }

        reflectorFileStream.write( includeStr.c_str(), includeStr.length() );
        reflectorFileStream.close();

        // Clang args
        std::vector<std::string> fullIncludePaths;
        fullIncludePaths.reserve(std::size(Settings::g_includePaths) + 32);
		std::vector<char const*> clangArgs;
        int32_t const numIncludePaths = std::size(Settings::g_includePaths);
        for ( auto i = 0; i < numIncludePaths; i++ )
        {
            std::string const fullPath = m_context.pSolution->path + "/" + Settings::g_includePaths[i];
            fullIncludePaths.push_back( "-I" + fullPath );
            clangArgs.push_back( fullIncludePaths.back().c_str() );

            if (!FileSystem::DirectoryExists( fullPath))
            {
                m_context.LogError("Invalid include path: {0}", fullPath );
                return false;
            }
        }

        clangArgs.push_back("--driver-mode=cl");
        clangArgs.push_back( "/TP" );
        clangArgs.push_back( "/std:c++17" );
        clangArgs.push_back( "/Od" );
        clangArgs.push_back( "/DNDEBUG" );
		clangArgs.push_back( "/DPLATFORM_WINDOWS" );
		clangArgs.push_back( "/DPLATFORM_WIN32" );
        clangArgs.push_back( "/D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH" );
        clangArgs.push_back( "/clang:-Werror" );
        clangArgs.push_back( "/clang:-Wno-deprecated-builtins" );
        clangArgs.push_back( "/clang:-fparse-all-comments" );
        clangArgs.push_back( "/clang:-fms-extensions" );
        clangArgs.push_back( "/clang:-fms-compatibility" );
        clangArgs.push_back( "/clang:-Wno-unknown-warning-option" );
        clangArgs.push_back( "/clang:-Wno-return-type-c-linkage" );
        clangArgs.push_back( "/clang:-Wno-gnu-folding-constant" );
        clangArgs.push_back( "/clang:-Wno-nonportable-include-path" );

        if (!AddMsvcToolchainArgs(fullIncludePaths, clangArgs))
        {
            return false;
        }

        //-------------------------------------------------------------------------
        // Set up clang
        auto idx = clang_createIndex( 0, 1 );
        uint32_t const clangOptions = CXTranslationUnit_DetailedPreprocessingRecord | CXTranslationUnit_SkipFunctionBodies | CXTranslationUnit_IncludeBriefCommentsInCodeCompletion;

        // Parse Headers
        CXTranslationUnit tu;
        CXErrorCode result = CXError_Failure;
        {
            ScopedTimer<PlatformClock> timer( m_totalParsingTime );
            result = clang_parseTranslationUnit2( idx, reflectorHeader.c_str(), Utils::Vector::Data(clangArgs), (int)clangArgs.size(), 0, 0, clangOptions, &tu );
        }

        // Handle result of parse
        if (result == CXError_Success)
        {
            LogClangDiagnostics(tu);

            ScopedTimer<PlatformClock> timer(m_totalVisitingTime);
            m_context.Reset( &tu );
            const auto cursor = clang_getTranslationUnitCursor(tu);
            clang_visitChildren( cursor, VisitTranslationUnit, &m_context );

            if (!m_context.HasErrorOccured())
            {
                PostProcessParsed();
            }
        }
        else
        {
            switch ( result )
            {
                case CXError_Failure:
                m_context.LogError("Clang Unknown failure");
                break;

                case CXError_Crashed:
                m_context.LogError( "Clang crashed" );
                break;

                case CXError_InvalidArguments:
                m_context.LogError("Clang Invalid arguments" );
                break;

                case CXError_ASTReadError:
                m_context.LogError("Clang AST read error" );
                break;
			case CXError_Success:
				break;
			}
        }
        clang_disposeIndex(idx);

        //-------------------------------------------------------------------------

        if (!m_context.HasErrorOccured())
        {
            m_context.CheckForOrphanedReflectionMacros();
        }

        // If we have an error from the parser, prepend the header to it
        if (m_context.HasErrorOccured())
        {
            m_context.LogError("\n{0}", m_context.GetErrorMessage());
        }

        return !m_context.HasErrorOccured();
    }

    void ClangParser::PostProcessParsed()
    {
        if (!m_context.ResolvePendingTypeDefs())
        {
            return;
        }

        UpdateStructPodFlags(*m_context.pDatabase);
        RegisterTypeNameAliases(*m_context.pDatabase);
    }
}

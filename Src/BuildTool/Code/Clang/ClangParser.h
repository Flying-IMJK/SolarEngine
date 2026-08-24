#pragma once

#include "ClangParserContext.h"
#include "Core/Time.h"
//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    class ClangParser
    {
    public:

        ClangParser( SolutionInfo* pSolution, TypeDatabase* pDatabase, std::string const& reflectionDataPath );

        inline Milliseconds GetParsingTime() const { return m_totalParsingTime; }
        inline Milliseconds GetVisitingTime() const { return m_totalVisitingTime; }

        bool Parse(std::vector<HeaderInfo*> const& headers);
        void             PostProcessParsed();
		std::string_view GetErrorMessage() const { return m_context.GetErrorMessage(); }

    private:

        void LogClangDiagnostics(CXTranslationUnit& translationUnit);

        std::string NormalizeToolchainPath(std::string path);
        std::string GetParentPath(std::string const& path);
        void AddUniquePath(std::vector<std::string>& paths, std::string path);
        std::string GetEnvironmentVariableValue(char const* environmentVariableName);
        void TryAddEnvironmentPath(std::vector<std::string>& paths, char const* environmentVariableName);
        void TryAddEnvironmentPathList(std::vector<std::string>& paths, char const* environmentVariableName);

        bool TryExtractMsvcToolsDirectoryFromPath(std::string const& path,
                                                  std::string& outToolsVersion,
                                                  std::string& outToolsDirectory);
        bool ReadCommandOutputLines(std::string const& command, std::vector<std::string>& outLines);
        void TryAddMsvcToolsDirectoryFromPath(std::vector<std::string>& exactToolDirectories,
                                              std::vector<std::string>& toolRoots,
                                              std::string const& path);
        void TryAddMsvcToolsDirectoriesFromPathEnvironment(std::vector<std::string>& exactToolDirectories,
                                                           std::vector<std::string>& toolRoots);
        void TryAddVsWherePaths(std::vector<std::string>& vsWherePaths);
        void TryAddMsvcToolRootsFromVisualStudioInstaller(std::vector<std::string>& toolRoots);

        bool TryExtractWindowsSdkIncludeRoot(std::string const& includePath,
                                             std::string& outSdkVersion,
                                             std::string& outIncludeRoot);
        void TryAddWindowsSdkRootsFromRegistry(std::vector<std::string>& sdkRoots);
        struct ToolchainVersionCandidate
        {
            std::string version;
            std::string path;
        };
        int CompareToolchainVersions(std::string const& lhs, std::string const& rhs);
        bool IsToolchainVersionAtMost(std::string const& version, std::string const& maxVersion);
        void AddToolchainVersionCandidate(std::vector<ToolchainVersionCandidate>& candidates,
                                          std::string const& version,
                                          std::string const& path);
        void TryAddMsvcToolsCandidatesFromRoot(std::vector<ToolchainVersionCandidate>& candidates,
                                               std::string const& toolRoot);
        void TryAddWindowsSdkIncludeCandidatesFromRoot(std::vector<ToolchainVersionCandidate>& candidates,
                                                       std::string const& sdkRoot);
        bool TrySelectNewestToolchainCandidate(std::vector<ToolchainVersionCandidate>& candidates,
                                               std::string const& maxVersion,
                                               std::string& outVersion,
                                               std::string& outPath);
        bool TryResolveMsvcToolsDirectory(std::string const& maxVersion,
                                          std::string& outToolsVersion,
                                          std::string& outToolsDirectory);
        bool TryResolveWindowsSdkIncludeRoot(std::string const& maxVersion,
                                             std::string& outSdkVersion,
                                             std::string& outIncludeRoot);

        bool AddClangSystemInclude(std::vector<std::string>& argumentStorage,
                                   std::vector<char const*>& clangArgs,
                                   std::string const& includePath);
        bool AddMsvcToolchainArgs(std::vector<std::string>& argumentStorage,
                                  std::vector<char const*>& clangArgs);

        ClangParserContext                  m_context;
        Milliseconds                        m_totalParsingTime;
        Milliseconds                        m_totalVisitingTime;
		std::string                    		m_reflectionDataPath;
    };
}

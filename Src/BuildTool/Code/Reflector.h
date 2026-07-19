#pragma once

#include <iostream>
#include "Database/ReflectionDatabase.h"

//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    class Reflector
    {
        enum class HeaderProcessResult
        {
            ErrorOccured,
            ParseHeader,
            IgnoreHeader,
        };

        struct HeaderTimestamp
        {
            HeaderTimestamp(HeaderID ID, uint64_t timestamp) : m_ID(ID), m_timestamp(timestamp) {}

            HeaderID m_ID;
            uint64_t m_timestamp;
        };

        struct ProjectObject
        {
			std::string name;
            std::string path;
			std::string includeFile;
        };

    public:
        Reflector() = default;

        bool ParseSolution(std::string slnRootPath, std::string &slnPath);
        bool Clean();
        bool Build();

    private:

		bool LogError(char const *pErrorFormat) const
		{
			std::cout << "Error: " << pErrorFormat << std::endl;
			return false;
		}

        template<typename... Params>
        bool LogError(char const *pErrorFormat, Params&... args) const
        {
			std::string msg = Utils::String::Format(pErrorFormat, args...);

            std::cout << "Error: " << msg << std::endl;
            return false;
        }

		template<typename... Params>
		bool LogError(char const *pErrorFormat, Params const &... args) const
		{
			std::string msg = Utils::String::Format(pErrorFormat, args...);

			std::cout << "Error: " << msg << std::endl;
			return false;
		}

        bool ParseProject(ProjectObject &prjPath);

        HeaderProcessResult ProcessHeaderFile(std::string &filePath, std::string &exportMacro, std::vector<std::string> &headerFileContents);
		uint64_t CalculateHeaderChecksum(std::string &engineIncludePath, std::string &filePath);

        bool UpToDateCheck();
        bool ReflectRegisteredHeaders();
        bool WriteTypeData();

    private:
		std::string m_reflectionDataPath;
        SolutionInfo m_solution;
        ReflectionDatabase m_database;

        // Up to data checks
		std::vector<HeaderTimestamp> m_registeredHeaderTimestamps;
    };
}

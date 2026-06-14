#pragma once

#include <iostream>
#include "Database/ReflectionDatabase.h"
#include "Core/Utilities/Time.h"
#include "Core/Types/Strings/String.h"

//-------------------------------------------------------------------------

namespace SE::ReflectTool
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
			String name;
            String path;
			String includeFile;
        };

    public:
        Reflector() = default;

        bool ParseSolution(String slnRootPath, String &slnPath);
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
			StringAnsi msg = StringAnsi::Format(pErrorFormat, args...);

            std::cout << "Error: " << msg.Get() << std::endl;
            return false;
        }

		template<typename... Params>
		bool LogError(char const *pErrorFormat, Params const &... args) const
		{
			StringAnsi msg = StringAnsi::Format(pErrorFormat, args...);

			std::cout << "Error: " << msg.Get() << std::endl;
			return false;
		}

        bool ParseProject(ProjectObject &prjPath);

        HeaderProcessResult ProcessHeaderFile(String &filePath, String &exportMacro, List<StringAnsi> &headerFileContents);
		DateTime CalculateHeaderChecksum(String &engineIncludePath, String &filePath);

        bool UpToDateCheck();
        bool ReflectRegisteredHeaders();
        bool WriteTypeData();

    private:
		String m_reflectionDataPath;
        SolutionInfo m_solution;
        ReflectionDatabase m_database;

        // Up to data checks
		std::vector<HeaderTimestamp> m_registeredHeaderTimestamps;
    };
}
#include "Reflector.h"
#include "Clang/ClangParser.h"
#include "CodeGenerators/CodeGenerator_CPP.h"
#include "Core/Json.h"
#include "ThirdParty/cmdParser/cmdParser.h"
#include "Core/TopologicalSort.h"
#include "Core/FileSystem.h"
#include "Core/Time.h"

#include <fstream>
#include <iostream>
#include <filesystem>



//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    namespace
    {
        bool SortProjectsByDependencies(std::vector<ProjectInfo> &projects)
        {
            int const numProjects = projects.size();
            if (numProjects <= 1)
            {
                return true;
            }

            // Create list to sort
			std::vector<TopologicalSorter::Node> list;
            for (auto p = 0; p < numProjects; p++)
            {
                list.push_back(TopologicalSorter::Node(p));
            }

            for (auto p = 0; p < numProjects; p++)
            {
                int32_t const numDependencies = (int32_t)projects[p].dependencies.size();
                for (auto d = 0; d < numDependencies; d++)
                {
                    for (auto pp = 0; pp < numProjects; pp++)
                    {
                        if (p != pp && projects[pp].id == projects[p].dependencies[d])
                        {
                            list[p].m_children.push_back(&list[pp]);
                        }
                    }
                }
            }

            // Try to sort
            if (!TopologicalSorter::Sort(list))
            {
                return false;
            }

            // Update type list
            int depValue = 0;
            std::vector<ProjectInfo> sortedProjects;
            sortedProjects.reserve(numProjects);

            for (auto &node : list)
            {
                sortedProjects.push_back(projects[node.m_ID]);
                sortedProjects.back().dependencyCount = depValue++;
            }
            projects.swap(sortedProjects);
            return true;
        }
    }

    bool Reflector::ParseSolution(std::string slnRootPath, std::string &slnPath)
    {
        if (slnPath.empty() || !FileSystem::MatchesExtension(slnPath, "json"))
        {
            return LogError("Invalid solution file name: {0}", slnPath);
        }

        if (!FileSystem::FileExists(slnPath))
        {
            return LogError("Solution doesnt exist: {0}", slnPath);
        }

        //-------------------------------------------------------------------------

        std::cout << " * Parsing Solution: " << slnPath << std::endl;
        std::cout << " ----------------------------------------------" << std::endl
                  << std::endl;

		Milliseconds parsingTime = 0.0f;
        {
            ScopedTimer<PlatformClock> timer(parsingTime);


			std::string jsonText;
            if (!Utils::ReadAllText(slnPath, jsonText))
            {
                return LogError("Could not open solution: {0}", slnPath);
            }

			Json::Document jsonReader;
			jsonReader.ParseInsitu(jsonText.data());

            std::ifstream slnFile(slnPath);
            if (!slnFile.is_open())
            {
                return LogError("Could not open solution: {0}", slnPath);
            }

            m_solution.path = slnRootPath;
			std::string currentProcessDirectory = FileSystem::GetCurrentProcessDirectory();
            m_reflectionDataPath = currentProcessDirectory + Settings::g_temporaryDirectoryPath;
            FileSystem::NormalizePath(m_reflectionDataPath);
            FileSystem::CreateDirectory(m_reflectionDataPath);

            auto documentArray = jsonReader.GetArray();
            
            std::vector<ProjectObject> projectObjects;

            for (size_t i = 0; i < documentArray.Size(); i++)
            {
                auto &arrayValue = documentArray[i];
                auto targetName = arrayValue.FindMember("TargetName");
                auto targetDir = arrayValue.FindMember("TargetDir");
				std::string targetInclude(arrayValue.FindMember("TargetInclude")->value.GetString());

                std::string projectPathString(targetDir->value.GetString());
				std::string projectName(targetName->value.GetString());
				FileSystem::NormalizePath(projectPathString);

				std::string projectPath = projectPathString; //m_solution.m_path + projectPathString;

                ProjectObject projectObject;
                projectObject.name = projectName;
                projectObject.path = projectPathString;
                projectObject.includeFile = targetInclude;

                projectObjects.emplace_back(projectObject);
            }

            slnFile.close();

            // Sort and parse project files
            //-------------------------------------------------------------------------

            std::sort(projectObjects.begin(), projectObjects.end(), [](ProjectObject const &projectA, ProjectObject const &projectB)
			{
			  return projectA.path < projectB.path;
			});

            for (auto &projectObject : projectObjects)
            {
                if (!ParseProject(projectObject))
                {
                    return false;
                }
            }

            //-------------------------------------------------------------------------

            // Sort projects by dependencies
            if (!SortProjectsByDependencies(m_solution.projects))
            {
                return LogError("Illegal dependency in projects detected!");
            }
        }

        std::cout << std::endl
                  << " >>> Solution Parsing - Complete! ( " << (float)parsingTime.ToSeconds() << " ms )" << std::endl
                  << std::endl;
        return true;
    }

	uint64_t Reflector::CalculateHeaderChecksum(std::string &engineIncludePath, std::string &filePath)
    {
        static char const *headerString = "Note: including file: ";
        return FileSystem::GetFileLastEditTime(filePath);
    }

    bool Reflector::ParseProject(ProjectObject &prjObject)
    {
        ENGINE_ASSERT(FileSystem::IsUnderDirectory(prjObject.path, m_solution.path));

        ProjectInfo prj;
        prj.name = prjObject.name;
        prj.id = ProjectInfo::GetProjectID(prjObject.path);
		std::string projectPath = prjObject.path;
		prj.parentPath = FileSystem::GetParentDirectory(projectPath);
        prj.path = prjObject.path;
        prj.isToolsModule = Utils::IsFileUnderToolsProject(prjObject.path);


        std::vector<std::string> sourceFiles;
		Utils::String::Split(prjObject.includeFile, ';', sourceFiles);
        
        for (std::string sourceFile : sourceFiles)
        {
            // Ignore auto-generated files
            if (Utils::String::Find(sourceFile, Settings::g_autogeneratedDirectory) != INVALID_INDEX)
            {
                continue;
            }

			std::string const headerFilePath(prj.parentPath + "/" + sourceFile);
            std::string headerFileFullPath = prj.parentPath + "/" + sourceFile;

            std::vector<std::string> headerFileContents;
            HeaderProcessResult const result = ProcessHeaderFile(headerFileFullPath, prj.exportMacro, headerFileContents);
            switch (result)
            {
                case HeaderProcessResult::ParseHeader:
                {
                    HeaderInfo &headerInfo = prj.headerFiles.emplace_back();
                    headerInfo.headerId = HeaderInfo::GetHeaderID(headerFileFullPath);
                    headerInfo.projectID = prj.id;
                    headerInfo.filePath = headerFileFullPath;
                    headerInfo.timestamp = FileSystem::GetFileLastEditTime(headerFileFullPath);
                    headerInfo.fileContents.swap(headerFileContents);

                    // emplace_back to registered timestamp cache, use in up to date checks
                    m_registeredHeaderTimestamps.emplace_back(HeaderTimestamp(headerInfo.headerId, headerInfo.timestamp));
                }
                break;

                case HeaderProcessResult::IgnoreHeader:
                {
                }
                break;

                case HeaderProcessResult::ErrorOccured:
				{
					std::cout << "Error processing: " << headerFileFullPath << std::endl;
					//prjFile.close();
					return false;
				}
            }
        }
        
        //-------------------------------------------------------------------------

        if (prj.name.empty())
        {
            return LogError("Invalid project file detected: {0}", prjObject.path);
        }

        // Print parse results
        //-------------------------------------------------------------------------

        std::cout << " * Project: " << prj.name << " - ";

        // Only add projects to the list that have headers to parse
        if (prj.headerFiles.empty())
        {
            std::cout << "Ignored! ( No headers found! )" << std::endl;
            return true;
        }

        std::cout << "Done! ( " << prj.headerFiles.size() << " header(s) found! )";
        std::cout << std::endl;

		m_solution.projects.push_back(prj);
        return true;
    }

    Reflector::HeaderProcessResult Reflector::ProcessHeaderFile(std::string &filePath, std::string &exportMacroName, std::vector<std::string> &headerFileContents)
    {
        // Open header file
        bool const isModuleAPIHeader = Utils::String::Contains(filePath, "API.h");
        std::ifstream hdrFile(filePath, std::ios::in | std::ios::binary | std::ios::ate);
        if (!hdrFile.is_open())
        {
            LogError("Could not open header file: {0}", filePath);
            return HeaderProcessResult::ErrorOccured;
        }

        // Check file size
        uint32_t const size = (uint32_t)hdrFile.tellg();
        if (size == 0)
        {
            hdrFile.close();
            return HeaderProcessResult::IgnoreHeader;
        }
        hdrFile.seekg(0, std::ios::beg);

        // Read file contents
        headerFileContents.clear();
        std::string stdLine;
        while (std::getline(hdrFile, stdLine))
        {
            headerFileContents.push_back(stdLine.c_str());
        }
        hdrFile.close();

        //-------------------------------------------------------------------------

        // Check for the SE registration macros
        bool exportMacroFound = false;
        uint32_t openCommentBlock = 0;

        for (auto const &line : headerFileContents)
        {
            // Check for comment blocks
            if (Utils::String::Find(line, "/*") != INVALID_INDEX)
                openCommentBlock++;
            if (Utils::String::Find(line, "*/") != INVALID_INDEX)
                openCommentBlock--;

            if (openCommentBlock == 0)
            {
                // Check for line comment
                auto const foundCommentIdx = Utils::String::Find(line, "//");

                // Check for registration macros
                for (auto i = 0u; i < (uint32_t)ReflectionMacroType::NumMacros; i++)
                {
                    ReflectionMacroType const macro = (ReflectionMacroType)i;
                    auto const foundMacroIdx = Utils::String::Find(line, GetMarkMacroText(macro));
                    bool const macroExists = foundMacroIdx != INVALID_INDEX;
                    bool const uncommentedMacro = foundCommentIdx == INVALID_INDEX || foundCommentIdx > foundMacroIdx;

                    if (macroExists && uncommentedMacro)
                    {
                        // We should never have registration macros and the export definition in the same file
                        if (exportMacroFound)
                        {
                            LogError("SE registration macro found in the module export API header({0})!", filePath);
                            return HeaderProcessResult::ErrorOccured;
                        }
                        else
                        {
                            return HeaderProcessResult::ParseHeader;
                        }
                    }
                }

                // Check header for the module export definition
                if (isModuleAPIHeader)
                {
                    auto const foundExportIdx0 = Utils::String::Find(line, "__declspec");
                    auto const foundExportIdx1 = Utils::String::Find(line, "dllexport");
                    if (foundExportIdx0 != INVALID_INDEX && foundExportIdx1 != INVALID_INDEX)
                    {
                        if (!exportMacroName.empty())
                        {
                            LogError("Duplicate export macro definitions found!", filePath);
                            return HeaderProcessResult::ErrorOccured;
                        }
                        else
                        {
                            auto defineIdx = Utils::String::Find(line, "#define");
                            if (defineIdx != INVALID_INDEX)
                            {
                                defineIdx += 8;
                                exportMacroName = line.substr(defineIdx, foundExportIdx0 - 1 - defineIdx);
                            }

                            return HeaderProcessResult::IgnoreHeader;
                        }
                    }
                }
            }
        }

        return HeaderProcessResult::IgnoreHeader;
    }

    bool Reflector::Clean()
    {
        std::cout << " * Cleaning Solution: " << m_solution.path << std::endl;
        std::cout << " ----------------------------------------------" << std::endl
                  << std::endl;

        // Delete all auto-generated directories for valid projects
        for (auto &prj : m_solution.projects)
        {
            std::cout << " * Cleaning Project - " << prj.name << std::endl;

            std::string autoGeneratedDirectory = prj.path + "/" + Settings::g_autogeneratedDirectory;
            FileSystem::DeleteDirectory(autoGeneratedDirectory);

            autoGeneratedDirectory = prj.path + "/" + Settings::g_autogeneratedModuleFileSuffix;
            FileSystem::DeleteFile(autoGeneratedDirectory);
            Utils::WriteAllText(autoGeneratedDirectory, std::string());
        }

        std::cout << std::endl;

        // Delete all auto-generated directories for excluded projects (just to be safe)
        for (auto &prjPath : m_solution.excludedProjects)
        {
            std::string autoGeneratedDirectory = FileSystem::GetParentDirectory(prjPath);
			autoGeneratedDirectory += Settings::g_autogeneratedDirectory;
            std::cout << " * Cleaning Excluded Project - " << autoGeneratedDirectory << std::endl;

            if (!FileSystem::DeleteDirectory(autoGeneratedDirectory))
            {
                return LogError(" * Error erasing directory: {0}", autoGeneratedDirectory);
            }
        }

        std::cout << std::endl;

		std::string const databasePath(m_reflectionDataPath + "TypeDatabase.db");
        bool const result = FileSystem::FileExists(databasePath) || FileSystem::DeleteFile(databasePath);
        if (result)
        {
            std::cout << " * Deleted: " << databasePath << std::endl;
        }
        else
        {
            std::cout << " * Error deleting: " << databasePath << std::endl;
        }

        std::cout << std::endl;
        std::cout << " >>> Cleaning - Complete!" << std::endl
                  << std::endl;
        return result;
    }

    bool Reflector::Build()
    {
        std::cout << " * Reflecting Solution: " << m_solution.path << std::endl;
        std::cout << " ----------------------------------------------" << std::endl
                  << std::endl;

        Milliseconds time = 0;
        {
            ScopedTimer<PlatformClock> timer(time);

            // Open connection to type database and read all previous data
			std::string databasePath(m_reflectionDataPath + "TypeDatabase.db");
            m_database.ReadDatabase(databasePath);

            //-------------------------------------------------------------------------

            if (!UpToDateCheck())
            {
                return false;
            }

            //-------------------------------------------------------------------------

            if (!ReflectRegisteredHeaders())
            {
                return false;
            }
        }

        std::cout << std::endl;
        std::cout << " >>> Reflection - Complete! ( " << (float)time.ToSeconds() << "s )" << std::endl;

        return true;
    }

    bool Reflector::UpToDateCheck()
    {
        std::cout << " * Performing Up-to-date check - ";
        Milliseconds time = 0;
        {
            ScopedTimer<PlatformClock> timer(time);
            std::vector<HeaderID> registeredHeaders;
            for (auto &prj : m_solution.projects)
            {
                std::string autoGeneratedDirectory = prj.path + Settings::g_autogeneratedDirectory;
				std::vector<std::string> existingFiles;
                FileSystem::DirectoryGetFiles(existingFiles, autoGeneratedDirectory, nullptr, DirectorySearchOption::All);

                for (auto i = 0u; i < prj.headerFiles.size(); i++)
                {
                    bool isDirty = false;
                    auto &header = prj.headerFiles[i];
                    registeredHeaders.push_back(header.headerId);

                    // Does the output file exist?
                    std::string const autoGeneratedFilePath = header.GetAutogeneratedTypeInfoFileName(autoGeneratedDirectory);
                    isDirty = !FileSystem::FileExists(autoGeneratedFilePath);

                    // Remove this file from the from existing files list - this is used to clean up the auto-generated directory of old files
                    for (auto j = 0u; j < existingFiles.size(); j++)
                    {
                        if (FileSystem::GetFileNameWithoutExtension(existingFiles[j]) == FileSystem::GetFileNameWithoutExtension(autoGeneratedFilePath))
                        {
                            if (j != existingFiles.size() - 1)
                            {
                                existingFiles[j] = existingFiles.back();
                            }

                            existingFiles.pop_back();
                            j--;
                        }
                    }

                    // Try to get existing record
                    if (!isDirty)
                    {
                        HeaderInfo const *pExistingRecord = m_database.GetHeaderDesc(header.headerId);
                        if (pExistingRecord != nullptr)
                        {
                            ENGINE_ASSERT(pExistingRecord->headerId != HeaderID::Invalid);

                            // Check timestamp
                            if (header.timestamp > pExistingRecord->timestamp)
                            {
                                isDirty = true;
                            }
                            else
                            {
                                std::string filePath = header.filePath;
                                header.checksum = CalculateHeaderChecksum(m_solution.path, filePath);
                                if (header.checksum == 0)
                                {
                                    return LogError("Failed to perform up to date check for: {0}", header.filePath);
                                }
                                else if (header.checksum != pExistingRecord->checksum)
                                {
                                    isDirty = true;
                                }
                            }
                        }
                        else
                        {
                            isDirty = true;
                        }
                    }

                    // Update file checksum if file is dirty
                    if (isDirty)
                    {
                        m_database.UpdateHeaderRecord(header);
                        prj.dirtyHeaders.emplace_back(i);
                    }
                }
            }

            // Delete all old types
            m_database.DeleteObseleteHeadersAndTypes(registeredHeaders);
        }

        std::cout << "Complete! ( " << time << "ms )" << std::endl;
        return true;
    }

    bool Reflector::ReflectRegisteredHeaders()
    {
        // Create list of all headers to parse
        std::vector<HeaderInfo *> headersToParse;
        for (auto &prj : m_solution.projects)
        {
            if (!prj.dirtyHeaders.empty())
            {
                // emplace_back all dirty headers to the list of file to be parsed
                for (auto &hdr : prj.dirtyHeaders)
                {
                    headersToParse.push_back(&prj.headerFiles[hdr]);

                    // Erase all types associated with this header from the database
                    m_database.DeleteTypesForHeader(prj.headerFiles[hdr].headerId);
                }
            }
        }

        //-------------------------------------------------------------------------
        if (!headersToParse.empty())
        {
            ClangParser clangParser(&m_solution, &m_database, m_reflectionDataPath);

            std::cout << " * Parser C++ Code -" << std::endl;;
            if (!clangParser.Parse(headersToParse))
            {
                std::cout << "Error occurred!\n\n  Error: " << clangParser.GetErrorMessage() << std::endl;
                return false;
            }
            std::cout << "Complete! ( P:" << clangParser.GetParsingTime() << "ms, V:" << clangParser.GetVisitingTime() << "ms )" << std::endl;
        }

        // Keep project records in sync before generation. UpdateProjectList preserves
        // module metadata from the database when no dirty header re-parsed it.
        m_database.UpdateProjectList(m_solution.projects);

        //-------------------------------------------------------------------------
        // Run code generation (C++ type-info + bindings from unified data model)
        //-------------------------------------------------------------------------
        {
            Generator generator;
            if (!generator.Generate(m_database, m_solution))
            {
                return LogError(generator.GetErrorMessage());
            }
        }

        //-------------------------------------------------------------------------

        if (!WriteTypeData())
        {
            return false;
        }

        return true;
    }

    bool Reflector::WriteTypeData()
    {
        std::cout << " * Writing Type Database - ";

        Milliseconds time = 0;
        {
            ScopedTimer<PlatformClock> timer(time);
            Generator generator;
            if (!m_database.WriteDatabase(m_reflectionDataPath + "TypeDatabase.db"))
            {
                std::cout << "Error Occurred: " << generator.GetErrorMessage() << std::endl;
                return LogError("{0}", m_database.GetError());
            }
        }

        std::cout << "Complete! ( " << (float)time << "ms )" << std::endl;
        return true;
    }
}

//-------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    std::cout << std::endl;
    std::cout << "==============================================" << std::endl;
    std::cout << "|             SE Build                        |" << std::endl;
    std::cout << "==============================================" << std::endl
              << std::endl;


    // Set precision of cout
    //-------------------------------------------------------------------------

    std::cout.setf(std::ios::fixed, std::ios::floatfield);
    std::cout.precision(2);

    // Read CMD line arguments
    //-------------------------------------------------------------------------

    cli::Parser cmdParser(argc, argv);
    cmdParser.set_required<std::string>("r", "SlnRootPath", "Solution Root Path.");
    cmdParser.set_required<std::string>("s", "SlnFilePath", "Solution Config Path.");
    cmdParser.set_optional<bool>("clean", "Clean", false, "Clean Solution.");
    cmdParser.set_optional<bool>("rebuild", "Rebuild", false, "Clean Solution.");
 #define Debug
#ifndef Debug
    if (!cmdParser.run())
    {
        std::cout << "Type System Reflector Invalid commandline arguments";
        return 1;
    }

	std::string r = cmdParser.get<std::string>("r").c_str();
	std::string s = cmdParser.get<std::string>("s").c_str();
	std::string slnRootPath = r;
	std::string slnPath = s;
    bool const shouldClean = cmdParser.get<bool>("clean");
    bool const shouldRebuild = cmdParser.get<bool>("rebuild");
#else
    std::string slnRootPath = "E:/EngineProject/SolarEngine/Src";
    std::string slnPath = "E:/EngineProject/SolarEngine/Src/BuildTool/Precompile/precompilefile.json";
    bool const shouldClean = cmdParser.get<bool>("clean");
    bool const shouldRebuild = cmdParser.get<bool>("rebuild");
#endif

    // Execute reflector
    //-------------------------------------------------------------------------
	int exitCode = 1;

    SE::BuildTool::FileSystem::NormalizePath(slnRootPath);
    SE::BuildTool::FileSystem::NormalizePath(slnPath);

    // Parse solution
    SE::BuildTool::Reflector reflector;
    if (reflector.ParseSolution(slnRootPath, slnPath))
    {
        if (shouldRebuild)
        {
            reflector.Clean();
			exitCode = reflector.Build() ? 0 : 1;
        }
        else if (shouldClean)
        {
			exitCode = reflector.Clean() ? 0 : 1;
        }
        else
        {
			exitCode = reflector.Build() ? 0 : 1;
        }
    }

    return exitCode;
}

#include "Reflector.h"
#include "ReflectorSettingsAndUtils.h"
#include "Clang/ClangParser.h"
#include "CodeGenerators/CodeGenerator_CPP.h"
#include "Core/Serialization/Json.h"
#include "ThirdParty/cmdParser/cmdParser.h"

#include "Core/Platform/File.h"
#include "Core/Utilities/Timers.h"
#include "Core/Types/Collections/Sorting.h"
#include "Core/Types/Strings/StringView.h"
#include "Core/Types/DateTime.h"

#include "TopologicalSort.h"
#include <fstream>
#include <iostream>
#include <filesystem>

#include "Core/CoreModule.h"
#include "Core/Logging/LoggingSystem.h"

//-------------------------------------------------------------------------

namespace SE::ReflectTool
{
    namespace
    {
        bool SortProjectsByDependencies(List<ProjectInfo> &projects)
        {
            int const numProjects = projects.Count();
            if (numProjects <= 1)
            {
                return true;
            }

            // Create list to sort
			List<TopologicalSorter::Node> list;
            for (auto p = 0; p < numProjects; p++)
            {
                list.Add(TopologicalSorter::Node(p));
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
                            list[p].m_children.Add(&list[pp]);
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
            List<ProjectInfo> sortedProjects;
            sortedProjects.SetCapacity(numProjects);

            for (auto &node : list)
            {
                sortedProjects.Add(projects[node.m_ID]);
                sortedProjects.Last().dependencyCount = depValue++;
            }
            projects.Swap(sortedProjects);
            return true;
        }
    }

    bool Reflector::ParseSolution(String slnRootPath, String &slnPath)
    {
        if (slnPath.IsEmpty() || !FileSystem::MatchesExtension(slnPath, SE_TEXT("json")))
        {
            return LogError("Invalid solution file name: {0}", slnPath);
        }

        if (!FileSystem::FileExists(slnPath))
        {
            return LogError("Solution doesnt exist: {0}", slnPath);
        }

        //-------------------------------------------------------------------------

        std::cout << " * Parsing Solution: " << slnPath.ToStringAnsi().Get() << std::endl;
        std::cout << " ----------------------------------------------" << std::endl
                  << std::endl;

		Milliseconds parsingTime = 0.0f;
        {
            ScopedTimer<PlatformClock> timer(parsingTime);


			StringAnsi jsonText;
            if (!File::ReadAllText(slnPath, jsonText))
            {
                return LogError("Could not open solution: {0}", slnPath);
            }

			Json::Document jsonReader;
			jsonReader.ParseInsitu(jsonText.Get());

            std::ifstream slnFile(slnPath.Get());
            if (!slnFile.is_open())
            {
                return LogError("Could not open solution: {0}", slnPath);
            }

            m_solution.path = slnRootPath;
			String currentProcessDirectory = FileSystem::GetCurrentProcessDirectory();
            m_reflectionDataPath = currentProcessDirectory + Settings::g_temporaryDirectoryPath;

            auto documentArray = jsonReader.GetArray();
            
            std::vector<ProjectObject> projectObjects;

            for (size_t i = 0; i < documentArray.Size(); i++)
            {
                auto &arrayValue = documentArray[i];
                auto targetName = arrayValue.FindMember("TargetName");
                auto targetDir = arrayValue.FindMember("TargetDir");
				String targetInclude(arrayValue.FindMember("TargetInclude")->value.GetString());

                String projectPathString(targetDir->value.GetString());
				String projectName(targetName->value.GetString());
				FileSystem::NormalizePath(projectPathString);

				String projectPath = projectPathString; //m_solution.m_path + projectPathString;


                // Filter projects
                //-------------------------------------------------------------------------

                bool excludeProject = true;

                // Ensure projects are within the allowed layers
                for (auto i = 0; i < Settings::g_numAllowedProjects; i++)
                {
                    if (projectName.Find(Settings::g_allowedProjectNames[i]) != INVALID_INDEX)
                    {
                        excludeProject = false;
                        break;
                    }
                }

                // 额外的工程
                if (excludeProject)
                {
                    m_solution.excludedProjects.Add(projectPath);
                }
                else
                {
                    ProjectObject projectObject;
                    projectObject.name = projectName;
                    projectObject.path = projectPathString;
                    projectObject.includeFile = targetInclude;

                    projectObjects.emplace_back(projectObject);
                }

            }

            /*
            m_solution.m_path = slnPath.GetParentDirectory();
            m_reflectionDataPath = FileSystem::Path(FileSystem::GetCurrentProcessPath() + Settings::g_temporaryDirectoryPath);

            Vector<FileSystem::Path> projectFiles;

            std::string stdLine;
            while (std::getline(slnFile, stdLine))
            {
                String line(stdLine.c_str());

                if (line.find("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") != String::npos) // VS Project UID
                {
                    auto projectNameStartIdx = line.find(" = \"");
                    ENGINE_ASSERT(projectNameStartIdx != std::string::npos);
                    projectNameStartIdx += 4;
                    auto projectNameEndIdx = line.find("\", \"", projectNameStartIdx);
                    ENGINE_ASSERT(projectNameEndIdx != std::string::npos);

                    auto projectPathStartIdx = projectNameEndIdx + 4;
                    ENGINE_ASSERT(projectPathStartIdx < line.length());
                    auto projectPathEndIdx = line.find("\"", projectPathStartIdx);
                    ENGINE_ASSERT(projectNameEndIdx != std::string::npos);

                    String const projectPathString = line.substr(projectPathStartIdx, projectPathEndIdx - projectPathStartIdx);
                    FileSystem::Path const projectPath = m_solution.m_path + projectPathString;

                    // Filter projects
                    //-------------------------------------------------------------------------

                    bool excludeProject = true;

                    // Ensure projects are within the allowed layers
                    for (auto i = 0; i < Settings::g_numAllowedProjects; i++)
                    {
                        if (line.find(Settings::g_allowedProjectNames[i]) != String::npos)
                        {
                            excludeProject = false;
                            break;
                        }
                    }

                    // 额外的工程
                    if (excludeProject)
                    {
                        m_solution.m_excludedProjects.emplace_back(projectPath);
                    }
                    else
                    {
                        projectFiles.push_back(projectPath);
                    }
                }
            }*/

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

	DateTime Reflector::CalculateHeaderChecksum(String &engineIncludePath, String &filePath)
    {
        static char const *headerString = "Note: including file: ";
		DateTime checksum = FileSystem::GetFileLastEditTime(filePath);
        return checksum;
    }

    bool Reflector::ParseProject(ProjectObject &prjObject)
    {
        ENGINE_ASSERT(FileSystem::IsUnderDirectory(prjObject.path, m_solution.path));

        ProjectInfo prj;
        prj.name = prjObject.name;
        prj.id = ProjectInfo::GetProjectID(prjObject.path);
		String projectPath = prjObject.path;
		prj.parentPath = FileSystem::GetParentDirectory(projectPath);
        prj.path = prjObject.path;
        prj.isToolsModule = Utils::IsFileUnderToolsProject(prjObject.path);


        List<String> sourceFiles;
		prjObject.includeFile.Split(SE_TEXT(';'), sourceFiles);
        
        for (String sourceFile : sourceFiles)
        {
            // Ignore auto-generated files
            if (sourceFile.Find(Settings::g_autogeneratedDirectory) != INVALID_INDEX)
            {
                continue;
            }

			String const headerFilePath(prj.parentPath + SE_TEXT("/") + sourceFile);
            String headerFileFullPath = prj.parentPath + SE_TEXT("/") + sourceFile;

            List<StringAnsi> headerFileContents;
            HeaderProcessResult const result = ProcessHeaderFile(headerFileFullPath, prj.exportMacro, headerFileContents);
            switch (result)
            {
                case HeaderProcessResult::ParseHeader:
                {
                    HeaderInfo &headerInfo = prj.headerFiles.emplace_back();
                    headerInfo.headerId = HeaderInfo::GetHeaderID(headerFileFullPath);
                    headerInfo.projectID = prj.id;
                    headerInfo.filePath = headerFileFullPath.ToStringAnsi();
                    headerInfo.timestamp = FileSystem::GetFileLastEditTime(headerFileFullPath).Ticks;
                    headerInfo.fileContents.Swap(headerFileContents);

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
					std::cout << "Error processing: " << headerFileFullPath.ToStringAnsi().Get() << std::endl;
					//prjFile.close();
					return false;
				}
            }
        }
        
        //-------------------------------------------------------------------------

        if (prj.name.IsEmpty())
        {
            return LogError("Invalid project file detected: {0}", prjObject.path);
        }

        // Print parse results
        //-------------------------------------------------------------------------

        std::cout << " * Project: " << prj.name.ToStringAnsi().Get() << " - ";

        // Only add projects to the list that have headers to parse
        if (prj.headerFiles.empty())
        {
            std::cout << "Ignored! ( No headers found! )" << std::endl;
            return true;
        }

        std::cout << "Done! ( " << prj.headerFiles.size() << " header(s) found! )";
        std::cout << std::endl;

		m_solution.projects.Add(prj);
        return true;
    }

    Reflector::HeaderProcessResult Reflector::ProcessHeaderFile(String &filePath, String &exportMacroName, List<StringAnsi> &headerFileContents)
    {
        // Open header file
        bool const isModuleAPIHeader = filePath.Contains(SE_TEXT("API.h"));
        std::ifstream hdrFile(filePath.Get(), std::ios::in | std::ios::binary | std::ios::ate);
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
        headerFileContents.Clear();
        std::string stdLine;
        while (std::getline(hdrFile, stdLine))
        {
            headerFileContents.Add(stdLine.c_str());
        }
        hdrFile.close();

        //-------------------------------------------------------------------------

        // Check for the SE registration macros
        bool exportMacroFound = false;
        uint32_t openCommentBlock = 0;

        for (auto const &line : headerFileContents)
        {
            // Check for comment blocks
            if (line.Find("/*") != INVALID_INDEX)
                openCommentBlock++;
            if (line.Find("*/") != INVALID_INDEX)
                openCommentBlock--;

            if (openCommentBlock == 0)
            {
                // Check for line comment
                auto const foundCommentIdx = line.Find("//");

                // Check for registration macros
                for (auto i = 0u; i < (uint32_t)ReflectionMacroType::NumMacros; i++)
                {
                    ReflectionMacroType const macro = (ReflectionMacroType)i;
                    auto const foundMacroIdx = line.Find(StringAnsi(GetReflectionMacroText(macro)));
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
                    auto const foundExportIdx0 = line.Find("__declspec");
                    auto const foundExportIdx1 = line.Find("dllexport");
                    if (foundExportIdx0 != INVALID_INDEX && foundExportIdx1 != INVALID_INDEX)
                    {
                        if (!exportMacroName.IsEmpty())
                        {
                            LogError("Duplicate export macro definitions found!", filePath);
                            return HeaderProcessResult::ErrorOccured;
                        }
                        else
                        {
                            auto defineIdx = line.Find("#define");
                            if (defineIdx != INVALID_INDEX)
                            {
                                defineIdx += 8;
                                exportMacroName = line.Substring(defineIdx, foundExportIdx0 - 1 - defineIdx).ToString();
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
        std::cout << " * Cleaning Solution: " << m_solution.path.ToStringAnsi().Get() << std::endl;
        std::cout << " ----------------------------------------------" << std::endl
                  << std::endl;

        StringBuilder autoGeneratedDirectory;
        // Delete all auto-generated directories for valid projects
        for (auto &prj : m_solution.projects)
        {
            std::cout << " * Cleaning Project - " << prj.name.ToStringAnsi().Get() << std::endl;

            autoGeneratedDirectory.Clear();
			autoGeneratedDirectory.Append(prj.path);
            autoGeneratedDirectory.Append("/");
			autoGeneratedDirectory.Append(Settings::g_autogeneratedDirectory);
            FileSystem::DeleteDirectory(autoGeneratedDirectory.ToString());


            autoGeneratedDirectory.Clear();
            autoGeneratedDirectory.Append(prj.path);
            autoGeneratedDirectory.Append("/");
            autoGeneratedDirectory.Append(Settings::g_autogeneratedModuleFileSuffix);
            FileSystem::DeleteFile(autoGeneratedDirectory.ToString());
            File::WriteAllText(autoGeneratedDirectory.ToString(), String::Empty, Encoding::EncodingType::UTF8);
        }

        std::cout << std::endl;

        // Delete all auto-generated directories for excluded projects (just to be safe)
        for (auto &prjPath : m_solution.excludedProjects)
        {
            String autoGeneratedDirectory = FileSystem::GetParentDirectory(prjPath);
			autoGeneratedDirectory += Settings::g_autogeneratedDirectory;
            std::cout << " * Cleaning Excluded Project - " << autoGeneratedDirectory.ToStringAnsi().Get() << std::endl;

            if (!FileSystem::DeleteDirectory(autoGeneratedDirectory))
            {
                return LogError(" * Error erasing directory: {0}", autoGeneratedDirectory);
            }
        }

        std::cout << std::endl;

		String const databasePath(m_reflectionDataPath + SE_TEXT("TypeDatabase.db"));
        bool const result = FileSystem::FileExists(databasePath) || FileSystem::DeleteFile(databasePath);
        if (result)
        {
            std::cout << " * Deleted: " << databasePath.ToStringAnsi().Get() << std::endl;
        }
        else
        {
            std::cout << " * Error deleting: " << databasePath.ToStringAnsi().Get() << std::endl;
        }

        std::cout << std::endl;
        std::cout << " >>> Cleaning - Complete!" << std::endl
                  << std::endl;
        return result;
    }

    bool Reflector::Build()
    {
        std::cout << " * Reflecting Solution: " << m_solution.path.ToStringAnsi().Get() << std::endl;
        std::cout << " ----------------------------------------------" << std::endl
                  << std::endl;

        Milliseconds time = 0;
        {
            ScopedTimer<PlatformClock> timer(time);

            // Open connection to type database and read all previous data
			StringAnsi databasePath(m_reflectionDataPath + SE_TEXT("TypeDatabase.db"));
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
            List<HeaderID> registeredHeaders;
            for (auto &prj : m_solution.projects)
            {
                String autoGeneratedDirectory = prj.path + Settings::g_autogeneratedDirectory;
				List<String> existingFiles;
                FileSystem::DirectoryGetFiles(existingFiles, autoGeneratedDirectory, nullptr, DirectorySearchOption::All);

                for (auto i = 0u; i < prj.headerFiles.size(); i++)
                {
                    bool isDirty = false;
                    auto &header = prj.headerFiles[i];
                    registeredHeaders.Add(header.headerId);

                    // Does the output file exist?
                    String const autoGeneratedFilePath = header.GetAutogeneratedTypeInfoFileName(autoGeneratedDirectory);
                    isDirty = !FileSystem::FileExists(autoGeneratedFilePath);

                    // Remove this file from the from existing files list - this is used to clean up the auto-generated directory of old files
                    for (auto j = 0u; j < existingFiles.Count(); j++)
                    {
                        if (FileSystem::GetFileNameWithoutExtension(existingFiles[j]) == FileSystem::GetFileNameWithoutExtension(autoGeneratedFilePath))
                        {
                            if (j != existingFiles.Count() - 1)
                            {
                                existingFiles[j] = existingFiles.Last();
                            }

                            existingFiles.Pop();
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
                                String filePath = header.filePath.ToString();
                                header.checksum = CalculateHeaderChecksum(m_solution.path, filePath).Ticks;
                                if (header.checksum == 0)
                                {
                                    return LogError("Failed to perform up to date check for: {0}", header.filePath.Get());
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
        List<HeaderInfo *> headersToParse;
        for (auto &prj : m_solution.projects)
        {
            if (!prj.dirtyHeaders.empty())
            {
                // emplace_back all dirty headers to the list of file to be parsed
                for (auto &hdr : prj.dirtyHeaders)
                {
                    headersToParse.Add(&prj.headerFiles[hdr]);

                    // Erase all types associated with this header from the database
                    m_database.DeleteTypesForHeader(prj.headerFiles[hdr].headerId);
                }
            }
        }

        //-------------------------------------------------------------------------

        ClangParser clangParser(&m_solution, &m_database, m_reflectionDataPath);

        if (!headersToParse.IsEmpty())
        {
            std::cout << " * Reflecting C++ Code -" << std::endl;;
            if (!clangParser.Parse(headersToParse))
            {
                std::cout << "Error occurred!\n\n  Error: " << clangParser.GetErrorMessage().Get() << std::endl;
                return false;
            }
            Milliseconds clangParsingTime = clangParser.GetParsingTime();
            Milliseconds clangVisitingTime = clangParser.GetVisitingTime();
            std::cout << "Complete! ( P:" << clangParsingTime << "ms, V:" << clangVisitingTime << "ms )" << std::endl;

            // Finalize database data
            m_database.UpdateProjectList(m_solution.projects);
        }

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
            if (!m_database.WriteDatabase(m_reflectionDataPath.ToStringAnsi() + "TypeDatabase.db"))
            {
                std::cout << "Error Occurred: " << generator.GetErrorMessage() << std::endl;
                return LogError(m_database.GetError().Get());
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
    std::cout << "|             SE Reflector             |" << std::endl;
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
	SE::String slnRootPath = SE::String(r.c_str());
	SE::String slnPath = SE::String(s.c_str());
    bool const shouldClean = cmdParser.get<bool>("clean");
    bool const shouldRebuild = cmdParser.get<bool>("rebuild");
#else
    SE::String slnRootPath = SE_TEXT("E:/EngineProject/SolarEngine/Src");
    SE::String slnPath = SE_TEXT("E:/EngineProject/SolarEngine/Src/Reflector/Precompile/precompilefile.json");
    bool const shouldClean = cmdParser.get<bool>("clean");
    bool const shouldRebuild = cmdParser.get<bool>("rebuild");
#endif

    // Execute reflector
    //-------------------------------------------------------------------------
    SE::CoreTypeRegistry::Initialize();
    SE::Log::System::Initialize();

	int exitCode = 1;

	SE::FileSystem::NormalizePath(slnRootPath);
	SE::FileSystem::NormalizePath(slnPath);

    // Parse solution
    SE::ReflectTool::Reflector reflector;
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

    SE::Log::System::Shutdown();
    SE::CoreTypeRegistry::Shutdown();

    return exitCode;
}
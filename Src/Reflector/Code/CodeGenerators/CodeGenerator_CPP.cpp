#include "CodeGenerator_CPP.h"
#include "CodeGenerator_CPP_Type.h"
#include "CodeGenerator_CPP_Enum.h"
#include "../ReflectorSettingsAndUtils.h"
#include "Core/TypeSystem/TypeID.h"
#include "Core/Types/Collections/Sorting.h"
#include "Core/Platform/File.h"

#include "../TopologicalSort.h"
#include <fstream>
#include <string>

#include "CodeGenerator_CPP_Meta.h"

//-------------------------------------------------------------------------

namespace SE::ReflectTool
{
    enum class TypeRegistrationHeaderType
    {
        Engine,
        Tools
    };

    static bool LoadTemplateFileString(Generator &generator, StringAnsi path, StringAnsi &code)
    {
        std::ifstream hdrFile(path.Get(), std::ios::in | std::ios::ate);
        if (!hdrFile.is_open())
        {
            return generator.LogError("Could not open Code Template file: {0}", path);
        }

        // Check file Count
        uint32_t const Count = (uint32_t)hdrFile.tellg();
        if (Count == 0)
        {
            hdrFile.close();
        }
        hdrFile.seekg(0, std::ios::beg);

        // Read file contents
        code.Clear();
  
        std::string stdLine;
        while (getline(hdrFile, stdLine))
        {
            code.Append((stdLine + "\n").c_str());
        }
        hdrFile.close();

        return true;
    }

    static bool SortTypesByDependencies( List<ReflectedType>& structureTypes )
    {
        int32 const numTypes = (int32_t) structureTypes.Count();
        if ( numTypes <= 1 )
        {
            return true;
        }

        // Create list to sort
        List<TopologicalSorter::Node > list;
        for ( auto i = 0; i < numTypes; i++ )
        {
            list.Add( TopologicalSorter::Node( i ) );
        }

        for ( auto i = 0; i < numTypes; i++ )
        {
            for ( auto j = 0; j < numTypes; j++ )
            {
                if ( i != j && structureTypes[j].typeID == structureTypes[i].parentTypeID )
                {
                    list[i].m_children.Add( &list[j] );
                }
            }
        }

        // Try to sort
        if ( !TopologicalSorter::Sort( list ) )
        {
            return false;
        }

        // Update type list
        List<ReflectedType> sortedTypes;
        sortedTypes.SetCapacity( numTypes );

        for ( auto& node : list )
        {
            sortedTypes.Add( structureTypes[node.m_ID] );
        }
        structureTypes.Swap( sortedTypes );

        return true;
    }

    static bool GenerateTypeRegistrationHeaderFiles( ReflectionDatabase const& database, SolutionInfo const& solution, 
                    std::stringstream& typeRegistrationStr, TypeRegistrationHeaderType headerType, StringAnsi templateStr)
    {
        mustache::data generateData;

        // Get all modules from database
        std::vector<ProjectInfo> modules = database.GetAllRegisteredProjects();
//		std::vector<ReflectedResourceType> const& registeredResourceTypes = database.GetAllRegisteredResourceTypes();

        // Module includes
        //-------------------------------------------------------------------------
        mustache::data moduleIncludeseListData(mustache::data::type::list);
        for ( auto i = 0; i < modules.size(); i++ )
        {
            // Ignore tools modules for engine headers
            if ( headerType == TypeRegistrationHeaderType::Engine )
            {
                if ( modules[i].isToolsModule )
                {
                    modules.erase(modules.begin() + i );
                    i--;
                    continue;
                }
            }

            // Ignore module-less modules
            if (modules[i].moduleHeaderID == HeaderID::Invalid )
            {
                modules.erase(modules.begin() + i);
                i--;
                continue;
            }


            // Output module include
            HeaderInfo const* pHeader = database.GetHeaderDesc( modules[i].moduleHeaderID );
            if ( pHeader != nullptr )
            {
                mustache::data moduleIncludeseData;
                moduleIncludeseData.set("moduleInclude", pHeader->filePath.Get());
                moduleIncludeseListData.push_back(moduleIncludeseData);
            }
            else
            {
                return false;
            }
        }
        generateData.set("moduleIncludeList", moduleIncludeseListData);

        // Sort modules according to original project dependency order
        //-------------------------------------------------------------------------

        std::sort(modules.begin(), modules.end(), [] ( ProjectInfo const& pA, ProjectInfo const& pB )
		{
		  return pA.dependencyCount < pB.dependencyCount;
		} );


        if ( headerType == TypeRegistrationHeaderType::Engine )
        {
            generateData.set("headerType", "Engine");
        }
        else
        {
            generateData.set("headerType", "Tools");
        }

        mustache::data moduleClassListData(mustache::data::type::list);
        for ( auto& module : modules )
        {
            mustache::data moduleClass;
            moduleClass.set("moduleClassName", module.moduleClassNameFull.Get());
            moduleClassListData.push_back(moduleClass);
        }

/*        auto GetResourceTypeIDForTypeID = [&registeredResourceTypes] ( TypeID typeID )
        {
            for ( auto const& registeredResourceType : registeredResourceTypes )
            {
                if ( registeredResourceType.m_typeID == typeID )
                {
                    return registeredResourceType.m_resourceTypeID;
                }
            }

            ENGINE_UNREACHABLE_CODE();
            return ResTypeID();
        };*/

        mustache::data registeredResourceTypeListData(mustache::data::type::list);

        /*for ( auto& registeredResourceType : registeredResourceTypes )
        {
            mustache::data registeredResourceTypeData;
            registeredResourceTypeData.set("typeID", registeredResourceType.m_typeID.ToString().Get());
            registeredResourceTypeData.set("resourceTypeID", registeredResourceType.m_resourceTypeID.ToString().Get());

            mustache::data registeredResourceParentTypeList;
            for ( auto const& parentType : registeredResourceType.m_parents)
            {
				ResTypeID const resourceTypeID = GetResourceTypeIDForTypeID( parentType );

                mustache::data registeredResourceParentType;
                registeredResourceParentType.set("resourceParentTypeID", resourceTypeID.ToString().Get());
                registeredResourceParentTypeList.push_back(registeredResourceParentType);
            }

            registeredResourceTypeData.set("registeredResourceParentTypes", registeredResourceParentTypeList);
            registeredResourceTypeData.set("friendlyName", registeredResourceType.m_friendlyName.Get());

            registeredResourceTypeListData.push_back(registeredResourceTypeData);
        }*/

        generateData.set("registeredResourceTypes", registeredResourceTypeListData);

        mustache::mustache tmpl(templateStr.Get());
        typeRegistrationStr.str( std::string() );
        typeRegistrationStr.clear();
        typeRegistrationStr << tmpl.render(generateData);

        return true;
    }

    //-------------------------------------------------------------------------

    bool Generator::SaveStreamToFile(String const& filePath, std::stringstream& stream)
    {
        bool fileContentsEqual = true;

        // Rewind stream to beginning
        stream.seekg(std::ios::beg);

        // Open existing file and compare contents to the newly generated stream
        std::ifstream fileStream(filePath.Get(), std::ios::in);
        if (fileStream.is_open())
        {
            std::string lineNew, lineOld;
            while (getline(fileStream, lineOld) && fileContentsEqual)
            {
                if ( !getline(stream, lineNew) || (lineOld != lineNew))
                {
                    fileContentsEqual = false;
                }
            }

            // Set different if the stream is longer than the file
            if ( fileContentsEqual && getline( stream, lineNew ) )
            {
                fileContentsEqual = false;
            }

            fileStream.close();
        }
        else
        {
            // std::cout << "Error opening existing file: " << strerror( errno );
            fileContentsEqual = false;
        }

        // If the contents differ overwrite the existing file
        if (!fileContentsEqual)
        {
			stream.seekg(std::ios::beg);
			File ::WriteAllText(filePath, String(stream.str().c_str()), Encoding::EncodingType::UTF8);
        }

        return true;
    }

    void Generator::LoadTemplateFile(SolutionInfo const &solution)
    {
        String rootPath = solution.path;

        StringAnsi codeModuleTemplateFilePath(StringAnsi::Format("{0}/Reflector/Code/Template/CodeModuleTemplate.mustache", rootPath));

        StringAnsi codeCppMetaTemplateFilePath(StringAnsi::Format("{0}/Reflector/Code/Template/CodeCppMetaTemplate.mustache", rootPath));
		StringAnsi codeCppEnumTemplateFilePath(StringAnsi::Format("{0}/Reflector/Code/Template/CodeCppEnumTemplate.mustache", rootPath));
		StringAnsi codeCppClassTemplateFilePath(StringAnsi::Format("{0}/Reflector/Code/Template/CodeCppClassTemplate.mustache", rootPath));
		StringAnsi codeTypeRegistrationFilePath(StringAnsi::Format("{0}/Reflector/Code/Template/CodeTypeRegistrationTemplate.mustache", rootPath));
            

        if (m_CodeModuleTemplate.IsEmpty())
        {
            LoadTemplateFileString(*this, codeModuleTemplateFilePath, m_CodeModuleTemplate);
        }
        if (m_CodeCppMetaTemplate.IsEmpty())
        {
            LoadTemplateFileString(*this, codeCppMetaTemplateFilePath, m_CodeCppMetaTemplate);
        }
        if (m_CodeCppEnumTemplate.IsEmpty())
        {
            LoadTemplateFileString(*this, codeCppEnumTemplateFilePath, m_CodeCppEnumTemplate);
        }
        if (m_CodeCppClassTemplate.IsEmpty())
        {
            LoadTemplateFileString(*this, codeCppClassTemplateFilePath, m_CodeCppClassTemplate);
        }

        if (m_CodeTypeRegistrationTemplate.IsEmpty())
        {
            LoadTemplateFileString(*this, codeTypeRegistrationFilePath, m_CodeTypeRegistrationTemplate);
        }
    }

    void Generator::GenerateTypeInfoFileHeader(HeaderInfo const &hdr, StringView solutionPath)
    {
        m_typeInfoFile.str(std::string());
        m_typeInfoFile.clear();
        m_typeInfoFile << "#pragma once" << std::endl;
        m_typeInfoFile << "//*************************************************************************\n";
        m_typeInfoFile << "// This is an auto-generated file - DO NOT edit"  << std::endl;
        m_typeInfoFile << "//*************************************************************************\n";
        m_typeInfoFile << "#include \"" << std::string(hdr.filePath.Get()) << "\"" << std::endl;
    }

    void Generator::GenerateModuleCodeFile(ReflectionDatabase const& database, ProjectInfo const& prj, List<ReflectedType> const& typesInModule)
    {
        mustache::data generateData;
 
        //-------------------------------------------------------------------------
        // Header
		StringAnsi moduleHeaderFilePath = prj.GetModuleHeaderDesc().filePath;
        generateData.set("moduleHeaderFile", std::string(moduleHeaderFilePath.Get()));
        //-------------------------------------------------------------------------
        // Includes

        List<String> autoGeneratedFiles;
		String autoGeneratedDirectory = String(prj.path + SE_TEXT("/") + Settings::g_autogeneratedDirectory);
		FileSystem::NormalizePath(autoGeneratedDirectory);
		FileSystem::DirectoryGetFiles(autoGeneratedFiles, autoGeneratedDirectory, SE_TEXT("*.h"), DirectorySearchOption::TopOnly);

        mustache::data includeFileListData(mustache::data::type::list);

        for (auto& file : autoGeneratedFiles)
        {
            mustache::data includeFile;
			includeFile.set("includeFile", std::string(file.ToStringAnsi().Get()));
			includeFileListData.push_back(includeFile);
        }

        generateData.set("includeFileList", includeFileListData);

        //-------------------------------------------------------------------------
        // Registration functions

        generateData.set("moduleClassName", std::string(prj.moduleClassName.ToStringAnsi().Get()));

        mustache::data registrationTypeListData(mustache::data::type::list);
        mustache::data registrationEnumListData(mustache::data::type::list);
        mustache::data registrationMetaListData(mustache::data::type::list);


        for ( auto& type : typesInModule )
        {
            mustache::data registrationTypeData;
            if (type.m_isDevOnly)
            {
                registrationTypeData.set("isDevOnlyBegin", "#ifdef SGE_DEVELOPMENT");
                registrationTypeData.set("isDevOnlyEnd", "#endif");
            }
            registrationTypeData.set("registerNamespace", std::string(type.namespaceName.Get()));
            registrationTypeData.set("registerName", std::string(type.name.Get()));

            if (type.IsEnum())
            {
                registrationEnumListData.push_back(registrationTypeData);
            }
            else if (type.IsMeta())
            {
                registrationMetaListData.push_back(registrationTypeData);
            }
            else
            {
                registrationTypeListData.push_back(registrationTypeData);
            }
        }

        generateData.set("compositeTypeList", registrationTypeListData);
        generateData.set("enumTypeList", registrationEnumListData);
        generateData.set("metaTypeList", registrationMetaListData);

        //-------------------------------------------------------------------------
        // generate

        mustache::mustache tmpl(m_CodeModuleTemplate.Get());
        m_moduleFile.str(std::string());
        m_moduleFile.clear();
        m_moduleFile.flush();
        m_moduleFile << tmpl.render(generateData);
    }

    bool Generator::Generate(ReflectionDatabase const& database, SolutionInfo const& solution)
    {
        LoadTemplateFile(solution);

        m_pDatabase = &database;
        for ( auto& prj : solution.projects)
        {
            // Ignore module less projects
            if (prj.moduleHeaderID == HeaderID::Invalid )
            {
                continue;
            }
            

            // Ensure the auto generated directory exists
			String autoGeneratedDirectory = ( prj.path + SE_TEXT("/") + Settings::g_autogeneratedDirectory );
			String autoGeneratedModuleFile = ( prj.path + SE_TEXT("/") + Settings::g_moduleHeaderParentDirectoryName );
			FileSystem::NormalizePath(autoGeneratedDirectory);
			FileSystem::NormalizePath(autoGeneratedModuleFile);
//            autoGeneratedDirectory.EnsureDirectoryExists();

			if (!FileSystem::DirectoryExists(autoGeneratedDirectory))
			{
				FileSystem::CreateDirectory(autoGeneratedDirectory);
			}

            // Generate list of all expected header files in the auto generated directory
            List<String> expectedFiles;
            //expectedFiles.Add( FileSystem::Path( autoGeneratedDirectory + Reflection::Settings::g_autogeneratedModuleFile ) );

            for ( auto const& headerInfo : prj.headerFiles )
            {
                expectedFiles.Add(headerInfo.GetAutogeneratedTypeInfoFileName(autoGeneratedDirectory));
            }

            // Delete any unknown files from the auto generated directory
            List<String> files;
			FileSystem::DirectoryGetFiles(files, autoGeneratedDirectory, nullptr, DirectorySearchOption::TopOnly);

            for (auto const& file : files)
            {
                if (!expectedFiles.Contains(file))
                {
					FileSystem::DeleteFile(file);
                }
            }

            // Generate code files for the dirty headers
            for ( auto& dirtyHeaderIdx : prj.dirtyHeaders )
            {
                auto& headerInfo = prj.headerFiles[dirtyHeaderIdx];

                if (headerInfo.headerId == prj.moduleHeaderID)
                {
                    continue;
                }

				String const typeInfoFilename = headerInfo.GetAutogeneratedTypeInfoFileName(autoGeneratedDirectory);

                // Generate files
                GenerateTypeInfoFileHeader(headerInfo, solution.path);

                // Get all types for the header
                List<ReflectedType> typesInHeader;
                m_pDatabase->GetAllTypesForHeader( headerInfo.headerId, typesInHeader );

                for ( auto& type : typesInHeader )
                {
                    // Generate enum info
                    if (type.IsEnum())
                    {
                        CppGenerateEnum(this, m_typeInfoFile, prj.exportMacro, type, m_CodeCppEnumTemplate.Get());
                    }
                    // Generate meta info
                    else if (type.IsMeta())
                    {
                        CppGenerateMeta(this, database, m_typeInfoFile, type, m_CodeCppMetaTemplate.Get());
                    }
                    else // Generate type info
                    {
                        if (type.parentTypeID == StringID::Invalid )
                        {
							StringAnsi const fullTypeName = type.namespaceName + type.name;
                            return LogError( "Invalid parent hierarchy for type {0}, all registered types must derived from a registered type.", fullTypeName);
                        }

                        auto pTypeDesc = m_pDatabase->GetType( type.parentTypeID );
                        ENGINE_ASSERT( pTypeDesc != nullptr );

                        CppGenerateType(this, database, m_typeInfoFile, prj.exportMacro, type, *pTypeDesc, m_CodeCppClassTemplate.Get());
                    }
                }

                // Save generated file
                if (!SaveStreamToFile(typeInfoFilename, m_typeInfoFile) )
                {
                    return LogError( "Could not save typeinfo file to disk: {0}", typeInfoFilename);
                }
            }

            // Get project info from database as that will contain all necessary info like module class name
            ProjectInfo const* pProjectDesc = m_pDatabase->GetProjectDesc( prj.id );
            if ( pProjectDesc == nullptr )
            {
                return LogError("Could not retrieve description for project: {0}", prj.name);
            }
            ENGINE_ASSERT( prj.id == pProjectDesc->id );

            // Get all types in project
			List<ReflectedType> typesInProject;
            m_pDatabase->GetAllTypesForProject(pProjectDesc->id, typesInProject);
            if (!SortTypesByDependencies( typesInProject))
            {
                return LogError("Cyclic header dependency detected in project: {0}", pProjectDesc->name);
            }

            // Generate and save the module file
            GenerateModuleCodeFile(database, *pProjectDesc, typesInProject);

			String const module_cpp = autoGeneratedModuleFile + SE_TEXT("/") + pProjectDesc->moduleClassName + Settings::g_autogeneratedModuleFileSuffix;
            if (!SaveStreamToFile(module_cpp, m_moduleFile))
            {
                return LogError("Could not save module file to disk: {0}", module_cpp);
            }
        }

        // Generate and save module type registration header
        //-------------------------------------------------------------------------

        /*
        if (!GenerateTypeRegistrationHeaderFiles(database, solution, m_engineTypeRegistrationFile, TypeRegistrationHeaderType::Engine, m_CodeTypeRegistrationTemplate))
        {
            return LogError( "Could not generate engine type registration header!" );
        }

        if (!GenerateTypeRegistrationHeaderFiles(database, solution, m_toolsTypeRegistrationFile, TypeRegistrationHeaderType::Tools, m_CodeTypeRegistrationTemplate))
        {
            return LogError( "Could not generate tools type registration header!" );
        }

        //-------------------------------------------------------------------------

        FileSystem::Path const autoGeneratedPath = solution.m_path + Reflection::Settings::g_globalAutoGeneratedDirectory;
        FileSystem::Path const engineTypeRegistration_h = autoGeneratedPath + Reflection::Settings::g_engineTypeRegistrationHeaderPath;
        FileSystem::Path const toolsTypeRegistration_h = autoGeneratedPath + Reflection::Settings::g_toolsTypeRegistrationHeaderPath;

        autoGeneratedPath.EnsureDirectoryExists();
        if (!SaveStreamToFile(engineTypeRegistration_h, m_engineTypeRegistrationFile))
        {
            return LogError( "Could not save type registration header to disk: %s", engineTypeRegistration_h.c_str() );
        }

        autoGeneratedPath.EnsureDirectoryExists();
        if (!SaveStreamToFile(toolsTypeRegistration_h, m_toolsTypeRegistrationFile))
        {
            return LogError( "Could not save type registration header to disk: %s", toolsTypeRegistration_h.c_str() );
        }
        */
        return true;
    }
}
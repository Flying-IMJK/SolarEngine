#include "CodeGenerator_CPP.h"
#include "CodeGenerator_CPP_Type.h"
#include "CodeGenerator_CPP_Enum.h"
#include "CodeGenerator_BindingsCpp.h"
#include "CodeGenerator_BindingsCSharp.h"
#include "CodeGenerator_BindingsTypeMap.h"
#include "../ReflectorSettingsAndUtils.h"
#include "Core/TypeSystem/TypeID.h"
#include "Core/Types/Collections/Sorting.h"
#include "Core/Platform/File.h"
#include "Core/Utilities/Timers.h"

#include "../TopologicalSort.h"
#include <fstream>
#include <string>
#include <iostream>

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

    static bool SortTypesByDependencies( List<DataType>& structureTypes )
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
        List<DataType> sortedTypes;
        sortedTypes.SetCapacity( numTypes );

        for ( auto& node : list )
        {
            sortedTypes.Add( structureTypes[node.m_ID] );
        }
        structureTypes.Swap( sortedTypes );

        return true;
    }

    /*static bool GenerateTypeRegistrationHeaderFiles( ReflectionDatabase const& database, SolutionInfo const& solution,
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
        };#1#

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
        }#1#

        generateData.set("registeredResourceTypes", registeredResourceTypeListData);

        mustache::mustache tmpl(templateStr.Get());
        typeRegistrationStr.str( std::string() );
        typeRegistrationStr.clear();
        typeRegistrationStr << tmpl.render(generateData);

        return true;
    }*/

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
            if (fileContentsEqual && getline( stream, lineNew ) )
            {
                fileContentsEqual = false;
            }

            fileStream.close();
        }
        else
        {
            fileContentsEqual = false;
        }

        // If the contents differ overwrite the existing file
        if (!fileContentsEqual)
        {
			stream.seekg(std::ios::beg);
			File::WriteAllText(filePath, String(stream.str().c_str()), Encoding::EncodingType::UTF8);
        }

        return true;
    }

    void Generator::LoadTemplateFile(SolutionInfo const &solution)
    {
        String rootPath = solution.path;

        StringAnsi codeModuleTemplateFilePath(StringAnsi::Format("{0}/BuildTool/Code/Template/CodeModuleTemplate.mustache", rootPath));

        StringAnsi codeCppMetaTemplateFilePath(StringAnsi::Format("{0}/BuildTool/Code/Template/CodeCppMetaTemplate.mustache", rootPath));
		StringAnsi codeCppEnumTemplateFilePath(StringAnsi::Format("{0}/BuildTool/Code/Template/CodeCppEnumTemplate.mustache", rootPath));
		StringAnsi codeCppClassTemplateFilePath(StringAnsi::Format("{0}/BuildTool/Code/Template/CodeCppClassTemplate.mustache", rootPath));
            

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
    }

    void Generator::GenerateTypeInfoFileHeader(HeaderInfo const &hdr, StringView solutionPath)
    {
        m_typeInfoFile.str(std::string());
        m_typeInfoFile.clear();
        m_typeInfoFile << "#pragma once" << std::endl;
        m_typeInfoFile << "//*************************************************************************\n";
        m_typeInfoFile << "// This is an auto-generated file - DO NOT edit\n";
        m_typeInfoFile << "//*************************************************************************\n";
        m_typeInfoFile << "#include \"" << std::string(hdr.filePath.Get()) << "\"\n";
        m_typeInfoFileHasBinding = false;
    }

    void Generator::AppendBindingIncludesIfNeeded()
    {
        if (!m_typeInfoFileHasBinding)
        {
            return;
        }
        m_typeInfoFile << "#include \"Runtime/Scripting/ManagedCLR/CLRUtils.h\"\n";
        m_typeInfoFile << "#include \"Runtime/Scripting/ScriptingObject.h\"\n";
        m_typeInfoFile << "#include \"Runtime/Scripting/Internal/InternalCalls.h\"\n";
        m_typeInfoFile << "#include \"Runtime/Scripting/ScriptingType.h\"\n";
    }

    // -------------------------------------------------------------------------
    // Helper: Create ApiClass from ReflectedType::bindingInfo
    // -------------------------------------------------------------------------

    static ApiClass MakeApiClassFromReflectedType(DataType const& type)
    {
        ApiClass cls;
        cls.name              = type.name;
        cls.namespaceName     = type.namespaceName;
        cls.baseClassName     = type.bindingInfo.baseClassName;
        cls.isAbstract        = type.IsAbstract();
        cls.isStruct           = type.IsStruct();
        cls.isScriptingObject = type.bindingInfo.isScriptingObject;
        cls.functions          = type.bindingInfo.functions;
        cls.properties        = type.bindingInfo.bindingProperties;
        cls.fields             = type.bindingInfo.fields;
        cls.interfaces         = type.bindingInfo.interfaces;
        cls.events             = type.bindingInfo.events;
        cls.isSealed           = type.bindingInfo.isSealed;
        cls.isStatic           = type.bindingInfo.isStatic;
        cls.noSpawn            = type.bindingInfo.noSpawn;
        cls.isAbstract         = type.bindingInfo.IsAbstract;
        cls.noConstructor      = type.bindingInfo.noConstructor;
        cls.attributes         = type.bindingInfo.attributes;
        cls.tag                = type.bindingInfo.tag;
        return cls;
    }

    static ApiEnum MakeApiEnumFromReflectedType(DataType const& type)
    {
        ApiEnum en;
        en.name          = type.name;
        en.namespaceName = type.namespaceName;
        // Enum constants
        for (auto& constant : type.enumConstants)
        {
            en.valueNames.Add(constant.label);
            en.values.Add(constant.value);
        }
        return en;
    }

    // -------------------------------------------------------------------------
    // Helper: Generate C++ binding code for a ReflectedType
    // -------------------------------------------------------------------------

    static void GenerateBindingCppForType(DataType const& type, StringAnsi const& assemblyType, StringAnsi& output)
    {
        BindingsCppGenerator gen;
        ApiClass cls = MakeApiClassFromReflectedType(type);
        if (cls.isStruct)
        {
            gen.GenerateCppStruct(cls, assemblyType, false, output);
        }
        else
        {
            gen.GenerateCppClass(cls, assemblyType, false, output);
        }
    }

    static void GenerateBindingCppForEnum(DataType const& type, StringAnsi const& assemblyType, StringAnsi& output)
    {
        BindingsCppGenerator gen;
        ApiEnum en = MakeApiEnumFromReflectedType(type);
        gen.GenerateCppEnum(en, assemblyType, output);
    }

    static StringAnsi DeriveAssemblyType(StringAnsi const& assemblyName)
    {
        StringAnsi assemblyType = assemblyName;
        if (assemblyType.StartsWith("SE."))
        {
            assemblyType = assemblyType.Substring(3);
        }
        return assemblyType;
    }

    bool Generator::Generate(ReflectionDatabase const& database, SolutionInfo const& solution)
    {
        LoadTemplateFile(solution);

        m_pDatabase = &database;
        for ( auto& prj : solution.projects)
        {
            // Ensure the auto generated directory exists
			String autoGeneratedDirectory = String::Format(SE_TEXT("{0}/{1}"), prj.path, Settings::g_autogeneratedDirectory);
			String autoGeneratedModuleFile =  String::Format(SE_TEXT("{0}/{1}"), prj.path, Settings::g_moduleHeaderParentDirectoryName);
			FileSystem::NormalizePath(autoGeneratedDirectory);
			FileSystem::NormalizePath(autoGeneratedModuleFile);

			if (!FileSystem::DirectoryExists(autoGeneratedDirectory))
			{
				FileSystem::CreateDirectory(autoGeneratedDirectory);
			}

            // Generate list of all expected header files in the auto generated directory
            List<String> expectedFiles;
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

				String const typeInfoFilename = headerInfo.GetAutogeneratedTypeInfoFileName(autoGeneratedDirectory);

                // Generate files
                GenerateTypeInfoFileHeader(headerInfo, solution.path);

                // Get all types for the header
                List<DataType> typesInHeader;
                m_pDatabase->GetAllTypesForHeader( headerInfo.headerId, typesInHeader );

                // Check if any type in this header has binding info
                bool headerHasBinding = false;
                for ( auto& type : typesInHeader )
                {
                    if (type.isAPI)
                    {
                        headerHasBinding = true;
                        break;
                    }
                }
                if (headerHasBinding)
                {
                    m_typeInfoFileHasBinding = true;
                    AppendBindingIncludesIfNeeded();
                }

                // Derive assembly type for binding code
                StringAnsi assemblyType;
                if (headerHasBinding)
                {
                    for ( auto& type : typesInHeader )
                    {
                        if (type.isAPI && !type.bindingInfo.assemblyName.IsEmpty())
                        {
                            assemblyType = DeriveAssemblyType(type.bindingInfo.assemblyName);
                        }
                    }
                }

                if (headerHasBinding)
                {
                    m_typeInfoFile << "namespace SE {\n";
                    m_typeInfoFile << "extern \"C\" BinaryModule* GetBinaryModule" << assemblyType.Get() << "();\n";
                    m_typeInfoFile << "}\n";
                }

                for ( auto& type : typesInHeader )
                {
                    // Generate enum info
                    if (type.IsEnum())
                    {
                        if (type.isReflect)
                        {
                            CppGenerateEnum(this, m_typeInfoFile, prj.exportMacro, type, m_CodeCppEnumTemplate.Get());
                        }

                        // Append C++ binding code for API_ENUM
                        if (type.isAPI)
                        {
                            StringAnsi bindingOutput;
                            GenerateBindingCppForEnum(type, assemblyType, bindingOutput);
                            m_typeInfoFile << std::string(bindingOutput.Get());
                        }
                    }
                    // Generate meta info
                    else if (type.IsMeta())
                    {
                        CppGenerateMeta(this, database, m_typeInfoFile, type, m_CodeCppMetaTemplate.Get());
                    }
                    // Generate type info
                    else
                    {
                        // API-only types (HasBinding but no parent) skip TTypeCompositeInfo
                        if (type.isAPI && type.parentTypeID == StringID::Invalid)
                        {
                            StringAnsi bindingOutput;
                            GenerateBindingCppForType(type, assemblyType, bindingOutput);
                            m_typeInfoFile << std::string(bindingOutput.Get());
                            continue;
                        }

                        if (type.parentTypeID == StringID::Invalid )
                        {
                            return LogError( "Invalid parent hierarchy for type {0}::{1}, all registered types must derived from a registered type.", type.namespaceName, type.name);
                        }

                        if (type.isReflect)
                        {
                            auto pTypeDesc = m_pDatabase->GetType( type.parentTypeID );
                            if (pTypeDesc == nullptr)
                            {
                                return LogError(
                                    "Unable to resolve reflected parent type for {0}::{1} while generating project {2}, header {3}. Parent TypeID: {4}. Ensure the parent type is reflected and present in the global reflection database.",
                                    type.namespaceName,
                                    type.name,
                                    prj.name,
                                    headerInfo.filePath,
                                    (uint32)type.parentTypeID);
                            }

                            CppGenerateType(this, database, m_typeInfoFile, prj.exportMacro, type, *pTypeDesc, m_CodeCppClassTemplate.Get());
                        }

                        // Append C++ binding code for API_CLASS/API_STRUCT
                        if (type.isAPI)
                        {
                            StringAnsi bindingOutput;
                            GenerateBindingCppForType(type, assemblyType, bindingOutput);
                            m_typeInfoFile << std::string(bindingOutput.Get());
                       }
                    }
                }

                // Save generated file
                SaveStreamToFile(typeInfoFilename, m_typeInfoFile);
            }

            // Get project info from database as that will contain all necessary info like module class name
            ProjectInfo const* pProjectDesc = m_pDatabase->GetProjectDesc( prj.id );
            if ( pProjectDesc == nullptr )
            {
                return LogError("Could not retrieve description for project: {0}", prj.name);
            }
            ENGINE_ASSERT( prj.id == pProjectDesc->id );

            // Get all types in project
			List<DataType> typesInProject;
            m_pDatabase->GetAllTypesForProject(pProjectDesc->id, typesInProject);
            if (!SortTypesByDependencies( typesInProject))
            {
                return LogError("Cyclic header dependency detected in project: {0}", pProjectDesc->name);
            }

            // Generate and save the module file
            GenerateModuleCodeFile(database, *pProjectDesc, typesInProject);

			String const module_cpp = String::Format(SE_TEXT("{0}/{1}"), autoGeneratedModuleFile, StringView(Settings::g_autogeneratedModuleFileSuffix));
            SaveStreamToFile(module_cpp, m_moduleFile);
        }

        // Generate C# bindings from unified data model
        //-------------------------------------------------------------------------
        {
            std::cout << " * Generating C# Bindings - ";
            Milliseconds csharpTime = 0;
            {
                ScopedTimer<PlatformClock> timer(csharpTime);

                BindingsCSharpGenerator csharpGen;

                for (auto& prj : solution.projects)
                {
                    ProjectInfo const* pProjectDesc = m_pDatabase->GetProjectDesc(prj.id);
                    if (pProjectDesc == nullptr)
                    {
                        continue;
                    }

                    List<DataType> typesInProject;
                    m_pDatabase->GetAllTypesForProject(pProjectDesc->id, typesInProject);

                    // Build BindingsHeaderInfo from ReflectedTypes with binding data
                    Dictionary<HeaderID, BindingsHeaderInfo> headersByHeaderID;

                    for (auto& type : typesInProject)
                    {
                        if (!type.isAPI)
                            continue;

                        auto& info = headersByHeaderID[type.headerID];
                        if (info.assemblyName.IsEmpty())
                        {
                            info.assemblyName = type.bindingInfo.assemblyName;
                            info.assemblyDir  = type.bindingInfo.assemblyDir;
                            // Find header file path
                            HeaderInfo const* pHdr = m_pDatabase->GetHeaderDesc(type.headerID);
                            if (pHdr)
                            {
                                info.filePath = pHdr->filePath;
                            }
                        }

                        if (type.IsEnum())
                        {
                            ApiEnum en = MakeApiEnumFromReflectedType(type);
                            info.enums.Add(en);
                        }
                        else
                        {
                            ApiClass cls = MakeApiClassFromReflectedType(type);
                            info.classes.Add(cls);
                        }
                    }

                    // Generate C# files per header
                    for (auto& pair : headersByHeaderID)
                    {
                        BindingsHeaderInfo& hdrInfo = pair.Value;
                        if (!csharpGen.Generate(hdrInfo, solution.path))
                        {
                            std::cout << "Warning: C# generation failed for header: " << hdrInfo.filePath.Get() << std::endl;
                        }
                    }

                    // Generate binary module files
                    /*BindingsCppGenerator cppGen;
                    BinaryModuleInfo modInfo;
                    modInfo.name = pProjectDesc->name.ToStringAnsi();
                    modInfo.assemblyType = DeriveAssemblyType(modInfo.name);
                    modInfo.assemblyDir = pProjectDesc->path.ToStringAnsi();
                    for (auto& pair : headersByHeaderID)
                    {
                        modInfo.headers.Add(&pair.Value);
                    }
                    if (!modInfo.headers.IsEmpty())
                    {
                        cppGen.GenerateBinaryModule(modInfo, solution.path);
                    }*/
                }
            }
            std::cout << "Complete! ( " << (float)csharpTime << "ms )" << std::endl;
        }

        return true;
    }

    void Generator::GenerateModuleCodeFile(ReflectionDatabase const& database, ProjectInfo const& prj, List<DataType> const& typesInModule)
    {
        mustache::data generateData;

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

        generateData.set("moduleClassName", std::string(prj.name.ToStringAnsi().Get()));

        mustache::data registrationTypeListData(mustache::data::type::list);
        mustache::data registrationEnumListData(mustache::data::type::list);
        mustache::data registrationMetaListData(mustache::data::type::list);

        bool hasBinding = false;
        for ( auto& type : typesInModule )
        {
            if (!type.isReflect)
            {
                continue;
            }

            mustache::data registrationTypeData;
            if (type.isDevOnly)
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

            if (type.isAPI)
            {
                hasBinding = true;
            }
        }

        generateData.set("compositeTypeList", registrationTypeListData);
        generateData.set("enumTypeList", registrationEnumListData);
        generateData.set("metaTypeList", registrationMetaListData);

        if (hasBinding)
        {
            std::string assemblyType = std::string(DeriveAssemblyType(prj.name.ToStringAnsi()).Get());
            generateData.set("assemblyType", assemblyType);
            generateData.set("modeName", std::string(prj.name.ToStringAnsi().Get()));
        }


        //-------------------------------------------------------------------------
        // generate
        mustache::mustache tmpl(m_CodeModuleTemplate.Get());
        m_moduleFile.str({});
        m_moduleFile.clear();
        m_moduleFile.flush();
        m_moduleFile << tmpl.render(generateData);
    }

}

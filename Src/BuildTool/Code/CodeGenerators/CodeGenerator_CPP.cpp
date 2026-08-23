#include "CodeGenerator_CPP.h"
#include "CodeGenerator_Utils.h"
#include "CodeGenerator_CPP_Enum.h"
#include "CodeGenerator_BindingsCpp.h"
#include "CodeGenerator_BindingsCSharp.h"

#include "Core/TopologicalSort.h"
#include "Core/String.h"
#include "Core/StringID.h"
#include <fstream>
#include <iostream>

#include "CodeGenerator_CPP_Meta.h"
#include "CodeGenerator_BindingsModel.h"
#include "CodeGenerator_CPP_Type.h"
#include "Core/Time.h"

//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    enum class TypeRegistrationHeaderType
    {
        Engine,
        Tools
    };

    static bool LoadTemplateFileString(Generator &generator, std::string path, std::string &code)
    {
        std::ifstream hdrFile(path.c_str(), std::ios::in | std::ios::ate);
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
        code.clear();

        std::string stdLine;
        while (getline(hdrFile, stdLine))
        {
            code.append((stdLine + "\n").c_str());
        }
        hdrFile.close();

        return true;
    }


    static bool IsEscaped(std::string const& text, size_t index)
    {
        size_t slashCount = 0;
        while (index > slashCount && text[index - slashCount - 1] == '\\')
        {
            slashCount++;
        }
        return (slashCount % 2) != 0;
    }

    static bool FindMatchingParen(std::string const& text, size_t openIndex, size_t& closeIndex)
    {
        int32 depth = 0;
        bool inQuote = false;

        for (size_t i = openIndex; i < text.length(); i++)
        {
            char const c = text[i];
            if (c == '"' && !IsEscaped(text, i))
            {
                inQuote = !inQuote;
                continue;
            }

            if (inQuote)
            {
                continue;
            }

            if (c == '(')
            {
                depth++;
            }
            else if (c == ')')
            {
                depth--;
                if (depth == 0)
                {
                    closeIndex = i;
                    return true;
                }
            }
        }

        return false;
    }

    static void SplitInjectCodeArguments(std::string const& text, std::vector<std::string>& outArgs)
    {
        outArgs.clear();
        int32 depth = 0;
        bool inQuote = false;
        size_t start = 0;

        for (size_t i = 0; i < text.length(); i++)
        {
            char const c = text[i];
            if (c == '"' && !IsEscaped(text, i))
            {
                inQuote = !inQuote;
                continue;
            }

            if (!inQuote)
            {
                if (c == '(' || c == '[' || c == '{')
                {
                    depth++;
                }
                else if (c == ')' || c == ']' || c == '}')
                {
                    depth--;
                }
                else if (c == ',' && depth == 0)
                {
                    outArgs.push_back(text.substr(start, i - start));
                    start = i + 1;
                }
            }
        }

        outArgs.push_back(text.substr(start));
    }

    static std::string DecodeInjectCodeString(std::string text)
    {
        Utils::String::TrimStart(text);
        Utils::String::TrimEnd(text);

        size_t quoteStart = 0;
        if (Utils::String::StartsWith(text, "u8\""))
        {
            quoteStart = 2;
        }
        else if (Utils::String::StartsWith(text, "L\"") ||
                 Utils::String::StartsWith(text, "u\"") ||
                 Utils::String::StartsWith(text, "U\""))
        {
            quoteStart = 1;
        }

        if (quoteStart >= text.length() || text[quoteStart] != '"' || text.back() != '"')
        {
            return text;
        }

        std::string result;
        for (size_t i = quoteStart + 1; i + 1 < text.length(); i++)
        {
            char const c = text[i];
            if (c != '\\' || i + 2 >= text.length())
            {
                result += c;
                continue;
            }

            char const next = text[++i];
            switch (next)
            {
            case '"':
                result += '"';
                break;
            case '\\':
                result += '\\';
                break;
            case 'n':
                result += '\n';
                break;
            case 'r':
                result += '\r';
                break;
            case 't':
                result += '\t';
                break;
            case '\n':
                result += '\n';
                break;
            default:
                result += next;
                break;
            }
        }

        return result;
    }

    static HeaderInfo const* FindHeaderInProject(ProjectInfo const& projectInfo, HeaderID headerID)
    {
        for (auto const& header : projectInfo.headerFiles)
        {
            if (header.headerId == headerID)
            {
                return &header;
            }
        }
        return nullptr;
    }
    //-------------------------------------------------------------------------

    bool Generator::SaveStreamToFile(std::string const& filePath, std::stringstream& stream)
    {
        bool fileContentsEqual = true;

        // Rewind stream to beginning
        stream.seekg(std::ios::beg);

        // Open existing file and compare contents to the newly generated stream
        std::ifstream fileStream(filePath.c_str(), std::ios::in);
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
			Utils::WriteAllText(filePath, std::string(stream.str().c_str()));
        }

        return true;
    }

    void Generator::LoadTemplateFile(SolutionInfo const &solution)
    {
        std::string rootPath = solution.path;

        std::string codeModuleTemplateFilePath(Utils::String::Format("{0}/BuildTool/Code/Template/CodeModuleTemplate.mustache", rootPath));

        std::string codeCppMetaTemplateFilePath(Utils::String::Format("{0}/BuildTool/Code/Template/CodeCppMetaTemplate.mustache", rootPath));
		std::string codeCppEnumTemplateFilePath(Utils::String::Format("{0}/BuildTool/Code/Template/CodeCppEnumTemplate.mustache", rootPath));
		std::string codeCppClassTemplateFilePath(Utils::String::Format("{0}/BuildTool/Code/Template/CodeCppClassTemplate.mustache", rootPath));


        if (m_CodeModuleTemplate.empty())
        {
            LoadTemplateFileString(*this, codeModuleTemplateFilePath, m_CodeModuleTemplate);
        }
        if (m_CodeCppMetaTemplate.empty())
        {
            LoadTemplateFileString(*this, codeCppMetaTemplateFilePath, m_CodeCppMetaTemplate);
        }
        if (m_CodeCppEnumTemplate.empty())
        {
            LoadTemplateFileString(*this, codeCppEnumTemplateFilePath, m_CodeCppEnumTemplate);
        }
        if (m_CodeCppClassTemplate.empty())
        {
            LoadTemplateFileString(*this, codeCppClassTemplateFilePath, m_CodeCppClassTemplate);
        }
    }

    void Generator::GenerateTypeInfoFileHeader(HeaderInfo const &hdr, std::string_view solutionPath)
    {
        m_typeInfoFile.str(std::string());
        m_typeInfoFile.clear();
        m_typeInfoFile << "#pragma once" << std::endl;
        m_typeInfoFile << "//*************************************************************************\n";
        m_typeInfoFile << "// This is an auto-generated file - DO NOT edit\n";
        m_typeInfoFile << "//*************************************************************************\n";
        m_typeInfoFile << "#include \"" << std::string(hdr.filePath.c_str()) << "\"\n";
        m_typeInfoFileHasBinding = false;
    }

    void Generator::AppendAPIIncludesIfNeeded()
    {
        if (!m_typeInfoFileHasBinding)
        {
            return;
        }
        m_typeInfoFile << "#include \"Runtime/Core/Scripting/ManagedCLR/CLRUtils.h\"\n";
        m_typeInfoFile << "#include \"Runtime/Core/Scripting/ScriptingObject.h\"\n";
        m_typeInfoFile << "#include \"Runtime/Core/Scripting/Internal/InternalCalls.h\"\n";
        m_typeInfoFile << "#include \"Runtime/Core/Scripting/ScriptingType.h\"\n";
    }

    static void EnsureUniqueAPIFunctionNames(TypeInfoStruct* cls)
    {
        for (int i = 0; i < cls->functions.size(); ++i)
        {
            TypeInfoFunc& fn             = cls->functions[i];
            std::string   baseName       = fn.uniqueName.empty() ? fn.name : fn.uniqueName;
            int           duplicateIndex = 0;
            for (int j = 0; j < i; ++j)
            {
                std::string previousBaseName = cls->functions[j].name;
                if (Utils::String::StartsWith(cls->functions[j].uniqueName, baseName + "_"))
                {
                    previousBaseName = baseName;
                }
                else if (!cls->functions[j].uniqueName.empty())
                {
                    previousBaseName = cls->functions[j].uniqueName;
                }

                if (previousBaseName == baseName || cls->functions[j].name == fn.name)
                {
                    duplicateIndex++;
                }
            }

            if (duplicateIndex > 0)
            {
                fn.uniqueName = Utils::String::Format("{0}_{1}", baseName, duplicateIndex);
            }
            else
            {
                fn.uniqueName = baseName;
            }

            fn.entryPoint = Utils::String::Format("{0}_{1}", cls->name, fn.uniqueName);
        }
    }

    static void BuildBindingsHeaderInfoFromTypes(TypeDatabase const& database, HeaderInfo const& headerInfo, std::vector<TypeInfoBase*> & typesInHeader, BindingsHeaderInfo& outInfo)
    {
        outInfo.filePath = headerInfo.filePath;
        outInfo.contentHash = headerInfo.checksum;

        std::vector<TypeInfoInjectedCode*> injectedCode;
        database.GetInjectCodeFormHead(headerInfo.headerId, injectedCode);

        for (auto type : typesInHeader)
        {
            if (!type->isAPI)
            {
                continue;
            }

            if (outInfo.assemblyName.empty())
            {
                outInfo.assemblyName = type->assemblyName;
                outInfo.assemblyDir  = type->assemblyDir;
            }

            if (type->IsFlag(TypeInfoBase::Flag::IsEnum))
            {
                outInfo.enums.push_back(static_cast<TypeInfoEnum*>(type));
            }
            else if (type->IsFlag(TypeInfoBase::Flag::IsStruct))
            {
                TypeInfoStruct* structType = static_cast<TypeInfoStruct*>(type);

                if (structType->APIIsInterface)
                {
                    outInfo.interfaces.push_back(structType);
                }
                else
                {
                    EnsureUniqueAPIFunctionNames(structType);
                    outInfo.classes.push_back(structType);
                }
            }
        }

        if (outInfo.assemblyName.empty() && !outInfo.injectedCode.empty())
        {
            ProjectInfo const* projectInfo = database.GetProjectDesc(headerInfo.projectID);
            if (projectInfo != nullptr)
            {
                outInfo.assemblyName = projectInfo->name;
                outInfo.assemblyDir = projectInfo->path;
            }
        }
    }


    bool Generator::Generate(TypeDatabase const& database, SolutionInfo const& solution)
    {
        LoadTemplateFile(solution);

        m_pDatabase = &database;


        for ( auto& prj : solution.projects)
        {
            // Ensure the auto generated directory exists
			std::string autoGeneratedDirectory = Utils::String::Format("{0}/{1}", prj.path, Settings::g_autogeneratedDirectory);
			std::string autoGeneratedModuleFile =  Utils::String::Format("{0}/{1}", prj.path, Settings::g_moduleHeaderParentDirectoryName);
			FileSystem::NormalizePath(autoGeneratedDirectory);
			FileSystem::NormalizePath(autoGeneratedModuleFile);

			if (!FileSystem::DirectoryExists(autoGeneratedDirectory))
			{
				FileSystem::CreateDirectory(autoGeneratedDirectory);
			}

            // Generate list of all expected header files in the auto generated directory
            std::vector<std::string> expectedFiles;
            for ( auto const& headerInfo : prj.headerFiles )
            {
                expectedFiles.push_back(headerInfo.GetAutogeneratedTypeInfoFileName(autoGeneratedDirectory));
            }
            const std::string interopHeaderFilename = autoGeneratedDirectory + "/BindingsInterop.h";
            expectedFiles.push_back(interopHeaderFilename);

            // Delete any unknown files from the auto generated directory
            std::vector<std::string> files;
			FileSystem::DirectoryGetFiles(files, autoGeneratedDirectory, nullptr, DirectorySearchOption::TopOnly);

            for (auto const& file : files)
            {
                if (!Utils::Vector::Contains(expectedFiles, file))
                {
					FileSystem::DeleteFile(file);
                }
            }

            // Generate one module-wide ABI bridge before the per-header binding
            // files. Individual wrappers include this header when they need to
            // marshal a non-blittable API struct.
            std::vector<BindingsHeaderInfo> projectBindingHeaders;
            for (auto const& headerInfo : prj.headerFiles)
            {
                std::vector<TypeInfoBase*> typesInHeader;
                m_pDatabase->GetAllTypesForHeader(headerInfo.headerId, typesInHeader);

                bool headerHasBinding = false;
                for (auto const& type : typesInHeader)
                {
                    if (type->isAPI)
                    {
                        headerHasBinding = true;
                        break;
                    }
                }

                if (!headerHasBinding)
                    continue;

                BindingsHeaderInfo bindingsHeaderInfo;
                BuildBindingsHeaderInfoFromTypes(database, headerInfo, typesInHeader, bindingsHeaderInfo);
                projectBindingHeaders.push_back(std::move(bindingsHeaderInfo));
            }

            BindingsCppGenerator interopGenerator(database);
            std::string interopHeader;
            std::stringstream interopHeaderStream;
            if (!interopGenerator.GenerateInteropHeader(projectBindingHeaders, interopHeader))
            {
                return LogError("Failed to generate bindings interop header for project: {0}", prj.name);
            }
            interopHeaderStream << interopHeader;
            if (!SaveStreamToFile(interopHeaderFilename, interopHeaderStream))
            {
                return LogError("Failed to save bindings interop header for project: {0}", prj.name);
            }

            // Generate code files for the dirty headers
            for ( auto& dirtyHeaderIdx : prj.dirtyHeaders )
            {
                auto& headerInfo = prj.headerFiles[dirtyHeaderIdx];

				std::string const typeInfoFilename = headerInfo.GetAutogeneratedTypeInfoFileName(autoGeneratedDirectory);

                // Generate files
                GenerateTypeInfoFileHeader(headerInfo, solution.path);

                // Get all types for the header
                std::vector<TypeInfoBase*> typesInHeader;
                m_pDatabase->GetAllTypesForHeader( headerInfo.headerId, typesInHeader );

                // Check if any type in this header has binding info
                bool headerHasAPI = false;
                for ( auto& type : typesInHeader )
                {
                    if (type->isAPI)
                    {
                        headerHasAPI = true;
                        break;
                    }
                }

                // Derive assembly type for binding code
                if (headerHasAPI)
                {
                    m_typeInfoFileHasBinding = true;
                    AppendAPIIncludesIfNeeded();
                }

                for ( auto& type : typesInHeader )
                {
                    // Generate enum info
                    if (type->IsFlag(TypeInfoBase::Flag::IsEnum))
                    {
                        if (type->isReflect)
                        {
                            TypeInfoEnum* enumType = static_cast<TypeInfoEnum*>(type);

                            CppGenerateEnum(this, m_typeInfoFile, prj.exportMacro, *enumType, m_CodeCppEnumTemplate.c_str());
                        }

                    }
                    // Generate meta info
                    else if (type->IsFlag(TypeInfoBase::Flag::IsMeta))
                    {
                        CppGenerateMeta(this, database, m_typeInfoFile, *type, m_CodeCppMetaTemplate.c_str());
                    }
                    // Generate type info
                    else
                    {
                        TypeInfoStruct* structType = static_cast<TypeInfoStruct*>(type);

                        // API-only types are emitted below as bindings code and do not need reflection type info.
                        if (structType->isAPI && !structType->isReflect &&
                            structType->parentTypeID == StringID::Invalid)
                        {
                            continue;
                        }

                        if (type->isReflect)
                        {
                            if (structType->parentTypeID == StringID::Invalid)
                            {
                                return LogError("Invalid parent hierarchy for type {0}::{1}, all registered types must "
                                                "derived from a registered type.",
                                                CodeGeneratorUtils::GetFullCNameSpaceName(type->namespaceScopeList),
                                                type->name);
                            }

                            TypeInfoBase const* pTypeDesc = m_pDatabase->GetType(structType->parentTypeID);
                            if (pTypeDesc == nullptr)
                            {
                                return LogError(
                                    "Unable to resolve reflected parent type for {0}::{1} while generating project {2}, header {3}. Parent TypeID: {4}. Ensure the parent type is reflected and present in the global reflection database.",
                                                CodeGeneratorUtils::GetFullCSNameSpaceName(type->namespaceScopeList),
                                    type->name,
                                    prj.name,
                                    headerInfo.filePath,
                                                (uint32)structType->parentTypeID);
                            }

                            ENGINE_ASSERT(pTypeDesc->IsFlag(TypeInfoBase::Flag::IsStruct));

                            TypeInfoStruct const* structParentType = static_cast<TypeInfoStruct const*>(pTypeDesc);
                            CppGenerateType(this,
                                            database,
                                            m_typeInfoFile,
                                            prj.exportMacro,
                                            *structType,
                                            *structParentType,
                                            m_CodeCppClassTemplate.c_str());
                        }

                    }
                }

                if (headerHasAPI)
                {
                    BindingsHeaderInfo bindingsHeaderInfo;
                    BuildBindingsHeaderInfoFromTypes(database, headerInfo, typesInHeader, bindingsHeaderInfo);

                    BindingsCppGenerator cppGen(database);
                    std::string bindingOutput;
                    if (!cppGen.GenerateSource(bindingsHeaderInfo, bindingOutput))
                    {
                        return LogError("C++ bindings generation failed for header: {0}", headerInfo.filePath);
                    }

                    if (!bindingOutput.empty())
                    {
                        m_typeInfoFile << "\n//-------------------------------------------------------------------------\n";
                        m_typeInfoFile << "// Bindings\n";
                        m_typeInfoFile << "//-------------------------------------------------------------------------\n";
                        m_typeInfoFile << std::string(bindingOutput.c_str());
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
			std::vector<TypeInfoBase*> typesInProject;
            m_pDatabase->GetAllTypesForProject(pProjectDesc->id, typesInProject);

            // Generate and save the module file
            GenerateModuleCodeFile(database, *pProjectDesc, typesInProject);

			std::string const module_cpp = Utils::String::Format("{0}/{1}", autoGeneratedModuleFile, std::string_view(Settings::g_autogeneratedModuleFileSuffix));
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

                    std::vector<TypeInfoBase*> typesInProject;
                    m_pDatabase->GetAllTypesForProject(pProjectDesc->id, typesInProject);

                    std::vector<HeaderID> apiHeaderIDs;
                    for (auto const& type : typesInProject)
                    {
                        if (!type->isAPI)
                        {
                            continue;
                        }

                        if (!Utils::Vector::Contains(apiHeaderIDs, type->headerID))
                        {
                            apiHeaderIDs.push_back(type->headerID);
                        }
                    }

                    // Generate C# files per header
                    std::vector<TypeInfoBase*> typesInHeader;
                    std::vector<BindingsHeaderInfo> projectBindingHeaders;

                    for (auto const& headerID : apiHeaderIDs)
                    {
                        HeaderInfo const* pHdr = FindHeaderInProject(prj, headerID);
                        if (pHdr == nullptr)
                        {
                            pHdr = m_pDatabase->GetHeaderDesc(headerID);
                        }
                        if (pHdr == nullptr)
                        {
                            continue;
                        }

                        typesInHeader.clear();
                        m_pDatabase->GetAllTypesForHeader(headerID, typesInHeader);

                        BindingsHeaderInfo hdrInfo;
                        BuildBindingsHeaderInfoFromTypes(database, *pHdr, typesInHeader, hdrInfo);
                        projectBindingHeaders.push_back(hdrInfo);
                        if (!csharpGen.Generate(hdrInfo, solution.path))
                        {
                            std::cout << "Warning: C# generation failed for header: " << hdrInfo.filePath.c_str() << std::endl;
                        }
                    }

                    if (!csharpGen.GenerateNativeTypeStubs(projectBindingHeaders))
                    {
                        std::cout << "Warning: C# native type stub generation failed for project: "
                                  << pProjectDesc->name.c_str() << std::endl;
                    }

                }
            }
            std::cout << "Complete! ( " << (float)csharpTime << "ms )" << std::endl;
        }

        return true;
    }

    void Generator::GenerateModuleCodeFile(TypeDatabase const& database, ProjectInfo const& prj, std::vector<TypeInfoBase*> const& typesInModule)
    {
        mustache::data generateData;

        //-------------------------------------------------------------------------
        // Includes

        std::vector<std::string> autoGeneratedFiles;
		std::string autoGeneratedDirectory = std::string(prj.path + "/" + Settings::g_autogeneratedDirectory);
		FileSystem::NormalizePath(autoGeneratedDirectory);
		FileSystem::DirectoryGetFiles(autoGeneratedFiles, autoGeneratedDirectory, "*.h", DirectorySearchOption::TopOnly);

        mustache::data includeFileListData(mustache::data::type::list);
        for (auto& file : autoGeneratedFiles)
        {
            mustache::data includeFile;
            includeFile.set("includeFile", std::string(file.c_str()));
            includeFileListData.push_back(includeFile);
        }
        generateData.set("includeFileList", includeFileListData);

        //-------------------------------------------------------------------------
        // Registration functions

        generateData.set("moduleClassName", std::string(prj.name.c_str()));

        mustache::data registrationTypeListData(mustache::data::type::list);
        mustache::data registrationEnumListData(mustache::data::type::list);
        mustache::data registrationMetaListData(mustache::data::type::list);

        bool hasBinding = false;
        for ( auto& type : typesInModule )
        {
            if (!type->isReflect)
            {
                continue;
            }

            mustache::data registrationTypeData;
            if (type->isDevOnly)
            {
                registrationTypeData.set("isDevOnlyBegin", "#ifdef SGE_DEVELOPMENT");
                registrationTypeData.set("isDevOnlyEnd", "#endif");
            }

            std::string nameSpace;
            if (!type->namespaceScopeList.empty() && !type->structScopeList.empty())
            {
                nameSpace = Utils::String::Format("::{0}::{1}",
                    CodeGeneratorUtils::GetFullCNameSpaceName(type->namespaceScopeList), CodeGeneratorUtils::GetFullCNameSpaceName(type->structScopeList));
            }else if (!type->namespaceScopeList.empty())
            {
                nameSpace = Utils::String::Format("::{0}", CodeGeneratorUtils::GetFullCNameSpaceName(type->namespaceScopeList));
            }else if (!type->structScopeList.empty())
            {
                nameSpace = Utils::String::Format("{0}", CodeGeneratorUtils::GetFullCNameSpaceName(type->structScopeList));
            }

            std::string nativeName = CodeGeneratorUtils::GetNativeName(type->namespaceScopeList, type->structScopeList, type->name);

            registrationTypeData.set("registerNamespace", std::string(nameSpace.c_str()));
            registrationTypeData.set("registerName", std::string(type->name.c_str()));
            registrationTypeData.set("registerNativeName", std::string(nativeName.c_str()));

            if (type->IsFlag(TypeInfoBase::Flag::IsEnum))
            {
                registrationEnumListData.push_back(registrationTypeData);
            }
            else if (type->IsFlag(TypeInfoBase::Flag::IsMeta))
            {
                registrationMetaListData.push_back(registrationTypeData);
            }
            else
            {
                registrationTypeListData.push_back(registrationTypeData);
            }

            if (type->isAPI)
            {
                hasBinding = true;
            }
        }

        generateData.set("compositeTypeList", registrationTypeListData);
        generateData.set("enumTypeList", registrationEnumListData);
        generateData.set("metaTypeList", registrationMetaListData);

        if (hasBinding)
        {
            std::string assemblyType = std::string(CodeGeneratorUtils::DeriveAssemblyCSharpType(prj.name).c_str());
            generateData.set("assemblyType", assemblyType);
            generateData.set("modeName", std::string(prj.name.c_str()));
        }


        //-------------------------------------------------------------------------
        // generate
        mustache::mustache tmpl(m_CodeModuleTemplate.c_str());
        m_moduleFile.str({});
        m_moduleFile.clear();
        m_moduleFile.flush();
        m_moduleFile << tmpl.render(generateData);
    }

}

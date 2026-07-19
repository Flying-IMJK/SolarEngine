#include "CodeGenerator_CPP.h"
#include "CodeGenerator_Utils.h"
#include "CodeGenerator_CPP_Enum.h"
#include "CodeGenerator_BindingsCpp.h"
#include "CodeGenerator_BindingsCSharp.h"
#include "CodeGenerator_BindingsTypeMap.h"

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

    static bool SortTypesByDependencies( std::vector<TypeData>& structureTypes )
    {
        int32 const numTypes = (int32_t) structureTypes.size();
        if ( numTypes <= 1 )
        {
            return true;
        }

        // Create list to sort
        std::vector<TopologicalSorter::Node > list;
        for ( auto i = 0; i < numTypes; i++ )
        {
            list.push_back( TopologicalSorter::Node( i ) );
        }

        for ( auto i = 0; i < numTypes; i++ )
        {
            for ( auto j = 0; j < numTypes; j++ )
            {
                if ( i != j && structureTypes[j].typeID == structureTypes[i].parentTypeID )
                {
                    list[i].m_children.push_back( &list[j] );
                }
            }
        }

        // Try to sort
        if ( !TopologicalSorter::Sort( list ) )
        {
            return false;
        }

        // Update type list
        std::vector<TypeData> sortedTypes;
        sortedTypes.reserve( numTypes );

        for ( auto& node : list )
        {
            sortedTypes.push_back( structureTypes[node.m_ID] );
        }
        structureTypes.swap( sortedTypes );

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

    static bool TryParseApiInjectedCode(std::string const& invocation, int lineNumber, ApiInjectedCode& outCode)
    {
        char const* macroName = GetMarkMacroText(ReflectionMacroType::APIInjectCode);
        int32 const macroIdx = Utils::String::Find(invocation, macroName);
        if (macroIdx == INVALID_INDEX)
        {
            return false;
        }

        int32 const openIdx = Utils::String::Find(invocation, '(', macroIdx);
        if (openIdx == INVALID_INDEX)
        {
            return false;
        }

        size_t closeIdx = 0;
        if (!FindMatchingParen(invocation, (size_t)openIdx, closeIdx))
        {
            return false;
        }

        std::string argsText = invocation.substr((size_t)openIdx + 1, closeIdx - (size_t)openIdx - 1);
        std::vector<std::string> args;
        SplitInjectCodeArguments(argsText, args);
        if (args.size() < 2)
        {
            return false;
        }

        outCode.lang = DecodeInjectCodeString(args[0]);
        Utils::String::TrimStart(outCode.lang);
        Utils::String::TrimEnd(outCode.lang);
        outCode.code = DecodeInjectCodeString(args[1]);
        Utils::String::TrimStart(outCode.code);
        Utils::String::TrimEnd(outCode.code);
        outCode.lineNumber = lineNumber;
        return !outCode.lang.empty() && !outCode.code.empty();
    }

    static void CollectApiInjectedCode(HeaderInfo const& headerInfo, std::vector<ApiInjectedCode>& outCodes)
    {
        uint32 openCommentBlock = 0;
        char const* macroName = GetMarkMacroText(ReflectionMacroType::APIInjectCode);

        for (int32 lineIdx = 0; lineIdx < (int32)headerInfo.fileContents.size(); lineIdx++)
        {
            std::string const& line = headerInfo.fileContents[(size_t)lineIdx];
            int32 const blockStartIdx = Utils::String::Find(line, "/*");
            int32 const blockEndIdx = Utils::String::Find(line, "*/");
            int32 const macroIdx = Utils::String::Find(line, macroName);

            if (macroIdx != INVALID_INDEX && openCommentBlock == 0)
            {
                int32 const lineCommentIdx = Utils::String::Find(line, "//");
                bool const macroIsLineCommented = lineCommentIdx != INVALID_INDEX && lineCommentIdx < macroIdx;
                bool const macroIsBlockCommented = blockStartIdx != INVALID_INDEX && blockStartIdx < macroIdx &&
                                                   (blockEndIdx == INVALID_INDEX || blockEndIdx > macroIdx);
                if (!macroIsLineCommented && !macroIsBlockCommented)
                {
                    std::string invocation = line.substr((size_t)macroIdx);
                    size_t openIdx = invocation.find('(');
                    size_t closeIdx = 0;
                    int32 endLineIdx = lineIdx;
                    while (openIdx != std::string::npos && !FindMatchingParen(invocation, openIdx, closeIdx) &&
                           endLineIdx + 1 < (int32)headerInfo.fileContents.size())
                    {
                        endLineIdx++;
                        invocation += "\n";
                        invocation += headerInfo.fileContents[(size_t)endLineIdx];
                    }

                    ApiInjectedCode code;
                    if (TryParseApiInjectedCode(invocation, lineIdx + 1, code))
                    {
                        outCodes.push_back(code);
                    }
                    lineIdx = endLineIdx;
                }
            }

            if (blockStartIdx != INVALID_INDEX)
            {
                openCommentBlock++;
            }
            if (blockEndIdx != INVALID_INDEX && openCommentBlock > 0)
            {
                openCommentBlock--;
            }
        }
    }

    static bool IsInjectedCodeForLang(ApiInjectedCode const& code, char const* lang)
    {
        return Utils::String::ToLowerCopy(code.lang) == Utils::String::ToLowerCopy(lang);
    }

    static bool HeaderHasInjectedCode(HeaderInfo const& headerInfo, char const* lang)
    {
        std::vector<ApiInjectedCode> injectedCode;
        CollectApiInjectedCode(headerInfo, injectedCode);
        for (auto const& code : injectedCode)
        {
            if (IsInjectedCodeForLang(code, lang))
            {
                return true;
            }
        }
        return false;
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

    void Generator::AppendBindingIncludesIfNeeded()
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

    // -------------------------------------------------------------------------
    // Helper: Create ApiClass from ReflectedType::bindingInfo
    // -------------------------------------------------------------------------

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
        std::string result = StripCppKeywordPrefixes(cppType);
        int pos = INVALID_INDEX;
        int searchStart = 0;
        while (true)
        {
            std::string tail = result.substr(searchStart);
            int found = Utils::String::Find(tail, "::");
            if (found == INVALID_INDEX)
            {
                break;
            }
            pos = searchStart + found;
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
        std::string stripped = StripCppKeywordPrefixes(StripTypeQualifiers(cppType));
        TypeMapping const* mapping = FindTypeMapping(stripped.c_str());
        if (mapping != nullptr)
        {
            return mapping;
        }

        std::string unqualified = GetUnqualifiedCppTypeName(stripped);
        return FindTypeMapping(unqualified.c_str());
    }

    static bool IsKnownNonPodValueType(std::string const& cppType)
    {
        std::string type = GetUnqualifiedCppTypeName(StripTypeQualifiers(cppType));
        return type == "Array"
            || type == "String"
            || type == "StringView"
            || type == "StringAnsi"
            || type == "StringAnsiView";
    }

    static TypeData const* FindApiTypeByCppType(ReflectionDatabase const& database, std::string const& cppType)
    {
        std::string stripped = StripCppKeywordPrefixes(StripTypeQualifiers(cppType));
        std::string unqualified = GetUnqualifiedCppTypeName(stripped);

        for (auto const& type : database.GetAllTypes())
        {
            if (!type.isAPI)
            {
                continue;
            }

            std::string fullName = CodeGeneratorUtils::GetNativeName(type.namespaceScopeList, type.structScopeList, type.name);
            if (stripped == StripCppKeywordPrefixes(fullName))
            {
                return &type;
            }
        }

        for (auto const& type : database.GetAllTypes())
        {
            if (type.isAPI && (stripped == type.name || unqualified == type.name))
            {
                return &type;
            }
        }

        return nullptr;
    }

    static bool CalculateStructureIsPod(ReflectionDatabase const& database, TypeData const& type, std::vector<TypeID>& stack);

    static bool IsBindingFieldTypePod(ReflectionDatabase const& database, std::string const& cppType, std::vector<TypeID>& stack)
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

        std::string valueType = typeInfo.baseType.empty() ? cppType : typeInfo.baseType;
        TypeMapping const* mapping = FindTypeMappingForPod(valueType);
        if (mapping != nullptr)
        {
            return mapping->isBlittable;
        }
        if (IsKnownNonPodValueType(valueType))
        {
            return false;
        }

        TypeData const* apiType = FindApiTypeByCppType(database, valueType);
        if (apiType != nullptr)
        {
            if (apiType->IsFlag(TypeData::Flags::IsEnum))
            {
                return true;
            }
            if (!apiType->IsFlag(TypeData::Flags::IsStruct))
            {
                return false;
            }
            return CalculateStructureIsPod(database, *apiType, stack);
        }

        return true;
    }

    static bool CalculateStructureIsPod(ReflectionDatabase const& database, TypeData const& type, std::vector<TypeID>& stack)
    {
        if (!type.IsFlag(TypeData::Flags::IsStruct))
        {
            return false;
        }
        if (type.bindingInfo.isInterface || !type.bindingInfo.interfaces.empty())
        {
            return false;
        }
        // IsTemplate also marks SE_TYPEDEF instantiations here, which can still be POD.
        if (Utils::Vector::Contains(stack, type.typeID))
        {
            return true;
        }

        stack.push_back(type.typeID);
        bool isPod = true;

        if (!type.bindingInfo.baseClassName.empty())
        {
            TypeData const* baseType = FindApiTypeByCppType(database, type.bindingInfo.baseClassName);
            isPod = baseType != nullptr
                && baseType->IsFlag(TypeData::Flags::IsStruct)
                && CalculateStructureIsPod(database, *baseType, stack);
        }

        for (int i = 0; isPod && i < type.bindingInfo.fields.size(); ++i)
        {
            ApiField const& field = type.bindingInfo.fields[i];
            if (!field.isStatic && !IsBindingFieldTypePod(database, field.cppType, stack))
            {
                isPod = false;
            }
        }

        stack.pop_back();
        return isPod;
    }

    static bool CalculateStructureIsPod(ReflectionDatabase const& database, TypeData const& type)
    {
        std::vector<TypeID> stack;
        return CalculateStructureIsPod(database, type, stack);
    }

    static std::string GetApiBindingName(TypeData const& type)
    {
        return type.bindingInfo.name.empty() ? type.name : type.bindingInfo.name;
    }

    static void RegisterApiTypeNameAliases(ReflectionDatabase const& database)
    {
        ClearApiTypeNameAliases();
        for (auto const& type : database.GetAllTypes())
        {
            if (!type.isAPI || type.IsFlag(TypeData::Flags::IsEnum))
            {
                continue;
            }

            std::string nativeFullName = CodeGeneratorUtils::GetNativeName(type.namespaceScopeList, type.structScopeList, type.name);
            std::string publicName = GetApiBindingName(type);
            std::string publicFullName = CodeGeneratorUtils::GetFullCSTypeName(type.namespaceScopeList, publicName);
            RegisterApiTypeNameAlias(type.name, nativeFullName, publicName, publicFullName);
            if (type.bindingInfo.isScriptingObject)
            {
                RegisterApiScriptingObjectType(type.name, nativeFullName);
            }
            else if (!type.IsFlag(TypeData::Flags::IsStruct) && !type.bindingInfo.isStatic)
            {
                RegisterApiNativeObjectType(type.name, nativeFullName);
            }
        }
    }

    static ApiClass MakeApiClassFromReflectedType(ReflectionDatabase const& database, TypeData const& type)
    {
        ApiClass cls;
        cls.name              = GetApiBindingName(type);
        cls.nativeName        = type.name;
        cls.namespaceNameList     = type.namespaceScopeList;
        cls.structScopeList     = type.structScopeList;
        cls.baseClassName     = type.bindingInfo.baseClassName;
        cls.isTemplate        = type.IsFlag(TypeData::Flags::IsTemplate);
        cls.isStruct           = type.IsFlag(TypeData::Flags::IsStruct);
        cls.isPod              = cls.isStruct && CalculateStructureIsPod(database, type);
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
        cls.comment            = type.bindingInfo.comment;
        cls.marshalAs          = type.bindingInfo.marshalAs;
        cls.isInterface        = type.bindingInfo.isInterface;
        cls.isDeprecated       = type.bindingInfo.isDeprecated;
        return cls;
    }

    static ApiEnum MakeApiEnumFromTypeData(TypeData const& type)
    {
        ApiEnum en;
        en.name          = type.name;
        en.namespaceScopeList = type.namespaceScopeList;
        en.structScopeList = type.structScopeList;
        en.underlyingType = GetEnumUnderlyingTypeName(static_cast<Utils::TypeIDCore>(type.underlyingType));
        // Enum constants
        for (auto& constant : type.enumConstants)
        {
            en.valueNames.push_back(constant.label);
            en.values.push_back(constant.value);
            en.valueComments.push_back(constant.description);
        }
        en.attributes = type.bindingInfo.attributes;
        en.comment = type.bindingInfo.comment;
        return en;
    }

    static void EnsureUniqueBindingFunctionNames(ApiClass& cls)
    {
        for (int i = 0; i < cls.functions.size(); ++i)
        {
            ApiFunction& fn = cls.functions[i];
            std::string baseName = fn.uniqueName.empty() ? fn.name : fn.uniqueName;
            int duplicateIndex = 0;
            for (int j = 0; j < i; ++j)
            {
                std::string previousBaseName = cls.functions[j].name;
                if (Utils::String::StartsWith(cls.functions[j].uniqueName, baseName + "_"))
                {
                    previousBaseName = baseName;
                }
                else if (!cls.functions[j].uniqueName.empty())
                {
                    previousBaseName = cls.functions[j].uniqueName;
                }

                if (previousBaseName == baseName || cls.functions[j].name == fn.name)
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

            if (cls.namespaceNameList.size() > 0 && cls.structScopeList.size() > 0)
            {
                fn.entryPoint = Utils::String::Format("{0}::{1}::{2}::{3}",
                                CodeGeneratorUtils::GetFullCNameSpaceName(cls.namespaceNameList),
                                CodeGeneratorUtils::GetFullCNameSpaceName(cls.structScopeList),
                                Utils::String::Format("{0}Internal", cls.name),
                                fn.uniqueName);
            }
            else if (cls.namespaceNameList.size() > 0)
            {
                fn.entryPoint = Utils::String::Format("{0}::{1}::{2}",
                                CodeGeneratorUtils::GetFullCNameSpaceName(cls.namespaceNameList),
                                Utils::String::Format("{0}Internal", cls.name),
                                fn.uniqueName);
            }
            else if (cls.structScopeList.size() > 0)
            {
                fn.entryPoint = Utils::String::Format("{0}::{1}::{2}",
                                CodeGeneratorUtils::GetFullCNameSpaceName(cls.structScopeList),
                                Utils::String::Format("{0}Internal", cls.name),
                                fn.uniqueName);
            }
            else
            {
                fn.entryPoint = Utils::String::Format("{0}_{1}", cls.name, fn.uniqueName);
            }
        }
    }

    static void BuildBindingsHeaderInfoFromTypes(ReflectionDatabase const& database, HeaderInfo const& headerInfo, std::vector<TypeData> const& typesInHeader, BindingsHeaderInfo& outInfo)
    {
        outInfo.filePath = headerInfo.filePath;
        outInfo.contentHash = headerInfo.checksum;
        CollectApiInjectedCode(headerInfo, outInfo.injectedCode);

        for (auto const& type : typesInHeader)
        {
            if (!type.isAPI)
            {
                continue;
            }

            if (outInfo.assemblyName.empty())
            {
                outInfo.assemblyName = type.bindingInfo.assemblyName;
                outInfo.assemblyDir = type.bindingInfo.assemblyDir;
            }

            if (type.IsFlag(TypeData::Flags::IsEnum))
            {
                outInfo.enums.push_back(MakeApiEnumFromTypeData(type));
            }
            else if (type.bindingInfo.isInterface)
            {
                ApiInterface iface;
                iface.name = GetApiBindingName(type);
                iface.nativeName = type.name;
                iface.namespaceNameList = type.namespaceScopeList;
                iface.functions = type.bindingInfo.functions;
                iface.attributes = type.bindingInfo.attributes;
                iface.comment = type.bindingInfo.comment;
                outInfo.interfaces.push_back(iface);
            }
            else
            {
                ApiClass cls = MakeApiClassFromReflectedType(database, type);
                EnsureUniqueBindingFunctionNames(cls);
                outInfo.classes.push_back(cls);
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

    bool Generator::Generate(ReflectionDatabase const& database, SolutionInfo const& solution)
    {
        LoadTemplateFile(solution);

        m_pDatabase = &database;
        RegisterApiTypeNameAliases(database);
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

            // Generate code files for the dirty headers
            for ( auto& dirtyHeaderIdx : prj.dirtyHeaders )
            {
                auto& headerInfo = prj.headerFiles[dirtyHeaderIdx];

				std::string const typeInfoFilename = headerInfo.GetAutogeneratedTypeInfoFileName(autoGeneratedDirectory);

                // Generate files
                GenerateTypeInfoFileHeader(headerInfo, solution.path);

                // Get all types for the header
                std::vector<TypeData> typesInHeader;
                m_pDatabase->GetAllTypesForHeader( headerInfo.headerId, typesInHeader );

                // Check if any type in this header has binding info
                bool headerHasBinding = HeaderHasInjectedCode(headerInfo, "cpp");
                for ( auto& type : typesInHeader )
                {
                    if (type.isAPI)
                    {
                        headerHasBinding = true;
                        break;
                    }
                }

                // Derive assembly type for binding code
                if (headerHasBinding)
                {
                    m_typeInfoFileHasBinding = true;
                    AppendBindingIncludesIfNeeded();
                }

                for ( auto& type : typesInHeader )
                {
                    // Generate enum info
                    if (type.IsFlag(TypeData::Flags::IsEnum))
                    {
                        if (type.isReflect)
                        {
                            CppGenerateEnum(this, m_typeInfoFile, prj.exportMacro, type, m_CodeCppEnumTemplate.c_str());
                        }

                    }
                    // Generate meta info
                    else if (type.IsFlag(TypeData::Flags::IsMeta))
                    {
                        CppGenerateMeta(this, database, m_typeInfoFile, type, m_CodeCppMetaTemplate.c_str());
                    }
                    // Generate type info
                    else
                    {
                        // API-only types are emitted below as bindings code and do not need reflection type info.
                        if (type.isAPI && !type.isReflect && type.parentTypeID == StringID::Invalid)
                        {
                            continue;
                        }

                        if (type.parentTypeID == StringID::Invalid )
                        {
                            return LogError( "Invalid parent hierarchy for type {0}::{1}, all registered types must derived from a registered type.", CodeGeneratorUtils::GetFullCNameSpaceName(type.namespaceScopeList), type.name);
                        }

                        if (type.isReflect)
                        {
                            auto pTypeDesc = m_pDatabase->GetType( type.parentTypeID );
                            if (pTypeDesc == nullptr)
                            {
                                return LogError(
                                    "Unable to resolve reflected parent type for {0}::{1} while generating project {2}, header {3}. Parent TypeID: {4}. Ensure the parent type is reflected and present in the global reflection database.",
                                    CodeGeneratorUtils::GetFullCSNameSpaceName(type.namespaceScopeList),
                                    type.name,
                                    prj.name,
                                    headerInfo.filePath,
                                    (uint32)type.parentTypeID);
                            }

                            CppGenerateType(this, database, m_typeInfoFile, prj.exportMacro, type, *pTypeDesc, m_CodeCppClassTemplate.c_str());
                        }

                    }
                }

                if (headerHasBinding)
                {
                    BindingsHeaderInfo bindingsHeaderInfo;
                    BuildBindingsHeaderInfoFromTypes(database, headerInfo, typesInHeader, bindingsHeaderInfo);

                    BindingsCppGenerator cppGen;
                    std::string bindingOutput;
                    if (!cppGen.GenerateSource(bindingsHeaderInfo, true, bindingOutput))
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
			std::vector<TypeData> typesInProject;
            m_pDatabase->GetAllTypesForProject(pProjectDesc->id, typesInProject);
            if (!SortTypesByDependencies( typesInProject))
            {
                return LogError("Cyclic header dependency detected in project: {0}", pProjectDesc->name);
            }

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

                    std::vector<TypeData> typesInProject;
                    m_pDatabase->GetAllTypesForProject(pProjectDesc->id, typesInProject);

                    std::vector<HeaderID> apiHeaderIDs;
                    for (auto const& type : typesInProject)
                    {
                        if (!type.isAPI)
                        {
                            continue;
                        }

                        if (!Utils::Vector::Contains(apiHeaderIDs, type.headerID))
                        {
                            apiHeaderIDs.push_back(type.headerID);
                        }
                    }
                    for (auto const& headerInfo : prj.headerFiles)
                    {
                        if (HeaderHasInjectedCode(headerInfo, "csharp") && !Utils::Vector::Contains(apiHeaderIDs, headerInfo.headerId))
                        {
                            apiHeaderIDs.push_back(headerInfo.headerId);
                        }
                    }

                    std::vector<BindingsHeaderInfo> projectBindingHeaders;

                    // Generate C# files per header
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

                        std::vector<TypeData> typesInHeader;
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

    void Generator::GenerateModuleCodeFile(ReflectionDatabase const& database, ProjectInfo const& prj, std::vector<TypeData> const& typesInModule)
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

            std::string nameSpace;
            if (!type.namespaceScopeList.empty() && !type.structScopeList.empty())
            {
                nameSpace = Utils::String::Format("::{0}::{1}",
                    CodeGeneratorUtils::GetFullCNameSpaceName(type.namespaceScopeList), CodeGeneratorUtils::GetFullCNameSpaceName(type.structScopeList));
            }else if (!type.namespaceScopeList.empty())
            {
                nameSpace = Utils::String::Format("::{0}", CodeGeneratorUtils::GetFullCNameSpaceName(type.namespaceScopeList));
            }else if (!type.structScopeList.empty())
            {
                nameSpace = Utils::String::Format("{0}", CodeGeneratorUtils::GetFullCNameSpaceName(type.structScopeList));
            }

            std::string nativeName = CodeGeneratorUtils::GetNativeName(type.namespaceScopeList, type.structScopeList, type.name);

            registrationTypeData.set("registerNamespace", std::string(nameSpace.c_str()));
            registrationTypeData.set("registerName", std::string(type.name.c_str()));
            registrationTypeData.set("registerNativeName", std::string(nativeName.c_str()));

            if (type.IsFlag(TypeData::Flags::IsEnum))
            {
                registrationEnumListData.push_back(registrationTypeData);
            }
            else if (type.IsFlag(TypeData::Flags::IsMeta))
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

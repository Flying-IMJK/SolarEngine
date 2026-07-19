#include "ClangParserContext.h"
#include "Database/ReflectionDatabase.h"

#include <algorithm>
#include <cctype>

//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    static std::string GetFullTypeName(std::vector<std::string> const& namespaces,
                                       std::vector<std::string> const& structScopes,
                                       std::string const& name)
    {
        std::string result;
        if (!namespaces.empty())
        {
            result = Utils::CombineStringList(namespaces, "::");
        }
        if (!structScopes.empty())
        {
            if (!result.empty())
                result += "::";
            result += Utils::CombineStringList(structScopes, "::");
        }
        if (!result.empty())
            result += "::";
        result += name;
        return result;
    }

    static std::string GetUnqualifiedTypeName(std::string const& typeName)
    {
        int32 pos = Utils::String::FindLast(typeName, ':');
        if (pos != INVALID_INDEX && pos > 0 && typeName[(size_t)pos - 1] == ':')
        {
            return typeName.substr((size_t)pos + 1);
        }
        return typeName;
    }

    static bool IsIdentifierChar(char value)
    {
        return std::isalnum((unsigned char)value) || value == '_';
    }

    static void ReplaceTemplateParameter(std::string& text, std::string const& parameterName, std::string const& argumentName)
    {
        if (text.empty() || parameterName.empty())
            return;

        size_t pos = 0;
        while ((pos = text.find(parameterName, pos)) != std::string::npos)
        {
            bool leftBoundary = pos == 0 || !IsIdentifierChar(text[pos - 1]);
            size_t endPos = pos + parameterName.length();
            bool rightBoundary = endPos >= text.length() || !IsIdentifierChar(text[endPos]);
            if (leftBoundary && rightBoundary)
            {
                text.replace(pos, parameterName.length(), argumentName);
                pos += argumentName.length();
            }
            else
            {
                pos = endPos;
            }
        }
    }

    static void InflateText(std::string& text, std::vector<std::string> const& parameters, std::vector<std::string> const& arguments)
    {
        uint32 const count = (uint32)std::min(parameters.size(), arguments.size());
        for (uint32 i = 0; i < count; i++)
        {
            ReplaceTemplateParameter(text, parameters[i], arguments[i]);
        }
    }

    static void InflateApiParam(ApiParam& param, std::vector<std::string> const& parameters, std::vector<std::string> const& arguments)
    {
        InflateText(param.cppType, parameters, arguments);
    }

    static void InflateApiFunction(ApiFunction& fn, std::vector<std::string> const& parameters, std::vector<std::string> const& arguments)
    {
        InflateText(fn.returnType, parameters, arguments);
        for (auto& param : fn.params)
        {
            InflateApiParam(param, parameters, arguments);
        }
    }

    static void InflateApiProperty(ApiProperty& prop, std::vector<std::string> const& parameters, std::vector<std::string> const& arguments)
    {
        InflateText(prop.cppType, parameters, arguments);
    }

    static void InflateApiField(ApiField& field, std::vector<std::string> const& parameters, std::vector<std::string> const& arguments)
    {
        InflateText(field.cppType, parameters, arguments);
    }

    static void InflateApiEvent(ApiEvent& evt, std::vector<std::string> const& parameters, std::vector<std::string> const& arguments)
    {
        InflateText(evt.cppType, parameters, arguments);
        for (auto& param : evt.params)
        {
            InflateApiParam(param, parameters, arguments);
        }
    }

    static void InflateTypeData(TypeData& type, std::vector<std::string> const& parameters, std::vector<std::string> const& arguments)
    {
        for (auto& property : type.properties)
        {
            InflateText(property.typeName, parameters, arguments);
            InflateText(property.templateArgTypeName, parameters, arguments);
            if (!property.typeName.empty())
            {
                property.typeID = TypeID(property.typeName);
            }
        }

        for (auto& fn : type.bindingInfo.functions)
        {
            InflateApiFunction(fn, parameters, arguments);
        }
        for (auto& prop : type.bindingInfo.bindingProperties)
        {
            InflateApiProperty(prop, parameters, arguments);
        }
        for (auto& field : type.bindingInfo.fields)
        {
            InflateApiField(field, parameters, arguments);
        }
        for (auto& iface : type.bindingInfo.interfaces)
        {
            for (auto& fn : iface.functions)
            {
                InflateApiFunction(fn, parameters, arguments);
            }
        }
        for (auto& evt : type.bindingInfo.events)
        {
            InflateApiEvent(evt, parameters, arguments);
        }
    }

    static void CalculateFullNamespace(std::vector<std::string> const &namespaceStack, std::string &fullNamespace)
    {
        fullNamespace.clear();
        for (int i = 0; i < namespaceStack.size(); i++)
        {
            fullNamespace.append(namespaceStack[i]);
            if (i != namespaceStack.size() - 1)
            {
                fullNamespace.append("::");
            }
        }
    }

    static void CalculateFullStructScope(std::vector<std::string> const &structScope, std::string &fullStructScope)
    {
        fullStructScope.clear();
        for (int i = 0; i < structScope.size(); i++)
        {
            fullStructScope.append(structScope[i]);
            if (i != structScope.size() - 1)
            {
                fullStructScope.append("_");
            }
        }
    }

    // TODO: Support block comments
    static std::string TryToParseMacro(std::vector<std::string> const &fileContents, int32 parsedMacroLineNumber)
    {
        // Clang seems to have one less line of code in its parsed data
        int32 const lineNumber = parsedMacroLineNumber - 1;

        //-------------------------------------------------------------------------

		std::string macroComment;

        auto SanitizeCommentString = [](std::string &comment)
        {
		  	Utils::String::ReplaceAll(comment, "\r", " ");
		  	Utils::String::TrimStart(comment);
			Utils::String::TrimEnd(comment);
        };

        // Check same line as the macro
        //-------------------------------------------------------------------------
        // This takes precedence over comments placed above

        int32 const sameLineFoundPos = Utils::String::Find(fileContents[lineNumber], "//");
        if (sameLineFoundPos != INVALID_INDEX)
        {
            if (!macroComment.empty())
            {
                macroComment += "\\n";
            }

            macroComment = fileContents[lineNumber].substr(sameLineFoundPos + 2, fileContents[lineNumber].length() - sameLineFoundPos - 2);
            SanitizeCommentString(macroComment);
            return macroComment;
        }

        // Check lines directly above the macro
        //-------------------------------------------------------------------------
        // TODO: check for other macros, etc....

        if (lineNumber > 0)
        {
            int32 const foundCommentPos = Utils::String::Find(fileContents[lineNumber - 1], "//");
            if (foundCommentPos != INVALID_INDEX)
            {
                macroComment = fileContents[lineNumber - 1].substr(foundCommentPos + 2, fileContents[lineNumber - 1].length() - foundCommentPos - 2);
                SanitizeCommentString(macroComment);
                return macroComment;
            }
        }

        //-------------------------------------------------------------------------

        return macroComment;
    }

    //-------------------------------------------------------------------------

    static void SplitRespectingBrackets(std::string const &str, Char delimiter, std::vector<std::string> &outParts)
    {
        int32 depth = 0;
        bool inQuote = false;
        int32 start = 0;

        for (int32 i = 0; i < str.length(); i++)
        {
            Char c = str[i];
            if (c == '"')
            {
                inQuote = !inQuote;
            }
            else if (!inQuote)
            {
                if (c == '(' || c == '[' || c == '{')
                {
                    depth++;
                }
                else if (c == ')' || c == ']' || c == '}')
                {
                    depth--;
                }
                else if (c == delimiter && depth == 0)
                {
                    outParts.push_back(str.substr(start, i - start));
                    start = i + 1;
                }
            }
        }

        if (start < str.length())
        {
            outParts.push_back(str.substr(start, str.length() - start));
        }
    }

    //-------------------------------------------------------------------------

    MarkMacro::MarkMacro(HeaderInfo const *pHeaderInfo, CXCursor cursor, CXSourceRange& sourceRange, ReflectionMacroType type)
        : headerID(pHeaderInfo->headerId), type(type)
    {
        ENGINE_ASSERT(type < ReflectionMacroType::NumMacros);

        clang_getExpansionLocation(clang_getRangeStart(sourceRange), nullptr, &fileLine, &fileColumn, nullptr);

        //-------------------------------------------------------------------------

        // macroComment = TryToParseMacro(pHeaderInfo->fileContents, lineNumber);

        //-------------------------------------------------------------------------

        // Read the contents of the macro for all annotation types
        //-------------------------------------------------------------------------
        bool needsParamParsing = (type == ReflectionMacroType::SEClass ||
                                   type == ReflectionMacroType::SEStruct ||
                                   type == ReflectionMacroType::SEInterface ||
                                   type == ReflectionMacroType::SEEnum ||
                                   type == ReflectionMacroType::SEProperty ||
                                   type == ReflectionMacroType::SEFunction ||
                                   type == ReflectionMacroType::SEEvent ||
                                   type == ReflectionMacroType::SETypeDef);

        if (needsParamParsing)
        {
            CXToken *tokens = nullptr;
            uint32 numTokens = 0;
            CXTranslationUnit translationUnit = clang_Cursor_getTranslationUnit(cursor);
            clang_tokenize(translationUnit, sourceRange, &tokens, &numTokens);
            std::string rawContents;
            for (uint32 n = 0; n < numTokens; n++)
            {
                rawContents += ClangUtils::GetString(clang_getTokenSpelling(translationUnit, tokens[n]));
            }
            clang_disposeTokens(translationUnit, tokens, numTokens);

            std::string macroContent;
            // Extract content between parentheses
            int32 startIdx = Utils::String::Find(rawContents, "(");
            int32 endIdx = Utils::String::FindLast(rawContents, ')');
            if (startIdx != INVALID_INDEX && endIdx != INVALID_INDEX && endIdx > (startIdx + 1))
            {
                macroContent = rawContents.substr(startIdx + 1, endIdx - startIdx - 1);
            }
            else
            {
                macroContent.clear();
            }

            if (!macroContent.empty())
            {
                std::vector<std::string> params;
                SplitRespectingBrackets(macroContent, ',', params);

                for (auto &param : params)
                {
                    Utils::String::TrimStart(param);
                    Utils::String::TrimEnd(param);

                    if (param == "Reflect")
                    {
                        hasReflect = true;
                    }else if (param == "API")
                    {
                        hasAPI = true;
                    }
                    else
                    {
                        macroContents.push_back(param);
                    }
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    bool MarkMacro::HasContent(std::string_view value) const
    {
        for (auto const& content : macroContents)
        {
            if (content == value)
            {
                return true;
            }
        }
        return false;
    }

    //-------------------------------------------------------------------------

    HeaderInfo const *ClangParserContext::GetHeaderInfo(HeaderID headerID) const
    {
        for (int i = 0; i < headersToVisit.size(); ++i)
        {
            if (headersToVisit[i].m_ID == headerID)
            {
                return headersToVisit[i].m_pHeaderInfo;
            }
        }

        return nullptr;
    }

    void ClangParserContext::Reset(CXTranslationUnit *pTU)
    {
        assert(m_namespaceStack.empty());
        assert(m_structureStack.empty());

        this->pTU = pTU;
        m_MarkMacros.Clear();
        m_TemplateTypes.clear();
        m_TypeDefs.clear();
        m_errorMessage.clear();
    }

    void ClangParserContext::PushNamespace(std::string const &name)
    {
        m_namespaceStack.push_back(name);
        CalculateFullNamespace(m_namespaceStack, m_currentNamespace);
    }

    void ClangParserContext::PopNamespace()
    {
        m_namespaceStack.pop_back();
        CalculateFullNamespace(m_namespaceStack, m_currentNamespace);
    }

    void ClangParserContext::PushStruct(std::string const& name)
    {
        m_structureStack.push_back(name);
        CalculateFullNamespace(m_structureStack, m_currentStructScope);
    }

    void ClangParserContext::PopStruct()
    {
        m_structureStack.pop_back();
    }

    std::vector<std::string> ClangParserContext::GetStructScopes()
    {
        return m_structureStack;
    }

    std::vector<std::string> ClangParserContext::GetNamespaces()
    {
        return m_namespaceStack;
    }

    bool ClangParserContext::SetModuleClassName(std::string_view const &headerFilePath, std::string const &moduleClassName)
    {
        for (auto &prj : pSolution->projects)
        {
            if (FileSystem::IsUnderDirectory(std::string(headerFilePath), prj.path))
            {
                ENGINE_ASSERT(prj.moduleClassNameFull.empty());
                prj.moduleClassNameFull = moduleClassName;

                std::vector<std::string> sp;
				Utils::String::Split(moduleClassName, "::", sp);
                if(sp.size() > 1)
                {
                    prj.moduleClassName = sp[sp.size() - 1];
                }
                else
                {
                    prj.moduleClassName = moduleClassName;
                }

                return true;
            }
        }

        return false;
    }

    //-------------------------------------------------------------------------

    void ClangParserContext::AddMarkMacro(MarkMacro const &foundMacro)
    {
        ENGINE_ASSERT(foundMacro.headerID != StringID::Invalid);
        ENGINE_ASSERT(foundMacro.type != ReflectionMacroType::Unknown);

        std::vector<MarkMacro> &macrosForHeader = m_MarkMacros[foundMacro.headerID];
        macrosForHeader.push_back(foundMacro);
    }

    bool ClangParserContext::FindMarkMacro(HeaderID headerID, CXCursor const& cr, MarkMacro& macro, ReflectionMacroType macroType)
    {
        // Try get macros for this header
        //-------------------------------------------------------------------------
        auto headerIter = m_MarkMacros.Find(headerID);
        if (headerIter == m_MarkMacros.end())
        {
            return false;
        }

        std::vector<MarkMacro> &macrosForHeader = headerIter->Value;
        uint32_t line, column;
        ClangUtils::GetLineColumnNumberForCursor(cr, line, column);

        // Look for SE_ENUM with hasReflect == true
        //-------------------------------------------------------------------------
        int bestIndex = -1;

        for (int index = 0; index < macrosForHeader.size(); index++)
        {
            auto &item = macrosForHeader[index];

            if (item.type != macroType)
            {
                continue;
            }

            if (item.fileLine == line && item.fileColumn < column)
            {
                bestIndex = index;
                break;
            }
            else if (item.fileLine == line - 1)
            {
                bestIndex = index;
                break;
            }
        }

        if (bestIndex >= 0)
        {
            macro = macrosForHeader[bestIndex];
            Utils::Vector::RemoveAt(macrosForHeader, (size_t)bestIndex);
            return true;
        }

        return false;
    }

    bool ClangParserContext::FindReflectionMacroForMeta(HeaderID headerID, CXCursor const& cr, MarkMacro& reflectionMacro)
    {
        return FindMarkMacro(headerID, cr, reflectionMacro, ReflectionMacroType::ReflectMeta);
    }

    void ClangParserContext::AddTemplateType(TypeData const& type, std::vector<std::string> const& parameterNames)
    {
        TemplateTypeData data;
        data.type = type;
        data.parameterNames = parameterNames;
        m_TemplateTypes.emplace_back(data);
    }

    void ClangParserContext::AddTypeDef(TypeDefData const& typeDef)
    {
        m_TypeDefs.emplace_back(typeDef);
    }

    bool ClangParserContext::ResolvePendingTypeDefs()
    {
        for (auto const& pending : m_TypeDefs)
        {
            if (pending.macro.HasContent("Alias"))
            {
                continue;
            }

            TemplateTypeData const* pTemplateType = nullptr;
            for (auto const& templateType : m_TemplateTypes)
            {
                std::string const fullTemplateName = GetFullTypeName(templateType.type.namespaceScopeList, templateType.type.structScopeList, templateType.type.name);
                if (pending.templateTypeName == fullTemplateName || pending.templateTypeName == templateType.type.name || GetUnqualifiedTypeName(pending.templateTypeName) == templateType.type.name)
                {
                    pTemplateType = &templateType;
                    break;
                }
            }

            if (pTemplateType == nullptr)
            {
                LogError("SE_TYPEDEF target template type ({0}) was not found for typedef ({1})", pending.templateTypeName, pending.name);
                return false;
            }

            if (pending.templateArguments.size() != pTemplateType->parameterNames.size())
            {
                LogError("SE_TYPEDEF typedef ({0}) provides {1} template argument(s), but template ({2}) expects {3}",
                         pending.name,
                         pending.templateArguments.size(),
                         pTemplateType->type.name,
                         pTemplateType->parameterNames.size());
                return false;
            }

            TypeData type = pTemplateType->type;
            std::string const fullAliasName = GetFullTypeName(pending.namespaceScopeList, pending.structScopeList, pending.name);
            type.typeID = GenerateTypeID(fullAliasName);
            type.headerID = pTemplateType->type.headerID;
            type.name = pending.name;
            type.namespaceScopeList = pending.namespaceScopeList;
            type.structScopeList = pending.structScopeList;
            type.flags.SetFlag(TypeData::Flags::IsTemplate);
            type.isAPI = true;
            type.isReflect = pending.macro.hasReflect;
            if (!type.isReflect)
            {
                type.properties.clear();
            }
            type.bindingInfo.assemblyName.clear();
            type.bindingInfo.assemblyDir.clear();
            GetAssemblyInfoForHeader(type.headerID, type.bindingInfo.assemblyName, type.bindingInfo.assemblyDir);

            InflateTypeData(type, pTemplateType->parameterNames, pending.templateArguments);

            if (pDatabase->IsTypeRegistered(type.typeID))
            {
                LogError("SE_TYPEDEF typedef ({0}) generated a duplicate type ({1})", pending.name, fullAliasName);
                return false;
            }

            pDatabase->RegisterType(&type, false);
        }

        m_TypeDefs.clear();
        return true;
    }

    bool ClangParserContext::CheckForOrphanedReflectionMacros() const
    {
        ENGINE_ASSERT(!HasErrorOccured());

        bool hasOrphans = false;

        //-------------------------------------------------------------------------

        for (auto &macroHeaderPair : m_MarkMacros)
        {
            for (auto &macro : macroHeaderPair.Value)
            {
                m_errorMessage += Utils::String::Format(" TypeReflection Orphaned Macro Detected: {0}:{1}\n", macro.headerID.ToString(), macro.fileLine);
                hasOrphans = true;
            }
        }

        //-------------------------------------------------------------------------

        return hasOrphans;
    }

    void ClangParserContext::GetAssemblyInfoForHeader(HeaderID headerID, std::string& outAssemblyName, std::string& outAssemblyDir) const
    {
        if (!pSolution)
            return;
        for (auto& prj : pSolution->projects)
        {
            for (auto& hdr : prj.headerFiles)
            {
                if (hdr.headerId == headerID)
                {
                    outAssemblyName = prj.name;
                    outAssemblyDir  = prj.path;
                    return;
                }
            }
        }
    }
}

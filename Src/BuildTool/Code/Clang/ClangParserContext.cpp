#include "ClangParserContext.h"
#include "ClangTemplateTypes.h"
#include "Database/TypeDatabase.h"

#include <algorithm>

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
            bool isEscaped = false;
            for (int32 slashIndex = i - 1; slashIndex >= 0 && str[slashIndex] == '\\'; --slashIndex)
            {
                isEscaped = !isEscaped;
            }
            if (c == '"' && !isEscaped)
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

    static std::string GetMacroValue(std::string value)
    {
        Utils::String::TrimStart(value);
        Utils::String::TrimEnd(value);
        if (!Utils::String::StartsWith(value, "\"") || !Utils::String::EndsWith(value, "\"") || value.length() < 2)
        {
            return value;
        }

        std::string unescapedValue;
        unescapedValue.reserve(value.length() - 2);
        for (size_t i = 1; i + 1 < value.length(); ++i)
        {
            if (value[i] == '\\' && i + 2 < value.length())
            {
                Char const escaped = value[++i];
                switch (escaped)
                {
                case 'n': unescapedValue += '\n'; break;
                case 'r': unescapedValue += '\r'; break;
                case 't': unescapedValue += '\t'; break;
                default:  unescapedValue += escaped; break;
                }
            }
            else
            {
                unescapedValue += value[i];
            }
        }
        return unescapedValue;
    }

    static void AppendMacroMetadata(std::string& metadata, std::string const& value)
    {
        if (value.empty())
        {
            return;
        }

        if (!metadata.empty())
        {
            metadata += ", ";
        }
        metadata += value;
    }

    static std::string GetMacroAttributeValue(std::string value)
    {
        value = GetMacroValue(std::move(value));
        if (!value.empty() && !Utils::String::StartsWith(value, "["))
        {
            value.insert(0, "[");
            value += "]";
        }
        return value;
    }

    //-------------------------------------------------------------------------

    MarkToken::MarkToken(CXCursor& cursor, CXSourceRange& sourceRange)
    {
        translationUnit = clang_Cursor_getTranslationUnit(cursor);
        clang_tokenize(translationUnit, sourceRange, &tokens, &numTokens);
    }

    MarkToken::~MarkToken()
    {
        clang_disposeTokens(translationUnit, tokens, numTokens);
    }

    bool MarkToken::Next() { return ++tokenIndex < numTokens; }

    bool MarkToken::Peek(std::string& outToken) const
    {
        int32 const nextTokenIndex = tokenIndex + 1;
        if (nextTokenIndex >= numTokens)
        {
            return false;
        }

        outToken = ClangUtils::GetString(clang_getTokenSpelling(translationUnit, tokens[nextTokenIndex]));
        return true;
    }

    std::string MarkToken::Current() const
    {
        return ClangUtils::GetString(clang_getTokenSpelling(translationUnit, tokens[tokenIndex]));
    }

    static bool TryReadAssignmentValue(MarkToken& context, std::string& outValue)
    {
        if (!context.Next() || context.Current() != "=")
        {
            return false;
        }
        if (!context.Next())
        {
            return false;
        }

        outValue = GetMacroValue(context.Current());
        return true;
    }

    static std::string TryReadCallExpression(MarkToken& context, std::string firstToken)
    {
        std::string nextToken;
        if (!context.Peek(nextToken) || nextToken != "(")
        {
            return {};
        }

        context.Next();
        firstToken += "(";

        int32 depth = 1;
        while (context.Next())
        {
            std::string token = context.Current();
            firstToken += token;

            if (token == "(")
            {
                depth++;
            }
            else if (token == ")")
            {
                depth--;
                if (depth == 0)
                {
                    break;
                }
            }
        }

        return firstToken;
    }


    MarkAPI::MarkAPI(MarkToken& context)
    {
        if (!(context.Next() && context.Current() == "("))
        {
            return;
        }

        int insiderIntent = 1;

        while(context.Next())
        {
            std::string token = context.Current();
            if ( token == ",")
            {
                continue;
            }

            if ( token == "(")
            {
                insiderIntent++;
                continue;
            }

            if ( token == ")")
            {
                insiderIntent--;
                if (insiderIntent <= 0)
                {
                    break;
                }
                continue;
            }

            if (token == "Abstract")
            {
                IsAbstract = true;
            }
            else if (token == "NoConstructor")
            {
                IsNoConstructor = true;
            }
            else if (token == "NoSpawn")
            {
                IsNoSpawn = true;
            }
            else if (token == "ReadOnly")
            {
                IsReadOnly = true;
            }
            else if (token == "Sealed")
            {
                IsSealed = true;
            }
            else if (token == "Static")
            {
                IsStatic = true;
            }
            else if (token == "NoProxy")
            {
                IsNoProxy = true;
            }
            else if (token == "Prop")
            {
                IsProperty = true;
            }
            else if (token == "NativeInvokeUseName")
            {
                IsNativeInvokeUseName = true;
            }
            else if (token == "Name")
            {
                TryReadAssignmentValue(context, name);
            }
            else if (token == "Attributes")
            {
                std::string value;
                if (TryReadAssignmentValue(context, value))
                {
                    attributes = GetMacroAttributeValue(std::move(value));
                }
            }
            else if (token == "MarshalAs")
            {
                TryReadAssignmentValue(context, marshalAs);
            }
            else if (token == "InBuild")
            {
                if (context.Next() && context.Current() == "(")
                {
                    if (context.Next() && context.Current() != ")")
                    {
                        inBuildMapType = GetMacroValue(context.Current());
                        std::string nextToken;
                        if (context.Peek(nextToken) && nextToken == ")")
                        {
                            context.Next();
                        }
                    }
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    MarkMacro::MarkMacro(MarkMacro const& other) :
        headerID(other.headerID),
        fileLine(other.fileLine),
        fileColumn(other.fileColumn),
        fileEndLine(other.fileEndLine),
        fileEndColumn(other.fileEndColumn),
        type(other.type),
        macroComment(other.macroComment),
        hasReflect(other.hasReflect),
        hasTemplate(other.hasTemplate),
        isAlias(other.isAlias),
        api(other.api ? std::make_unique<MarkAPI>(*other.api) : nullptr),
        macroMetadata(other.macroMetadata)
    {}

    MarkMacro& MarkMacro::operator=(MarkMacro const& other)
    {
        if (this == &other)
        {
            return *this;
        }

        headerID          = other.headerID;
        fileLine          = other.fileLine;
        fileColumn        = other.fileColumn;
        fileEndLine       = other.fileEndLine;
        fileEndColumn     = other.fileEndColumn;
        type              = other.type;
        macroComment      = other.macroComment;
        hasReflect        = other.hasReflect;
        hasTemplate       = other.hasTemplate;
        isAlias           = other.isAlias;
        api               = other.api ? std::make_unique<MarkAPI>(*other.api) : nullptr;
        macroMetadata     = other.macroMetadata;
        return *this;
    }

    //-------------------------------------------------------------------------

    MarkMacro::MarkMacro(HeaderInfo const* pHeaderInfo,
                         CXCursor          cursor,
                         CXSourceRange&    sourceRange,
                         MacroTypeEnum     type) : headerID(pHeaderInfo->headerId), type(type)
    {
        ENGINE_ASSERT(type < MacroTypeEnum::NumMacros);

        clang_getExpansionLocation(clang_getRangeStart(sourceRange), nullptr, &fileLine, &fileColumn, nullptr);
        clang_getExpansionLocation(clang_getRangeEnd(sourceRange), nullptr, &fileEndLine, &fileEndColumn, nullptr);


        // Read the contents of the macro for all annotation types
        //-------------------------------------------------------------------------
        bool needsParamParsing =
            (type == MacroTypeEnum::SEClass || type == MacroTypeEnum::SEStruct || type == MacroTypeEnum::SEInterface ||
             type == MacroTypeEnum::SEEnum || type == MacroTypeEnum::SEField || type == MacroTypeEnum::SEFunction ||
             type == MacroTypeEnum::SEEvent || type == MacroTypeEnum::SETypeDef);

        if (needsParamParsing)
        {
            MarkToken markToken(cursor, sourceRange);

            while(markToken.Next())
            {
                std::string token = markToken.Current();
                if ( token == "," || token == "(" || token == ")")
                {
                    continue;
                }


                if (token == "Reflect")
                {
                    hasReflect = true;
                    continue;
                }

                if (token == "Template")
                {
                    hasTemplate = true;
                    continue;
                }

                if (token == "Alias")
                {
                    isAlias = true;
                    continue;
                }

                if (token == "API")
                {
                    api = std::make_unique<MarkAPI>(markToken);
                    continue;
                }

                if (token == "Meta" || token == "Metadata")
                {
                    std::string value;
                    if (TryReadAssignmentValue(markToken, value))
                    {
                        AppendMacroMetadata(macroMetadata, value);
                    }
                    continue;
                }

                AppendMacroMetadata(macroMetadata, TryReadCallExpression(markToken, token));
            }
        }
    }

    char const* MarkMacro::GetMarkMacroText(MacroTypeEnum macro)
    {
        switch (macro)
        {
            case MacroTypeEnum::SEMeta:
                return "SE_META";
            case MacroTypeEnum::SEClass:
                return "SE_CLASS";
            case MacroTypeEnum::SEStruct:
                return "SE_STRUCT";
            case MacroTypeEnum::SEInterface:
                return "SE_INTERFACE";
            case MacroTypeEnum::SEEnum:
                return "SE_ENUM";
            case MacroTypeEnum::SEField:
                return "SE_FIELD";
            case MacroTypeEnum::SEFunction:
                return "SE_FUNCTION";
            case MacroTypeEnum::SEEvent:
                return "SE_EVENT";
            case MacroTypeEnum::SETypeDef:
                return "SE_TYPEDEF";
            case MacroTypeEnum::SEInjectCode:
                return "SE_INJECT_CODE";
            case MacroTypeEnum::NumMacros:
                break;
        }
        return "";
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
        ENGINE_ASSERT(foundMacro.type != MacroTypeEnum::Unknown);

        std::vector<MarkMacro> &macrosForHeader = m_MarkMacros[foundMacro.headerID];
        macrosForHeader.push_back(foundMacro);
    }

    bool ClangParserContext::FindMarkMacro(HeaderID headerID, CXCursor const& cr, MarkMacro& macro, MacroTypeEnum macroType)
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

            bool const closesBeforeDeclaration = item.fileEndLine == line && item.fileEndColumn < column;
            bool const closesOnPreviousLine = item.fileEndLine + 1 == line;
            if (closesBeforeDeclaration || closesOnPreviousLine)
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



    void ClangParserContext::AddTemplateType(std::unique_ptr<TypeInfoStructTemplate> type)
    {
        ENGINE_ASSERT(type != nullptr);
        TemplateTypeData data;
        data.type = std::move(type);
        m_TemplateTypes.emplace_back(std::move(data));
    }

    void ClangParserContext::AddTypeDef(TypeDefData const& typeDef)
    {
        m_TypeDefs.emplace_back(typeDef);
    }

    bool ClangParserContext::ResolvePendingTypeDefs()
    {
        for (auto const& pending : m_TypeDefs)
        {
            if (pending.macro.isAlias)
            {
                continue;
            }

            TemplateTypeData const* pTemplateType = nullptr;
            for (auto const& templateType : m_TemplateTypes)
            {
                std::string const fullTemplateName = GetFullTypeName(templateType.type->namespaceScopeList, templateType.type->structScopeList, templateType.type->name);
                if (pending.targetType.name == fullTemplateName || pending.targetType.name == templateType.type->name || GetUnqualifiedTypeName(pending.targetType.name) == templateType.type->name)
                {
                    pTemplateType = &templateType;
                    break;
                }
            }

            if (pTemplateType == nullptr)
            {
                LogError("SE_TYPEDEF target template type ({0}) was not found for typedef ({1})", pending.targetType.name, pending.name);
                return false;
            }

            auto type = InstantiateTemplateType(this, *pTemplateType->type, pending);
            if (!type)
            {
                return false;
            }

            std::string const fullAliasName = GetFullTypeName(pending.namespaceScopeList, pending.structScopeList, pending.name);
            if (pDatabase->IsTypeRegistered(type->typeID))
            {
                LogError("SE_TYPEDEF typedef ({0}) generated a duplicate type ({1})", pending.name, fullAliasName);
                return false;
            }

            pDatabase->RegisterType(std::move(type), false);
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

} // namespace SE::BuildTool

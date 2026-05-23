#include "ClangParserContext.h"
#include "../ReflectorSettingsAndUtils.h"
#include "Core/Types/Collections/ListExtensions.h"

//-------------------------------------------------------------------------

namespace SE::ReflectTool
{
    static void CalculateFullNamespace(List<String> const &namespaceStack, String &fullNamespace)
    {
        fullNamespace.Clear();
        for (int i = 0; i < namespaceStack.Count(); i++)
        {
            fullNamespace.Append(namespaceStack[i]);
            if (i != namespaceStack.Count() - 1)
            {
                fullNamespace.Append(SE_TEXT("::"));
            }
        }
    }

    // TODO: Support block comments
    static String TryToParseMacro(List<StringAnsi> const &fileContents, int32 parsedMacroLineNumber)
    {
        // Clang seems to have one less line of code in its parsed data
        int32 const lineNumber = parsedMacroLineNumber - 1;

        //-------------------------------------------------------------------------

		String macroComment;

        auto SanitizeCommentString = [](String &comment)
        {
		  	comment.Replace(SE_TEXT("\r"), SE_TEXT(" "));
		  	comment.FirstTrim();
			comment.LastTrim();
        };

        // Check same line as the macro
        //-------------------------------------------------------------------------
        // This takes precedence over comments placed above

        int32 const sameLineFoundPos = fileContents[lineNumber].FindFirstOf("//");
        if (sameLineFoundPos != INVALID_INDEX)
        {
            if (!macroComment.IsEmpty())
            {
                macroComment += "\\n";
            }

            macroComment = fileContents[lineNumber].Substring(sameLineFoundPos + 2, fileContents[lineNumber].Length() - sameLineFoundPos - 2).ToString();
            SanitizeCommentString(macroComment);
            return macroComment;
        }

        // Check lines directly above the macro
        //-------------------------------------------------------------------------
        // TODO: check for other macros, etc....

        if (lineNumber > 0)
        {
            int32 const foundCommentPos = fileContents[lineNumber - 1].FindFirstOf("//");
            if (foundCommentPos != INVALID_INDEX)
            {
                macroComment = fileContents[lineNumber - 1].Substring(foundCommentPos + 2, fileContents[lineNumber - 1].Length() - foundCommentPos - 2).ToString();
                SanitizeCommentString(macroComment);
                return macroComment;
            }
        }

        //-------------------------------------------------------------------------

        return macroComment;
    }

    //-------------------------------------------------------------------------

    static void SplitRespectingBrackets(String const &str, Char delimiter, List<String> &outParts)
    {
        int32 depth = 0;
        bool inQuote = false;
        int32 start = 0;

        for (int32 i = 0; i < str.Length(); i++)
        {
            Char c = str[i];
            if (c == SE_TEXT('"'))
            {
                inQuote = !inQuote;
            }
            else if (!inQuote)
            {
                if (c == SE_TEXT('(') || c == SE_TEXT('[') || c == SE_TEXT('{'))
                {
                    depth++;
                }
                else if (c == SE_TEXT(')') || c == SE_TEXT(']') || c == SE_TEXT('}'))
                {
                    depth--;
                }
                else if (c == delimiter && depth == 0)
                {
                    outParts.Add(str.Substring(start, i - start));
                    start = i + 1;
                }
            }
        }

        if (start < str.Length())
        {
            outParts.Add(str.Substring(start, str.Length() - start));
        }
    }

    //-------------------------------------------------------------------------

    ReflectionMacro::ReflectionMacro(HeaderInfo const *pHeaderInfo, CXCursor cursor, CXSourceRange sourceRange, ReflectionMacroType type)
        : headerID(pHeaderInfo->headerId), positionStart(sourceRange.begin_int_data), positionEnd(sourceRange.end_int_data), type(type)
    {
        ENGINE_ASSERT(type < ReflectionMacroType::NumMacros);

        clang_getExpansionLocation(clang_getRangeStart(sourceRange), nullptr, &lineNumber, nullptr, nullptr);

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
                                   type == ReflectionMacroType::SEEvent);

        if (needsParamParsing)
        {
            CXToken *tokens = nullptr;
            uint32 numTokens = 0;
            CXTranslationUnit translationUnit = clang_Cursor_getTranslationUnit(cursor);
            clang_tokenize(translationUnit, sourceRange, &tokens, &numTokens);
            String rawContents;
            for (uint32 n = 0; n < numTokens; n++)
            {
                rawContents += ClangUtils::GetString(clang_getTokenSpelling(translationUnit, tokens[n]));
            }
            clang_disposeTokens(translationUnit, tokens, numTokens);

            String macroContent;
            // Extract content between parentheses
            int32 startIdx = rawContents.FindFirstOf(SE_TEXT("("));
            int32 endIdx = rawContents.FindLast(SE_TEXT(')'));
            if (startIdx != INVALID_INDEX && endIdx != INVALID_INDEX && endIdx > (startIdx + 1))
            {
                macroContent = rawContents.Substring(startIdx + 1, endIdx - startIdx - 1);
            }
            else
            {
                macroContent.Clear();
            }

            if (!macroContent.IsEmpty())
            {
                List<String> params;
                SplitRespectingBrackets(macroContent, SE_TEXT(','), params);

                for (auto &param : params)
                {
                    param.FirstTrim();
                    param.LastTrim();

                    if (param == SE_TEXT("Reflect"))
                    {
                        hasReflect = true;
                    }else if (param == SE_TEXT("API"))
                    {
                        hasAPI = true;
                    }
                    else
                    {
                        macroContents.Add(param);
                    }
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    HeaderInfo const *ClangParserContext::GetHeaderInfo(HeaderID headerID) const
    {
		Function<bool(HeaderToVisit const &)> predicate = [headerID](HeaderToVisit const & headerToVisit){ return headerToVisit.m_ID == headerID;};
		int index = ListExtensions::IndexOf(headersToVisit, predicate);
        if (index != INVALID_INDEX)
        {
            return headersToVisit[index].m_pHeaderInfo;
        }

        return nullptr;
    }

    void ClangParserContext::Reset(CXTranslationUnit *pTU)
    {
        ENGINE_ASSERT(m_namespaceStack.IsEmpty());
        ENGINE_ASSERT(m_structureStack.IsEmpty());

        pTU = pTU;
        m_InTypeReflectionMacros.Clear();
        m_TypeReflectionMacros.Clear();
        m_inEngineNamespace = false;
        m_errorMessage.Clear();
    }

    void ClangParserContext::PushNamespace(String const &name)
    {
        m_namespaceStack.Push(name);
        CalculateFullNamespace(m_namespaceStack, m_currentNamespace);

        // On Root namespace
        if (m_namespaceStack.Count() == 1)
        {
            m_inEngineNamespace = (name == Settings::g_engineNamespace);
        }
    }

    void ClangParserContext::PopNamespace()
    {
        m_namespaceStack.Pop();
        CalculateFullNamespace(m_namespaceStack, m_currentNamespace);

        // Reset engine namespace flag
        if (m_namespaceStack.IsEmpty())
        {
            m_inEngineNamespace = false;
        }
    }

    bool ClangParserContext::SetModuleClassName(StringView const &headerFilePath, String const &moduleClassName)
    {
        for (auto &prj : pSolution->projects)
        {
            if (FileSystem::IsUnderDirectory(headerFilePath, prj.path))
            {
                ENGINE_ASSERT(prj.moduleClassNameFull.IsEmpty());
                prj.moduleClassNameFull = moduleClassName;

                List<String> sp;
				moduleClassName.Split(SE_TEXT("::"), sp);
                if(sp.Count() > 1)
                {
                    prj.moduleClassName = sp[sp.Count() - 1];
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

    bool ClangParserContext::IsEngineNamespace(String const &namespaceString) const
    {
        return namespaceString == Settings::g_engineNamespace;
    }

    //-------------------------------------------------------------------------

    void ClangParserContext::AddFoundReflectionMacro(ReflectionMacro const &foundMacro)
    {
        ENGINE_ASSERT(foundMacro.headerID != StringID::Invalid);
        ENGINE_ASSERT(foundMacro.type != ReflectionMacroType::Unknown);

        // Property/member level macros
        if (foundMacro.type == ReflectionMacroType::SEProperty
            || foundMacro.type == ReflectionMacroType::SEFunction
            || foundMacro.type == ReflectionMacroType::SEEvent)
        {
            List<ReflectionMacro> &macrosForHeader = m_InTypeReflectionMacros[foundMacro.headerID];
            macrosForHeader.Add(foundMacro);
        }
        else // Type-level macros (DefineClass, DefineClassDefault, ReflectModule, SEClass, SEStruct, SEInterface, SEEnum, ReflectMeta)
        {
            List<ReflectionMacro> &macrosForHeader = m_TypeReflectionMacros[foundMacro.headerID];
            macrosForHeader.Add(foundMacro);
        }
    }

    bool ClangParserContext::FindReflectionMacroForEnum(HeaderID headerID, CXCursor const &cr, ReflectionMacro &macro)
    {
        // Try get macros for this header
        //-------------------------------------------------------------------------
        auto headerIter = m_TypeReflectionMacros.Find(headerID);
        if (headerIter == m_TypeReflectionMacros.end())
        {
            return false;
        }

        List<ReflectionMacro> &macrosForHeader = headerIter->Value;
        uint32_t const declBeginPosition = ClangUtils::GetStartPositionForCursor(cr);

        // Look for SE_ENUM with hasReflect == true
        //-------------------------------------------------------------------------
        int bestIndex = -1;
        uint32_t bestDistance = UINT32_MAX;

        for (int index = 0; index < macrosForHeader.Count(); index++)
        {
            auto &item = macrosForHeader[index];

            if (item.type != ReflectionMacroType::SEEnum || !item.hasReflect)
                continue;

            // Macro must be before the declaration
            if (item.positionEnd > declBeginPosition)
                continue;

            uint32_t distance = declBeginPosition - item.positionEnd;
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestIndex = index;
            }
        }

        if (bestIndex >= 0)
        {
            macro = macrosForHeader[bestIndex];
            macrosForHeader.RemoveAt(bestIndex);
            return true;
        }

        return false;
    }

    bool ClangParserContext::FindReflectionMacroForType(HeaderID headerID, CXCursor const& cr, ReflectionMacro& macro)
    {
        // Try get macros for this header
        //-------------------------------------------------------------------------

        auto headerIter = m_TypeReflectionMacros.Find(headerID);
        if (headerIter == m_TypeReflectionMacros.end())
        {
            return false;
        }

        List<ReflectionMacro> &macrosForHeader = headerIter->Value;
        uint32_t const declBeginPosition = ClangUtils::GetStartPositionForCursor(cr);

        // Look for SE_CLASS that is closest before the declaration
        //-------------------------------------------------------------------------
        int bestIndex = -1;
        uint32_t bestDistance = UINT32_MAX;

        for (int index = 0; index < macrosForHeader.Count(); index++)
        {
            auto& item = macrosForHeader[index];

            if (item.type != ReflectionMacroType::SEClass)
                continue;

            // Macro must be before the declaration
            if (item.positionEnd > declBeginPosition)
                continue;

            uint32_t distance = declBeginPosition - item.positionEnd;
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestIndex = index;
            }
        }

        if (bestIndex >= 0)
        {
            macro = macrosForHeader[bestIndex];
            macrosForHeader.RemoveAt(bestIndex);
            return true;
        }

        return false;
    }

    bool ClangParserContext::FindReflectionMacroForProperty(HeaderID headerID, uint32_t declStartPosition, ReflectionMacro &reflectionMacro)
    {
        // Try get macros for this header
        //-------------------------------------------------------------------------
        auto headerIter = m_InTypeReflectionMacros.Find(headerID);
        if (headerIter == m_InTypeReflectionMacros.end())
        {
            return false;
        }

        List<ReflectionMacro> &macrosForHeader = headerIter->Value;

        // Look for SE_PROPERTY with hasReflect == true, nearest to the declaration
        int bestIndex = -1;
        uint32_t bestDistance = UINT32_MAX;

        for (int index = 0; index < macrosForHeader.Count(); index++)
        {
            auto &item = macrosForHeader[index];

            if (item.type == ReflectionMacroType::SEProperty)
            {
                // Macro must be before the declaration
                if (item.positionEnd > declStartPosition)
                {
                    continue;
                }

                uint32_t distance = declStartPosition - item.positionEnd;
                if (distance < bestDistance)
                {
                    bestDistance = distance;
                    bestIndex = index;
                }
            }
        }

        if (bestIndex >= 0)
        {
            reflectionMacro = macrosForHeader[bestIndex];
            macrosForHeader.RemoveAt(bestIndex);
            return true;
        }

        return false;
    }

    bool ClangParserContext::FindReflectionMacroForMeta(HeaderID headerID, uint32_t declStartPosition, ReflectionMacro& reflectionMacro)
    {
        // Try get macros for this header
        //-------------------------------------------------------------------------
        auto headerIter = m_TypeReflectionMacros.Find(headerID);
        if (headerIter == m_TypeReflectionMacros.end())
        {
            return false;
        }

        List<ReflectionMacro> &macrosForHeader = headerIter->Value;
        return FindReflectionMacro(macrosForHeader, headerID, declStartPosition, ReflectionMacroType::ReflectMeta, reflectionMacro);
    }

    bool ClangParserContext::FindReflectionMacro(List<ReflectionMacro> &macrosForHeader, HeaderID headerID, uint32 declStartPosition, ReflectionMacroType macroType, ReflectionMacro& reflectionMacro)
    {
        // Find the nearest macro of the given type that appears before the declaration
        //-------------------------------------------------------------------------
        int bestIndex = -1;
        uint32_t bestDistance = UINT32_MAX;

        for (int index = 0; index < macrosForHeader.Count(); index++)
        {
            auto& iter = macrosForHeader[index];

            if (iter.type != macroType)
            {
                continue;
            }

            // Macro must be before the declaration
            if (iter.positionEnd > declStartPosition)
                continue;

            uint32_t distance = declStartPosition - iter.positionEnd;
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestIndex = index;
            }
        }

        if (bestIndex < 0)
        {
            return false;
        }

        reflectionMacro = macrosForHeader[bestIndex];
        macrosForHeader.RemoveAt(bestIndex);
        return true;
    }

    bool ClangParserContext::CheckForOrphanedReflectionMacros() const
    {
        ENGINE_ASSERT(!HasErrorOccured());

        bool hasOrphans = false;

        //-------------------------------------------------------------------------

        for (auto &macroHeaderPair : m_TypeReflectionMacros)
        {
            for (auto &macro : macroHeaderPair.Value)
            {
                m_errorMessage += StringAnsi::Format(" TypeReflection Orphaned Macro Detected: {0}:{1}\n", macro.headerID.ToString(), macro.lineNumber);
                hasOrphans = true;
            }
        }

        //-------------------------------------------------------------------------

        for (auto &macroHeaderPair : m_InTypeReflectionMacros)
        {
            for (auto &macro : macroHeaderPair.Value)
            {
                m_errorMessage += StringAnsi::Format(" PropertyReflection Orphaned Macro Detected: {0}:{1}\n", macro.headerID.ToString(), macro.lineNumber);
                hasOrphans = true;
            }
        }

        //-------------------------------------------------------------------------

        return hasOrphans;
    }

    // -------------------------------------------------------------------------
    // Bindings macro lookup helpers
    // -------------------------------------------------------------------------

    bool ClangParserContext::FindBindingMacroForType(HeaderID headerID, CXCursor const& cr, ReflectionMacro& macro)
    {
        auto headerIter = m_TypeReflectionMacros.Find(headerID);
        if (headerIter == m_TypeReflectionMacros.end())
            return false;

        List<ReflectionMacro>& macrosForHeader = headerIter->Value;
        uint32_t const declBeginPosition = ClangUtils::GetStartPositionForCursor(cr);

        // Look for SE_CLASS/SE_STRUCT/SE_INTERFACE with hasAPI that is closest before the declaration
        //-------------------------------------------------------------------------
        int bestIndex = -1;
        uint32_t bestDistance = UINT32_MAX;

        for (int index = 0; index < macrosForHeader.Count(); index++)
        {
            auto& item = macrosForHeader[index];

            if ((item.type != ReflectionMacroType::SEClass &&
                 item.type != ReflectionMacroType::SEStruct &&
                 item.type != ReflectionMacroType::SEInterface) || !item.hasAPI)
                continue;

            // Macro must be before the declaration
            if (item.positionEnd > declBeginPosition)
                continue;

            uint32_t distance = declBeginPosition - item.positionEnd;
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestIndex = index;
            }
        }

        if (bestIndex >= 0)
        {
            macro = macrosForHeader[bestIndex];
            macrosForHeader.RemoveAt(bestIndex);
            return true;
        }

        return false;
    }

    bool ClangParserContext::FindBindingMacroForEnum(HeaderID headerID, CXCursor const& cr, ReflectionMacro& macro)
    {
        auto headerIter = m_TypeReflectionMacros.Find(headerID);
        if (headerIter == m_TypeReflectionMacros.end())
            return false;

        List<ReflectionMacro>& macrosForHeader = headerIter->Value;
        uint32_t const declBeginPosition = ClangUtils::GetStartPositionForCursor(cr);

        // Look for SE_ENUM with hasAPI == true, nearest to the declaration
        int bestIndex = -1;
        uint32_t bestDistance = UINT32_MAX;

        for (int index = 0; index < macrosForHeader.Count(); index++)
        {
            auto &item = macrosForHeader[index];

            if (item.type != ReflectionMacroType::SEEnum || !item.hasAPI)
                continue;

            // Macro must be before the declaration
            if (item.positionEnd > declBeginPosition)
                continue;

            uint32_t distance = declBeginPosition - item.positionEnd;
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestIndex = index;
            }
        }

        if (bestIndex >= 0)
        {
            macro = macrosForHeader[bestIndex];
            macrosForHeader.RemoveAt(bestIndex);
            return true;
        }

        return false;
    }

    bool ClangParserContext::FindBindingMacroForMember(HeaderID headerID, uint32_t declStartPosition, ReflectionMacroType memberType, ReflectionMacro& macro)
    {
        auto headerIter = m_InTypeReflectionMacros.Find(headerID);
        if (headerIter == m_InTypeReflectionMacros.end())
            return false;

        List<ReflectionMacro>& macrosForHeader = headerIter->Value;

        // Look for matching SE_PROPERTY/SE_FUNCTION/SE_EVENT with hasAPI, nearest to the declaration
        int bestIndex = -1;
        uint32_t bestDistance = UINT32_MAX;

        for (int index = 0; index < macrosForHeader.Count(); index++)
        {
            auto &item = macrosForHeader[index];

            if (item.type != memberType || !item.hasAPI)
                continue;

            // Macro must be before the declaration
            if (item.positionEnd > declStartPosition)
                continue;

            uint32_t distance = declStartPosition - item.positionEnd;
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestIndex = index;
            }
        }

        if (bestIndex >= 0)
        {
            macro = macrosForHeader[bestIndex];
            macrosForHeader.RemoveAt(bestIndex);
            return true;
        }

        return false;
    }

    void ClangParserContext::GetAssemblyInfoForHeader(HeaderID headerID, StringAnsi& outAssemblyName, StringAnsi& outAssemblyDir) const
    {
        if (!pSolution)
            return;
        for (auto& prj : pSolution->projects)
        {
            for (auto& hdr : prj.headerFiles)
            {
                if (hdr.headerId == headerID)
                {
                    outAssemblyName = prj.name.ToStringAnsi();
                    outAssemblyDir  = prj.path.ToStringAnsi();
                    return;
                }
            }
        }
    }
}
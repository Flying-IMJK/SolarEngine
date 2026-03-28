#include "ClangParserContext.h"
#include "../ReflectorSettingsAndUtils.h"
#include "Core/Types/Collections/ListExtensions.h"

//-------------------------------------------------------------------------

namespace SE::ReflectTool
{
    static void CalculateFullNamespace(List<String> const &namespaceStack, String &fullNamespace)
    {
        fullNamespace.Clear();
        for (auto &str : namespaceStack)
        {
            fullNamespace.Append(str);
            fullNamespace.Append(SE_TEXT("::"));
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

    ReflectionMacro::ReflectionMacro(HeaderInfo const *pHeaderInfo, CXCursor cursor, CXSourceRange sourceRange, ReflectionMacroType type)
        : headerID(pHeaderInfo->headerId), type(type), positionStart(sourceRange.begin_int_data), positionEnd(sourceRange.end_int_data)
    {
        ENGINE_ASSERT(type < ReflectionMacroType::NumMacros);

        clang_getExpansionLocation(clang_getRangeStart(sourceRange), nullptr, &lineNumber, nullptr, nullptr);

        //-------------------------------------------------------------------------

        macroComment = TryToParseMacro(pHeaderInfo->fileContents, lineNumber);

        //-------------------------------------------------------------------------

        if (type == ReflectionMacroType::ReflectProperty)
        {
            // Read the contents of the macro
            //-------------------------------------------------------------------------
            CXToken *tokens = nullptr;
            uint32 numTokens = 0;
            CXTranslationUnit translationUnit = clang_Cursor_getTranslationUnit(cursor);
            clang_tokenize(translationUnit, sourceRange, &tokens, &numTokens);
            for (uint32 n = 0; n < numTokens; n++)
            {
                macroContents += ClangUtils::GetString(clang_getTokenSpelling(translationUnit, tokens[n]));
            }
            clang_disposeTokens(translationUnit, tokens, numTokens);

            // Check if we have a metadata macro
            //-------------------------------------------------------------------------

            int32 startIdx = macroContents.FindFirstOf(SE_TEXT("("));
            int32 endIdx = macroContents.FindLast(SE_TEXT(')'));
            if (startIdx != INVALID_INDEX && endIdx != INVALID_INDEX && endIdx > (startIdx + 1))
            {
                // Property macro contents are JSON, so apply some enclosing formatting
                if (type == ReflectionMacroType::ReflectProperty)
                {
                    macroContents = macroContents.Substring(startIdx + 1, endIdx - startIdx - 1);
                }
                else // Just keep the contents without the braces
                {
                    macroContents = macroContents.Substring(startIdx + 1, endIdx - startIdx - 1);
                }
            }
            else
            {
                macroContents.Clear();
            }
        }
    }

    //-------------------------------------------------------------------------

    HeaderInfo const *ClangParserContext::GetHeaderInfo(HeaderID headerID) const
    {
		Function<bool(HeaderToVisit const &)> predicate = [headerID](HeaderToVisit const & headerToVisit){ return headerToVisit.m_ID == headerID;};
		int index = ListExtensions::IndexOf(m_headersToVisit, predicate);
        if (index != INVALID_INDEX)
        {
            return m_headersToVisit[index].m_pHeaderInfo;
        }

        return nullptr;
    }

    void ClangParserContext::Reset(CXTranslationUnit *pTU)
    {
        ENGINE_ASSERT(m_namespaceStack.IsEmpty());
        ENGINE_ASSERT(m_structureStack.IsEmpty());

        m_pTU = pTU;
        m_propertyReflectionMacros.Clear();
        m_typeReflectionMacros.Clear();
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
        for (auto &prj : m_pSolution->projects)
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

        if (foundMacro.type == ReflectionMacroType::ReflectProperty)
        {
            List<ReflectionMacro> &macrosForHeader = m_propertyReflectionMacros[foundMacro.headerID];
            macrosForHeader.Add(foundMacro);
        }
        else // All other types
        {
            List<ReflectionMacro> &macrosForHeader = m_typeReflectionMacros[foundMacro.headerID];
            macrosForHeader.Add(foundMacro);
        }
    }

    bool ClangParserContext::FindReflectionMacroForEnum(HeaderID headerID, CXCursor const &cr, int lineNumber, ReflectionMacro &macro)
    {
        // Try get macros for this header
        //-------------------------------------------------------------------------
        auto headerIter = m_typeReflectionMacros.Find(headerID);
        if (headerIter == m_typeReflectionMacros.end())
        {
            return false;
        }

        List<ReflectionMacro> &macrosForHeader = headerIter->Value;
        return FindReflectionMacro(macrosForHeader, headerID, lineNumber, ReflectionMacroType::ReflectEnum, macro);
    }

    bool ClangParserContext::FindReflectionMacroForType(HeaderID headerID, CXCursor const& cr, ReflectionMacro& macro)
    {
        // Try get macros for this header
        //-------------------------------------------------------------------------

        auto headerIter = m_typeReflectionMacros.Find(headerID);
        if (headerIter == m_typeReflectionMacros.end())
        {
            return false;
        }

        List<ReflectionMacro> &macrosForHeader = headerIter->Value;

        // Check the header macros
        //-------------------------------------------------------------------------

        CXSourceRange const typeRange = clang_getCursorExtent(cr);

        for (int index = 0; index < macrosForHeader.Count(); index++)
        {
            auto item = macrosForHeader.At(index);
            bool macroWithinCursorExtents = item.positionEnd == (typeRange.begin_int_data - 1);
            macroWithinCursorExtents |= item.positionStart > typeRange.begin_int_data && item.positionStart < typeRange.end_int_data;

            if (macroWithinCursorExtents)
            {
                macro = item;
                macrosForHeader.RemoveAt(index);
                return true;
            }
        }

        return false;
    }

    bool ClangParserContext::FindReflectionMacroForProperty(HeaderID headerID, uint32 lineNumber, ReflectionMacro &reflectionMacro)
    {
        // Try get macros for this header
        //-------------------------------------------------------------------------
        auto headerIter = m_propertyReflectionMacros.Find(headerID);
        if (headerIter == m_propertyReflectionMacros.end())
        {
            return false;
        }

        List<ReflectionMacro> &macrosForHeader = headerIter->Value;
        return FindReflectionMacro(macrosForHeader, headerID, lineNumber, ReflectionMacroType::ReflectProperty, reflectionMacro);
    }

    bool ClangParserContext::FindReflectionMacroForMeta(HeaderID headerID, uint32_t lineNumber, ReflectionMacro& reflectionMacro)
    {
        // Try get macros for this header
        //-------------------------------------------------------------------------
        auto headerIter = m_typeReflectionMacros.Find(headerID);
        if (headerIter == m_typeReflectionMacros.end())
        {
            return false;
        }

        List<ReflectionMacro> &macrosForHeader = headerIter->Value;
        return FindReflectionMacro(macrosForHeader, headerID, lineNumber, ReflectionMacroType::ReflectMeta, reflectionMacro);
    }

    bool ClangParserContext::FindReflectionMacro(List<ReflectionMacro> &macrosForHeader, HeaderID headerID, uint32 lineNumber, ReflectionMacroType macroType, ReflectionMacro& reflectionMacro)
    {
        // Try to find the macro
        //-------------------------------------------------------------------------
        int foundMacros[2] = {macrosForHeader.Count(), macrosForHeader.Count()};

        bool hasMacroOnLineAbove = false;
        bool hasMacroOnSameLine = false;

        for (int index = 0; index < macrosForHeader.Count(); index++)
        {
            auto& iter = macrosForHeader[index];

            if (iter.type != macroType)
            {
                continue;
            }

            if (iter.lineNumber == lineNumber - 1)
            {
                foundMacros[0] = index;
                hasMacroOnLineAbove = true;
            }
            else if (iter.lineNumber == lineNumber)
            {
                foundMacros[1] = index;
                hasMacroOnSameLine = true;
                // Macros on the same line take priority!
                break;
            }
        }

        // If we have a macro on the same line as well as the line above this is a mistake in the source file
        if (hasMacroOnLineAbove && hasMacroOnSameLine)
        {
            LogError(SE_TEXT("Multiple reflection macros detected on line {0} in file: {1}"), lineNumber, headerID.ToString());
            return false;
        }

        // We didnt find a macro
        if (!hasMacroOnLineAbove && !hasMacroOnSameLine)
        {
            return false;
        }

        int foundMacroIter = hasMacroOnLineAbove ? foundMacros[0] : foundMacros[1];
        reflectionMacro = macrosForHeader[foundMacroIter];
        macrosForHeader.RemoveAt(foundMacroIter);
        return true;
    }

    bool ClangParserContext::CheckForOrphanedReflectionMacros() const
    {
        ENGINE_ASSERT(!HasErrorOccured());

        bool hasOrphans = false;

        //-------------------------------------------------------------------------

        for (auto &macroHeaderPair : m_typeReflectionMacros)
        {
            for (auto &macro : macroHeaderPair.Value)
            {
                m_errorMessage += StringAnsi::Format(" TypeReflection Orphaned Macro Detected: {0}:{1}\n", macro.headerID.ToString(), macro.lineNumber);
                hasOrphans = true;
            }
        }

        //-------------------------------------------------------------------------

        for (auto &macroHeaderPair : m_propertyReflectionMacros)
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
}

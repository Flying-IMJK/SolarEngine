#include "ClangVisitors_Macro.h"
#include "Core/Utils.h"
#include "clang-c/Documentation.h"
#include <Database/TypeDatabase.h>

#include <memory>

//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    CXChildVisitResult VisitMacro(ClangParserContext *pContext, HeaderInfo const *pHeaderInfo, CXCursor cr, std::string const &cursorName)
    {
        CXSourceRange range = clang_getCursorExtent(cr);

        if (cursorName == MarkMacro::GetMarkMacroText(MacroTypeEnum::SEMeta))
        {
            pContext->AddMarkMacro(MarkMacro(pHeaderInfo, cr, range, MacroTypeEnum::SEMeta));
        }

        //-------------------------------------------------------------------------
        // Unified annotation macros (parsed for Reflect/API(...) parameters)

        else if (cursorName == MarkMacro::GetMarkMacroText(MacroTypeEnum::SEClass))
        {
            pContext->AddMarkMacro(MarkMacro(pHeaderInfo, cr, range, MacroTypeEnum::SEClass));
        }
        else if (cursorName == MarkMacro::GetMarkMacroText(MacroTypeEnum::SEStruct))
        {
            pContext->AddMarkMacro(MarkMacro(pHeaderInfo, cr, range, MacroTypeEnum::SEStruct));
        }
        else if (cursorName == MarkMacro::GetMarkMacroText(MacroTypeEnum::SEInterface))
        {
            pContext->AddMarkMacro(MarkMacro(pHeaderInfo, cr, range, MacroTypeEnum::SEInterface));
        }
        else if (cursorName == MarkMacro::GetMarkMacroText(MacroTypeEnum::SEEnum))
        {
            pContext->AddMarkMacro(MarkMacro(pHeaderInfo, cr, range, MacroTypeEnum::SEEnum));
        }
        else if (cursorName == MarkMacro::GetMarkMacroText(MacroTypeEnum::SEFunction))
        {
            pContext->AddMarkMacro(MarkMacro(pHeaderInfo, cr, range, MacroTypeEnum::SEFunction));
        }
        else if (cursorName == MarkMacro::GetMarkMacroText(MacroTypeEnum::SEField))
        {
            pContext->AddMarkMacro(MarkMacro(pHeaderInfo, cr, range, MacroTypeEnum::SEField));
        }
        else if (cursorName == MarkMacro::GetMarkMacroText(MacroTypeEnum::SEEvent))
        {
            pContext->AddMarkMacro(MarkMacro(pHeaderInfo, cr, range, MacroTypeEnum::SEEvent));
        }
        else if (cursorName == MarkMacro::GetMarkMacroText(MacroTypeEnum::SETypeDef))
        {
            pContext->AddMarkMacro(MarkMacro(pHeaderInfo, cr, range, MacroTypeEnum::SETypeDef));
        }
        else if (cursorName == MarkMacro::GetMarkMacroText(MacroTypeEnum::SEInjectCode))
        {
            CXToken*          tokens          = nullptr;
            uint32            numTokens       = 0;
            CXTranslationUnit translationUnit = clang_Cursor_getTranslationUnit(cr);
            clang_tokenize(translationUnit, range, &tokens, &numTokens);

            std::string rawContents;

            std::string inject  = ClangUtils::GetString(clang_getTokenSpelling(translationUnit, tokens[2]));
            std::string content = ClangUtils::GetString(clang_getTokenSpelling(translationUnit, tokens[4]));

            auto injectCode = std::make_unique<TypeInfoInjectedCode>();

            if (inject == "cpp")
            {
                injectCode->lang = InjectEnum::CPP;
            }
            else if (inject == "csharp")
            {
                injectCode->lang = InjectEnum::CS;
            }
            else
            {
                pContext->LogError("error injectCode type");
                return CXChildVisit_Break;
            }

            injectCode->code  = content;
            injectCode->headID = pHeaderInfo->headerId;

            pContext->pDatabase->RegisterInjectCode(std::move(injectCode));
        }
        //-------------------------------------------------------------------------

        return CXChildVisit_Continue;
    }
}

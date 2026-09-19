#include "ClangVisitors_Macro.h"
#include "Core/Utils.h"
#include "clang-c/Documentation.h"
#include <Database/TypeDatabase.h>

#include <memory>
#include <string_view>

//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    static int32 HexDigitValue(Char c)
    {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }

    static bool DecodeInjectCodeLiteral(std::string_view literal, std::string& decoded)
    {
        if (literal.size() < 2 || literal.front() != '"' || literal.back() != '"')
        {
            return false;
        }

        decoded.clear();
        decoded.reserve(literal.size() - 2);
        for (size_t i = 1; i + 1 < literal.size(); ++i)
        {
            Char c = literal[i];
            if (c != '\\')
            {
                decoded += c;
                continue;
            }

            if (++i + 1 >= literal.size())
            {
                return false;
            }

            c = literal[i];
            switch (c)
            {
                case '\'': decoded += '\''; break;
                case '"':  decoded += '"'; break;
                case '?':  decoded += '?'; break;
                case '\\': decoded += '\\'; break;
                case 'a':  decoded += '\a'; break;
                case 'b':  decoded += '\b'; break;
                case 'f':  decoded += '\f'; break;
                case 'n':  decoded += '\n'; break;
                case 'r':  decoded += '\r'; break;
                case 't':  decoded += '\t'; break;
                case 'v':  decoded += '\v'; break;
                case 'x':
                {
                    uint32 value = 0;
                    bool hasDigit = false;
                    while (i + 2 < literal.size())
                    {
                        int32 digit = HexDigitValue(literal[i + 1]);
                        if (digit < 0) break;
                        hasDigit = true;
                        value = value * 16 + digit;
                        if (value > 0xFF) return false;
                        ++i;
                    }
                    if (!hasDigit) return false;
                    decoded += static_cast<Char>(value);
                    break;
                }
                default:
                {
                    if (c < '0' || c > '7') return false;
                    uint32 value = c - '0';
                    for (int32 digitIndex = 0; digitIndex < 2 && i + 2 < literal.size(); ++digitIndex)
                    {
                        Char next = literal[i + 1];
                        if (next < '0' || next > '7') break;
                        value = value * 8 + (next - '0');
                        ++i;
                    }
                    if (value > 0xFF) return false;
                    decoded += static_cast<Char>(value);
                    break;
                }
            }
        }
        return true;
    }

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

            if (numTokens < 6)
            {
                clang_disposeTokens(translationUnit, tokens, numTokens);
                pContext->LogError("error SE_INJECT_CODE arguments");
                return CXChildVisit_Break;
            }

            std::string inject  = ClangUtils::GetString(clang_getTokenSpelling(translationUnit, tokens[2]));
            std::string content = ClangUtils::GetString(clang_getTokenSpelling(translationUnit, tokens[4]));
            std::string closing = ClangUtils::GetString(clang_getTokenSpelling(translationUnit, tokens[5]));
            clang_disposeTokens(translationUnit, tokens, numTokens);

            if (closing != ")")
            {
                pContext->LogError("error SE_INJECT_CODE content must be one string literal");
                return CXChildVisit_Break;
            }

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

            if (!DecodeInjectCodeLiteral(content, injectCode->code))
            {
                pContext->LogError("error SE_INJECT_CODE content is not a supported string literal");
                return CXChildVisit_Break;
            }

            injectCode->headID = pHeaderInfo->headerId;

            pContext->pDatabase->RegisterInjectCode(std::move(injectCode));
        }
        //-------------------------------------------------------------------------

        return CXChildVisit_Continue;
    }
}

#include "ClangVisitors_Macro.h"
#include "../ReflectorSettingsAndUtils.h"
#include "clang-c/Documentation.h"

//-------------------------------------------------------------------------

namespace SE::ReflectTool
{
    CXChildVisitResult VisitMacro(ClangParserContext *pContext, HeaderInfo const *pHeaderInfo, CXCursor cr, String const &cursorName)
    {
        CXSourceRange range = clang_getCursorExtent(cr);

        //-------------------------------------------------------------------------
        // Code-injecting macros (always recorded)

        /*if (cursorName == GetReflectionMacroText(ReflectionMacroType::DefineClass))
        {
            pContext->AddFoundReflectionMacro(ReflectionMacro(pHeaderInfo, cr, range, ReflectionMacroType::DefineClass));
        }
        else if (cursorName == GetReflectionMacroText(ReflectionMacroType::DefineClassDefault))
        {
            pContext->AddFoundReflectionMacro(ReflectionMacro(pHeaderInfo, cr, range, ReflectionMacroType::DefineClassDefault));
        }
        else if (cursorName == GetReflectionMacroText(ReflectionMacroType::ReflectModule))
        {
            pContext->AddFoundReflectionMacro(ReflectionMacro(pHeaderInfo, cr, range, ReflectionMacroType::ReflectModule));
        }
        else */if (cursorName == GetMarkMacroText(ReflectionMacroType::ReflectMeta))
        {
            pContext->AddMarkMacro(MarkMacro(pHeaderInfo, cr, range, ReflectionMacroType::ReflectMeta));
        }

        //-------------------------------------------------------------------------
        // Unified annotation macros (parsed for Reflect/API parameters)

        else if (cursorName == GetMarkMacroText(ReflectionMacroType::SEClass))
        {
            pContext->AddMarkMacro(MarkMacro(pHeaderInfo, cr, range, ReflectionMacroType::SEClass));
        }
        else if (cursorName == GetMarkMacroText(ReflectionMacroType::SEStruct))
        {
            pContext->AddMarkMacro(MarkMacro(pHeaderInfo, cr, range, ReflectionMacroType::SEStruct));
        }
        else if (cursorName == GetMarkMacroText(ReflectionMacroType::SEInterface))
        {
            pContext->AddMarkMacro(MarkMacro(pHeaderInfo, cr, range, ReflectionMacroType::SEInterface));
        }
        else if (cursorName == GetMarkMacroText(ReflectionMacroType::SEEnum))
        {
            pContext->AddMarkMacro(MarkMacro(pHeaderInfo, cr, range, ReflectionMacroType::SEEnum));
        }
        else if (cursorName == GetMarkMacroText(ReflectionMacroType::SEProperty))
        {
            pContext->AddMarkMacro(MarkMacro(pHeaderInfo, cr, range, ReflectionMacroType::SEProperty));
        }
        else if (cursorName == GetMarkMacroText(ReflectionMacroType::SEFunction))
        {
            pContext->AddMarkMacro(MarkMacro(pHeaderInfo, cr, range, ReflectionMacroType::SEFunction));
        }
        else if (cursorName == GetMarkMacroText(ReflectionMacroType::SEEvent))
        {
            pContext->AddMarkMacro(MarkMacro(pHeaderInfo, cr, range, ReflectionMacroType::SEEvent));
        }

        //-------------------------------------------------------------------------

        return CXChildVisit_Continue;
    }
}
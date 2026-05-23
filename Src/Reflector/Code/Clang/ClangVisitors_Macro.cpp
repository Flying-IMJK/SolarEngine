#include "ClangVisitors.h"
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
        else */if (cursorName == GetReflectionMacroText(ReflectionMacroType::ReflectMeta))
        {
            pContext->AddFoundReflectionMacro(ReflectionMacro(pHeaderInfo, cr, range, ReflectionMacroType::ReflectMeta));
        }

        //-------------------------------------------------------------------------
        // Unified annotation macros (parsed for Reflect/API parameters)

        else if (cursorName == GetReflectionMacroText(ReflectionMacroType::SEClass))
        {
            pContext->AddFoundReflectionMacro(ReflectionMacro(pHeaderInfo, cr, range, ReflectionMacroType::SEClass));
        }
        else if (cursorName == GetReflectionMacroText(ReflectionMacroType::SEStruct))
        {
            pContext->AddFoundReflectionMacro(ReflectionMacro(pHeaderInfo, cr, range, ReflectionMacroType::SEStruct));
        }
        else if (cursorName == GetReflectionMacroText(ReflectionMacroType::SEInterface))
        {
            pContext->AddFoundReflectionMacro(ReflectionMacro(pHeaderInfo, cr, range, ReflectionMacroType::SEInterface));
        }
        else if (cursorName == GetReflectionMacroText(ReflectionMacroType::SEEnum))
        {
            pContext->AddFoundReflectionMacro(ReflectionMacro(pHeaderInfo, cr, range, ReflectionMacroType::SEEnum));
        }
        else if (cursorName == GetReflectionMacroText(ReflectionMacroType::SEProperty))
        {
            pContext->AddFoundReflectionMacro(ReflectionMacro(pHeaderInfo, cr, range, ReflectionMacroType::SEProperty));
        }
        else if (cursorName == GetReflectionMacroText(ReflectionMacroType::SEFunction))
        {
            pContext->AddFoundReflectionMacro(ReflectionMacro(pHeaderInfo, cr, range, ReflectionMacroType::SEFunction));
        }
        else if (cursorName == GetReflectionMacroText(ReflectionMacroType::SEEvent))
        {
            pContext->AddFoundReflectionMacro(ReflectionMacro(pHeaderInfo, cr, range, ReflectionMacroType::SEEvent));
        }

        //-------------------------------------------------------------------------

        return CXChildVisit_Continue;
    }
}
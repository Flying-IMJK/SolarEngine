#include "ClangVisitors_Macro.h"
#include "../ReflectorSettingsAndUtils.h"
#include "clang-c/Documentation.h"
#include <iostream>

//-------------------------------------------------------------------------

namespace SE::ReflectTool
{
    CXChildVisitResult VisitMacro(ClangParserContext *pContext, HeaderInfo const *pHeaderInfo, CXCursor cr, String const &cursorName)
    {
        CXSourceRange range = clang_getCursorExtent(cr);

        //-------------------------------------------------------------------------

        if (cursorName.StartsWith(GetReflectionMacroText(ReflectionMacroType::ReflectProperty)))
        {
            pContext->AddFoundReflectionMacro(ReflectionMacro(pHeaderInfo, cr, range, ReflectionMacroType::ReflectProperty));
        }
        else if (cursorName.StartsWith(GetReflectionMacroText(ReflectionMacroType::ReflectEnum)))
        {
            pContext->AddFoundReflectionMacro(ReflectionMacro(pHeaderInfo, cr, range, ReflectionMacroType::ReflectEnum));
        }
        else if (cursorName.StartsWith(GetReflectionMacroText(ReflectionMacroType::ReflectType)))
        {
            pContext->AddFoundReflectionMacro(ReflectionMacro(pHeaderInfo, cr, range, ReflectionMacroType::ReflectType));
        }
        else if (cursorName.StartsWith(GetReflectionMacroText(ReflectionMacroType::ReflectMeta)))
        {
            pContext->AddFoundReflectionMacro(ReflectionMacro(pHeaderInfo, cr, range, ReflectionMacroType::ReflectMeta));
        }

        //-------------------------------------------------------------------------

        return CXChildVisit_Continue;
    }
}
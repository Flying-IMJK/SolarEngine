#pragma once

#pragma warning(disable : 4714 4267 4244 4512 4800 4389 4146 4291 4324 4458 4141 4459 4245 4624 4996)

#include <clang/AST/Ast.h>
#include <clang/AST/Type.h>
#include <clang-c/Index.h>
#include <Core/String.h>

#include "Core/StringID.h"
#include "Core/Utils.h"

//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    namespace ClangUtils
    {
        //-------------------------------------------------------------------------
        // Cursors
        //-------------------------------------------------------------------------

        inline std::string GetString(CXString &&string)
        {
			auto s = clang_getCString(string);
			std::string str(s, Utils::String::Length(s));
            clang_disposeString(string);
            return str;
        }

        inline std::string GetCursorDisplayName(CXCursor const &cr)
        {
            auto displayName = clang_getCursorDisplayName(cr);
			auto s = clang_getCString(displayName);
			std::string str(s, Utils::String::Length(s));
			clang_disposeString(displayName);
            return str;
        }

        inline uint32_t GetLineNumberForCursor(CXCursor const &cr)
        {
            uint32_t line, column, offset;
            CXSourceRange range = clang_getCursorExtent(cr);
            CXSourceLocation start = clang_getRangeStart(range);
            clang_getExpansionLocation(start, nullptr, &line, &column, &offset);
            return line;
        }

        inline void GetLineColumnNumberForCursor(CXCursor const &cr, uint32_t &line, uint32_t &column)
        {
            CXSourceRange range = clang_getCursorExtent(cr);
            CXSourceLocation start = clang_getRangeStart(range);
            clang_getExpansionLocation(start, nullptr, &line, &column, nullptr);
        }

        inline uint32_t GetStartPositionForCursor(CXCursor const &cr)
        {
            CXSourceRange range = clang_getCursorExtent(cr);
            return range.begin_int_data;
        }

        inline std::string GetTypeSpellingAnsi(CXType type)
        {
            CXString spelling = clang_getTypeSpelling(type);
            std::string result(clang_getCString(spelling));
            clang_disposeString(spelling);
            return result;
        }

        inline std::string GetCursorSpellingAnsi(CXCursor cr)
        {
            CXString spelling = clang_getCursorSpelling(cr);
            std::string result(clang_getCString(spelling));
            clang_disposeString(spelling);
            return result;
        }

        void GetDiagnostics(CXTranslationUnit &TU, std::vector<std::string> &diagnostics);

		std::string GetHeaderPathForCursor(CXCursor cr);

        //-------------------------------------------------------------------------
        // QualTypes
        //-------------------------------------------------------------------------

        bool GetQualifiedNameForType(clang::QualType type, std::string &qualifiedName);

        inline clang::QualType GetQualType(CXType type)
        {
            return clang::QualType::getFromOpaquePtr(type.data[0]);
        }

        inline bool GetQualifiedNameForType(CXType type, std::string &qualifiedName)
        {
            return GetQualifiedNameForType(GetQualType(type), qualifiedName);
        }

        //-------------------------------------------------------------------------
        // Misc
        //-------------------------------------------------------------------------

        inline bool GetAllBaseClasses(std::vector<StringID> &baseClasses, clang::CXXBaseSpecifier &baseSpecifier)
        {
			std::string fullyQualifiedName;
            if (!ClangUtils::GetQualifiedNameForType(baseSpecifier.getType(), fullyQualifiedName))
            {
                return false;
            }

            baseClasses.push_back(StringID(fullyQualifiedName));

            clang::CXXRecordDecl *pBaseSpecifierRecordDecl = baseSpecifier.getType()->getAsCXXRecordDecl();
            // 检查指针是否为NULL
            if (!pBaseSpecifierRecordDecl)
            {
                return true;
            }
            
            for (auto parentBaseSpecifier : pBaseSpecifierRecordDecl->bases())
            {
                if (!GetAllBaseClasses(baseClasses, parentBaseSpecifier))
                {
                    return false;
                }
            }

            return true;
        }
    }
}

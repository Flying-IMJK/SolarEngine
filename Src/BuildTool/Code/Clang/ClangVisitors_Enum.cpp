#include "ClangVisitors_Enum.h"
#include "../Database/TypeDatabase.h"

#include <memory>

//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    static CXChildVisitResult VisitEnumContents( CXCursor cr, CXCursor parent, CXClientData pClientData )
    {
        auto pContext = static_cast<ClangParserContext*>( pClientData );

        CXCursorKind kind = clang_getCursorKind( cr );
        if ( kind == CXCursor_EnumConstantDecl )
        {
            auto pEnum = reinterpret_cast<TypeInfoEnum*>( pContext->pParentReflectedType );
            clang::EnumConstantDecl* pEnumConstantDecl = ( clang::EnumConstantDecl* ) cr.data[0];

            EnumDataConstant constant;

            // Set Label
            constant.label = ClangUtils::GetCursorDisplayName(cr);
            constant.ID = StringID(constant.label);

            // Set Value
            auto const& initVal = pEnumConstantDecl->getInitVal();
            constant.value = (int32_t) initVal.getExtValue();

            // Set property description
            CXString const commentString = clang_Cursor_getBriefCommentText(cr);
            if ( commentString.data != nullptr )
            {
                constant.description = clang_getCString( commentString );
            }

            clang_disposeString( commentString );

            pEnum->AddEnumConstant( constant );
        }

        return CXChildVisit_Continue;
    }

    CXChildVisitResult VisitEnum( ClangParserContext* pContext, CXCursor cr, HeaderID const headerID )
    {
        auto cursorName = ClangUtils::GetCursorDisplayName( cr );

        std::string fullyQualifiedCursorName;
        if ( !ClangUtils::GetQualifiedNameForType( clang_getCursorType( cr ), fullyQualifiedCursorName ) )
        {
            pContext->LogError("Failed to get qualified type for cursor: {0}", cursorName);
            return CXChildVisit_Break;
        }

        //-------------------------------------------------------------------------

        clang::EnumDecl* pEnumDecl = ( clang::EnumDecl* ) cr.data[0];
        clang::QualType integerType = pEnumDecl->getIntegerType();

        if ( integerType.isNull() )
        {
            pContext->LogError("Failed to get underlying type for enum: {0}. You must specify the underlying integer type for exposed enums!", fullyQualifiedCursorName);
            return CXChildVisit_Break;
        }

        Utils::TypeIDCore underlyingCoreType;

        auto const* pBT = integerType.getTypePtr()->getAs<clang::BuiltinType>();
        switch ( pBT->getKind() )
        {
        case clang::BuiltinType::UChar:
            underlyingCoreType = Utils::TypeIDCore::Uint8;
            break;

        case clang::BuiltinType::SChar:
            underlyingCoreType = Utils::TypeIDCore::Int8;
            break;

        case clang::BuiltinType::UShort:
            underlyingCoreType = Utils::TypeIDCore::Uint16;
            break;

        case clang::BuiltinType::Short:
            underlyingCoreType = Utils::TypeIDCore::Int16;
            break;

        case clang::BuiltinType::UInt:
            underlyingCoreType = Utils::TypeIDCore::Uint32;
            break;

        case clang::BuiltinType::Int:
            underlyingCoreType = Utils::TypeIDCore::Int32;
            break;

        case clang::BuiltinType::ULongLong:
        case clang::BuiltinType::LongLong:
        {
            pContext->LogError("64bit enum detected: {0}. This is not supported!", fullyQualifiedCursorName);
            return CXChildVisit_Break;
        }
            break;

        default:
        {
            pContext->LogError("Unknown underlying type for enum: {0}", fullyQualifiedCursorName);
            return CXChildVisit_Break;
        }
            break;
        }

        //-------------------------------------------------------------------------

        auto enumTypeID = pContext->GenerateTypeID( fullyQualifiedCursorName );

        MarkMacro macro;
        if (pContext->FindMarkMacro(headerID, cr, macro, MacroTypeEnum::SEEnum) )
        {
            if (!pContext->pDatabase->IsTypeRegistered(enumTypeID) )
            {
                auto enumDescriptor = std::make_unique<TypeInfoEnum>(enumTypeID, cursorName);
                enumDescriptor->headerID = headerID;
                enumDescriptor->namespaceScopeList = pContext->GetNamespaces();
                enumDescriptor->structScopeList = pContext->GetStructScopes();
                enumDescriptor->underlyingType = underlyingCoreType;
                enumDescriptor->isReflect = macro.hasReflect;

                // Check for SE_ENUM binding (API(...) group)
                if (macro.HasApi())
                {
                    if (!macro.GetApi().inBuildMapType.empty())
                    {
                        pContext->LogError("API(InBuild(\"{0}\")) is not supported on enum ({1})", macro.GetApi().inBuildMapType, cursorName);
                        return CXChildVisit_Break;
                    }

                    // Same macro carries both Reflect and API
                    enumDescriptor->isAPI = true;
                    std::string assemblyName, assemblyDir;
                    pContext->GetAssemblyInfoForHeader(headerID, assemblyName, assemblyDir);
                    enumDescriptor->assemblyName = assemblyName;
                    enumDescriptor->assemblyDir = assemblyDir;
                    CXString const commentString = clang_Cursor_getBriefCommentText(cr);
                    if (commentString.data != nullptr)
                    {
                        enumDescriptor->comment = clang_getCString(commentString);
                        Utils::String::ReplaceAll(enumDescriptor->comment, "\r", " ");
                        Utils::String::TrimStart(enumDescriptor->comment);
                        Utils::String::TrimEnd(enumDescriptor->comment);
                    }
                    clang_disposeString(commentString);
                    enumDescriptor->APIAttributes = macro.GetApi().attributes;
                }

                // Record current parent type, and update it to the new type
                void* pPreviousParentReflectedType = pContext->pParentReflectedType;
                pContext->pParentReflectedType = enumDescriptor.get();
                {
                    clang_visitChildren( cr, VisitEnumContents, pContext );
                }
                // Reset parent type back to original parent
                pContext->pParentReflectedType = pPreviousParentReflectedType;

                pContext->pDatabase->RegisterType(std::move(enumDescriptor), false);
            }
        }

        return CXChildVisit_Continue;
    }
}

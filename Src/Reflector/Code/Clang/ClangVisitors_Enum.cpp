#include "ClangVisitors_Enum.h"
#include "../Database/ReflectionDatabase.h"

//-------------------------------------------------------------------------

namespace SE::ReflectTool
{
    static CXChildVisitResult VisitEnumContents( CXCursor cr, CXCursor parent, CXClientData pClientData )
    {
        auto pContext = reinterpret_cast<ClangParserContext*>( pClientData );

        CXCursorKind kind = clang_getCursorKind( cr );
        if ( kind == CXCursor_EnumConstantDecl )
        {
            auto pEnum = reinterpret_cast<ReflectedType*>( pContext->m_pParentReflectedType );
            clang::EnumConstantDecl* pEnumConstantDecl = ( clang::EnumConstantDecl* ) cr.data[0];

            ReflectedEnumConstant constant;

            // Set Label
            constant.label = ClangUtils::GetCursorDisplayName(cr).ToStringAnsi();
            constant.ID = StringID(constant.label.ToString());

            // Set Value
            auto const& initVal = pEnumConstantDecl->getInitVal();
            constant.value = (int32_t) initVal.getExtValue();

            // Set property description
            CXString const commentString = clang_Cursor_getBriefCommentText( cr );
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

        String fullyQualifiedCursorName;
        if ( !ClangUtils::GetQualifiedNameForType( clang_getCursorType( cr ), fullyQualifiedCursorName ) )
        {
            pContext->LogError(SE_TEXT("Failed to get qualified type for cursor: {0}"), cursorName);
            return CXChildVisit_Break;
        }

        //-------------------------------------------------------------------------

        clang::EnumDecl* pEnumDecl = ( clang::EnumDecl* ) cr.data[0];
        clang::QualType integerType = pEnumDecl->getIntegerType();

        if ( integerType.isNull() )
        {
            pContext->LogError(SE_TEXT("Failed to get underlying type for enum: {0}. You must specify the underlying integer type for exposed enums!"), fullyQualifiedCursorName);
            return CXChildVisit_Break;
        }

        TypeIDCore underlyingCoreType;

        auto const* pBT = integerType.getTypePtr()->getAs<clang::BuiltinType>();
        switch ( pBT->getKind() )
        {
        case clang::BuiltinType::UChar:
            underlyingCoreType = TypeIDCore::Uint8;
            break;

        case clang::BuiltinType::SChar:
            underlyingCoreType = TypeIDCore::Int8;
            break;

        case clang::BuiltinType::UShort:
            underlyingCoreType = TypeIDCore::Uint16;
            break;

        case clang::BuiltinType::Short:
            underlyingCoreType = TypeIDCore::Int16;
            break;

        case clang::BuiltinType::UInt:
            underlyingCoreType = TypeIDCore::Uint32;
            break;

        case clang::BuiltinType::Int:
            underlyingCoreType = TypeIDCore::Int32;
            break;

        case clang::BuiltinType::ULongLong:
        case clang::BuiltinType::LongLong:
        {
            pContext->LogError(SE_TEXT("64bit enum detected: {0}. This is not supported!"), fullyQualifiedCursorName);
            return CXChildVisit_Break;
        }
            break;

        default:
        {
            pContext->LogError(SE_TEXT("Unknown underlying type for enum: {0}"), fullyQualifiedCursorName);
            return CXChildVisit_Break;
        }
            break;
        }

        //-------------------------------------------------------------------------

        auto enumTypeID = pContext->GenerateTypeID( fullyQualifiedCursorName );
        if ( !pContext->IsInEngineNamespace() )
        {
            return CXChildVisit_Continue;
        }

        int const lineNumber = ClangUtils::GetLineNumberForCursor(cr);

        ReflectionMacro macro;
        if ( pContext->FindReflectionMacroForEnum( headerID, cr, lineNumber, macro ) )
        {
            if ( pContext->m_detectDevOnlyTypesAndProperties || !pContext->m_pDatabase->IsTypeRegistered( enumTypeID ) )
            {
                ReflectedType enumDescriptor( enumTypeID, cursorName.ToStringAnsi());
                enumDescriptor.headerID = headerID;
                enumDescriptor.namespaceName = pContext->GetCurrentNamespace().ToStringAnsi();
                enumDescriptor.flags.SetFlag( ReflectedType::Flags::IsEnum );
                enumDescriptor.underlyingType = underlyingCoreType;

                // Record current parent type, and update it to the new type
                void* pPreviousParentReflectedType = pContext->m_pParentReflectedType;
                pContext->m_pParentReflectedType = &enumDescriptor;
                {
                    clang_visitChildren( cr, VisitEnumContents, pContext );
                }
                // Reset parent type back to original parent
                pContext->m_pParentReflectedType = pPreviousParentReflectedType;

                pContext->m_pDatabase->RegisterType( &enumDescriptor, pContext->m_detectDevOnlyTypesAndProperties );
            }
        }

        return CXChildVisit_Continue;
    }
}
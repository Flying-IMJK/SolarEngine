#pragma once

#include "ClangUtils.h"
#include "../ReflectorSettingsAndUtils.h"
#include "../Database/ReflectionProjectTypes.h"
#include "../Database/DataTypes.h"
#include "Core/Types/Collections/Dictionary.h"
#include "Core/Types/Collections/List.h"
#include "Core/Types/Strings/String.h"
#include "Core/Types/Strings/StringID.h"

//-------------------------------------------------------------------------

namespace SE::ReflectTool
{
    class ReflectionDatabase;

    //-------------------------------------------------------------------------

    struct MarkMacro
    {
    public:
        MarkMacro() = default;
        MarkMacro( HeaderInfo const* pHeaderInfo, CXCursor cursor, CXSourceRange& sourceLocation, ReflectionMacroType type );

        bool IsValid() const { return type != ReflectionMacroType::Unknown; }
        bool IsModuleMacro() const { return false; }
        bool IsEnumMacro() const { return type == ReflectionMacroType::SEEnum && hasReflect; }
        bool IsMetaMacro() const { return type == ReflectionMacroType::ReflectMeta; }
        bool IsTypeMacro() const { return type == ReflectionMacroType::SEClass; }
    public:

        HeaderID            headerID;
        uint32_t            fileLine = 0;
        uint32_t            fileColumn = 0;
        ReflectionMacroType type = ReflectionMacroType::Unknown;
		String	            macroComment;
        bool                hasReflect = false;
        bool                hasAPI = false;
        String              macroMetadata;
        List<String>        macroContents;
    };

    //-------------------------------------------------------------------------

    class ClangParserContext
    {

    public:

        struct HeaderToVisit
        {
            HeaderToVisit( HeaderID ID, HeaderInfo const* pHeaderInfo ) : m_ID( ID ), m_pHeaderInfo( pHeaderInfo ) {}
			HeaderToVisit() : m_ID(), m_pHeaderInfo() {}

            inline bool operator==( HeaderID const& ID ) const { return m_ID == ID; }

        public:

            HeaderID            m_ID;
            HeaderInfo const*   m_pHeaderInfo;
        };

    public:

        ClangParserContext( SolutionInfo* pSolution, ReflectionDatabase* pDatabase )
            : pTU( nullptr )
            , pDatabase( pDatabase )
            , pParentReflectedType( nullptr )
            , m_inEngineNamespace( false )
            , pSolution( pSolution )
        {
            ENGINE_ASSERT( pSolution != nullptr && pDatabase != nullptr );
        }

        template<typename... Params>
        void LogError(const Char *pErrorFormat, Params... args) const
        {
            m_errorMessage = String::Format(pErrorFormat, args...).ToStringAnsi();
        }
        
        StringAnsiView const GetErrorMessage() const { return m_errorMessage.Get(); }
        inline bool HasErrorOccured() const { return !m_errorMessage.IsEmpty(); }

        HeaderInfo const* GetHeaderInfo( HeaderID headerID ) const;

        void Reset( CXTranslationUnit* pTU );
        void PushNamespace( String const& name );
        void PopNamespace();

        bool SetModuleClassName( StringView const& headerFilePath, String const& moduleClassName );
        TypeID GenerateTypeID( String const& fullyQualifiedTypeName ) const { return TypeID( fullyQualifiedTypeName); }

		String const& GetCurrentNamespace() const { return m_currentNamespace; }
        bool IsInEngineNamespace() const { return m_inEngineNamespace; }
        bool IsEngineNamespace( String const& namespaceString ) const;

        void AddMarkMacro( MarkMacro const& foundMacro );
        bool FindMarkMacro(HeaderID headerID, CXCursor const& cr, MarkMacro& macro, ReflectionMacroType macroType);

        bool FindReflectionMacroForMeta( HeaderID headerID, CXCursor const& cr, MarkMacro& reflectionMacro );

        // Check if we have any orphaned reflection macros
        // If we have any then we will populate the error message with all the details
        bool CheckForOrphanedReflectionMacros() const;

        // Get assembly name and directory for a given header
        void GetAssemblyInfoForHeader(HeaderID headerID, StringAnsi& outAssemblyName, StringAnsi& outAssemblyDir) const;
    public:

        CXTranslationUnit*                                      pTU;

        SolutionInfo*                                           pSolution;
        ReflectionDatabase*                                     pDatabase;
        List<HeaderToVisit>                                     headersToVisit;

        // The current parent/enclosing reflected type
        void*                                                   pParentReflectedType;

    private:

        Dictionary<HeaderID, List<MarkMacro>>                   m_MarkMacros;

        mutable StringAnsi                                      m_errorMessage;
		List<String>                                          	m_namespaceStack;
		List<String>                                          	m_structureStack;
		String                                              	m_currentNamespace;
        bool                                                  	m_inEngineNamespace;
    };
}
#pragma once

#include "ClangUtils.h"
#include "../ReflectorSettingsAndUtils.h"
#include "../Database/ReflectionProjectTypes.h"
#include "Core/Types/Collections/Dictionary.h"
#include "Core/Types/Collections/List.h"
#include "Core/Types/Strings/String.h"
#include "Core/Types/Strings/StringID.h"

//-------------------------------------------------------------------------

namespace SE::ReflectTool
{
    class ReflectionDatabase;

    //-------------------------------------------------------------------------

    struct ReflectionMacro
    {
    public:
        ReflectionMacro() = default;
        ReflectionMacro( HeaderInfo const* pHeaderInfo, CXCursor cursor, CXSourceRange sourceRange, ReflectionMacroType type );

        bool IsValid() const { return type != ReflectionMacroType::Unknown; }
        bool IsModuleMacro() const { return type == ReflectionMacroType::ReflectModule; }
        bool IsEnumMacro() const { return type == ReflectionMacroType::ReflectEnum; }
        bool IsMetaMacro() const { return type == ReflectionMacroType::ReflectMeta; }

        // Should be registered as a type
        bool IsReflectedTypeMacro() const { return type == ReflectionMacroType::ReflectType; }
    public:

        HeaderID            headerID;
        uint32_t            lineNumber = 0;
        uint32_t            positionStart = 0xFFFFFFFF;
        uint32_t            positionEnd = 0xFFFFFFFF;
        ReflectionMacroType type = ReflectionMacroType::Unknown;
		String	            macroComment;
        String    	        macroContents;
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
            : m_pTU( nullptr )
            , m_pDatabase( pDatabase )
            , m_pParentReflectedType( nullptr )
            , m_inEngineNamespace( false )
            , m_pSolution( pSolution )
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

        void AddFoundReflectionMacro( ReflectionMacro const& foundMacro );

        bool FindReflectionMacroForType( HeaderID headerID, CXCursor const& cr, ReflectionMacro& macro );
        // Try to find a reflection macro for a property
        // If we found a macro we will remove it from the list of macros to reduce the cost of future searches
        // Returns true if a macro was found
        bool FindReflectionMacroForEnum( HeaderID headerID, CXCursor const& cr, int lineNumber, ReflectionMacro& macro );
        bool FindReflectionMacroForProperty( HeaderID headerID, uint32_t lineNumber, ReflectionMacro& reflectionMacro );
        bool FindReflectionMacroForMeta( HeaderID headerID, uint32_t lineNumber, ReflectionMacro& reflectionMacro );

        // Check if we have any orphaned reflection macros
        // If we have any then we will populate the error message with all the details
        bool CheckForOrphanedReflectionMacros() const;

    public:

        CXTranslationUnit*                                      m_pTU;

        // Should we do a full pass or just update the flags?
        bool                                                    m_detectDevOnlyTypesAndProperties = false;

        SolutionInfo*                                           m_pSolution;
        ReflectionDatabase*                                     m_pDatabase;
        List<HeaderToVisit>                                     m_headersToVisit;

        // The current parent/enclosing reflected type
        void*                                                   m_pParentReflectedType;

    private:

        Dictionary<HeaderID, List<ReflectionMacro>>              m_typeReflectionMacros;
		Dictionary<HeaderID, List<ReflectionMacro>>              m_propertyReflectionMacros;

        mutable StringAnsi                                      m_errorMessage;
		List<String>                                          	m_namespaceStack;
		List<String>                                          	m_structureStack;
		String                                              	m_currentNamespace;
        bool                                                  	m_inEngineNamespace;


        bool FindReflectionMacro(List<ReflectionMacro> &macrosForHeader, HeaderID headerID, uint32 lineNumber, ReflectionMacroType macroType, ReflectionMacro& reflectionMacro );
    };
}
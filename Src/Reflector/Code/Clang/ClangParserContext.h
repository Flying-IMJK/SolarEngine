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

    struct ReflectionMacro
    {
    public:
        ReflectionMacro() = default;
        ReflectionMacro( HeaderInfo const* pHeaderInfo, CXCursor cursor, CXSourceRange sourceRange, ReflectionMacroType type );

        bool IsValid() const { return type != ReflectionMacroType::Unknown; }
        bool IsModuleMacro() const { return false; }
        bool IsEnumMacro() const { return type == ReflectionMacroType::SEEnum && hasReflect; }
        bool IsMetaMacro() const { return type == ReflectionMacroType::ReflectMeta; }
        bool IsTypeMacro() const { return type == ReflectionMacroType::SEClass; }
    public:

        HeaderID            headerID;
        uint32_t            lineNumber = 0;
        uint32_t            positionStart = 0xFFFFFFFF;
        uint32_t            positionEnd = 0xFFFFFFFF;
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

        void AddFoundReflectionMacro( ReflectionMacro const& foundMacro );

        bool FindReflectionMacroForType( HeaderID headerID, CXCursor const& cr, ReflectionMacro& macro );
        // Try to find a reflection macro for a property
        // If we found a macro we will remove it from the list of macros to reduce the cost of future searches
        // Returns true if a macro was found
        bool FindReflectionMacroForEnum( HeaderID headerID, CXCursor const& cr, ReflectionMacro& macro );
        bool FindReflectionMacroForProperty( HeaderID headerID, uint32_t declStartPosition, ReflectionMacro& reflectionMacro );
        bool FindReflectionMacroForMeta( HeaderID headerID, uint32_t declStartPosition, ReflectionMacro& reflectionMacro );

        // Check if we have any orphaned reflection macros
        // If we have any then we will populate the error message with all the details
        bool CheckForOrphanedReflectionMacros() const;

        // ---- Bindings macro lookup (API_CLASS, API_ENUM, API_FUNCTION, etc.) ----
        bool FindBindingMacroForType(HeaderID headerID, CXCursor const& cr, ReflectionMacro& macro);
        bool FindBindingMacroForEnum(HeaderID headerID, CXCursor const& cr, ReflectionMacro& macro);
        bool FindBindingMacroForMember(HeaderID headerID, uint32_t declStartPosition, ReflectionMacroType memberType, ReflectionMacro& macro);

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

        Dictionary<HeaderID, List<ReflectionMacro>>              m_TypeReflectionMacros;
		Dictionary<HeaderID, List<ReflectionMacro>>              m_InTypeReflectionMacros;

        mutable StringAnsi                                      m_errorMessage;
		List<String>                                          	m_namespaceStack;
		List<String>                                          	m_structureStack;
		String                                              	m_currentNamespace;
        bool                                                  	m_inEngineNamespace;


        bool FindReflectionMacro(List<ReflectionMacro> &macrosForHeader, HeaderID headerID, uint32 declStartPosition, ReflectionMacroType macroType, ReflectionMacro& reflectionMacro );
    };
}
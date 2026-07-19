#pragma once

#include "ClangUtils.h"
#include "Database/DataTypes.h"
#include "Database/ReflectionProjectTypes.h"
#include "Core/Dictionary.h"

//-------------------------------------------------------------------------

namespace SE::BuildTool
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
        bool IsTypeMacro() const { return type == ReflectionMacroType::SEClass || type == ReflectionMacroType::SEStruct || type == ReflectionMacroType::SEInterface; }
        bool HasContent(std::string_view value) const;
    public:

        HeaderID            headerID;
        uint32_t            fileLine = 0;
        uint32_t            fileColumn = 0;
        ReflectionMacroType type = ReflectionMacroType::Unknown;
		std::string	            macroComment;
        bool                hasReflect = false;
        bool                hasAPI = false;
        std::string              macroMetadata;
        std::vector<std::string>        macroContents;
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

        struct TemplateTypeData
        {
            TypeData type;
            std::vector<std::string> parameterNames;
        };

        struct TypeDefData
        {
            HeaderID headerID;
            int32 lineNumber = -1;
            MarkMacro macro;
            std::string name;
            std::string templateTypeName;
            std::vector<std::string> templateArguments;
            std::vector<std::string> namespaceScopeList;
            std::vector<std::string> structScopeList;
        };

    public:

        ClangParserContext( SolutionInfo* pSolution, ReflectionDatabase* pDatabase )
            : pTU( nullptr )
            , pDatabase( pDatabase )
            , pParentReflectedType( nullptr )
            , pSolution( pSolution )
        {
            ENGINE_ASSERT( pSolution != nullptr && pDatabase != nullptr );
        }

        template<typename... Params>
        void LogError(const Char *pErrorFormat, Params... args) const
        {
            m_errorMessage = Utils::String::Format(pErrorFormat, args...);
        }
        
        std::string_view const GetErrorMessage() const { return m_errorMessage; }
        inline bool HasErrorOccured() const { return !m_errorMessage.empty(); }

        HeaderInfo const* GetHeaderInfo( HeaderID headerID ) const;

        void Reset( CXTranslationUnit* pTU );
        void PushNamespace( std::string const& name );
        void PopNamespace();
        void PushStruct( std::string const& name );
        void PopStruct();
        std::vector<std::string> GetStructScopes();
        std::vector<std::string> GetNamespaces();

        bool SetModuleClassName( std::string_view const& headerFilePath, std::string const& moduleClassName );
        TypeID GenerateTypeID( std::string const& fullyQualifiedTypeName ) const { return TypeID( fullyQualifiedTypeName); }

		std::string const& GetCurrentNamespace() const { return m_currentNamespace; }
        std::string const& GetCurrentStructScope() const { return m_currentStructScope; }

        void AddMarkMacro( MarkMacro const& foundMacro );
        bool FindMarkMacro(HeaderID headerID, CXCursor const& cr, MarkMacro& macro, ReflectionMacroType macroType);

        bool FindReflectionMacroForMeta( HeaderID headerID, CXCursor const& cr, MarkMacro& reflectionMacro );

        void AddTemplateType(TypeData const& type, std::vector<std::string> const& parameterNames);
        void AddTypeDef(TypeDefData const& typeDef);
        bool ResolvePendingTypeDefs();

        // Check if we have any orphaned reflection macros
        // If we have any then we will populate the error message with all the details
        bool CheckForOrphanedReflectionMacros() const;

        // Get assembly name and directory for a given header
        void GetAssemblyInfoForHeader(HeaderID headerID, std::string& outAssemblyName, std::string& outAssemblyDir) const;
    public:

        CXTranslationUnit*                                      pTU;

        SolutionInfo*                                           pSolution;
        ReflectionDatabase*                                     pDatabase;
        std::vector<HeaderToVisit>                                     headersToVisit;

        // The current parent/enclosing reflected type
        void*                                                   pParentReflectedType;

    private:

        Dictionary<HeaderID, std::vector<MarkMacro>>             m_MarkMacros;
        std::vector<TemplateTypeData>                            m_TemplateTypes;
        std::vector<TypeDefData>                                 m_TypeDefs;

        mutable std::string                                      m_errorMessage;
		std::vector<std::string>                                 m_namespaceStack;
		std::vector<std::string>                                 m_structureStack;
		std::string                                              m_currentNamespace;
        std::string                                              m_currentStructScope;
    };
}

#pragma once

#include "ClangUtils.h"
#include "Database/DataTypes.h"
#include "Database/DataTypes_Template.h"
#include "Database/ReflectionProjectTypes.h"
#include "Core/Dictionary.h"

#include <memory>

//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    class TypeDatabase;

    //-------------------------------------------------------------------------

    enum class MacroTypeEnum
    {
        SEMeta,

        SEClass,       // SE_CLASS(Reflect, API(...))
        SEStruct,      // SE_STRUCT(...)
        SEInterface,   // SE_INTERFACE(...)
        SEEnum,        // SE_ENUM(Reflect, API(...))
        SEField,       // SE_FIELD(Reflect, API(...))
        SEFunction,    // SE_FUNCTION(API(...))
        SEEvent,       // SE_EVENT(API(...))
        SETypeDef,     // SE_TYPEDEF(...)
        SEInjectCode,  // SE_INJECT_CODE(cpp|csharp, "code")

        NumMacros,
        Unknown = NumMacros,
    };

    struct MarkToken
    {
        MarkToken(CXCursor &cursor, CXSourceRange& sourceRange);
        ~MarkToken();
        bool Next();
        bool Peek(std::string& outToken) const;
        std::string Current() const;

    private:
        CXToken*          tokens          = nullptr;
        uint32            numTokens       = 0;
        int32             tokenIndex      = 0;
        CXTranslationUnit translationUnit;
    };


    struct MarkAPI
    {
    public:
        MarkAPI(MarkToken& context);
    public:
        bool IsAbstract      = false;
        bool IsStatic        = false;
        bool IsSealed        = false;
        bool IsReadOnly      = false;
        bool IsNoSpawn       = false;
        bool IsNoProxy       = false;
        bool IsNoConstructor = false;
        bool IsProperty      = false;
        bool IsNativeInvokeUseName = false;
        std::string name;
        std::string attributes;
        std::string marshalAs;
        std::string inBuildMapType;
    };

    struct MarkMacro
    {
    public:
        MarkMacro() = default;
        MarkMacro(MarkMacro const& other);
        MarkMacro& operator=(MarkMacro const& other);
        MarkMacro(MarkMacro&& other) noexcept = default;
        MarkMacro& operator=(MarkMacro&& other) noexcept = default;
        MarkMacro(HeaderInfo const* pHeaderInfo, CXCursor cursor, CXSourceRange& sourceLocation, MacroTypeEnum type);

        bool IsValid() const { return type != MacroTypeEnum::Unknown; }
        bool IsEnumMacro() const { return type == MacroTypeEnum::SEEnum; }
        bool IsMetaMacro() const { return type == MacroTypeEnum::SEMeta; }
        bool IsTypeMacro() const { return type == MacroTypeEnum::SEClass || type == MacroTypeEnum::SEStruct || type == MacroTypeEnum::SEInterface; }

        bool HasApi() const { return api != nullptr; }
        MarkAPI& GetApi() { ENGINE_ASSERT(api != nullptr); return *api; }
        MarkAPI const& GetApi() const { ENGINE_ASSERT(api != nullptr); return *api; }

        //-------------------------------------------------------------------------

        static char const* GetMarkMacroText(MacroTypeEnum macro);

    public:
        HeaderID                          headerID;
        uint32_t                          fileLine      = 0;
        uint32_t                          fileColumn    = 0;
        uint32_t                          fileEndLine   = 0;
        uint32_t                          fileEndColumn = 0;
        MacroTypeEnum                     type          = MacroTypeEnum::Unknown;
        std::string                       macroComment;
        bool                              hasReflect = false;
        bool                              hasTemplate = false;
        bool                              isAlias = false;
        std::unique_ptr<MarkAPI>          api;
        std::string                       macroMetadata;
    };

    //-------------------------------------------------------------------------

    class ClangParserContext
    {

    public:
        struct HeaderToVisit
        {
            HeaderToVisit(HeaderID ID, HeaderInfo const* pHeaderInfo) : m_ID(ID), m_pHeaderInfo(pHeaderInfo) {}
            HeaderToVisit() : m_ID(), m_pHeaderInfo() {}

            inline bool operator==(HeaderID const& ID) const { return m_ID == ID; }

        public:
            HeaderID          m_ID;
            HeaderInfo const* m_pHeaderInfo;
        };

        struct TemplateTypeData
        {
            std::unique_ptr<TypeInfoStructTemplate> type;
        };

        struct TypeDefData
        {
            HeaderID                 headerID;
            int32                    lineNumber = -1;
            MarkMacro                macro;
            std::string              name;
            TypeRefTemplate          targetType;
            std::vector<std::string> namespaceScopeList;
            std::vector<std::string> structScopeList;
        };

    public:
        ClangParserContext(SolutionInfo* pSolution, TypeDatabase* pDatabase) :
            pTU(nullptr), pDatabase(pDatabase), pParentReflectedType(nullptr), pSolution(pSolution)
        {
            ENGINE_ASSERT(pSolution != nullptr && pDatabase != nullptr);
        }

        template<typename... Params>
        void LogError(const Char* pErrorFormat, Params... args) const
        {
            m_errorMessage = Utils::String::Format(pErrorFormat, args...);
        }

        std::string_view const GetErrorMessage() const { return m_errorMessage; }
        inline bool            HasErrorOccured() const { return !m_errorMessage.empty(); }

        HeaderInfo const* GetHeaderInfo(HeaderID headerID) const;

        void                     Reset(CXTranslationUnit* pTU);
        void                     PushNamespace(std::string const& name);
        void                     PopNamespace();
        void                     PushStruct(std::string const& name);
        void                     PopStruct();
        std::vector<std::string> GetStructScopes();
        std::vector<std::string> GetNamespaces();

        bool   SetModuleClassName(std::string_view const& headerFilePath, std::string const& moduleClassName);
        TypeID GenerateTypeID(std::string const& fullyQualifiedTypeName) const
        {
            return TypeID(fullyQualifiedTypeName);
        }

        std::string const& GetCurrentNamespace() const { return m_currentNamespace; }
        std::string const& GetCurrentStructScope() const { return m_currentStructScope; }

        void AddMarkMacro(MarkMacro const& foundMacro);
        bool FindMarkMacro(HeaderID headerID, CXCursor const& cr, MarkMacro& macro, MacroTypeEnum macroType);

        void FindInjectCodeFormHead(HeaderID headerID, std::vector<TypeInfoInjectedCode>& injectCodes);

        void AddTemplateType(std::unique_ptr<TypeInfoStructTemplate> type);
        void AddTypeDef(TypeDefData const& typeDef);
        bool ResolvePendingTypeDefs();

        // Check if we have any orphaned reflection macros
        // If we have any then we will populate the error message with all the details
        bool CheckForOrphanedReflectionMacros() const;

        // Get assembly name and directory for a given header
        void
        GetAssemblyInfoForHeader(HeaderID headerID, std::string& outAssemblyName, std::string& outAssemblyDir) const;

    public:
        CXTranslationUnit* pTU;

        SolutionInfo*              pSolution;
        TypeDatabase*        pDatabase;
        std::vector<HeaderToVisit> headersToVisit;

        // The current parent/enclosing reflected type
        void* pParentReflectedType;

    private:
        Dictionary<HeaderID, std::vector<MarkMacro>> m_MarkMacros;
        std::vector<TemplateTypeData>                m_TemplateTypes;
        std::vector<TypeDefData>                     m_TypeDefs;

        mutable std::string      m_errorMessage;
        std::vector<std::string> m_namespaceStack;
        std::vector<std::string> m_structureStack;
        std::string              m_currentNamespace;
        std::string              m_currentStructScope;
    };
} // namespace SE::BuildTool

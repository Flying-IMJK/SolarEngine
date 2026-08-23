#pragma once

#include "DataTypes.h"

#include <memory>
#include <vector>
#include <sqlite3.h>
//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    class TypeDatabase
    {
        static uint32_t const constexpr s_defaultStatementBufferSize = 8096;

    public:
        TypeDatabase();
        ~TypeDatabase();

        // Database functions
        //-------------------------------------------------------------------------

        bool IsConnected() const { return m_pDatabase != nullptr; }
        inline bool HasErrorOccurred() const { return !m_errorMessage.empty(); }
        inline std::string const &GetError() const { return m_errorMessage; }

        bool ReadDatabase(std::string const &databasePath);
        bool WriteDatabase(std::string const &databasePath);

        // Module functions
        //-------------------------------------------------------------------------

		std::vector<ProjectInfo> const &GetAllRegisteredProjects() const { return m_reflectedProjects; }
        bool IsProjectRegistered(ProjectID projectID) const;
        ProjectInfo const *GetProjectDesc(ProjectID projectID) const;
        void UpdateProjectList(std::vector<ProjectInfo> const &registeredProjects);

        bool IsHeaderRegistered(HeaderID headerID) const;
        HeaderInfo const *GetHeaderDesc(HeaderID headerID) const;
        void UpdateHeaderRecord(HeaderInfo const &header);

        // Type functions
        //-------------------------------------------------------------------------

        TypeInfoBase const *GetType(StringID typeID) const;
        TypeInfoBase *GetType(StringID typeID);
        std::vector<TypeInfoBase const*> GetAllTypes() const;
		std::vector<TypeInfoBase*> GetAllTypes();
        bool IsTypeRegistered(StringID typeID) const;
        bool IsTypeDerivedFrom(StringID typeID, StringID parentTypeID) const;
        void GetAllTypesForHeader(HeaderID headerID, std::vector<TypeInfoBase*> &types) const;
        void GetAllTypesForProject(ProjectID projectID, std::vector<TypeInfoBase*> &types) const;
        TypeInfoBase* RegisterType(std::unique_ptr<TypeInfoBase> type, bool onlyUpdateDevFlag);
        TypeInfoInjectedCode* RegisterInjectCode(std::unique_ptr<TypeInfoInjectedCode> injectCode);

        void GetInjectCodeFormHead(HeaderID headerID, std::vector<TypeInfoInjectedCode*>& injectCodes) const;

        // Cleaning
        //-------------------------------------------------------------------------

        void DeleteTypesForHeader(HeaderID headerID);
        void DeleteObseleteHeadersAndTypes(std::vector<HeaderID> const &registeredHeaders);
        void DeleteObseleteProjects(std::vector<ProjectInfo> const &registeredProjects);

    private:
        // Data
        //-------------------------------------------------------------------------

        bool CreateTables();
        bool DropTables();

        bool ReadAdditionalTypeData(TypeInfoBase &type);
        bool ReadAdditionalEnumData(TypeInfoBase &type);
        bool ReadAdditionalResourceTypeData(ReflectedResourceType &type);

        bool WriteAdditionalTypeData(TypeInfoBase const &type);
        bool WriteAdditionalEnumData(TypeInfoBase const &type);
        bool WriteAdditionalResourceTypeData(ReflectedResourceType const &type);

        // SQLite
        //-------------------------------------------------------------------------

        bool Connect(std::string const &databasePath, bool readOnlyAccess = false, bool useMutex = false);
        bool Disconnect();

        bool IsValidSQLiteResult(int result, char const *pErrorMessage = nullptr) const;

        void FillStatementBuffer(char const *pFormat, ...) const;
        bool ExecuteSimpleQuery(char const *pFormat, ...) const;

        bool BeginTransaction() const;
        bool EndTransaction() const;

    private:
        sqlite3 *m_pDatabase = nullptr;
        mutable std::string m_errorMessage;
        mutable char m_statementBuffer[s_defaultStatementBufferSize] = {0};

        std::unique_ptr<TypeInfoBase> m_reflectedTypeBase;
        std::vector<std::unique_ptr<TypeInfoInjectedCode>> m_injectedCode;
        std::vector<std::unique_ptr<TypeInfoBase>> m_ownedTypes;
		std::vector<HeaderInfo> m_reflectedHeaders;
		std::vector<ProjectInfo> m_reflectedProjects;
    };
}

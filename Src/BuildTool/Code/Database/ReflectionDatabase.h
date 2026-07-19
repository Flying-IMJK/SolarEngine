#pragma once

#include "DataTypes.h"
#include "Core/TypeSystem/Property/TypePropertyPath.h"

#include <vector>
#include <sqlite3.h>
//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    class ReflectionDatabase
    {
        static uint32_t const constexpr s_defaultStatementBufferSize = 8096;

    public:
        ReflectionDatabase();
        ~ReflectionDatabase();

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

        TypeData const *GetType(StringID typeID) const;
        TypeData *GetType(StringID typeID);
		std::vector<TypeData> const &GetAllTypes() const { return m_reflectedTypes; }
        bool IsTypeRegistered(StringID typeID) const;
        bool IsTypeDerivedFrom(StringID typeID, StringID parentTypeID) const;
        void GetAllTypesForHeader(HeaderID headerID, std::vector<TypeData> &types) const;
        void GetAllTypesForProject(ProjectID projectID, std::vector<TypeData> &types) const;
        void RegisterType(TypeData const *pType, bool onlyUpdateDevFlag);

        // Property functions
        //-------------------------------------------------------------------------

        PropertyData const *GetPropertyTypeDescriptor(StringID typeID, TypePropertyPath const &pathID) const;

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

        bool ReadAdditionalTypeData(TypeData &type);
        bool ReadAdditionalEnumData(TypeData &type);
        bool ReadAdditionalResourceTypeData(ReflectedResourceType &type);

        bool WriteAdditionalTypeData(TypeData const &type);
        bool WriteAdditionalEnumData(TypeData const &type);
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

        TypeData m_reflectedTypeBase;
        std::vector<TypeData> m_reflectedTypes;
		std::vector<HeaderInfo> m_reflectedHeaders;
		std::vector<ProjectInfo> m_reflectedProjects;
		std::vector<ReflectedResourceType> m_reflectedResourceTypes;
    };
}
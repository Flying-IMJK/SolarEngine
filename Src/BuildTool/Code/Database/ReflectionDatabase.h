#pragma once

#include "DataTypes.h"
#include "Core/TypeSystem/Property/TypePropertyPath.h"

#include <vector>
#include <sqlite3.h>
//-------------------------------------------------------------------------

namespace SE::ReflectTool
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
        inline bool HasErrorOccurred() const { return !m_errorMessage.IsEmpty(); }
        inline StringAnsi const &GetError() const { return m_errorMessage; }

        bool ReadDatabase(StringAnsi const &databasePath);
        bool WriteDatabase(StringAnsi const &databasePath);

        // Module functions
        //-------------------------------------------------------------------------

		std::vector<ProjectInfo> const &GetAllRegisteredProjects() const { return m_reflectedProjects; }
        bool IsProjectRegistered(ProjectID projectID) const;
        ProjectInfo const *GetProjectDesc(ProjectID projectID) const;
        void UpdateProjectList(List<ProjectInfo> const &registeredProjects);

        bool IsHeaderRegistered(HeaderID headerID) const;
        HeaderInfo const *GetHeaderDesc(HeaderID headerID) const;
        void UpdateHeaderRecord(HeaderInfo const &header);

        // Type functions
        //-------------------------------------------------------------------------

        DataType const *GetType(StringID typeID) const;
        DataType *GetType(StringID typeID);
		std::vector<DataType> const &GetAllTypes() const { return m_reflectedTypes; }
        bool IsTypeRegistered(StringID typeID) const;
        bool IsTypeDerivedFrom(StringID typeID, StringID parentTypeID) const;
        void GetAllTypesForHeader(HeaderID headerID, List<DataType> &types) const;
        void GetAllTypesForProject(ProjectID projectID, List<DataType> &types) const;
        void RegisterType(DataType const *pType, bool onlyUpdateDevFlag);

        // Property functions
        //-------------------------------------------------------------------------

        DataProperty const *GetPropertyTypeDescriptor(StringID typeID, TypePropertyPath const &pathID) const;

        // Cleaning
        //-------------------------------------------------------------------------

        void DeleteTypesForHeader(HeaderID headerID);
        void DeleteObseleteHeadersAndTypes(List<HeaderID> const &registeredHeaders);
        void DeleteObseleteProjects(List<ProjectInfo> const &registeredProjects);

    private:
        // Data
        //-------------------------------------------------------------------------

        bool CreateTables();
        bool DropTables();

        bool ReadAdditionalTypeData(DataType &type);
        bool ReadAdditionalEnumData(DataType &type);
        bool ReadAdditionalResourceTypeData(ReflectedResourceType &type);

        bool WriteAdditionalTypeData(DataType const &type);
        bool WriteAdditionalEnumData(DataType const &type);
        bool WriteAdditionalResourceTypeData(ReflectedResourceType const &type);

        // SQLite
        //-------------------------------------------------------------------------

        bool Connect(StringAnsi const &databasePath, bool readOnlyAccess = false, bool useMutex = false);
        bool Disconnect();

        bool IsValidSQLiteResult(int result, char const *pErrorMessage = nullptr) const;

        void FillStatementBuffer(char const *pFormat, ...) const;
        bool ExecuteSimpleQuery(char const *pFormat, ...) const;

        bool BeginTransaction() const;
        bool EndTransaction() const;

    private:
        sqlite3 *m_pDatabase = nullptr;
        mutable StringAnsi m_errorMessage;
        mutable char m_statementBuffer[s_defaultStatementBufferSize] = {0};

        DataType m_reflectedTypeBase;
        std::vector<DataType> m_reflectedTypes;
		std::vector<HeaderInfo> m_reflectedHeaders;
		std::vector<ProjectInfo> m_reflectedProjects;
		std::vector<ReflectedResourceType> m_reflectedResourceTypes;
    };
}
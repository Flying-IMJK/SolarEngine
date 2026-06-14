#include "ReflectionDatabase.h"
#include "../ReflectorSettingsAndUtils.h"
#include "Core/TypeSystem/Types.h"
#include "Core/Types/Collections/Sorting.h"

//-------------------------------------------------------------------------

namespace SE::ReflectTool
{
    ReflectionDatabase::ReflectionDatabase()
    {
        // Create the base class for all registered engine types
        //-------------------------------------------------------------------------
        String str = String::Format(SE_TEXT("{0}::{1}"), Settings::g_engineNamespace, Settings::g_reflectedTypeInterfaceClassName);
        m_reflectedTypeBase = DataType(StringID(str), StringAnsi(Settings::g_reflectedTypeInterfaceClassName));
        m_reflectedTypeBase.flags.SetFlag(DataType::Flags::IsAbstract);

        m_reflectedTypeBase.namespaceName = Settings::g_engineNamespace;
    }

    ReflectionDatabase::~ReflectionDatabase()
    {
        if (IsConnected())
        {
            Disconnect();
        }
    }

    //-------------------------------------------------------------------------

    bool ReflectionDatabase::Connect(StringAnsi const &databasePath, bool readOnlyAccess, bool useMutex)
    {
        int32 sqlFlags = 0;

        if (readOnlyAccess)
        {
            sqlFlags = SQLITE_OPEN_READONLY;
        }
        else
        {
            sqlFlags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_WAL;
        }

        if (useMutex)
        {
            sqlFlags |= SQLITE_OPEN_FULLMUTEX;
        }

        //-------------------------------------------------------------------------

        auto const result = sqlite3_open_v2(databasePath.Get(), &m_pDatabase, sqlFlags, nullptr);
        sqlite3_busy_timeout(m_pDatabase, 2500);

        if (result != SQLITE_OK)
        {
            sqlite3_close(m_pDatabase);
            m_pDatabase = nullptr;
            m_errorMessage = StringAnsi::Format("Couldn't open sqlite database: {0}", databasePath);
            return false;
        }

        return true;
    }

    bool ReflectionDatabase::Disconnect()
    {
        if (m_pDatabase != nullptr)
        {
            // Close connection
            auto result = sqlite3_close(m_pDatabase);
            ENGINE_ASSERT(result == SQLITE_OK); // If we get SQLITE_BUSY, this means we are leaking sqlite resources
            m_pDatabase = nullptr;
            return IsValidSQLiteResult(result);
        }

        return true;
    }

    bool ReflectionDatabase::IsValidSQLiteResult(int result, char const *pErrorMessage) const
    {
        if (result != SQLITE_OK)
        {
            m_errorMessage = StringAnsi::Format("{0} ( {1} )", sqlite3_errstr(result), sqlite3_errmsg(m_pDatabase));
            return false;
        }

        return true;
    }

    void ReflectionDatabase::FillStatementBuffer(char const *pFormat, ...) const
    {
        ENGINE_ASSERT(IsConnected());

        // Create the statement using the sqlite printf so we can use the extra format specifiers i.e. %Q
        va_list args;
        va_start(args, pFormat);
        sqlite3_vsnprintf(s_defaultStatementBufferSize, m_statementBuffer, pFormat, args);
        va_end(args);
    }

    bool ReflectionDatabase::ExecuteSimpleQuery(char const *pFormat, ...) const
    {
        ENGINE_ASSERT(IsConnected());

        // Create the statement using the sqlite printf so we can use the extra format specifiers i.e. %Q
        va_list args;
        va_start(args, pFormat);
        sqlite3_vsnprintf(s_defaultStatementBufferSize, m_statementBuffer, pFormat, args);
        va_end(args);

        // Execute statement
        if (!IsValidSQLiteResult(sqlite3_exec(m_pDatabase, m_statementBuffer, nullptr, nullptr, nullptr)))
        {
            StringAnsi const sqlStatementStr = StringAnsi::Format(" ( SQL: {0} )", m_statementBuffer);
            m_errorMessage += sqlStatementStr;
            return false;
        }

        return true;
    }

    bool ReflectionDatabase::BeginTransaction() const
    {
        ENGINE_ASSERT(IsConnected());
        return IsValidSQLiteResult(sqlite3_exec(m_pDatabase, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr));
    }

    bool ReflectionDatabase::EndTransaction() const
    {
        ENGINE_ASSERT(IsConnected());
        return IsValidSQLiteResult(sqlite3_exec(m_pDatabase, "END TRANSACTION;", nullptr, nullptr, nullptr));
    }

    //-------------------------------------------------------------------------

    bool ReflectionDatabase::IsProjectRegistered(ProjectID projectID) const
    {
        for (auto const &prj : m_reflectedProjects)
        {
            if (prj.id == projectID)
            {
                return true;
            }
        }

        return false;
    }

    ProjectInfo const *ReflectionDatabase::GetProjectDesc(ProjectID projectID) const
    {
        for (auto &prj : m_reflectedProjects)
        {
            if (prj.id == projectID)
            {
                return &prj;
            }
        }

        return nullptr;
    }

    void ReflectionDatabase::UpdateProjectList(List<ProjectInfo> const &registeredProjects)
    {
        DeleteObseleteProjects(registeredProjects);

        for (auto &prj : registeredProjects)
        {
            auto const prjID = prj.id;

            auto existingProj = std::find_if(m_reflectedProjects.begin(), m_reflectedProjects.end(), [prjID](const ProjectInfo &desc) -> bool
			{
			  return desc.id == prjID;
			});

            if (existingProj == m_reflectedProjects.end())
            {
                m_reflectedProjects.emplace_back(prj);
            }
            else
            {
                ProjectInfo updatedProject = prj;
                if (updatedProject.moduleHeaderID == HeaderID::Invalid)
                {
                    updatedProject.moduleHeaderID = existingProj->moduleHeaderID;
                }
                if (updatedProject.moduleClassNameFull.IsEmpty())
                {
                    updatedProject.moduleClassNameFull = existingProj->moduleClassNameFull;
                }
                if (updatedProject.moduleClassName.IsEmpty())
                {
                    updatedProject.moduleClassName = existingProj->moduleClassName;
                }

				*existingProj = updatedProject;
            }
        }
    }

    bool ReflectionDatabase::IsHeaderRegistered(HeaderID headerID) const
    {
        for (auto const &hdr : m_reflectedHeaders)
        {
            if (hdr.headerId == headerID)
            {
                return true;
            }
        }

        return false;
    }

    HeaderInfo const *ReflectionDatabase::GetHeaderDesc(HeaderID headerID) const
    {
        for (auto &hdr : m_reflectedHeaders)
        {
            if (hdr.headerId == headerID)
            {
                return &hdr;
            }
        }

        return nullptr;
    }

    void ReflectionDatabase::UpdateHeaderRecord(HeaderInfo const &header)
    {
        HeaderInfo *pHdr = const_cast<HeaderInfo *>(GetHeaderDesc(header.headerId));
        if (pHdr != nullptr)
        {
            *pHdr = header;
        }
        else
        {
            m_reflectedHeaders.emplace_back(header);
        }
    }

    //-------------------------------------------------------------------------

    DataType const *ReflectionDatabase::GetType(StringID typeID) const
    {
        if (m_reflectedTypeBase.typeID == typeID)
        {
            return &m_reflectedTypeBase;
        }

        for (auto const &type : m_reflectedTypes)
        {
            if (type.typeID == typeID)
            {
                return &type;
            }
        }

        return nullptr;
    }

    DataType* ReflectionDatabase::GetType(StringID typeID)
    {
        return const_cast<DataType *>(const_cast<ReflectionDatabase const *>(this)->GetType(typeID));
    }

    bool ReflectionDatabase::IsTypeRegistered(StringID typeID) const
    {
        for (auto const &type : m_reflectedTypes)
        {
            if (type.typeID == typeID)
            {
                return true;
            }
        }

        return false;
    }

    bool ReflectionDatabase::IsTypeDerivedFrom(StringID typeID, StringID parentTypeID) const
    {
        auto pTypeDesc = GetType(typeID);
        ENGINE_ASSERT(pTypeDesc != nullptr);           // Unknown Type
        ENGINE_ASSERT(IsTypeRegistered(parentTypeID)); // Unknown Type

        // Check for same type
        if (typeID == parentTypeID)
        {
            return true;
        }

        // Check for immediate parents
        if (pTypeDesc->parentTypeID == parentTypeID)
        {
            return true;
        }

        // Recursively check parents
        if (IsTypeDerivedFrom(pTypeDesc->parentTypeID, parentTypeID))
        {
            return true;
        }

        return false;
    }

    void ReflectionDatabase::GetAllTypesForHeader(HeaderID headerID, List<DataType> &types) const
    {
        for (auto const &type : m_reflectedTypes)
        {
            if (type.headerID == headerID)
            {
                types.Add(type);
            }
        }
    }

    void ReflectionDatabase::GetAllTypesForProject(ProjectID projectID, List<DataType> &types) const
    {
        for (auto const &hdr : m_reflectedHeaders)
        {
            if (hdr.projectID == projectID)
            {
                GetAllTypesForHeader(hdr.headerId, types);
            }
        }
    }

    void ReflectionDatabase::RegisterType(DataType const *pType, bool onlyUpdateDevFlag)
    {
        if (onlyUpdateDevFlag)
        {
            auto pReflectedType = GetType(pType->typeID);
            ENGINE_ASSERT(pReflectedType != nullptr);
            pReflectedType->isDevOnly = false;

            for (auto const &property : pType->properties)
            {
                int foundIterIndex = pReflectedType->properties.Find(property);
                if (foundIterIndex != INVALID_INDEX)
                {
					pReflectedType->properties.At(foundIterIndex).isDevOnly = false;
                }
            }
        }
        else
        {
            ENGINE_ASSERT(pType != nullptr && !IsTypeRegistered(pType->typeID));
            m_reflectedTypes.push_back(*pType);
        }
    }

    DataProperty const *ReflectionDatabase::GetPropertyTypeDescriptor(StringID typeID, TypePropertyPath const &pathID) const
    {
        DataProperty const *pResolvedPropertyTypeDesc = nullptr;

        DataType const *pCurrentTypeDesc = GetType(typeID);
        if (pCurrentTypeDesc == nullptr)
        {
            return pResolvedPropertyTypeDesc;
        }

        // For each path element, get the type desc for that type
        size_t const numPathElements = pathID.GetNumElements();
        size_t const lastElementIdx = numPathElements - 1;
        for (size_t i = 0; i < numPathElements; i++)
        {
            pResolvedPropertyTypeDesc = pCurrentTypeDesc->GetPropertyDescriptor(pathID[i].propertyID);
            if (pResolvedPropertyTypeDesc == nullptr)
            {
                break;
            }

            if (i != lastElementIdx)
            {
                // Get the type desc of the property
                // ENGINE_ASSERT(!IsCoreType(pResolvedPropertyTypeDesc->typeID) && !pResolvedPropertyTypeDesc->IsEnumProperty());
                pCurrentTypeDesc = GetType(pResolvedPropertyTypeDesc->typeID);
                if (pCurrentTypeDesc == nullptr)
                {
                    LOG_ERROR("Type System", "ReflectionDatabase Cant resolve property path since it contains an unknown type");
                    return nullptr;
                }
            }
        }

        return pResolvedPropertyTypeDesc;
    }

    //-------------------------------------------------------------------------

/*    bool ReflectionDatabase::IsResourceRegistered(ResTypeID typeID) const
    {
        for (auto const &type : m_reflectedResourceTypes)
        {
            if (type.m_resourceTypeID == typeID)
            {
                return true;
            }
        }

        return false;
    }*/

/*    void ReflectionDatabase::RegisterResource(ReflectedResourceType const *pResource)
    {
        ENGINE_ASSERT(pResource != nullptr && !IsResourceRegistered(pResource->m_resourceTypeID));
        m_reflectedResourceTypes.emplace_back(*pResource);
    }*/

/*
    void ReflectionDatabase::GetAllRegisteredResourceTypesForProject(ProjectID projectID, std::vector<ReflectedResourceType> &resourceTypes) const
    {
        std::vector<ReflectedResourceType> const& registeredResourceTypes = m_reflectedResourceTypes;

        // Resources includes
        auto GetResourceTypeIDForTypeID = [&registeredResourceTypes] ( TypeID typeID )
        {
            for ( auto const& registeredResourceType : registeredResourceTypes )
            {
                if ( registeredResourceType.m_typeID == typeID )
                {
                    return registeredResourceType.m_resourceTypeID;
                }
            }

            ENGINE_UNREACHABLE_CODE();
            return ResTypeID();
        };

        for (auto& registeredResourceType : registeredResourceTypes)
        {
            if (GetHeaderDesc(registeredResourceType.m_headerID)->m_projectID == projectID)
            {
                resourceTypes.emplace_back(registeredResourceType);
            }
        }
    }
*/

    //-------------------------------------------------------------------------

    void ReflectionDatabase::DeleteTypesForHeader(HeaderID headerID)
    {
        for (auto j = (int32_t)m_reflectedTypes.size() - 1; j >= 0; j--)
        {
            if (m_reflectedTypes[j].headerID == headerID)
            {
                m_reflectedTypes.erase(m_reflectedTypes.begin() + j);
            }
        }

        for (auto j = (int32_t)m_reflectedResourceTypes.size() - 1; j >= 0; j--)
        {
            if (m_reflectedResourceTypes[j].headerID == headerID)
            {
                m_reflectedResourceTypes.erase(m_reflectedResourceTypes.begin() + j);
            }
        }
    }

    void ReflectionDatabase::DeleteObseleteHeadersAndTypes(List<HeaderID> const &registeredHeaders)
    {
        for (auto i = (int32_t)m_reflectedHeaders.size() - 1; i >= 0; i--)
        {
            auto const hdrID = m_reflectedHeaders[i].headerId;
            if (!registeredHeaders.Contains( hdrID))
            {
                DeleteTypesForHeader(hdrID);
                m_reflectedHeaders.erase(m_reflectedHeaders.begin() + i);
            }
        }
    }

    void ReflectionDatabase::DeleteObseleteProjects(List<ProjectInfo> const &registeredProjects)
    {
        for (auto i = (int32_t)m_reflectedProjects.size() - 1; i >= 0; i--)
        {
            auto const prjID = m_reflectedProjects[i].id;
			auto  p = [prjID](ProjectInfo const &desc) -> bool
			{ return desc.id == prjID; };

            if (std::find_if(registeredProjects.begin(), registeredProjects.end(), p) == registeredProjects.end())
            {
                m_reflectedProjects.erase(m_reflectedProjects.begin() + i);
            }
        }
    }

    //-------------------------------------------------------------------------

    bool ReflectionDatabase::ReadDatabase(StringAnsi const &databasePath)
    {
        if (!Connect(databasePath, false))
        {
            return false;
        }

        if (!CreateTables())
        {
            return false;
        }

        m_reflectedProjects.clear();
        m_reflectedHeaders.clear();
        m_reflectedTypes.clear();

        // Read all projects
        //-------------------------------------------------------------------------

        sqlite3_stmt *pStatement = nullptr;
        FillStatementBuffer("SELECT * FROM `Modules` ORDER BY `DependencyCount` ASC;");
        if (IsValidSQLiteResult(sqlite3_prepare_v2(m_pDatabase, m_statementBuffer, -1, &pStatement, nullptr)))
        {
            while (sqlite3_step(pStatement) == SQLITE_ROW)
            {
                ProjectInfo project;
                project.id = StringID(sqlite3_column_int(pStatement, 0));
                project.name = (char const *)sqlite3_column_text(pStatement, 1);
                project.path = (char const *)sqlite3_column_text(pStatement, 2);
                project.exportMacro = (char const *)sqlite3_column_text(pStatement, 3);
                project.moduleClassNameFull = (char const *)sqlite3_column_text(pStatement, 4);
                project.moduleHeaderID = StringID(sqlite3_column_int(pStatement, 5));
                project.dependencyCount = sqlite3_column_int(pStatement, 6);
                m_reflectedProjects.emplace_back(project);
            }

            if (!IsValidSQLiteResult(sqlite3_finalize(pStatement)))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        // Read all headers
        //-------------------------------------------------------------------------

        FillStatementBuffer("SELECT * FROM `HeaderFiles`;");
        if (IsValidSQLiteResult(sqlite3_prepare_v2(m_pDatabase, m_statementBuffer, -1, &pStatement, nullptr)))
        {
            while (sqlite3_step(pStatement) == SQLITE_ROW)
            {
                HeaderInfo header;
                header.headerId = StringID(sqlite3_column_int(pStatement, 0));
                header.projectID = StringID(sqlite3_column_int(pStatement, 1));
                header.filePath = (char const *)sqlite3_column_text(pStatement, 2);
                header.timestamp = sqlite3_column_int64(pStatement, 3);
                header.checksum = sqlite3_column_int64(pStatement, 4);
                m_reflectedHeaders.emplace_back(header);
            }

            if (!IsValidSQLiteResult(sqlite3_finalize(pStatement)))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        // Read all types
        //-------------------------------------------------------------------------

        FillStatementBuffer("SELECT * FROM `Types`;");
        if (IsValidSQLiteResult(sqlite3_prepare_v2(m_pDatabase, m_statementBuffer, -1, &pStatement, nullptr)))
        {
            while (sqlite3_step(pStatement) == SQLITE_ROW)
            {
                DataType type;
                type.typeID = TypeID(sqlite3_column_int(pStatement, 0));
                type.parentTypeID = TypeID(sqlite3_column_int(pStatement, 1));
                type.headerID = StringID(sqlite3_column_int(pStatement, 2));
                type.name = (char const *)sqlite3_column_text(pStatement, 3);
                type.namespaceName = (char const *)sqlite3_column_text(pStatement, 4);
                type.flags.Set((uint32_t)sqlite3_column_int(pStatement, 5));

                // Read additional type data
                if (type.IsEnum())
                {
                    if (!ReadAdditionalEnumData(type))
                    {
                        return false;
                    }
                }
                else
                {
                    if (!ReadAdditionalTypeData(type))
                    {
                        return false;
                    }
                }

                // Add type to list
                m_reflectedTypes.push_back(type);
            }

            if (!IsValidSQLiteResult(sqlite3_finalize(pStatement)))
            {
                return false;
            }
            pStatement = nullptr;
        }
        else
        {
            return false;
        }

        // Read all resource types
        //-------------------------------------------------------------------------

        /*FillStatementBuffer("SELECT * FROM `ResourceTypes`;");
        if (IsValidSQLiteResult(sqlite3_prepare_v2(m_pDatabase, m_statementBuffer, -1, &pStatement, nullptr)))
        {
            while (sqlite3_step(pStatement) == SQLITE_ROW)
            {
                ReflectedResourceType resourceType;
                resourceType.m_typeID = TypeID(String((char const *)sqlite3_column_text(pStatement, 0)));
                resourceType.m_resourceTypeID = TypeID(sqlite3_column_int64(pStatement, 1));
                resourceType.m_friendlyName = (char const *)sqlite3_column_text(pStatement, 2);
                resourceType.m_headerID = StringID(sqlite3_column_int(pStatement, 3));
                resourceType.m_className = (char const *)sqlite3_column_text(pStatement, 4);
                resourceType.m_namespace = (char const *)sqlite3_column_text(pStatement, 5);
                m_reflectedResourceTypes.emplace_back(resourceType);

                if (!ReadAdditionalResourceTypeData(resourceType))
                {
                    return false;
                }
            }

            if (!IsValidSQLiteResult(sqlite3_finalize(pStatement)))
            {
                return false;
            }
        }
        else
        {
            return false;
        }*/

        return Disconnect();
    }

    bool ReflectionDatabase::WriteDatabase(StringAnsi const &databasePath)
    {
        if (!Connect(databasePath, false))
        {
            return false;
        }

        BeginTransaction();

        //-------------------------------------------------------------------------

        if (!DropTables())
        {
            return false;
        }

        if (!CreateTables())
        {
            return false;
        }

        // Write all projects
        //-------------------------------------------------------------------------

        for (auto const &project : m_reflectedProjects)
        {
            if (!ExecuteSimpleQuery("INSERT OR REPLACE INTO `Modules`(`ModuleID`, `Name`, `Path`, `ExportMacro`, `ModuleClassName`, `ModuleHeaderID`, `DependencyCount`) VALUES ( %u, \"%s\", \"%s\",\"%s\",\"%s\", %u, %u);",
				(uint32_t)project.id, project.name.Get(), project.path.ToStringAnsi().Get(), project.exportMacro.Get(), project.moduleClassNameFull.Get(), (uint32_t)project.moduleHeaderID, project.dependencyCount))
            {
                return false;
            }
        }

        // Write all headers
        //-------------------------------------------------------------------------

        for (auto const &header : m_reflectedHeaders)
        {
            if (!ExecuteSimpleQuery("INSERT OR REPLACE INTO `HeaderFiles`(`HeaderID`,`ModuleID`,`FilePath`,`TimeStamp`,`Checksum`) VALUES ( %u, %u, \"%s\",%llu,%llu);",
				(uint32_t)header.headerId, (uint32_t)header.projectID, header.filePath.Get(), header.timestamp, header.checksum))
            {
                return false;
            }
        }

        // Write all types
        //-------------------------------------------------------------------------

        for (auto const &type : m_reflectedTypes)
        {
            if (!ExecuteSimpleQuery("INSERT OR REPLACE INTO `Types`(`TypeID`, `ParentID`, `HeaderID`,`Name`,`Namespace`,`TypeFlags`) VALUES ( %u, %u, %u, \"%s\", \"%s\", %u );",
				(uint32_t)type.typeID, (uint32_t)type.parentTypeID, (uint32_t)type.headerID, type.name.Get(), type.namespaceName.Get(), (uint32_t)type.flags.Get()))
            {
                return false;
            }

            if (type.IsEnum())
            {
                if (!WriteAdditionalEnumData(type))
                {
                    return false;
                }
            }
            else
            {
                if (!WriteAdditionalTypeData(type))
                {
                    return false;
                }
            }
        }

        // Write all resources types
        //-------------------------------------------------------------------------

        /*for (auto const &resourceType : m_reflectedResourceTypes)
        {
            if (!ExecuteSimpleQuery("INSERT OR REPLACE INTO `ResourceTypes`( `TypeID`, `ResourceTypeID`, `FriendlyName`, `HeaderID`,`ClassName`,`Namespace`) VALUES ( \"%s\", %u, \"%s\", %u, \"%s\",\"%s\");",
				resourceType.m_typeID.ToString().Get(), (resourceType.m_resourceTypeID, resourceType.m_friendlyName.Get(), (uint32_t)resourceType.m_headerID, resourceType.m_className.Get(), resourceType.m_namespace.Get()))
            {
                return false;
            }

            if (!WriteAdditionalResourceTypeData(resourceType))
            {
                return false;
            }
        }*/

        // Update database info
        //-------------------------------------------------------------------------

        if (!ExecuteSimpleQuery("DELETE FROM `DatabaseInfo`; INSERT OR REPLACE INTO `DatabaseInfo`(`LastUpdated` ) VALUES( CURRENT_TIMESTAMP );"))
        {
            return false;
        }

        //-------------------------------------------------------------------------

        EndTransaction();

        return Disconnect();
    }

    bool ReflectionDatabase::CreateTables()
    {
        ENGINE_ASSERT(m_pDatabase != nullptr);

        // Project / Header tables
        //-------------------------------------------------------------------------

        if (!ExecuteSimpleQuery("CREATE TABLE IF NOT EXISTS `Modules` ( `ModuleID` INTEGER UNIQUE, `Name` TEXT UNIQUE, `Path` TEXT, `ExportMacro` TEXT, `ModuleClassName` TEXT NOT NULL, `ModuleHeaderID` INTEGER UNIQUE NOT NULL, `DependencyCount` INTEGER, PRIMARY KEY( `ModuleID` ) );"))
        {
            return false;
        }

        if (!ExecuteSimpleQuery("CREATE TABLE IF NOT EXISTS `HeaderFiles` ( `HeaderID` INTEGER NOT NULL UNIQUE, `ModuleID` INTEGER NOT NULL, `FilePath` TEXT NOT NULL, `TimeStamp` INTEGER NOT NULL, `Checksum` INTEGER NOT NULL, PRIMARY KEY( `HeaderID` ) );"))
        {
            return false;
        }

        // Type registration tables
        //-------------------------------------------------------------------------

        if (!ExecuteSimpleQuery("CREATE TABLE IF NOT EXISTS `Types` ( `TypeID` INTEGER UNIQUE, `ParentID` INTEGER, `HeaderID` INTEGER, `Name` TEXT, `Namespace` TEXT, `TypeFlags` INTEGER, PRIMARY KEY( `TypeID` ) );"))
        {
            return false;
        }

        if (!ExecuteSimpleQuery("CREATE TABLE IF NOT EXISTS `Properties` ( `PropertyID` INTEGER, `LineNumber` INTEGER, `OwnerTypeID` INTEGER, `TypeID` INTEGER, `Name` TEXT, `Description` TEXT, `TypeName` TEXT, `TemplateTypeName` TEXT, `PropertyFlags` INTEGER, `ArraySize` INTEGER DEFAULT -1, `MetaData` TEXT, PRIMARY KEY( PropertyID, OwnerTypeID ) );"))
        {
            return false;
        }

        if (!ExecuteSimpleQuery("CREATE TABLE IF NOT EXISTS `EnumConstants` ( `TypeID` INTEGER NOT NULL, `Label` TEXT NOT NULL, `Value` INTEGER, `Description` TEXT, PRIMARY KEY(TypeID, Label) );"))
        {
            return false;
        }

        if (!ExecuteSimpleQuery("CREATE TABLE IF NOT EXISTS `ResourceTypes` ( `TypeID` TEXT, `ResourceTypeID` INTEGER, `FriendlyName` TEXT, `HeaderID` INTEGER, `ClassName` TEXT, `Namespace` TEXT, PRIMARY KEY( `ResourceTypeID`) );"))
        {
            return false;
        }

        if (!ExecuteSimpleQuery("CREATE TABLE IF NOT EXISTS `ResourceTypeParents` ( `TypeID` TEXT, `ParentTypeID` TEXT, PRIMARY KEY( TypeID, ParentTypeID ) );"))
        {
            return false;
        }

        // Database info table
        //-------------------------------------------------------------------------

        if (!ExecuteSimpleQuery("CREATE TABLE IF NOT EXISTS `DatabaseInfo` ( `LastUpdated` NUMERIC, PRIMARY KEY( `LastUpdated`) );"))
        {
            return false;
        }

        return true;
    }

    bool ReflectionDatabase::DropTables()
    {
        ENGINE_ASSERT(m_pDatabase != nullptr);

        if (!ExecuteSimpleQuery("DROP TABLE IF EXISTS `Modules`;"))
        {
            return false;
        }

        if (!ExecuteSimpleQuery("DROP TABLE IF EXISTS `HeaderFiles`;"))
        {
            return false;
        }

        if (!ExecuteSimpleQuery("DROP TABLE IF EXISTS `Types`;"))
        {
            return false;
        }

        if (!ExecuteSimpleQuery("DROP TABLE IF EXISTS `Properties`;"))
        {
            return false;
        }

        if (!ExecuteSimpleQuery("DROP TABLE IF EXISTS `EnumConstants`;"))
        {
            return false;
        }

        if (!ExecuteSimpleQuery("DROP TABLE IF EXISTS `ResourceTypes`;"))
        {
            return false;
        }

        if (!ExecuteSimpleQuery("DROP TABLE IF EXISTS `ResourceTypeParents`;"))
        {
            return false;
        }

        if (!ExecuteSimpleQuery("DROP TABLE IF EXISTS `DatabaseInfo`;"))
        {
            return false;
        }

        return true;
    }

    //-------------------------------------------------------------------------

    bool ReflectionDatabase::ReadAdditionalTypeData(DataType &type)
    {
        ENGINE_ASSERT(type.typeID != StringID::Invalid && !type.IsEnum());

        sqlite3_stmt *pStatement = nullptr;

        // Get all properties
        //-------------------------------------------------------------------------

        FillStatementBuffer("SELECT * FROM `Properties` WHERE `OwnerTypeID` = %u;", (uint32_t)type.typeID);
        if (IsValidSQLiteResult(sqlite3_prepare_v2(m_pDatabase, m_statementBuffer, -1, &pStatement, nullptr)))
        {
            while (sqlite3_step(pStatement) == SQLITE_ROW)
            {
                DataProperty propDesc;
                propDesc.lineNumber = sqlite3_column_int(pStatement, 1);
                propDesc.typeID = TypeID(sqlite3_column_int(pStatement, 3));
                propDesc.name = (char const *)sqlite3_column_text(pStatement, 4);
                propDesc.description = (char const *)sqlite3_column_text(pStatement, 5);
                propDesc.typeName = (char const *)sqlite3_column_text(pStatement, 6);
                propDesc.templateArgTypeName = (char const *)sqlite3_column_text(pStatement, 7);
                propDesc.flags.Set((uint32_t)sqlite3_column_int(pStatement, 8));
                propDesc.arraySize = sqlite3_column_int(pStatement, 9);
                propDesc.metaData = (char const *)sqlite3_column_text(pStatement, 10);
                propDesc.propertyID = TypeID(propDesc.name.ToString());
                ENGINE_ASSERT(propDesc.propertyID == (uint32_t)sqlite3_column_int(pStatement, 0)); // Ensure the property ID matches the recorded one

                type.properties.Add(propDesc);
            }

            if (!IsValidSQLiteResult(sqlite3_finalize(pStatement)))
            {
                return false;
            }

            //-------------------------------------------------------------------------
			Function<bool(DataProperty const &, DataProperty const &)> compare = [](DataProperty const &a, DataProperty const &b)
			{ return a.lineNumber < b.lineNumber; };

			Sorting::QuickSort(type.properties, compare);

            //-------------------------------------------------------------------------

            pStatement = nullptr;
            return true;
        }

        return false;
    }

    bool ReflectionDatabase::ReadAdditionalEnumData(DataType &type)
    {
        ENGINE_ASSERT(type.typeID != StringID::Invalid && type.IsEnum());

        sqlite3_stmt *pStatement = nullptr;
        FillStatementBuffer("SELECT `Label`, `Value`, `Description` FROM `EnumConstants` WHERE `EnumConstants`.TypeID = %u;", (uint32_t)type.typeID);
        if (IsValidSQLiteResult(sqlite3_prepare_v2(m_pDatabase, m_statementBuffer, -1, &pStatement, nullptr)))
        {
            while (sqlite3_step(pStatement) == SQLITE_ROW)
            {
                ReflectedEnumConstant constantDesc;
                constantDesc.label = (char const *)sqlite3_column_text(pStatement, 0);
                constantDesc.ID = StringID(String(constantDesc.label));
                constantDesc.value = sqlite3_column_int(pStatement, 1);
                constantDesc.description = (char const *)sqlite3_column_text(pStatement, 2);
                type.AddEnumConstant(constantDesc);
            }

            if (!IsValidSQLiteResult(sqlite3_finalize(pStatement)))
            {
                return false;
            }

            pStatement = nullptr;
            return true;
        }

        return false;
    }

    bool ReflectionDatabase::ReadAdditionalResourceTypeData(ReflectedResourceType &type)
    {
        ENGINE_ASSERT(type.typeID != TypeID::Invalid);

        sqlite3_stmt *pStatement = nullptr;
        FillStatementBuffer("SELECT `ResourceTypeParents`.ParentTypeID FROM `ResourceTypeParents` INNER JOIN `ResourceTypes` ON `ResourceTypes`.TypeID = `ResourceTypeParents`.ParentTypeID WHERE `ResourceTypeParents`.TypeID = \"%s\";",
			type.typeID.ToString().Get());
        if (IsValidSQLiteResult(sqlite3_prepare_v2(m_pDatabase, m_statementBuffer, -1, &pStatement, nullptr)))
        {
            while (sqlite3_step(pStatement) == SQLITE_ROW)
            {
                type.parents.Add(StringID(String((char const *)sqlite3_column_text(pStatement, 0))));
            }

            if (!IsValidSQLiteResult(sqlite3_finalize(pStatement)))
            {
                return false;
            }
            pStatement = nullptr;
        }
        else
        {
            return false;
        }

        return true;
    }

    bool ReflectionDatabase::WriteAdditionalTypeData(DataType const &type)
    {
        // Delete old properties
        if (!ExecuteSimpleQuery("DELETE FROM `Properties` WHERE `OwnerTypeID` = %u;", (uint32_t)type.typeID))
        {
            return false;
        }

        // Update properties
        for (auto &propertyDesc : type.properties)
        {
			StringAnsi escapedDescription = propertyDesc.description;
			escapedDescription.Replace("\"", "\"\"");

			StringAnsi escapedMetaData;
            if (propertyDesc.HasMetaData())
            {
                escapedMetaData = propertyDesc.metaData;
				escapedMetaData.Replace("\"", "\"\"");
            }

            if (!ExecuteSimpleQuery("INSERT OR REPLACE INTO `Properties`(`PropertyID`, `LineNumber`, `OwnerTypeID`,`TypeID`,`Name`,`Description`,`TypeName`,`TemplateTypeName`,`PropertyFlags`,`ArraySize`,`MetaData`) VALUES ( %u, %d, %u, %u, \"%s\", \"%s\", \"%s\", \"%s\", %u, %d, \"%s\" );", (uint32_t)propertyDesc.propertyID, propertyDesc.lineNumber, (uint32_t)type.typeID, (uint32_t)propertyDesc.typeID, propertyDesc.name.Get(), escapedDescription.Get(), propertyDesc.typeName.Get(), propertyDesc.templateArgTypeName.Get(), (uint32_t)propertyDesc.flags.Get(), propertyDesc.arraySize, escapedMetaData.Get()))
            {
                return false;
            }
        }

        return true;
    }

    bool ReflectionDatabase::WriteAdditionalEnumData(DataType const &type)
    {
        // Fill enum values table with all constants
        if (!ExecuteSimpleQuery("DELETE FROM `EnumConstants` WHERE `TypeID` = %u;", (uint32_t)type.typeID))
        {
            return false;
        }

        for (auto const &enumConstant : type.enumConstants)
        {
			StringAnsi escapedDescription = enumConstant.description;
			escapedDescription.Replace("\"", "\"\"");

            if (!ExecuteSimpleQuery("INSERT INTO `EnumConstants`(`TypeID`,`Label`,`Value`, `Description`) VALUES ( %u, \"%s\", %u, \"%s\" );", (uint32_t)type.typeID, enumConstant.label.Get(), enumConstant.value, escapedDescription.Get()))
            {
                return false;
            }
        }

        return true;
    }

    bool ReflectionDatabase::WriteAdditionalResourceTypeData(ReflectedResourceType const &type)
    {
        // Delete old parents
        if (!ExecuteSimpleQuery("DELETE FROM `ResourceTypeParents` WHERE `TypeID` = \"%s\";", type.typeID.ToString().Get()))
        {
            return false;
        }

        // Update Type Parents
        for (auto &parent : type.parents)
        {
            if (!ExecuteSimpleQuery("INSERT INTO `ResourceTypeParents`(`TypeID`, `ParentTypeID`) VALUES ( \"%s\", \"%s\" );",
				type.typeID.ToString().Get(), parent.ToString().Get()))
            {
                return false;
            }
        }

        return true;
    }
}

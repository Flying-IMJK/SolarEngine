#include "TypeDatabase.h"
#include "Core/Utils.h"

//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    TypeDatabase::TypeDatabase()
    {
        // Create the base class for all registered engine types
        //-------------------------------------------------------------------------
        std::string str = Utils::String::Format("{0}::{1}", std::string_view(Settings::g_engineNamespace), Settings::g_reflectedTypeInterfaceClassName);
        auto typeBase = std::make_unique<TypeInfoStruct>(StringID(str), std::string(Settings::g_reflectedTypeInterfaceClassName));
        typeBase->isAbstract = true;
        typeBase->namespaceScopeList.push_back(Settings::g_engineNamespace);
        m_reflectedTypeBase = std::move(typeBase);
    }

    TypeDatabase::~TypeDatabase()
    {
        if (IsConnected())
        {
            Disconnect();
        }
    }

    //-------------------------------------------------------------------------

    bool TypeDatabase::Connect(std::string const &databasePath, bool readOnlyAccess, bool useMutex)
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

        auto const result = sqlite3_open_v2(databasePath.c_str(), &m_pDatabase, sqlFlags, nullptr);
        sqlite3_busy_timeout(m_pDatabase, 2500);

        if (result != SQLITE_OK)
        {
            sqlite3_close(m_pDatabase);
            m_pDatabase = nullptr;
            m_errorMessage = Utils::String::Format("Couldn't open sqlite database: {0}", databasePath);
            return false;
        }

        return true;
    }

    bool TypeDatabase::Disconnect()
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

    bool TypeDatabase::IsValidSQLiteResult(int result, char const *pErrorMessage) const
    {
        if (result != SQLITE_OK)
        {
            m_errorMessage = Utils::String::Format("{0} ( {1} )", sqlite3_errstr(result), sqlite3_errmsg(m_pDatabase));
            return false;
        }

        return true;
    }

    void TypeDatabase::FillStatementBuffer(char const *pFormat, ...) const
    {
        ENGINE_ASSERT(IsConnected());

        // Create the statement using the sqlite printf so we can use the extra format specifiers i.e. %Q
        va_list args;
        va_start(args, pFormat);
        sqlite3_vsnprintf(s_defaultStatementBufferSize, m_statementBuffer, pFormat, args);
        va_end(args);
    }

    bool TypeDatabase::ExecuteSimpleQuery(char const *pFormat, ...) const
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
            std::string const sqlStatementStr = Utils::String::Format(" ( SQL: {0} )", m_statementBuffer);
            m_errorMessage += sqlStatementStr;
            return false;
        }

        return true;
    }

    bool TypeDatabase::BeginTransaction() const
    {
        ENGINE_ASSERT(IsConnected());
        return IsValidSQLiteResult(sqlite3_exec(m_pDatabase, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr));
    }

    bool TypeDatabase::EndTransaction() const
    {
        ENGINE_ASSERT(IsConnected());
        return IsValidSQLiteResult(sqlite3_exec(m_pDatabase, "END TRANSACTION;", nullptr, nullptr, nullptr));
    }

    bool TypeDatabase::IsProjectRegistered(ProjectID projectID) const
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

    ProjectInfo const *TypeDatabase::GetProjectDesc(ProjectID projectID) const
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

    void TypeDatabase::UpdateProjectList(std::vector<ProjectInfo> const &registeredProjects)
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
                if (updatedProject.moduleClassNameFull.empty())
                {
                    updatedProject.moduleClassNameFull = existingProj->moduleClassNameFull;
                }
                if (updatedProject.moduleClassName.empty())
                {
                    updatedProject.moduleClassName = existingProj->moduleClassName;
                }

				*existingProj = updatedProject;
            }
        }
    }

    bool TypeDatabase::IsHeaderRegistered(HeaderID headerID) const
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

    HeaderInfo const *TypeDatabase::GetHeaderDesc(HeaderID headerID) const
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

    void TypeDatabase::UpdateHeaderRecord(HeaderInfo const &header)
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

    TypeInfoBase const *TypeDatabase::GetType(StringID typeID) const
    {
        if (m_reflectedTypeBase->typeID == typeID)
        {
            return m_reflectedTypeBase.get();
        }

        for (auto const &type : m_ownedTypes)
        {
            if (type->typeID == typeID)
            {
                return type.get();
            }
        }

        return nullptr;
    }

    TypeInfoBase* TypeDatabase::GetType(StringID typeID)
    {
        return const_cast<TypeInfoBase *>(const_cast<TypeDatabase const *>(this)->GetType(typeID));
    }

    bool TypeDatabase::IsTypeRegistered(StringID typeID) const
    {
        if (m_reflectedTypeBase->typeID == typeID)
        {
            return true;
        }

        for (auto const &type : m_ownedTypes)
        {
            if (type->typeID == typeID)
            {
                return true;
            }
        }

        return false;
    }

    std::vector<TypeInfoBase const*> TypeDatabase::GetAllTypes() const
    {
        std::vector<TypeInfoBase const*> types;
        types.reserve(m_ownedTypes.size());
        for (auto const& type : m_ownedTypes)
        {
            types.push_back(type.get());
        }
        return types;
    }

    std::vector<TypeInfoBase*> TypeDatabase::GetAllTypes()
    {
        std::vector<TypeInfoBase*> types;
        types.reserve(m_ownedTypes.size());
        for (auto& type : m_ownedTypes)
        {
            types.push_back(type.get());
        }
        return types;
    }

    bool TypeDatabase::IsTypeDerivedFrom(StringID typeID, StringID parentTypeID) const
    {
        // Check for same type
        if (typeID == parentTypeID)
        {
            return true;
        }

        auto pTypeDesc = GetType(typeID);

        ENGINE_ASSERT(pTypeDesc != nullptr);           // Unknown Type
        ENGINE_ASSERT(IsTypeRegistered(parentTypeID)); // Unknown Type

        if (!pTypeDesc->IsFlag(TypeInfoBase::Flag::IsStruct))
        {
            return false;
        }

        const TypeInfoStruct* pStructTypeDesc = static_cast<const TypeInfoStruct*>(pTypeDesc);

        // Check for immediate parents
        if (pStructTypeDesc->parentTypeID == parentTypeID)
        {
            return true;
        }

        // Recursively check parents
        if (IsTypeDerivedFrom(pStructTypeDesc->parentTypeID, parentTypeID))
        {
            return true;
        }

        return false;
    }

    void TypeDatabase::GetAllTypesForHeader(HeaderID headerID, std::vector<TypeInfoBase*> &types) const
    {
        for (auto const &type : m_ownedTypes)
        {
            if (type->headerID == headerID)
            {
                types.push_back(type.get());
            }
        }
    }

    void TypeDatabase::GetAllTypesForProject(ProjectID projectID, std::vector<TypeInfoBase*> &types) const
    {
        for (auto const &hdr : m_reflectedHeaders)
        {
            if (hdr.projectID == projectID)
            {
                GetAllTypesForHeader(hdr.headerId, types);
            }
        }
    }

    TypeInfoBase* TypeDatabase::RegisterType(std::unique_ptr<TypeInfoBase> type, bool onlyUpdateDevFlag)
    {
        (void)onlyUpdateDevFlag;
        ENGINE_ASSERT(type != nullptr && !IsTypeRegistered(type->typeID));
        TypeInfoBase* pType = type.get();
        m_ownedTypes.emplace_back(std::move(type));
        return pType;
    }

    TypeInfoInjectedCode* TypeDatabase::RegisterInjectCode(std::unique_ptr<TypeInfoInjectedCode> injectCode)
    {
        ENGINE_ASSERT(injectCode != nullptr);
        TypeInfoInjectedCode* pInjectCode = injectCode.get();
        m_injectedCode.emplace_back(std::move(injectCode));
        return pInjectCode;
    }

    //-------------------------------------------------------------------------

    void TypeDatabase::GetInjectCodeFormHead(HeaderID headerID, std::vector<TypeInfoInjectedCode*>& injectCodes) const
    {
        for (int index = 0; index < m_injectedCode.size(); index++)
        {
            auto const& item = m_injectedCode[index];

            if (item->headID != headerID)
            {
                continue;
            }

            injectCodes.emplace_back(item.get());
        }
    }

/*    void TypeDatabase::RegisterResource(ReflectedResourceType const *pResource)
    {
        ENGINE_ASSERT(pResource != nullptr && !IsResourceRegistered(pResource->m_resourceTypeID));
        m_reflectedResourceTypes.emplace_back(*pResource);
    }*/

/*
    void TypeDatabase::GetAllRegisteredResourceTypesForProject(ProjectID projectID, std::vector<ReflectedResourceType> &resourceTypes) const
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

    void TypeDatabase::DeleteTypesForHeader(HeaderID headerID)
    {
        for (auto j = (int32_t)m_ownedTypes.size() - 1; j >= 0; j--)
        {
            if (m_ownedTypes[j]->headerID == headerID)
            {
                m_ownedTypes.erase(m_ownedTypes.begin() + j);
            }
        }
        for (auto j = (int32_t)m_injectedCode.size() - 1; j >= 0; j--)
        {
            if (m_injectedCode[j]->headID == headerID)
            {
                m_injectedCode.erase(m_injectedCode.begin() + j);
            }
        }
    }

    void TypeDatabase::DeleteObseleteHeadersAndTypes(std::vector<HeaderID> const &registeredHeaders)
    {
        for (auto i = (int32_t)m_reflectedHeaders.size() - 1; i >= 0; i--)
        {
            auto const hdrID = m_reflectedHeaders[i].headerId;
            if (!Utils::Vector::Contains(registeredHeaders, hdrID))
            {
                DeleteTypesForHeader(hdrID);
                m_reflectedHeaders.erase(m_reflectedHeaders.begin() + i);
            }
        }
    }

    void TypeDatabase::DeleteObseleteProjects(std::vector<ProjectInfo> const &registeredProjects)
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

    bool TypeDatabase::ReadDatabase(std::string const &databasePath)
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
        m_ownedTypes.clear();
        m_injectedCode.clear();

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

        //FillStatementBuffer("SELECT * FROM `Types`;");
        //if (IsValidSQLiteResult(sqlite3_prepare_v2(m_pDatabase, m_statementBuffer, -1, &pStatement, nullptr)))
        //{
        //    while (sqlite3_step(pStatement) == SQLITE_ROW)
        //    {
        //        TypeInfoBase type = new ;
        //        type.typeID = TypeID(sqlite3_column_int(pStatement, 0));
        //        type.parentTypeID = TypeID(sqlite3_column_int(pStatement, 1));
        //        type.headerID = StringID(sqlite3_column_int(pStatement, 2));
        //        type.name = (char const *)sqlite3_column_text(pStatement, 3);
        //        type.namespaceScopeList = Utils::SplitString((char const *)sqlite3_column_text(pStatement, 4), "::");
        //        type.flags.Set((uint32_t)sqlite3_column_int(pStatement, 5));

        //        // Read additional type data
        //        if (type.IsFlag(TypeInfoBase::Flag::IsEnum))
        //        {
        //            if (!ReadAdditionalEnumData(type))
        //            {
        //                return false;
        //            }
        //        }
        //        else
        //        {
        //            if (!ReadAdditionalTypeData(type))
        //            {
        //                return false;
        //            }
        //        }

        //        // Add type to list
        //        m_ownedTypes.push_back(std::move(type));
        //    }

        //    if (!IsValidSQLiteResult(sqlite3_finalize(pStatement)))
        //    {
        //        return false;
        //    }
        //    pStatement = nullptr;
        //}
        //else
        //{
        //    return false;
        //}


        return Disconnect();
    }

    bool TypeDatabase::WriteDatabase(std::string const &databasePath)
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

    //    for (auto const &project : m_reflectedProjects)
    //    {
    //        if (!ExecuteSimpleQuery("INSERT OR REPLACE INTO `Modules`(`ModuleID`, `Name`, `Path`, `ExportMacro`, `ModuleClassName`, `ModuleHeaderID`, `DependencyCount`) VALUES ( %u, \"%s\", \"%s\",\"%s\",\"%s\", %u, %u);",
				//(uint32_t)project.id, project.name.c_str(), project.path.c_str(), project.exportMacro.c_str(), project.moduleClassNameFull.c_str(), (uint32_t)project.moduleHeaderID, project.dependencyCount))
    //        {
    //            return false;
    //        }
    //    }

        // Write all headers
        //-------------------------------------------------------------------------

    //    for (auto const &header : m_reflectedHeaders)
    //    {
    //        if (!ExecuteSimpleQuery("INSERT OR REPLACE INTO `HeaderFiles`(`HeaderID`,`ModuleID`,`FilePath`,`TimeStamp`,`Checksum`) VALUES ( %u, %u, \"%s\",%llu,%llu);",
				//(uint32_t)header.headerId, (uint32_t)header.projectID, header.filePath.c_str(), header.timestamp, header.checksum))
    //        {
    //            return false;
    //        }
    //    }

        // Write all types
        //-------------------------------------------------------------------------

    //    for (auto const &type : m_ownedTypes)
    //    {
    //        std::string namespaceName = Utils::CombineStringList(type.namespaceScopeList, "::");
    //        if (!ExecuteSimpleQuery("INSERT OR REPLACE INTO `Types`(`TypeID`, `ParentID`, `HeaderID`,`Name`,`Namespace`,`TypeFlags`) VALUES ( %u, %u, %u, \"%s\", \"%s\", %u );",
				//(uint32_t)type.typeID, (uint32_t)type.parentTypeID, (uint32_t)type.headerID, type.name.c_str(), namespaceName.c_str(), (uint32_t)type.flags.Get()))
    //        {
    //            return false;
    //        }

    //        if (type.IsFlag(TypeInfoBase::Flag::IsEnum))
    //        {
    //            if (!WriteAdditionalEnumData(type))
    //            {
    //                return false;
    //            }
    //        }
    //        else
    //        {
    //            if (!WriteAdditionalTypeData(type))
    //            {
    //                return false;
    //            }
    //        }
    //    }

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

    bool TypeDatabase::CreateTables()
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

    bool TypeDatabase::DropTables()
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

    bool TypeDatabase::ReadAdditionalTypeData(TypeInfoBase &type)
    {
        ENGINE_ASSERT(type.typeID != StringID::Invalid && !type.IsFlag(TypeInfoBase::Flag::IsEnum));

        sqlite3_stmt *pStatement = nullptr;

        // Get all properties
        //-------------------------------------------------------------------------

        FillStatementBuffer("SELECT * FROM `Properties` WHERE `OwnerTypeID` = %u;", (uint32_t)type.typeID);
        if (IsValidSQLiteResult(sqlite3_prepare_v2(m_pDatabase, m_statementBuffer, -1, &pStatement, nullptr)))
        {
            while (sqlite3_step(pStatement) == SQLITE_ROW)
            {
                PropertyData propDesc;
                propDesc.lineNumber = sqlite3_column_int(pStatement, 1);
                propDesc.typeID = TypeID(sqlite3_column_int(pStatement, 3));
                propDesc.name = (char const *)sqlite3_column_text(pStatement, 4);
                propDesc.description = (char const *)sqlite3_column_text(pStatement, 5);
                propDesc.typeName = (char const *)sqlite3_column_text(pStatement, 6);
                propDesc.templateArgTypeName = (char const *)sqlite3_column_text(pStatement, 7);
                propDesc.flags.Set((uint32_t)sqlite3_column_int(pStatement, 8));
                propDesc.arraySize = sqlite3_column_int(pStatement, 9);
                propDesc.metaData = (char const *)sqlite3_column_text(pStatement, 10);
                propDesc.propertyID = TypeID(propDesc.name);
                ENGINE_ASSERT(propDesc.propertyID == (uint32_t)sqlite3_column_int(pStatement, 0)); // Ensure the property ID matches the recorded one

            }

            if (!IsValidSQLiteResult(sqlite3_finalize(pStatement)))
            {
                return false;
            }

            //-------------------------------------------------------------------------
			Function<bool(PropertyData const &, PropertyData const &)> compare = [](PropertyData const &a, PropertyData const &b)
			{ return a.lineNumber < b.lineNumber; };


            //-------------------------------------------------------------------------

            pStatement = nullptr;
            return true;
        }

        return false;
    }

    bool TypeDatabase::ReadAdditionalEnumData(TypeInfoBase &type)
    {
        ENGINE_ASSERT(type.typeID != StringID::Invalid && type.IsFlag(TypeInfoBase::Flag::IsEnum));

        sqlite3_stmt *pStatement = nullptr;
        FillStatementBuffer("SELECT `Label`, `Value`, `Description` FROM `EnumConstants` WHERE `EnumConstants`.TypeID = %u;", (uint32_t)type.typeID);
        if (IsValidSQLiteResult(sqlite3_prepare_v2(m_pDatabase, m_statementBuffer, -1, &pStatement, nullptr)))
        {
            while (sqlite3_step(pStatement) == SQLITE_ROW)
            {
                EnumDataConstant constantDesc;
                constantDesc.label = (char const *)sqlite3_column_text(pStatement, 0);
                constantDesc.ID = StringID(std::string(constantDesc.label));
                constantDesc.value = sqlite3_column_int(pStatement, 1);
                constantDesc.description = (char const *)sqlite3_column_text(pStatement, 2);
                //type.AddEnumConstant(constantDesc);
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

    bool TypeDatabase::ReadAdditionalResourceTypeData(ReflectedResourceType &type)
    {
        ENGINE_ASSERT(type.typeID != TypeID::Invalid);

        sqlite3_stmt *pStatement = nullptr;
        FillStatementBuffer("SELECT `ResourceTypeParents`.ParentTypeID FROM `ResourceTypeParents` INNER JOIN `ResourceTypes` ON `ResourceTypes`.TypeID = `ResourceTypeParents`.ParentTypeID WHERE `ResourceTypeParents`.TypeID = \"%s\";",
			type.typeID.ToString().c_str());
        if (IsValidSQLiteResult(sqlite3_prepare_v2(m_pDatabase, m_statementBuffer, -1, &pStatement, nullptr)))
        {
            while (sqlite3_step(pStatement) == SQLITE_ROW)
            {
                type.parents.push_back(StringID(std::string((char const *)sqlite3_column_text(pStatement, 0))));
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

    bool TypeDatabase::WriteAdditionalTypeData(TypeInfoBase const &type)
    {
        // Delete old properties
        if (!ExecuteSimpleQuery("DELETE FROM `Properties` WHERE `OwnerTypeID` = %u;", (uint32_t)type.typeID))
        {
            return false;
        }

        // Update properties
  //      {
		//std::string escapedDescription = propertyDesc.description;
		//	Utils::String::ReplaceAll(escapedDescription, "\"", "\"\"");

		//	std::string escapedMetaData;
  //          if (propertyDesc.HasMetaData())
  //          {
  //              escapedMetaData = propertyDesc.metaData;
		//		Utils::String::ReplaceAll(escapedMetaData, "\"", "\"\"");
  //          }

  //          if (!ExecuteSimpleQuery("INSERT OR REPLACE INTO `Properties`(`PropertyID`, `LineNumber`, `OwnerTypeID`,`TypeID`,`Name`,`Description`,`TypeName`,`TemplateTypeName`,`PropertyFlags`,`ArraySize`,`MetaData`) VALUES ( %u, %d, %u, %u, \"%s\", \"%s\", \"%s\", \"%s\", %u, %d, \"%s\" );", (uint32_t)propertyDesc.propertyID, propertyDesc.lineNumber, (uint32_t)type.typeID, (uint32_t)propertyDesc.typeID, propertyDesc.name.c_str(), escapedDescription.c_str(), propertyDesc.typeName.c_str(), propertyDesc.templateArgTypeName.c_str(), (uint32_t)propertyDesc.flags.Get(), propertyDesc.arraySize, escapedMetaData.c_str()))
  //          {
  //              return false;
  //          }
  //      }

        return true;
    }

    bool TypeDatabase::WriteAdditionalEnumData(TypeInfoBase const &type)
    {
        // Fill enum values table with all constants
        if (!ExecuteSimpleQuery("DELETE FROM `EnumConstants` WHERE `TypeID` = %u;", (uint32_t)type.typeID))
        {
            return false;
        }

  //      for (auto const &enumConstant : type.enumConstants)
  //      {
		//std::string escapedDescription = enumConstant.description;
		//	Utils::String::ReplaceAll(escapedDescription, "\"", "\"\"");

  //          if (!ExecuteSimpleQuery("INSERT INTO `EnumConstants`(`TypeID`,`Label`,`Value`, `Description`) VALUES ( %u, \"%s\", %u, \"%s\" );", (uint32_t)type.typeID, enumConstant.label.c_str(), enumConstant.value, escapedDescription.c_str()))
  //          {
  //              return false;
  //          }
  //      }

        return true;
    }

    bool TypeDatabase::WriteAdditionalResourceTypeData(ReflectedResourceType const &type)
    {
        // Delete old parents
        if (!ExecuteSimpleQuery("DELETE FROM `ResourceTypeParents` WHERE `TypeID` = \"%s\";", type.typeID.ToString().c_str()))
        {
            return false;
        }

        // Update Type Parents
        for (auto &parent : type.parents)
        {
            if (!ExecuteSimpleQuery("INSERT INTO `ResourceTypeParents`(`TypeID`, `ParentTypeID`) VALUES ( \"%s\", \"%s\" );",
				type.typeID.ToString().c_str(), parent.ToString().c_str()))
            {
                return false;
            }
        }

        return true;
    }
}

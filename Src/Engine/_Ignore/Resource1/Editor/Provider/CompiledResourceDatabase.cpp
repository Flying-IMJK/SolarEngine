#include "CompiledResourceDatabase.h"
#include "Core/Platform//FileSystem.h"
#include <sqlite3.h>

//-------------------------------------------------------------------------

namespace SGE::Editor
{
    CompiledResourceDatabase::~CompiledResourceDatabase()
    {
        if (IsConnected())
        {
            Disconnect();
        }
    }

    bool CompiledResourceDatabase::Connect(String const& databasePath)
    {
		if (!FileSystem::DirectoryExists(FileSystem::GetParentDirectory(databasePath)))
		{
			FileSystem::CreateDirectory(databasePath);
		}

        int32 sqlFlags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_WAL;
		StringAnsi path = databasePath.ToStringAnsi();
        auto const result = sqlite3_open_v2( path.Get(), &m_pDatabase, sqlFlags, nullptr );
        sqlite3_busy_timeout( m_pDatabase, 2500 );

        if ( result != SQLITE_OK )
        {
            sqlite3_close( m_pDatabase );
            m_pDatabase = nullptr;
            m_errorMessage = String::Format(SE_TEXT("Couldn't open sqlite database: (0)"), databasePath);
            return false;
        }

        return CreateTables();
    }

    bool CompiledResourceDatabase::Disconnect()
    {
        if ( m_pDatabase != nullptr )
        {
            auto result = sqlite3_close( m_pDatabase );
            ENGINE_ASSERT( result == SQLITE_OK ); // If we get SQLITE_BUSY, this means we are leaking sqlite resources
            m_pDatabase = nullptr;
        }

        return true;
    }

    //-------------------------------------------------------------------------

    bool CompiledResourceDatabase::CreateTables()
    {
        ENGINE_ASSERT( m_pDatabase != nullptr );

        constexpr char const* const statement = "CREATE TABLE IF NOT EXISTS `CompiledResources` ( `ResID` TEXT UNIQUE,`ResTypeID` INTEGER,`CompilerVersion` INTEGER,`FileTimestamp` INTEGER, `SourceTimestampHash` INTEGER, PRIMARY KEY( ResID, ResTypeID ) );";
        sqlite3_snprintf( s_defaultStatementBufferSize, m_statementBuffer, statement );
        int32 result = sqlite3_exec( m_pDatabase, m_statementBuffer, nullptr, nullptr, nullptr );

        if ( result != SQLITE_OK )
        {
			StringBuilder s;
			s.Append(sqlite3_errstr( result ));
			s.Append(" (");
			s.Append(sqlite3_errmsg( m_pDatabase ));
			s.Append(")");
            m_errorMessage = s.ToString();
            return false;
        }

        return true;
    }

    bool CompiledResourceDatabase::DropTables()
    {
        ENGINE_ASSERT( m_pDatabase != nullptr );

        constexpr char const* const statement = "DROP TABLE IF EXISTS `CompiledResources`;";
        sqlite3_snprintf( s_defaultStatementBufferSize, m_statementBuffer, statement );
        int32_t result = sqlite3_exec( m_pDatabase, m_statementBuffer, nullptr, nullptr, nullptr );

        if ( result != SQLITE_OK )
        {
			StringBuilder s;
			s.Append(sqlite3_errstr( result ));
			s.Append(" (");
			s.Append(sqlite3_errmsg( m_pDatabase ));
			s.Append(")");
			m_errorMessage = s.ToString();
            return false;
        }

        return true;
    }

    //-------------------------------------------------------------------------

    bool CompiledResourceDatabase::GetRecord( ResID  resourceID, CompiledResourceRecord& outRecord ) const
    {
        outRecord.Clear();

        // Prepare the statement
        //-------------------------------------------------------------------------

        constexpr char const* const statement = "SELECT * FROM `CompiledResources` WHERE `ResID` = \"%s\" AND `ResTypeID` = %d;";
        sqlite3_snprintf( s_defaultStatementBufferSize, m_statementBuffer, statement, resourceID.GetID().ToString().ToStringAnsi().Get(), (uint32)resourceID.GetTypeID() );

        sqlite3_stmt* pStatement = nullptr;
        int32 result = sqlite3_prepare_v2( m_pDatabase, m_statementBuffer, -1, &pStatement, nullptr );
        if ( result != SQLITE_OK )
        {
			StringBuilder s;
			s.Append(sqlite3_errstr( result ));
			s.Append(" (");
			s.Append(sqlite3_errmsg( m_pDatabase ));
			s.Append(")");
			m_errorMessage = s.ToString();
            return false;
        }

        // Execute prepared statement
        //-------------------------------------------------------------------------

        while ( sqlite3_step( pStatement ) == SQLITE_ROW )
        {
            SGUID resourceID;
			SGUID::Parse(StringAnsiView((char const*)sqlite3_column_text( pStatement, 0 )), resourceID);
            TypeID typeID = sqlite3_column_int( pStatement, 1 );

			outRecord.resourceID = ResID( resourceID, typeID );
            outRecord.compilerVersion = sqlite3_column_int( pStatement, 2 );
            outRecord.fileTimestamp = sqlite3_column_int64( pStatement, 3 );
            outRecord.sourceTimestampHash = sqlite3_column_int64( pStatement, 4 );
        }

        result = sqlite3_finalize( pStatement );
        if ( result != SQLITE_OK )
        {
			StringBuilder s;
			s.Append(sqlite3_errstr( result ));
			s.Append(" (");
			s.Append(sqlite3_errmsg( m_pDatabase ));
			s.Append(")");
			m_errorMessage = s.ToString();
            return false;
        }

        return true;
    }

    bool CompiledResourceDatabase::WriteRecord( CompiledResourceRecord const& record )
    {
        ENGINE_ASSERT(IsConnected());

        constexpr char const* const statement = "BEGIN TRANSACTION;INSERT OR REPLACE INTO `CompiledResources` ( `ResID`, `ResTypeID`, `CompilerVersion`, `FileTimestamp`, `SourceTimestampHash` ) VALUES ( \"%s\", %d, %d, %llu, %llu );END TRANSACTION;";
        sqlite3_snprintf(s_defaultStatementBufferSize, m_statementBuffer, statement,
			record.resourceID.GetID().ToString().Get(), (uint32)record.resourceID.GetTypeID(), record.compilerVersion, record.fileTimestamp, record.sourceTimestampHash );

        int32_t result = sqlite3_exec( m_pDatabase, m_statementBuffer, nullptr, nullptr, nullptr );

        if ( result != SQLITE_OK )
        {
			StringBuilder s;
			s.Append(sqlite3_errstr( result ));
			s.Append(" (");
			s.Append(sqlite3_errmsg( m_pDatabase ));
			s.Append(")");
			m_errorMessage = s.ToString();
            return false;
        }

        return true;
    }
}
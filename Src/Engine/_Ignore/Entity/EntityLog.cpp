#include "EntityLog.h"
#include "Entity.h"
#include "Core/Types/Collections/ConcurrentQueue.h"
#include "Core/Types/DateTime.h"

//-------------------------------------------------------------------------

#ifdef SE_DEVELOPMENT
namespace SGE::EntityModel
{
    ConcurrentQueue<EntityLogRequest>* g_pEntryQueue = nullptr;

    //-------------------------------------------------------------------------

    static void EnqueueLogEntry( EntityID entityID, ComponentID componentID, Log::Severity severity, char const* pFilename, int pLineNumber, Char const* pMessage)
    {
        ENGINE_ASSERT( entityID.IsValid() );
        ENGINE_ASSERT(pFilename != nullptr && pMessage != nullptr );

        EntityLogRequest entry;
        entry.m_category = "Entity";
        entry.m_entityID = entityID;
        entry.m_componentID = componentID;
        entry.m_filename = pFilename;
        entry.m_lineNumber = pLineNumber;
        entry.m_severity = severity;

        // Message
        entry.m_message = pMessage;

        // Timestamp
		entry.m_timestamp.Clear();
		DateTime dateTime = DateTime::NowUTC();
		entry.m_timestamp.Append(dateTime.ToString());

        //-------------------------------------------------------------------------

        ENGINE_ASSERT( g_pEntryQueue != nullptr );
        g_pEntryQueue->enqueue( entry );
    }

    //-------------------------------------------------------------------------

    void InitializeLogQueue()
    {
        ENGINE_ASSERT( g_pEntryQueue == nullptr );
        g_pEntryQueue = New<ConcurrentQueue<EntityLogRequest>>();
    }

    void ShutdownLogQueue()
    {
        ENGINE_ASSERT( g_pEntryQueue != nullptr );
        Delete( g_pEntryQueue );
    }

    void EnqueueLogEntry(Entity const* pEntity, Log::Severity severity, char const* pFilename, int pLineNumber, Char const* pMessage)
    {
        ENGINE_ASSERT( pEntity != nullptr );

        EnqueueLogEntry( pEntity->GetID(), ComponentID(), severity, pFilename, pLineNumber, pMessage);
    }

    void EnqueueLogEntry(EntityComponent const* pComponent, Log::Severity severity, char const* pFilename, int pLineNumber, Char const* pMessage)
    {
        ENGINE_ASSERT( pComponent != nullptr && pComponent->GetEntityID().IsValid() );

        EnqueueLogEntry( pComponent->GetEntityID(), pComponent->GetID(), severity, pFilename, pLineNumber, pMessage);
    }

    //-------------------------------------------------------------------------

    List<EntityLogRequest> RetrieveQueuedLogRequests()
    {
		List<EntityLogRequest> entries;
        entries.Resize( g_pEntryQueue->size_approx() );
        entries.AddOne();

        while ( g_pEntryQueue->try_dequeue( entries.Last() ) )
        {
            entries.AddOne();
        }

        entries.Pop();
        return entries;
    }
}
#endif
#pragma once

#include "Runtime/API.h"

#include "EntityIDs.h"
#include "Core/Types/Collections/List.h"
#include "Core/Types/Strings/String.h"

//-------------------------------------------------------------------------

namespace SE
{
    class Entity;
    class EntityComponent;
}

//-------------------------------------------------------------------------

#ifdef SE_DEVELOPMENT
namespace SE::EntityModel
{
    struct EntityLogRequest
    {
        String          m_timestamp;
        String          m_category;
        EntityID        m_entityID;
        ComponentID     m_componentID;
        String          m_message;
        StringAnsi      m_filename;
        uint32          m_lineNumber;
        Log::Severity   m_severity;
    };

    //-------------------------------------------------------------------------

    inline void InitializeLogQueue();
    inline void ShutdownLogQueue();

	void EnqueueLogEntry(Entity const* pEntity, Log::Severity severity, char const* pFilename, int pLineNumber, Char const* pMessage);

    template<typename ...Args>
    inline void EnqueueLogEntry(Entity const* pEntity, Log::Severity severity, char const* pFilename, int pLineNumber, Char const* pMessageFormat, Args &&...args)
    {
        String msg = String::Format(pMessageFormat, args...);
        EnqueueLogEntry(pEntity, severity, pFilename, pLineNumber, msg.Get());
    }



	void EnqueueLogEntry(EntityComponent const* pComponent, Log::Severity severity, char const* pFilename, int pLineNumber, Char const* pMessage);

    template<typename ...Args>
    inline void EnqueueLogEntry(EntityComponent const* pComponent, Log::Severity severity, char const* pFilename, int pLineNumber, Char const* pMessageFormat, Args &&...args)
    {
        String msg = String::Format(pMessageFormat, args...);
        EnqueueLogEntry(pComponent, severity, pFilename, pLineNumber, msg.Get());
    }

    // Retrieves all queued log entries and flushes the queue
	List<EntityLogRequest> RetrieveQueuedLogRequests();
}
#endif

// Entity Logging
//-------------------------------------------------------------------------

#ifdef SE_DEVELOPMENT
#define LOG_ENTITY_MESSAGE( pEntityOrComponent, msg) ::SE::EntityModel::EnqueueLogEntry( pEntityOrComponent, Log::Severity::Message, __FILE__, __LINE__, msg)
#define LOG_ENTITY_WARNING( pEntityOrComponent, msg) ::SE::EntityModel::EnqueueLogEntry( pEntityOrComponent, Log::Severity::Warning, __FILE__, __LINE__, msg)
#define LOG_ENTITY_ERROR( pEntityOrComponent, msg) ::SE::EntityModel::EnqueueLogEntry( pEntityOrComponent, Log::Severity::Error, __FILE__, __LINE__, msg)
#define LOG_ENTITY_FATAL_ERROR( pEntityOrComponent, msg) ::SE::EntityModel::EnqueueLogEntry( pEntityOrComponent, Log::Severity::FatalError, __FILE__, __LINE__, msg); ENGINE_HALT()

#define LOG_ENTITY_MESSAGE_FORMAT( pEntityOrComponent, msg, ... ) ::SE::EntityModel::EnqueueLogEntry( pEntityOrComponent, Log::Severity::Message, __FILE__, __LINE__, SE_TEXT(msg), __VA_ARGS__ )
#define LOG_ENTITY_WARNING_FORMAT( pEntityOrComponent, msg, ... ) ::SE::EntityModel::EnqueueLogEntry( pEntityOrComponent, Log::Severity::Warning, __FILE__, __LINE__, SE_TEXT(msg), __VA_ARGS__ )
#define LOG_ENTITY_ERROR_FORMAT( pEntityOrComponent, msg, ... ) ::SE::EntityModel::EnqueueLogEntry( pEntityOrComponent, Log::Severity::Error, __FILE__, __LINE__, SE_TEXT(msg), __VA_ARGS__ )
#define LOG_ENTITY_FATAL_ERROR_FORMAT( pEntityOrComponent, msg, ... ) ::SE::EntityModel::EnqueueLogEntry( pEntityOrComponent, Log::Severity::FatalError, __FILE__, __LINE__, SE_TEXT(msg), __VA_ARGS__ ); ENGINE_HALT()

#else
#define LOG_ENTITY_MESSAGE( pEntityOrComponent, category, ... )
#define LOG_ENTITY_WARNING( pEntityOrComponent, category, ... )
#define LOG_ENTITY_ERROR( pEntityOrComponent, category, ... )
#define LOG_ENTITY_FATAL_ERROR( pEntityOrComponent, category, ... )
#endif
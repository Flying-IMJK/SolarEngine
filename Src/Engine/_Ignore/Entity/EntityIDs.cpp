#include "EntityIDs.h"
#include <atomic>

//-------------------------------------------------------------------------

namespace SE
{
	EntityMapID GenerateEntityMapID()
	{
		return SGUID::SGUID();
	}


    static std::atomic<uint64> g_entityWorldID = 1;

    EntityWorldID EntityWorldID::Generate()
    {
        EntityWorldID ID;
        ID.m_value = g_entityWorldID++;
        ENGINE_ASSERT( ID.m_value != UINT64_MAX );
        return ID;
    }

    //-------------------------------------------------------------------------

    static std::atomic<uint64> g_entityID = 1;

    EntityID EntityID::Generate()
    {
        EntityID ID;
        ID.m_value = g_entityID++;
        ENGINE_ASSERT( ID.m_value != UINT64_MAX );
        return ID;
    }

    //-------------------------------------------------------------------------

    static std::atomic<uint64> g_componentID = 1;

    ComponentID ComponentID::Generate()
    {
        ComponentID ID;
        ID.m_value = g_componentID++;
        ENGINE_ASSERT( ID.m_value != UINT64_MAX );
        return ID;
    }
}
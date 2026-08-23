#pragma once
#include "Core/Types/Collections/ConcurrentQueue.h"
#include "Core/TypeSystem/TypeID.h"
#include "Core/Types/Collections/Dictionary.h"

//-------------------------------------------------------------------------

namespace SE
{
    class Entity;
    class EntityMap;
    class EntityComponent;
    class TaskSystem;
    class EntityWorldSystem;
    class ResourceSystem;
    namespace Reflect { class Types; }
}

//-------------------------------------------------------------------------

namespace SE::EntityModel
{
    struct LoadingContext
    {
        LoadingContext() = default;

        LoadingContext(ResourceSystem* pResourceSystem)
            : m_pResourceSystem( pResourceSystem )
        {
            ENGINE_ASSERT(m_pResourceSystem != nullptr );
        }

        inline bool IsValid() const
        {
            return m_pResourceSystem != nullptr;
        }

    public:

        ResourceSystem*                                                 m_pResourceSystem = nullptr;
    };

    //-------------------------------------------------------------------------

    struct EntityComponentPair
    {
        EntityComponentPair() = default;
        EntityComponentPair( Entity* pEntity, EntityComponent* pComponent ) : m_pEntity( pEntity ), m_pComponent( pComponent ) {}

        Entity*             m_pEntity = nullptr;
        EntityComponent*    m_pComponent = nullptr;
    };

    using EntityComponentTypeMap = Dictionary<TypeID, List<EntityComponent const*>>;

    //-------------------------------------------------------------------------

    struct InitializationContext
    {
        friend EntityMap;

    public:

        InitializationContext( List<EntityWorldSystem*> const& worldSystems, List<Entity*>& entityUpdateList )
            : m_worldSystems( worldSystems )
            , m_entityUpdateList( entityUpdateList )
        {}

        #ifdef SE_DEVELOPMENT
        void SetComponentTypeMapPtr( EntityComponentTypeMap* pMap ) { m_pComponentTypeMap = pMap; }
        #endif

        inline bool IsValid() const
        {
            #if ENGINE_DEVELOPMENT_TOOLS
            if ( m_pComponentTypeMap == nullptr ) return false;
            #endif

            return true;
        }

    public:
        // World system registration
        ConcurrentQueue<EntityComponentPair>               m_componentsToRegister;
        ConcurrentQueue<EntityComponentPair>               m_componentsToUnregister;

        // Entity update registration
        ConcurrentQueue<Entity*>                           m_registerForEntityUpdate;
		ConcurrentQueue<Entity*>                           m_unregisterForEntityUpdate;

    private:

		List<EntityWorldSystem*> const&                             m_worldSystems;
		List<Entity*>&                                              m_entityUpdateList;

        #if ENGINE_DEVELOPMENT_TOOLS
        EntityComponentTypeMap*                                     m_pComponentTypeMap = nullptr;
        #endif
    };
}
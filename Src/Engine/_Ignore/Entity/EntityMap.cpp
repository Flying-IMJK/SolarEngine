#include "EntityMap.h"
#include "EntityLog.h"
#include "EntityContexts.h"
#include "EntitySerialization.h"
#include "EntityWorldSystem.h"
#include "Entity.h"
#include "Runtime/System/ResourceSystem.h"
#include "Core/TypeSystem/Types.h"
#include "Core/Profiler/Profiler.h"

//-------------------------------------------------------------------------

namespace SE::EntityModel
{
    EntityMap::EntityMap()
        : m_isTransientMap( true )
		, m_entityUpdateEventBindingID( Entity::OnEntityInternalStateUpdated().Bind( [this] ( Entity* pEntity ) { OnEntityStateUpdated( pEntity ); } ) )
    {
	}

    EntityMap::EntityMap( ResID mapResourceID )
        : m_pMapDesc( mapResourceID )
        , m_entityUpdateEventBindingID( Entity::OnEntityInternalStateUpdated().Bind( [this] ( Entity* pEntity ) { OnEntityStateUpdated( pEntity ); } ) )
    {}

    EntityMap::EntityMap( EntityMap const& map )
        : m_entityUpdateEventBindingID( Entity::OnEntityInternalStateUpdated().Bind( [this] ( Entity* pEntity ) { OnEntityStateUpdated( pEntity ); } ) )
    {
        operator=( map );
    }

    EntityMap::EntityMap( EntityMap&& map )
        : m_entityUpdateEventBindingID( Entity::OnEntityInternalStateUpdated().Bind( [this] ( Entity* pEntity ) { OnEntityStateUpdated( pEntity ); } ) )
    {
        operator=( std::move( map ) );
    }

    EntityMap::~EntityMap()
    {
        ENGINE_ASSERT( IsUnloaded() );
        ENGINE_ASSERT( m_entities.IsEmpty() && m_entityIDLookupMap.IsEmpty() );
        ENGINE_ASSERT( m_entitiesToLoad.IsEmpty() && m_entitiesToRemove.IsEmpty() );

        #ifdef SE_DEVELOPMENT
        ENGINE_ASSERT( m_entitiesToHotReload.IsEmpty() );
        ENGINE_ASSERT( m_editedEntities.IsEmpty() );
        #endif

        Entity::OnEntityInternalStateUpdated().Unbind(m_entityUpdateEventBindingID);
    }

    //-------------------------------------------------------------------------

    EntityMap& EntityMap::operator=( EntityMap const& map )
    {
        // Only allow copy constructor for unloaded maps
        ENGINE_ASSERT( m_status == Status::Unloaded && map.m_status == Status::Unloaded );

        #ifdef SE_DEVELOPMENT
        ENGINE_ASSERT( map.m_entitiesToHotReload.IsEmpty() );
        #endif

        m_pMapDesc = map.m_pMapDesc;
        const_cast<bool&>( m_isTransientMap ) = map.m_isTransientMap;
        return *this;
    }

    EntityMap& EntityMap::operator=( EntityMap&& map )
    {
        #ifdef SE_DEVELOPMENT
        ENGINE_ASSERT( map.m_entitiesToHotReload.IsEmpty() );
        #endif

        m_ID = map.m_ID;
        m_entities.Swap( map.m_entities );
        m_entityIDLookupMap.Swap( map.m_entityIDLookupMap );
        m_pMapDesc = std::move( map.m_pMapDesc );
        m_entitiesCurrentlyLoading = std::move( map.m_entitiesCurrentlyLoading );
        m_status = map.m_status;
        const_cast<bool&>( m_isTransientMap ) = map.m_isTransientMap;

        // Clear source map
        map.m_ID = EntityMapID::Empty;
        map.m_status = Status::Unloaded;
        return *this;
    }

    //-------------------------------------------------------------------------
    // Entity Management
    //-------------------------------------------------------------------------

    void EntityMap::AddEntities( List<Entity*> const& entities, Transform const& offsetTransform )
    {
        Threading::ScopeLock lock( m_mutex );

        m_entityIDLookupMap.SetCapacity( m_entityIDLookupMap.Count() + entities.Count() );

        #ifdef SE_DEVELOPMENT
        m_entityNameLookupMap.SetCapacity( m_entityNameLookupMap.Count() + entities.Count() );
        #endif

        //-------------------------------------------------------------------------

        bool const applyOffset = !offsetTransform.IsIdentity();
        for ( auto& pEntity : entities )
        {
            // Shift entity by the specified offset
            if ( applyOffset && pEntity->IsSpatialEntity() )
            {
                pEntity->SetWorldTransform( pEntity->GetWorldTransform() * offsetTransform );
            }

            AddEntity( pEntity );
        }
    }

    void EntityMap::AddEntity( Entity* pEntity )
    {
        // Ensure that the entity to add, is not already part of a collection and that it is not initialized
        ENGINE_ASSERT( pEntity != nullptr && !pEntity->IsAddedToMap() && !pEntity->HasRequestedComponentLoad() );
        ENGINE_ASSERT( !m_entitiesToLoad.Contains( pEntity ) );

        // Entity validation
        //-------------------------------------------------------------------------
        // Ensure spatial parenting and unique name

        #ifdef SE_DEVELOPMENT
        if ( pEntity->HasSpatialParent() )
        {
            ENGINE_ASSERT( ContainsEntity( pEntity->GetSpatialParent()->GetID() ) );
        }

        pEntity->m_name = GenerateUniqueEntityNameID( pEntity->m_name );
        #endif

        // Add entity
        //-------------------------------------------------------------------------

		Threading::ScopeLock lock( m_mutex );

        pEntity->m_mapID = m_ID;
        m_entities.Add( pEntity );
        m_entitiesToLoad.Add( pEntity );

        // Add to lookup maps
        //-------------------------------------------------------------------------

        m_entityIDLookupMap.Add(  pEntity->m_ID, pEntity );
        #ifdef SE_DEVELOPMENT
        m_entityNameLookupMap.Add( pEntity->m_name, pEntity );
        #endif
    }

    void EntityMap::AddEntityCollection(SerializedEntityCollection const& entityCollectionDesc, Transform const& offsetTransform, List<Entity*>* pOutCreatedEntities )
    {
        List<Entity*> scratchVector;
        List<Entity*>& createdEntities = ( pOutCreatedEntities != nullptr ) ? *pOutCreatedEntities : scratchVector;

        //-------------------------------------------------------------------------

        createdEntities.Clear();
        createdEntities = Serializer::CreateEntities(entityCollectionDesc);
        AddEntities( createdEntities, offsetTransform );
    }

    Entity* EntityMap::RemoveEntityInternal( EntityID entityID, bool destroyEntityOnceRemoved )
    {
        Threading::ScopeLock lock( m_mutex );

        // Handle spatial hierarchy
        //-------------------------------------------------------------------------

        Entity* pEntityToRemove = FindEntity( entityID );
        ENGINE_ASSERT( pEntityToRemove != nullptr );

        if ( !pEntityToRemove->m_attachedEntities.IsEmpty() )
        {
            // If we have a parent, re-parent all children to it
            if ( pEntityToRemove->m_pParentSpatialEntity != nullptr )
            {
                for ( auto pAttachedEntity : pEntityToRemove->m_attachedEntities )
                {
                    pAttachedEntity->SetSpatialParent( pEntityToRemove->m_pParentSpatialEntity );
                }
            }
            else // If we have no parent, remove spatial parent
            {
                for ( auto pAttachedEntity : pEntityToRemove->m_attachedEntities )
                {
                    pAttachedEntity->ClearSpatialParent();
                }
            }
        }

        // Remove from map
        //-------------------------------------------------------------------------
		int findIndex = m_entities.FindFirst(pEntityToRemove);
		if (findIndex != INVALID_INDEX)
		{
			m_entities.RemoveAt(findIndex);	
		}

        // Remove from internal lookup maps
        //-------------------------------------------------------------------------
        m_entityIDLookupMap.Remove( pEntityToRemove->m_ID  );

        #ifdef SE_DEVELOPMENT
        m_entityNameLookupMap.Remove( pEntityToRemove->m_name );
        #endif

        // Schedule unload
        //-------------------------------------------------------------------------

        // Check if the entity is in the add queue, if so just cancel the request
		Function<bool(Entity * const&)> predicate = [entityID] (Entity const* pEntity)
		{
		  return pEntity->GetID() == entityID;
		};
        int32 const entityIdx = ListExtensions::IndexOf(m_entitiesToLoad, predicate);

        if ( entityIdx != INVALID_INDEX )
        {
            pEntityToRemove = m_entitiesToLoad[entityIdx];
            m_entitiesToLoad.RemoveAt(entityIdx);

            if ( destroyEntityOnceRemoved )
            {
                Delete( pEntityToRemove );
            }
        }
        else // Queue removal
        {
            m_entitiesToRemove.Add( RemovalRequest( pEntityToRemove, destroyEntityOnceRemoved ) );
        }

        //-------------------------------------------------------------------------

        // Do not return anything if we are requesting destruction
        if ( destroyEntityOnceRemoved )
        {
            pEntityToRemove = nullptr;
        }

        return pEntityToRemove;
    }

    Entity* EntityMap::RemoveEntity( EntityID entityID )
    {
        Entity* pEntityToRemove = RemoveEntityInternal( entityID, false );
        ENGINE_ASSERT( pEntityToRemove != nullptr );
        return pEntityToRemove;
    }

    void EntityMap::DestroyEntity( EntityID entityID )
    {
        RemoveEntityInternal( entityID, true );
    }

    void EntityMap::OnEntityStateUpdated( Entity* pEntity )
    {
        if ( pEntity->GetMapID() == m_ID )
        {
            ENGINE_ASSERT( FindEntity( pEntity->GetID() ) );
            Threading::ScopeLock lock( m_mutex );
            if ( !m_entitiesCurrentlyLoading.Contains(pEntity) )
            {
                m_entitiesCurrentlyLoading.Add( pEntity );
            }
        }
    }

    //-------------------------------------------------------------------------
    // Loading
    //-------------------------------------------------------------------------

    void EntityMap::Load( LoadingContext const& loadingContext, InitializationContext& initializationContext )
    {
        ENGINE_ASSERT( Threading::IsMainThread() && loadingContext.IsValid() );
        ENGINE_ASSERT( m_status == Status::Unloaded );

        Threading::ScopeLock lock( m_mutex );

        if ( m_isTransientMap )
        {
            m_status = Status::Loaded;
        }
        else // Request loading of map resource
        {
            loadingContext.m_pResourceSystem->LoadResource( m_pMapDesc );
            m_status = Status::Loading;
        }
    }

    void EntityMap::ProcessMapLoading( LoadingContext const& loadingContext )
    {
        PROFILE_CPU_NAMED( "Map Loading" );
        ENGINE_ASSERT( m_status == Status::Loading );
        ENGINE_ASSERT( !m_isTransientMap );

        //-------------------------------------------------------------------------

        // Wait for the map descriptor to load
        if ( m_pMapDesc.IsLoading() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        // Check for load failure
        if ( m_pMapDesc.HasLoadingFailed() )
        {
            m_status = Status::LoadFailed;
            loadingContext.m_pResourceSystem->UnloadResource( m_pMapDesc );
            return;
        }

        //-------------------------------------------------------------------------

        // Instantiate the map
        if ( m_pMapDesc->IsValid() )
        {
            // Create all required entities
            List<Entity*> const createdEntities = Serializer::CreateEntities( *m_pMapDesc.GetPtr() );

            // Resize memory for new entities in internal structures
            m_entities.Resize( m_entities.Count() + createdEntities.Count() );
            m_entitiesToLoad.Resize( m_entitiesToLoad.Count() + createdEntities.Count() );
            m_entityIDLookupMap.SetCapacity( m_entityIDLookupMap.Count() + createdEntities.Count() );
            m_entitiesCurrentlyLoading.Resize( m_entitiesCurrentlyLoading.Count() + createdEntities.Count() );

            #ifdef SE_DEVELOPMENT
            m_entityNameLookupMap.SetCapacity( m_entityNameLookupMap.Count() + createdEntities.Count() );
            #endif

            // Add entities
            for ( auto pEntity : createdEntities )
            {
                AddEntity( pEntity );
            }

            m_status = Status::Loaded;
        }
        else // Invalid map data is treated as a failed load
        {
            m_status = Status::LoadFailed;
        }

        // Release map resource ptr once loading has completed
        loadingContext.m_pResourceSystem->UnloadResource( m_pMapDesc );
    }

    void EntityMap::Unload( LoadingContext const& loadingContext, InitializationContext& initializationContext )
    {
        ENGINE_ASSERT( m_status != Status::Unloaded );
        Threading::ScopeLock lock( m_mutex );
        m_status = Status::Unloading;
    }

    void EntityMap::ProcessMapUnloading( LoadingContext const& loadingContext, InitializationContext& initializationContext )
    {
        PROFILE_CPU_NAMED( "Map Unload" );
        ENGINE_ASSERT( m_status == Status::Unloading );

        // Cancel any load requests
        //-------------------------------------------------------------------------

        m_entitiesCurrentlyLoading.Clear();
        m_entitiesToLoad.Clear();

        // Shutdown all entities
        //-------------------------------------------------------------------------

        // Shutdown all entities that have been requested for removal
        ProcessEntityShutdownRequests( initializationContext );

        // Shutdown map entities
        {
//            struct EntityShutdownTask : public ITaskSet
//            {
//                EntityShutdownTask( InitializationContext& initializationContext, List<Entity*>& entities )
//                    : m_initializationContext( initializationContext )
//                    , m_entitiesToShutdown( entities )
//                {
//                    m_SetSize = (uint32) m_entitiesToShutdown.Count();
//                }
//
//                virtual void ExecuteRange( TaskSetPartition range, uint32 threadnum ) override final
//                {
//                    PROFILE_CPU_NAMED( "Shutdown Entities" );
//
//                    for ( uint64_t i = range.start; i < range.end; ++i )
//                    {
//                        auto pEntity = m_entitiesToShutdown[i];
//                        if ( pEntity->IsInitialized() )
//                        {
//                            // Only shutdown non-spatial and root spatial entities. Attached entities will be shutdown by their parents
//                            if ( !pEntity->IsSpatialEntity() || !pEntity->HasSpatialParent() )
//                            {
//                                pEntity->Shutdown( m_initializationContext );
//                            }
//                        }
//                    }
//                }
//
//            private:
//
//                InitializationContext&          m_initializationContext;
//                List<Entity*>&               m_entitiesToShutdown;
//            };
//
//            //-------------------------------------------------------------------------
//
//            EntityShutdownTask shutdownTask( initializationContext, m_entities );
//            initializationContext.m_pTaskSystem->ScheduleTask( &shutdownTask );
//            initializationContext.m_pTaskSystem->WaitForTask( &shutdownTask );
        }

        // Process all unregistration requests
        ProcessEntityRegistrationRequests( initializationContext );

        // Unload and destroy entities
        //-------------------------------------------------------------------------

        // Unload all entities that have been requested for removal
        ProcessEntityRemovalRequests( loadingContext );

        // Unload and destroy map entities
        for ( auto& pEntity : m_entities )
        {
            ENGINE_ASSERT( !pEntity->IsInitialized() );
            if ( pEntity->IsLoaded() )
            {
                pEntity->UnloadComponents( loadingContext );
            }
            Delete( pEntity );
        }

        m_entities.Clear();
        m_entityIDLookupMap.Clear();
         
        #ifdef SE_DEVELOPMENT
        m_entityNameLookupMap.Clear();
        #endif

        // Unload the map resource
        //-------------------------------------------------------------------------

        if ( !m_isTransientMap && m_pMapDesc.WasRequested() )
        {
            loadingContext.m_pResourceSystem->UnloadResource( m_pMapDesc );
        }

        m_status = Status::Unloaded;
    }

    //-------------------------------------------------------------------------

    void EntityMap::ProcessEntityShutdownRequests( InitializationContext& initializationContext )
    {
        PROFILE_CPU_NAMED( "Entity Dispose" );

        // No point parallelizing this, since this should never have that many entries in it
        for ( int32 i = (int32) m_entitiesToRemove.Count() - 1; i >= 0; i-- )
        {
            auto pEntityToShutdown = m_entitiesToRemove[i].m_pEntity;
            if ( pEntityToShutdown->IsInitialized() )
            {
                pEntityToShutdown->Shutdown( initializationContext );
            }
        }
    }

    void EntityMap::ProcessEntityRemovalRequests( LoadingContext const& loadingContext )
    {
        PROFILE_CPU_NAMED( "Entity Removal" );

        for ( int32 i = (int32) m_entitiesToRemove.Count() - 1; i >= 0; i-- )
        {
            auto& removalRequest = m_entitiesToRemove[i];
            auto pEntityToRemove = removalRequest.m_pEntity;
            ENGINE_ASSERT( !pEntityToRemove->IsInitialized() );

            // Remove from currently loading list
			int findIndex = m_entitiesCurrentlyLoading.FindFirst(pEntityToRemove);
			if (findIndex != INVALID_INDEX)
			{
				m_entitiesCurrentlyLoading.RemoveAt(findIndex);
			}

            // Unload entity
            pEntityToRemove->UnloadComponents( loadingContext );
            pEntityToRemove->m_mapID = EntityMapID::Empty;

            // Destroy the entity if this is a destruction request
            if ( removalRequest.m_shouldDestroy )
            {
                Delete( pEntityToRemove );
            }

            // Remove the request from the list
            m_entitiesToRemove.RemoveAt(i);
        }
    }

    void EntityMap::ProcessEntityLoadingAndInitialization( LoadingContext const& loadingContext, InitializationContext& initializationContext )
    {
        PROFILE_CPU_NAMED( "Entity Loading/Initialization" );

        // Request load for unloaded entities
        // No point parallelizing this since the resource system locks a mutex for each load/unload call!
        for ( auto pEntityToAdd : m_entitiesToLoad )
        {
            // Ensure that the entity to add, is not already part of a collection and that it's shutdown
            ENGINE_ASSERT( pEntityToAdd != nullptr && pEntityToAdd->m_mapID == m_ID && !pEntityToAdd->IsInitialized() );

            // Request component load
            pEntityToAdd->LoadComponents( loadingContext );
            ENGINE_ASSERT( !m_entitiesCurrentlyLoading.Contains(pEntityToAdd ) );
            m_entitiesCurrentlyLoading.Add( pEntityToAdd );
        }

        m_entitiesToLoad.Clear();

        //-------------------------------------------------------------------------

       /* struct EntityLoadingTask : public ITaskSet
        {
            EntityLoadingTask( LoadingContext const& loadingContext, InitializationContext& initializationContext, List<Entity*>& entitiesToLoad )
                : m_loadingContext( loadingContext )
                , m_initializationContext( initializationContext )
                , m_entitiesToLoad( entitiesToLoad )
            {
                m_SetSize = (uint32) m_entitiesToLoad.Count();
            }

            virtual void ExecuteRange( TaskSetPartition range, uint32 threadnum ) override final
            {
                PROFILE_CPU_NAMED( "Load and Initialize Entities" );
                for ( uint32 i = range.start; i < range.end; ++i )
                {
                    auto pEntity = m_entitiesToLoad[i];
                    if ( pEntity->UpdateEntityState( m_loadingContext, m_initializationContext ) )
                    {
                        #ifdef SE_DEVELOPMENT
                        for ( auto pComponent : pEntity->GetComponents() )
                        {
                            ENGINE_ASSERT( pComponent->IsInitialized() || pComponent->HasLoadingFailed() );
                        }
                        #endif

                        // Initialize any entities that loaded successfully
                        if ( pEntity->IsLoaded() )
                        {
                            // Prevent us from initializing entities whose parents are not yet initialized, this ensures that our attachment chains have a consistent initialized state
                            if ( pEntity->HasSpatialParent() )
                            {
                                // We need to recheck the initialization state of this entity since while waiting for the lock, it could have been initialized by the parent
                                Threading::ScopeLock parentLock( pEntity->GetSpatialParent()->m_internalStateMutex );
                                if ( !pEntity->IsInitialized() && pEntity->GetSpatialParent()->IsInitialized() )
                                {
                                    pEntity->Initialize( m_initializationContext );
                                }
                            }
                            else
                            {
                                pEntity->Initialize( m_initializationContext );
                            }
                        }
                    }
                    else // Entity is still loading
                    {
                        bool result = m_stillLoadingEntities.enqueue( pEntity );
                        ENGINE_ASSERT( result );
                    }
                }
            }

        public:

            Threading::LockFreeQueue<Entity*>       m_stillLoadingEntities;

        private:

            LoadingContext const&                   m_loadingContext;
            InitializationContext&                  m_initializationContext;
            List<Entity*>&                       m_entitiesToLoad;
        };*/

        //-------------------------------------------------------------------------

        if ( !m_entitiesCurrentlyLoading.IsEmpty() )
        {
/*            EntityLoadingTask loadingTask( loadingContext, initializationContext, m_entitiesCurrentlyLoading );
            loadingContext.m_pTaskSystem->ScheduleTask( &loadingTask );
            loadingContext.m_pTaskSystem->WaitForTask( &loadingTask );*/

            //-------------------------------------------------------------------------

            // Track the number of entities that still need loading
//            size_t const numEntitiesStillLoading = loadingTask.m_stillLoadingEntities.size_approx();
//            m_entitiesCurrentlyLoading.Resize( numEntitiesStillLoading );
//            size_t numDequeued = loadingTask.m_stillLoadingEntities.try_dequeue_bulk( m_entitiesCurrentlyLoading.Get(), numEntitiesStillLoading );
//            ENGINE_ASSERT( numEntitiesStillLoading == numDequeued );
        }
    }

    //-------------------------------------------------------------------------

    void EntityMap::ProcessEntityRegistrationRequests( InitializationContext& initializationContext )
    {
        ENGINE_ASSERT( Threading::IsMainThread() );

        //-------------------------------------------------------------------------
        // Entity Registrations
        //-------------------------------------------------------------------------

        {
            PROFILE_CPU_NAMED( "Entity Update Registration" );

            Entity* pEntity = nullptr;
            while ( initializationContext.m_unregisterForEntityUpdate.try_dequeue( pEntity ) )
            {
                ENGINE_ASSERT( pEntity != nullptr && pEntity->m_updateRegistrationStatus == Entity::UpdateRegistrationStatus::QueuedForUnregister );
				int findIndex = initializationContext.m_entityUpdateList.FindFirst(pEntity);
				if (findIndex != INVALID_INDEX)
				{
					initializationContext.m_entityUpdateList.RemoveAt(findIndex);
				}
                pEntity->m_updateRegistrationStatus = Entity::UpdateRegistrationStatus::Unregistered;
            }

            //-------------------------------------------------------------------------

            while ( initializationContext.m_registerForEntityUpdate.try_dequeue( pEntity ) )
            {
                ENGINE_ASSERT( pEntity != nullptr );
                ENGINE_ASSERT( pEntity->IsInitialized() );
                ENGINE_ASSERT( pEntity->m_updateRegistrationStatus == Entity::UpdateRegistrationStatus::QueuedForRegister );
                ENGINE_ASSERT( !pEntity->HasSpatialParent() ); // Attached entities are not allowed to be directly updated
                initializationContext.m_entityUpdateList.Add( pEntity );
                pEntity->m_updateRegistrationStatus = Entity::UpdateRegistrationStatus::Registered;
            }
        }

        //-------------------------------------------------------------------------
        // Component Registrations
        //-------------------------------------------------------------------------

        // Create a task that splits per-system registration across multiple threads
       /* struct ComponentRegistrationTask : public ITaskSet
        {
            ComponentRegistrationTask( List<EntityWorldSystem*> const& worldSystems, List<EntityModel::EntityComponentPair> const& componentsToRegister, List<EntityModel::EntityComponentPair> const& componentsToUnregister )
                : m_worldSystems( worldSystems )
                , m_componentsToRegister( componentsToRegister )
                , m_componentsToUnregister( componentsToUnregister )
            {
                m_SetSize = (uint32) worldSystems.Count();
            }

            virtual void ExecuteRange( TaskSetPartition range, uint32 threadnum ) override final
            {
                PROFILE_CPU_NAMED( "Component World Registration Task" );

                // Register/Unregister component with World Systems
                //-------------------------------------------------------------------------

                for ( uint64_t i = range.start; i < range.end; ++i )
                {
                    auto pSystem = m_worldSystems[i];

                    //-------------------------------------------------------------------------

                    size_t const numComponentsToUnregister = m_componentsToUnregister.Count();
                    for ( auto c = 0u; c < numComponentsToUnregister; c++ )
                    {
                        auto pEntity = m_componentsToUnregister[c].m_pEntity;
                        auto pComponent = m_componentsToUnregister[c].m_pComponent;

                        ENGINE_ASSERT( pEntity != nullptr );
                        ENGINE_ASSERT( pComponent != nullptr && pComponent->IsInitialized() && pComponent->m_isRegisteredWithWorld );
                        pSystem->UnregisterComponent( pEntity, pComponent );
                    }

                    //-------------------------------------------------------------------------

                    size_t const numComponentsToRegister = m_componentsToRegister.Count();
                    for ( auto c = 0u; c < numComponentsToRegister; c++ )
                    {
                        auto pEntity = m_componentsToRegister[c].m_pEntity;
                        auto pComponent = m_componentsToRegister[c].m_pComponent;

                        ENGINE_ASSERT( pEntity != nullptr && pEntity->IsInitialized() && !pComponent->m_isRegisteredWithWorld );
                        ENGINE_ASSERT( pComponent != nullptr && pComponent->IsInitialized() );
                        pSystem->RegisterComponent( pEntity, pComponent );
                    }
                }
            }

        private:

            List<EntityWorldSystem*> const&                         m_worldSystems;
            List<EntityModel::EntityComponentPair> const&            m_componentsToRegister;
            List<EntityModel::EntityComponentPair> const&            m_componentsToUnregister;
        };*/

        //-------------------------------------------------------------------------

        {
            PROFILE_CPU_NAMED( "Component World Registration" );

            // Get Components to register/unregister
            //-------------------------------------------------------------------------

            List<EntityModel::EntityComponentPair> componentsToUnregister;
            size_t const numComponentsToUnregister = initializationContext.m_componentsToUnregister.size_approx();
            componentsToUnregister.Resize( numComponentsToUnregister );

            size_t numDequeued = initializationContext.m_componentsToUnregister.try_dequeue_bulk( componentsToUnregister.Get(), numComponentsToUnregister );
            ENGINE_ASSERT( numComponentsToUnregister == numDequeued );

            //-------------------------------------------------------------------------

            List<EntityModel::EntityComponentPair> componentsToRegister;
            size_t const numComponentsToRegister = initializationContext.m_componentsToRegister.size_approx();
            componentsToRegister.Resize( numComponentsToRegister );

            numDequeued = initializationContext.m_componentsToRegister.try_dequeue_bulk( componentsToRegister.Get(), numComponentsToRegister );
            ENGINE_ASSERT( numComponentsToRegister == numDequeued );

            // Run registration task
            //-------------------------------------------------------------------------

            if ( ( numComponentsToUnregister + numComponentsToRegister ) > 0 )
            {
//                ComponentRegistrationTask componentRegistrationTask( initializationContext.m_worldSystems, componentsToRegister, componentsToUnregister );
//                initializationContext.m_pTaskSystem->ScheduleTask( &componentRegistrationTask );
//                initializationContext.m_pTaskSystem->WaitForTask( &componentRegistrationTask );
            }

            // Finalize component registration
            //-------------------------------------------------------------------------

            // Remove from type tracking table
            for ( auto const& pair : componentsToUnregister )
            {
                pair.m_pComponent->m_isRegisteredWithWorld = false;

                #ifdef SE_DEVELOPMENT
                EntityComponentTypeMap& componentTypeMap = *initializationContext.m_pComponentTypeMap;
                auto const castableTypeIDs = Types::GetAllCastableTypes( pair.m_pComponent );
                for ( auto castableTypeID : castableTypeIDs )
                {
                    List<EntityComponent const*> & v = componentTypeMap[castableTypeID];

                    v.Remove(pair.m_pComponent);
                }
                #endif
            }

            // Add to type tracking table
            for ( auto const& pair : componentsToRegister )
            {
                pair.m_pComponent->m_isRegisteredWithWorld = true;

                #if ENGINE_DEVELOPMENT_TOOLS
                EntityComponentTypeMap& componentTypeMap = *initializationContext.m_pComponentTypeMap;
                auto const castableTypeIDs = Types::GetAllCastableTypes( pair.m_pComponent );
                for ( auto castableTypeID : castableTypeIDs )
                {
                    componentTypeMap[castableTypeID].Add( pair.m_pComponent );
                }
                #endif
            }
        }
    }

    //-------------------------------------------------------------------------

    bool EntityMap::UpdateLoadingAndStateChanges( LoadingContext const& loadingContext, InitializationContext& initializationContext )
    {
        PROFILE_CPU_NAMED( "Map State Update" );
        ENGINE_ASSERT( Threading::IsMainThread() && loadingContext.IsValid() && initializationContext.IsValid() );

        #if ENGINE_DEVELOPMENT_TOOLS
        ENGINE_ASSERT( m_entitiesToHotReload.IsEmpty() );
        ENGINE_ASSERT( m_editedEntities.IsEmpty() ); // You are missing a EndComponentEdit call somewhere!
        #endif

        //-------------------------------------------------------------------------

        Threading::ScopeLock lock( m_mutex );

        //-------------------------------------------------------------------------

        switch ( m_status )
        {
            case Status::Unloading:
            {
                ProcessMapUnloading( loadingContext, initializationContext );
            }
            break;

            case Status::Loading:
            {
                ProcessMapLoading( loadingContext );
            }
            break;

            default:
            break;
        }

        // Process entity removal requests
        //-------------------------------------------------------------------------

        ProcessEntityShutdownRequests( initializationContext );
        ProcessEntityRegistrationRequests( initializationContext );
        ProcessEntityRemovalRequests( loadingContext );

        // Update entity load states
        //-------------------------------------------------------------------------

        ProcessEntityLoadingAndInitialization( loadingContext, initializationContext );
        ProcessEntityRegistrationRequests( initializationContext );

        // Return status
        //-------------------------------------------------------------------------

        if ( m_status == Status::Loading || !m_entitiesCurrentlyLoading.IsEmpty() )
        {
            return false;
        }

        return true;
    }

    //-------------------------------------------------------------------------
    // Tools
    //-------------------------------------------------------------------------

    #if ENGINE_DEVELOPMENT_TOOLS
    void EntityMap::RenameEntity( Entity* pEntity, StringID newNameID )
    {
        ENGINE_ASSERT( pEntity != nullptr && pEntity->m_mapID == m_ID );

        // Lock the map
        Threading::ScopeLock lock( m_mutex );

        // Remove from lookup map
        ENGINE_ASSERT( m_entityNameLookupMap.Remove( pEntity->m_name ) );

        // Rename
        pEntity->m_name = GenerateUniqueEntityNameID( newNameID );

        // Add to lookup map
        m_entityNameLookupMap.Add( pEntity->m_name, pEntity );
    }

    StringID EntityMap::GenerateUniqueEntityNameID( StringID desiredNameID ) const
    {
        auto GenerateUniqueName = [] ( String const& baseName, int32 counterValue )
        {
            String finalName;

            if ( baseName.Length() > 3 )
            {
                // Check if the last three characters are a numeric set, if so then increment the value and replace them
                if ( isdigit( baseName[baseName.Length() - 1] ) && isdigit( baseName[baseName.Length() - 2] ) && isdigit( baseName[baseName.Length() - 3] ) )
                {
                    finalName = String::Format(SE_TEXT("{0}{1}"), baseName.Substring(0, baseName.Length() - 3).Get(), counterValue );
                    return finalName;
                }
            }

            finalName = String::Format( SE_TEXT("{0} {1}"), baseName.Get(), counterValue );
            return finalName;
        };

        //-------------------------------------------------------------------------

        ENGINE_ASSERT( desiredNameID.IsValid() );

        StringView desiredName = desiredNameID.ToString();
        String finalName = desiredName;
        StringID finalNameID( finalName.Get() );

        uint32 counter = 0;
        bool isUniqueName = false;
        while ( !isUniqueName )
        {
            // Check the lookup map
            isUniqueName = m_entityNameLookupMap.Find( finalNameID ) == m_entityNameLookupMap.end();

            // If we found a name clash, generate a new name and try again
            if ( !isUniqueName )
            {
                finalName = GenerateUniqueName( desiredName, counter );
                finalNameID = StringID( finalName.Get() );
                counter++;
            }
        }

        //-------------------------------------------------------------------------

        return finalNameID;
    }

    void EntityMap::BeginComponentEdit( LoadingContext const& loadingContext, InitializationContext& initializationContext, EntityID const& entityID )
    {
        ENGINE_ASSERT( Threading::IsMainThread() );

        auto pEntity = FindEntity( entityID );
        ENGINE_ASSERT( pEntity != nullptr );
        ENGINE_ASSERT( !m_editedEntities.Contains(pEntity ) ); // Starting multiple edits for the same entity?!

        if ( pEntity->IsInitialized() )
        {
            pEntity->Shutdown( initializationContext );
            ProcessEntityRegistrationRequests( initializationContext );
        }

        pEntity->UnloadComponents( loadingContext );
		m_entitiesCurrentlyLoading.Remove(pEntity);
        m_editedEntities.Add( pEntity );
    }

    void EntityMap::EndComponentEdit( LoadingContext const& loadingContext, InitializationContext& initializationContext, EntityID const& entityID )
    {
        ENGINE_ASSERT( Threading::IsMainThread() );

        auto pEntity = FindEntity( entityID );

        ENGINE_ASSERT( pEntity != nullptr );
        ENGINE_ASSERT( !m_entitiesCurrentlyLoading.Contains(  pEntity ) );
        ENGINE_ASSERT( m_editedEntities.Contains( pEntity ) ); // Cant end an edit that was never started!

        pEntity->LoadComponents( loadingContext );
        m_entitiesCurrentlyLoading.Add( pEntity );
		m_editedEntities.Remove( pEntity);
    }

    //-------------------------------------------------------------------------

    void EntityMap::HotReload_UnloadEntities( LoadingContext const& loadingContext, InitializationContext& initializationContext, List<ResourceRequesterID> const& usersToReload )
    {
        ENGINE_ASSERT( Threading::IsMainThread() );
        ENGINE_ASSERT( !usersToReload.IsEmpty() );
        ENGINE_ASSERT( m_entitiesToHotReload.IsEmpty() );

        Threading::ScopeLock lock( m_mutex );

        // Run map loading code to ensure that any destroy entity request (that might be unloading resources are executed!)
        UpdateLoadingAndStateChanges( loadingContext, initializationContext );

        // There should never be any queued registration request at this stage!
        ENGINE_ASSERT( initializationContext.m_registerForEntityUpdate.size_approx() == 0 && initializationContext.m_unregisterForEntityUpdate.size_approx() == 0 );
        ENGINE_ASSERT( initializationContext.m_componentsToRegister.size_approx() == 0 && initializationContext.m_componentsToUnregister.size_approx() == 0 );

        // Generate list of entities to be reloaded
        for ( auto const& requesterID : usersToReload )
        {
            // See if the entity that needs a reload is in this map
            Entity* pFoundEntity = FindEntity( EntityID( requesterID.GetID() ) );
            if ( pFoundEntity != nullptr )
            {
                m_entitiesToHotReload.Add( pFoundEntity );
            }
        }

        // Request shutdown for any entities that are initialized
        bool someEntitiesRequireShutdown = false;
        for ( auto pEntityToHotReload : m_entitiesToHotReload )
        {
            if ( pEntityToHotReload->IsInitialized() )
            {
                pEntityToHotReload->Shutdown( initializationContext );
                someEntitiesRequireShutdown = true;
            }
        }

        // Process unregistration requests
        if ( someEntitiesRequireShutdown )
        {
            ProcessEntityRegistrationRequests( initializationContext );
        }

        // Unload entities
        for ( auto pEntityToHotReload : m_entitiesToHotReload )
        {
            ENGINE_ASSERT( !pEntityToHotReload->IsInitialized() );

            // We might still be loading this entity so remove it from the loading requests
			m_entitiesCurrentlyLoading.Remove(pEntityToHotReload);

            // Request unload of the components (client system needs to ensure that all resource requests are processed)
            pEntityToHotReload->UnloadComponents( loadingContext );
        }
    }

    void EntityMap::HotReload_ReloadEntities( LoadingContext const& loadingContext )
    {
        ENGINE_ASSERT( Threading::IsMainThread() );

        Threading::ScopeLock lock( m_mutex );

        for ( auto pEntityToHotReload : m_entitiesToHotReload )
        {
            ENGINE_ASSERT( pEntityToHotReload->IsUnloaded() );
            pEntityToHotReload->LoadComponents( loadingContext );
            m_entitiesCurrentlyLoading.Add( pEntityToHotReload );
        }

        m_entitiesToHotReload.Clear();
    }
    #endif
}
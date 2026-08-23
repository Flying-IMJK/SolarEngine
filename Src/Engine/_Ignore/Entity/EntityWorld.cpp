#include "EntityWorld.h"
#include "EntityWorldUpdateContext.h"
#include "EntityWorldSettings.h"
#include "Runtime/System/ResourceSystem.h"
#include "Core/Profiler/Profiler.h"
#include "Core/TypeSystem/Types.h"
#include "Core/Types/Collections/Sorting.h"
#include "Core/Thread/JobSystem.h"
//-------------------------------------------------------------------------

namespace SE
{
    EntityWorld::EntityWorld( EntityWorldType worldType )
        : m_initializationContext( m_worldSystems, m_entityUpdateList )
        , m_worldType( worldType )
    {}

    EntityWorld::~EntityWorld()
    {
        ENGINE_ASSERT( m_maps.IsEmpty());
        ENGINE_ASSERT( m_worldSystems.IsEmpty() );
        ENGINE_ASSERT( m_entityUpdateList.IsEmpty() );

/*        for ( int8_t i = 0; i < (int8_t) UpdateStage::NumStages; i++ )
        {
            ENGINE_ASSERT( m_systemUpdateLists[i].IsEmpty() );
        }*/

        //-------------------------------------------------------------------------

        #ifdef SE_DEVELOPMENT
        for ( auto& v : m_componentTypeLookup )
        {
            ENGINE_ASSERT( v.Value.IsEmpty() );
        }
        #endif
    }

    void EntityWorld::Initialize( Systems const& systemsRegistry, List<TypeInfo const*> worldSystemTypeInfos )
    {
        // Set up Contexts
        //-------------------------------------------------------------------------

        m_loadingContext = EntityModel::LoadingContext(systemsRegistry.GetSystem<ResourceSystem>() );
        ENGINE_ASSERT( m_loadingContext.IsValid() );

        //-------------------------------------------------------------------------
        
        #ifdef SE_DEVELOPMENT
        m_initializationContext.SetComponentTypeMapPtr( &m_componentTypeLookup );
        #endif

        ENGINE_ASSERT( m_initializationContext.IsValid() );

        // Create World Systems
        //-------------------------------------------------------------------------

        for ( auto pTypeInfo : worldSystemTypeInfos )
        {
            // Create and initialize world system
            auto pWorldSystem = Cast<EntityWorldSystem>( pTypeInfo->CreateType() );
            pWorldSystem->m_pWorld = this;
            pWorldSystem->InitializeSystem( systemsRegistry );
            m_worldSystems.Add( pWorldSystem );

            // Add to update lists
/*            for ( int8_t i = 0; i < (int8) UpdateStage::NumStages; i++ )
            {
                if ( pWorldSystem->GetRequiredUpdatePriorities().IsStageEnabled( (UpdateStage) i ) )
                {
                    m_systemUpdateLists[i].Add( pWorldSystem );
                }

                // Sort update list
                Function<bool(EntityWorldSystem* const&, EntityWorldSystem* const&)> comparator = [i] ( EntityWorldSystem* const& pSystemA, EntityWorldSystem* const& pSystemB )
                {
                    uint8 const A = pSystemA->GetRequiredUpdatePriorities().GetPriorityForStage( (UpdateStage) i );
                    uint8 const B = pSystemB->GetRequiredUpdatePriorities().GetPriorityForStage( (UpdateStage) i );
                    return A > B;
                };

                Sorting::QuickSort( m_systemUpdateLists[i], comparator );
            }*/
        }

        // Create World Settings
        //-------------------------------------------------------------------------

        ENGINE_ASSERT( m_pSettingsRegistry == nullptr );
        m_pSettingsRegistry = systemsRegistry.GetSystem<SettingsRegistry>();
        m_pSettingsRegistry->CreateGroup( m_worldID.m_value );

        List<TypeInfo const*> settingsTypes = Types::GetAllDerivedTypes( IEntityWorldSettings::GetStaticTypeID(), false, false, false );
        for ( auto pTypeInfo : settingsTypes )
        {
            m_pSettingsRegistry->CreateSettings( m_worldID.m_value, pTypeInfo );
        }

        // Create and initialize the persistent map
        //-------------------------------------------------------------------------

        m_maps.Add( New<EntityModel::EntityMap>() );
        m_maps[0]->Load( m_loadingContext, m_initializationContext );

        //-------------------------------------------------------------------------

        m_initialized = true;
    }

    void EntityWorld::Shutdown()
    {
        // Unload maps
        //-------------------------------------------------------------------------
        
        for ( auto& pMap : m_maps )
        {
            pMap->Unload( m_loadingContext, m_initializationContext );
        }

        // Run the loading update as this will immediately unload all maps
        //-------------------------------------------------------------------------

        while ( !m_maps.IsEmpty() )
        {
            UpdateLoading();
        }

        // Destroy all settings
        //-------------------------------------------------------------------------

        m_pSettingsRegistry->DestroyGroup( m_worldID.m_value );
        m_pSettingsRegistry = nullptr;

        // Shutdown all world systems
        //-------------------------------------------------------------------------

        for( auto pWorldSystem : m_worldSystems )
        {
            // Remove from update lists
/*            for ( int8_t i = 0; i < (int8_t) UpdateStage::NumStages; i++ )
            {
                if ( pWorldSystem->GetRequiredUpdatePriorities().IsStageEnabled( (UpdateStage) i ) )
                {
                    m_systemUpdateLists[i].Remove(pWorldSystem);
                }
            }*/

            // Shutdown and destroy world system
            pWorldSystem->ShutdownSystem();
            Delete( pWorldSystem );
        }

        m_worldSystems.Clear();

        //-------------------------------------------------------------------------
        m_initialized = false;
    }

    //-------------------------------------------------------------------------
    // Misc
    //-------------------------------------------------------------------------

    EntityWorldSystem* EntityWorld::GetWorldSystem( uint32 worldSystemID ) const
    {
        for ( EntityWorldSystem* pWorldSystem : m_worldSystems )
        {
            if ( pWorldSystem->GetSystemID() == worldSystemID )
            {
                return pWorldSystem;
            }
        }

        ENGINE_UNREACHABLE_CODE();
        return nullptr;
    }

    //-------------------------------------------------------------------------
    // Frame Update
    //-------------------------------------------------------------------------

    void EntityWorld::UpdateLoading()
    {
        PROFILE_CPU_NAMED( "World Loading" );

        // Update all maps internal loading state
        //-------------------------------------------------------------------------
        // This will fill the world initialization/registration lists used below
        // This will also handle all hot-reload unload/load requests

        for ( int32 i = (int32) m_maps.Count() - 1; i >= 0; i-- )
        {
            if ( m_maps[i]->UpdateLoadingAndStateChanges( m_loadingContext, m_initializationContext ) )
            {
                if ( m_maps[i]->IsUnloaded() )
                {
                    Delete( m_maps[i] );
                    m_maps.RemoveAt(i );
                }
            }
        }
    }

    void EntityWorld::Update()
    {
        ENGINE_ASSERT( Threading::IsMainThread() );
        ENGINE_ASSERT( !m_isSuspended );

		struct EntityUpdateJob : public Threading::IJob
        {
            EntityUpdateJob( EntityWorldUpdateContext const& context, List<Entity*>& updateList )
                : m_context( context )
                , m_updateList( updateList )
            {
            }

            // Only used for spatial dependency chain updates
            inline void RecursiveEntityUpdate( Entity* pEntity )
            {
                pEntity->UpdateSystems( m_context );

                for ( auto pAttachedEntity : pEntity->GetAttachedEntities() )
                {
                    RecursiveEntityUpdate( pAttachedEntity );
                }
            }

			void Run(int32 index) override
            {
				auto pEntity = m_updateList[index];

				// Ignore any entities with spatial parents, these will be updated by their parents
				if ( pEntity->HasSpatialParent() )
				{
					return;
				}

				//-------------------------------------------------------------------------

				if ( pEntity->HasAttachedEntities() )
				{
					PROFILE_CPU_NAMED( "Update Entity Chain" );
					RecursiveEntityUpdate( pEntity );
				}
				else // Direct entity update
				{
					PROFILE_CPU_NAMED( "Update Entity" );
					pEntity->UpdateSystems( m_context );
				}
            }

        private:

            EntityWorldUpdateContext const&              m_context;
            List<Entity*>&                            m_updateList;
        };

        //-------------------------------------------------------------------------

        bool const isWorldPaused = IsPaused() && !m_timeStepRequested;

        // Skip all non-pause updates for paused worlds
/*        if ( isWorldPaused && updateStage != UpdateStage::Paused )
        {
            return;
        }

        // Skip paused update for non-paused worlds
        if ( context.GetUpdateStage() == UpdateStage::Paused && !isWorldPaused )
        {
            return;
        }*/

        //-------------------------------------------------------------------------

        EntityWorldUpdateContext entityWorldUpdateContext(this );

        // Update entities
        //-------------------------------------------------------------------------

        EntityUpdateJob entityUpdateJob( entityWorldUpdateContext, m_entityUpdateList );
		Threading::JobHandle jobHandle = Threading::JobSystem::Dispatch(&entityUpdateJob, m_entityUpdateList.Count());
		Threading::JobSystem::Wait(jobHandle);

        // Force execution on main thread for debugging purposes
        //entityUpdateTask.ExecuteRange( { 0u, (uint32) m_entityUpdateList.size() }, 0 );

        // Update systems
        //-------------------------------------------------------------------------

/*        for ( auto pSystem : m_systemUpdateLists[(int8_t) updateStage] )
        {
			PROFILE_CPU_NAMED( "Update World Systems" );
            ENGINE_ASSERT( pSystem->GetRequiredUpdatePriorities().IsStageEnabled( updateStage ) );
            pSystem->UpdateSystem( entityWorldUpdateContext );
        }*/

        //-------------------------------------------------------------------------

/*        if ( updateStage == UpdateStage::FrameEnd )
        {
            m_timeStepRequested = false;
        }*/
    }

    //-------------------------------------------------------------------------
    // Maps
    //-------------------------------------------------------------------------

    bool EntityWorld::IsBusyLoading() const
    {
        for ( auto const& pMap : m_maps )
        {
            if( pMap->IsLoading() ) 
            {
                return true;
            }
        }

        return false;
    }

    bool EntityWorld::HasMap( ResID const& mapResourceID ) const
    {
        for ( auto const& pMap : m_maps )
        {
            if ( pMap->GetMapResourceID() == mapResourceID )
            {
                return true;
            }
        }

        return false;
    }

    bool EntityWorld::HasMap( EntityMapID const& mapID ) const
    {
        for ( auto const& pMap : m_maps )
        {
            if ( pMap->GetID() == mapID )
            {
                return true;
            }
        }

        return false;
    }

    bool EntityWorld::IsMapLoaded( ResID const& mapResourceID ) const
    {
        // Make sure the map isn't already loaded or loading, since duplicate loads are not allowed
        for ( auto const& pMap : m_maps )
        {
            if ( pMap->GetMapResourceID() == mapResourceID )
            {
                return pMap->IsLoaded();
            }
        }

        // Dont call this function with an unknown map
        ENGINE_UNREACHABLE_CODE();
        return false;
    }

    bool EntityWorld::IsMapLoaded( EntityMapID const& mapID ) const
    {
        // Make sure the map isn't already loaded or loading, since duplicate loads are not allowed
        for ( auto const& pMap : m_maps )
        {
            if ( pMap->GetID() == mapID )
            {
                return pMap->IsLoaded();
            }
        }

        // Dont call this function with an unknown map
        ENGINE_UNREACHABLE_CODE();
        return false;
    }

    EntityModel::EntityMap* EntityWorld::CreateTransientMap()
    {
        EntityModel::EntityMap* pNewMap = m_maps.AddOne();
        pNewMap->Load( m_loadingContext, m_initializationContext );
        return pNewMap;
    }

    EntityModel::EntityMap const* EntityWorld::GetMap( ResID const& mapResourceID ) const
    {
        ENGINE_ASSERT( mapResourceID.IsValid() && mapResourceID.GetTypeID() == Typeof<EntityModel::SerializedEntityMap>());
		Function<bool( EntityModel::EntityMap * const&)> predicate = [&mapResourceID] (EntityModel::EntityMap * const&pMap)
		{
		  return pMap->GetMapResourceID() == mapResourceID;
		};
        int foundMapIndex = ListExtensions::IndexOf(m_maps, predicate);
        ENGINE_ASSERT( foundMapIndex != INVALID_INDEX);
        return m_maps[foundMapIndex];
    }

    EntityModel::EntityMap const* EntityWorld::GetMap( EntityMapID const& mapID ) const
    {
        ENGINE_ASSERT( mapID.IsValid() );
		Function<bool(EntityModel::EntityMap * const&)> predicate = [&mapID] (EntityModel::EntityMap * const&pMap){
			return pMap->GetID() == mapID;
		};

		int foundMapIndex = ListExtensions::IndexOf(m_maps, predicate);

        ENGINE_ASSERT( foundMapIndex != INVALID_INDEX);
        return m_maps[foundMapIndex];
    }

    EntityModel::EntityMap const* EntityWorld::GetMapForEntity( Entity const* pEntity ) const
    {
        ENGINE_ASSERT( pEntity != nullptr && pEntity->IsAddedToMap() );
        auto const& mapID = pEntity->GetMapID();
        auto pMap = GetMap( mapID );
        ENGINE_ASSERT( pMap != nullptr );
        return pMap;
    }

    EntityMapID EntityWorld::LoadMap( ResID const& mapResourceID )
    {
        ENGINE_ASSERT( mapResourceID.IsValid() && mapResourceID.GetTypeID() == Typeof<EntityModel::SerializedEntityMap>());

        ENGINE_ASSERT( !HasMap( mapResourceID ) );
        auto pNewMap = New<EntityModel::EntityMap>( mapResourceID );
		m_maps.Add(pNewMap);
        pNewMap->Load( m_loadingContext, m_initializationContext );
        return pNewMap->GetID();
    }

    void EntityWorld::UnloadMap( ResID const& mapResourceID )
    {
        ENGINE_ASSERT( mapResourceID.IsValid() && mapResourceID.GetTypeID() == Typeof<EntityModel::SerializedEntityMap>());
		Function<bool(EntityModel::EntityMap * const&)> predicate = [&mapResourceID] ( EntityModel::EntityMap const* pMap){
			return pMap->GetMapResourceID() == mapResourceID;
		};

		int foundMapIndex = ListExtensions::IndexOf(m_maps, predicate);

		ENGINE_ASSERT( foundMapIndex != INVALID_INDEX);
        m_maps.At(foundMapIndex)->Unload( m_loadingContext, m_initializationContext );
    }

    //-------------------------------------------------------------------------
    // Editing / Hot Reload
    //-------------------------------------------------------------------------

    #ifdef SE_DEVELOPMENT

    void EntityWorld::BeginComponentEdit( Entity* pEntity )
    {
        ENGINE_ASSERT( pEntity != nullptr );

        auto pMap = GetMap( pEntity->GetMapID() );
        ENGINE_ASSERT( pMap != nullptr );
        pMap->BeginComponentEdit( m_loadingContext, m_initializationContext, pEntity->GetID() );
    }

    void EntityWorld::EndComponentEdit( Entity* pEntity )
    {
        ENGINE_ASSERT( pEntity != nullptr );

        auto pMap = GetMap( pEntity->GetMapID() );
        ENGINE_ASSERT( pMap != nullptr );
        pMap->EndComponentEdit( m_loadingContext, m_initializationContext, pEntity->GetID() );
    }

    void EntityWorld::BeginComponentEdit( EntityComponent* pComponent )
    {
        ENGINE_ASSERT( pComponent != nullptr );

        auto pEntity = FindEntity( pComponent->GetEntityID() );
        ENGINE_ASSERT( pEntity != nullptr );

        auto pMap = GetMap( pEntity->GetMapID() );
        ENGINE_ASSERT( pMap != nullptr );
        pMap->BeginComponentEdit( m_loadingContext, m_initializationContext, pEntity->GetID() );
    }

    void EntityWorld::EndComponentEdit( EntityComponent* pComponent )
    {
        ENGINE_ASSERT( pComponent != nullptr );

        auto pEntity = FindEntity( pComponent->GetEntityID() );
        ENGINE_ASSERT( pEntity != nullptr );

        auto pMap = GetMap( pEntity->GetMapID() );
        ENGINE_ASSERT( pMap != nullptr );
        pMap->EndComponentEdit( m_loadingContext, m_initializationContext, pEntity->GetID() );
    }

    //-------------------------------------------------------------------------

    void EntityWorld::HotReload_UnloadEntities( List<ResourceRequesterID> const& usersToReload )
    {
        ENGINE_ASSERT( !usersToReload.IsEmpty() );
        for ( auto& pMap : m_maps )
        {
            pMap->HotReload_UnloadEntities( m_loadingContext, m_initializationContext, usersToReload );
        }
    }

    void EntityWorld::HotReload_ReloadEntities()
    {
        for ( auto& pMap : m_maps )
        {
            pMap->HotReload_ReloadEntities( m_loadingContext );
        }
    }
    #endif
}
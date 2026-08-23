#pragma once

#include "Runtime/API.h"
#include "EntityDescriptors.h"

#include "Core/Types/Event.h"
#include "Core/Thread/Threading.h"
#include "Core/Math/Transform.h"

#include "Runtime/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------
// Entity Map
//-------------------------------------------------------------------------
// This is a logical grouping of entities whose lifetime is linked
// e.g. the game level, a cell in a open world game, etc.
//
// * Maps manage lifetime, loading and initialization of entities
// * All map operations are threadsafe using a standard mutex
//-------------------------------------------------------------------------

namespace SE
{
    class Entity;

    //-------------------------------------------------------------------------

    namespace EntityModel
    {
        struct LoadingContext;
        struct InitializationContext;
        class SerializedEntityCollection;

        //-------------------------------------------------------------------------

        class SE_API_RUNTIME EntityMap
        {
            friend struct Serializer;

            enum class Status
            {
                LoadFailed = -1,
                Unloaded = 0,
                Loading,
                Loaded,
                Unloading,
            };

            struct RemovalRequest
            {
				RemovalRequest() : m_pEntity(), m_shouldDestroy() {}
                RemovalRequest(Entity* pEntity, bool shouldDestroy) : m_pEntity( pEntity ), m_shouldDestroy( shouldDestroy ) {}

                Entity*     m_pEntity = nullptr;
                bool        m_shouldDestroy = false;
            };

        public:

            EntityMap(); // Default constructor creates a transient map
            EntityMap( ResID mapResourceID );
            EntityMap( EntityMap const& map );
            EntityMap( EntityMap&& map );
            ~EntityMap();

            EntityMap& operator=( EntityMap const& map );
            EntityMap& operator=( EntityMap&& map );

            // Map Info
            //-------------------------------------------------------------------------

            inline ResID const& GetMapResourceID() const { return m_pMapDesc.GetResourceID(); }
            inline EntityMapID GetID() const { return m_ID; }
            inline bool IsTransientMap() const { return m_isTransientMap; }

            // Loading
            //-------------------------------------------------------------------------

            void Load( LoadingContext const& loadingContext, InitializationContext& initializationContext );
            void Unload( LoadingContext const& loadingContext, InitializationContext& initializationContext );

            // Map State
            //-------------------------------------------------------------------------

            // Updates map loading and entity state, returns true if all loading/state changes are complete, false otherwise
            bool UpdateLoadingAndStateChanges( LoadingContext const& loadingContext, InitializationContext& initializationContext );

            // Do we have any pending entity addition or removal requests?
            inline bool HasPendingAddOrRemoveRequests() const { return ( m_entitiesToLoad.Count() + m_entitiesToRemove.Count() ) > 0; }

            bool IsLoading() const { return m_status == Status::Loading; }
            inline bool IsLoaded() const { return m_status == Status::Loaded; }
            inline bool IsUnloaded() const { return m_status == Status::Unloaded; }
            inline bool HasLoadingFailed() const { return m_status == Status::LoadFailed; }

            //-------------------------------------------------------------------------
            // Entity API
            //-------------------------------------------------------------------------

            // Get the number of entities in this map
            inline int32_t GetNumEntities() const { return (int32_t) m_entities.Count(); }

            // Get all entities in the map. This includes all entities that are still loading/unloading, etc...
            inline List<Entity*> const& GetEntities() const { return m_entities; }

            // Finds an entity via its ID. This uses a lookup map so is relatively performant
            inline Entity* FindEntity( EntityID entityID ) const
            {
                auto iter = m_entityIDLookupMap.Find( entityID );
                return ( iter != m_entityIDLookupMap.end() ) ? iter->Value : nullptr;
            }

            // Does this map contain this entity?
            // If you are working with an entity and a map, prefer to check the map ID vs the entity's map ID!
            inline bool ContainsEntity( EntityID entityID ) const { return FindEntity( entityID ) != nullptr; }

            // Adds a set of entities to this map - Transfers ownership of the entities to the map
            // Additionally allows you to offset all the entities via the supplied offset transform
            // Takes 1 frame to be fully added
            void AddEntities( List<Entity*> const& entities, Transform const& offsetTransform = Transform::Identity );

            // Instantiates and adds an entity collection to the map
            // Additionally allows you to offset all the entities via the supplied offset transform
            // Takes 1 frame to be fully added
            void AddEntityCollection(SerializedEntityCollection const& entityCollectionDesc, Transform const& offsetTransform = Transform::Identity, List<Entity*>* pOutCreatedEntities = nullptr );

            // Add a newly created entity to the map - Transfers ownership of the entity to the map
            void AddEntity( Entity* pEntity );

            // Unload and remove an entity from the map - Transfer ownership of the entity to the calling code
            // May take multiple frames to be fully destroyed, as the shutdown/unload occurs during the loading update
            // Entity is free to be deleted by the user ONLY once it is in an unloaded state
            Entity* RemoveEntity( EntityID entityID );

            // Unload, remove and destroy entity in this map
            // May take multiple frames to be fully destroyed, as the removal occurs during the loading update
            void DestroyEntity( EntityID entityID );

            //-------------------------------------------------------------------------
            // Tools API
            //-------------------------------------------------------------------------

            #ifdef SE_DEVELOPMENT
            // Gets a unique entity name for this map given a specified desired name
            StringID GenerateUniqueEntityNameID( StringID desiredNameID ) const;

            // Gets a unique entity name for this map given a specified desired name
            inline StringID GenerateUniqueEntityNameID( Char const* pName ) const { return GenerateUniqueEntityNameID( StringID( pName ) ); }

            // Rename an existing entity - allow renaming of existing entities, will ensure that the new name is unique
            void RenameEntity( Entity* pEntity, StringID newNameID );

            // Find an entity by name
            inline Entity* FindEntityByName( StringID nameID ) const
            {
                auto iter = m_entityNameLookupMap.Find( nameID );
                return ( iter != m_entityNameLookupMap.end() ) ? iter->Value : nullptr;
            }

            // This function will shutdown and unload the entity, allowing its components' properties to be edited safely!
            void BeginComponentEdit( LoadingContext const& loadingContext, InitializationContext& initializationContext, EntityID const& entityID );

            // Completes a component edit operation
            void EndComponentEdit( LoadingContext const& loadingContext, InitializationContext& initializationContext, EntityID const& entityID );

            // Shutdown and unload all entities that are affected by the hot-reload
            void HotReload_UnloadEntities( LoadingContext const& loadingContext, InitializationContext& initializationContext, List<ResourceRequesterID> const& usersToReload );

            // Load all entities unloaded due to the hot-reload
            void HotReload_ReloadEntities( LoadingContext const& loadingContext );
            #endif

        private:

            // Called whenever the internal state of an entity changes, schedules the entity for loading
            void OnEntityStateUpdated( Entity* pEntity );

            void ProcessMapLoading( LoadingContext const& loadingContext );
            void ProcessMapUnloading( LoadingContext const& loadingContext, InitializationContext& initializationContext );
            void ProcessEntityRegistrationRequests( InitializationContext& initializationContext );
            void ProcessEntityShutdownRequests( InitializationContext& initializationContext );
            void ProcessEntityRemovalRequests( LoadingContext const& loadingContext );
            void ProcessEntityLoadingAndInitialization( LoadingContext const& loadingContext, InitializationContext& initializationContext );

            // Remove entity
            Entity* RemoveEntityInternal( EntityID entityID, bool destroyEntityOnceRemoved );

        private:

            EntityMapID                                 m_ID = GenerateEntityMapID(); // ID is always regenerated at creation time, do not rely on the ID being the same for a map on different runs
            CriticalSection                  			m_mutex;
            TResPtr<SerializedEntityMap>           		m_pMapDesc;
            List<Entity*>                             	m_entities;
            Dictionary<EntityID, Entity*>               m_entityIDLookupMap;
            List<Entity*>                             	m_entitiesCurrentlyLoading;
            List<Entity*>                             	m_entitiesToLoad;
            List<RemovalRequest>                      	m_entitiesToRemove;
            EventBindingID                              m_entityUpdateEventBindingID;
            Status                                      m_status = Status::Unloaded;
            bool const                                  m_isTransientMap = false; // If this is set, then this is a transient map i.e.created and managed at runtime and not loaded from disk

            #ifdef SE_DEVELOPMENT
			Dictionary<StringID, Entity*>               m_entityNameLookupMap; // All entities that have attempted to load
            List<Entity*>                             	m_entitiesToHotReload;
            List<Entity*>                             	m_editedEntities;
            #endif
        };
    }
}
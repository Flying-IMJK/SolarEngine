#include "Entity.h"
#include "EntityWorldUpdateContext.h"
#include "EntityContexts.h"
#include "EntityDescriptors.h"
#include "EntityLog.h"
#include "Runtime/Resource/ResourceRequest.h"
#include "Core/TypeSystem/Types.h"
#include "Core/Memory/Memory.h"
#include "Core/Types/Collections/ListExtensions.h"
#include "Core/Types/Collections/Sorting.h"

//-------------------------------------------------------------------------

namespace SE
{
    TEvent<Entity*> Entity::s_entityInternalStateUpdatedEvent;

    //-------------------------------------------------------------------------

    Entity::~Entity()
    {
        ENGINE_ASSERT( m_status == Status::Unloaded );
        ENGINE_ASSERT( !m_isSpatialAttachmentCreated );
        ENGINE_ASSERT( m_updateRegistrationStatus == UpdateRegistrationStatus::Unregistered );

        // Break all inter-entity links
        if ( IsSpatialEntity() )
        {
            // Clear any child attachments
            for ( auto pAttachedEntity : m_attachedEntities )
            {
                ENGINE_ASSERT( pAttachedEntity->m_pParentSpatialEntity == this );
                pAttachedEntity->ClearSpatialParent();
            }

            // Clear any parent attachment we might have
            if ( m_pParentSpatialEntity != nullptr )
            {
                ClearSpatialParent();
            }
        }

        // Clean up deferred actions
        for ( auto& action : m_deferredActions )
        {
            // Add actions took ownership of the component to add, so it needs to be deleted
            // All other actions can be ignored
            if ( action.m_type == EntityInternalStateAction::Type::AddComponent )
            {
				EntityComponent * pComponent = const_cast<EntityComponent *>(reinterpret_cast<EntityComponent const*>( action.m_ptr ));
                Delete( pComponent );
            }
        }
        m_deferredActions.Clear();

        // Destroy Systems
        for ( auto& pSystem : m_systems )
        {
            Delete( pSystem );
        }

        m_systems.Clear();

        // Destroy components
        for ( auto& pComponent : m_components )
        {
            Delete( pComponent );
        }

        m_components.Clear();
    }

    void Entity::GetReferencedResources( List<ResID>& outReferencedResources ) const
    {
        for ( auto pComponent : m_components )
        {
//            pComponent->GetTypeInfo()->GetReferencedResources( pComponent, outReferencedResources );
        }
    }

    //-------------------------------------------------------------------------

    void Entity::LoadComponents( EntityModel::LoadingContext const& loadingContext )
    {
        ENGINE_ASSERT( m_status == Status::Unloaded );
        Threading::ScopeLock myLock( m_internalStateMutex );

        for ( auto pComponent : m_components )
        {
            ENGINE_ASSERT( pComponent->IsUnloaded() );
            pComponent->Load( loadingContext, ResourceRequesterID( m_ID.m_value ) );
        }

        m_status = Status::Loaded;
    }

    void Entity::UnloadComponents( EntityModel::LoadingContext const& loadingContext )
    {
        ENGINE_ASSERT( m_status == Status::Loaded );
        Threading::ScopeLock myLock( m_internalStateMutex );

        for ( auto pComponent : m_components )
        {
            if ( pComponent->IsUnloaded() )
            {
                continue;
            }

            //-------------------------------------------------------------------------

            if ( pComponent->IsInitialized() )
            {
                pComponent->Shutdown();
                ENGINE_ASSERT( !pComponent->IsInitialized() ); // Did you forget to call the parent class shutdown?
            }

            pComponent->Unload( loadingContext, ResourceRequesterID( m_ID.m_value ) );
        }

        m_status = Status::Unloaded;
    }

    //-------------------------------------------------------------------------

    void Entity::Initialize( EntityModel::InitializationContext& initializationContext )
    {
        ENGINE_ASSERT( m_status == Status::Loaded );
        ENGINE_ASSERT( m_updateRegistrationStatus == UpdateRegistrationStatus::Unregistered );

        Threading::ScopeLock myLock( m_internalStateMutex );

        // Initialize spatial hierarchy
        //-------------------------------------------------------------------------
        // Transforms are set at serialization time so we have all information available to update the world transforms

        if ( HasSpatialParent() )
        {
            // If we are attached to another entity we CANNOT be initialized if our parent is not. This ensures that attachment chains have consistent initialization state
            ENGINE_ASSERT( m_pParentSpatialEntity->IsInitialized() );

            if ( !m_isSpatialAttachmentCreated )
            {
                CreateSpatialAttachment();
            }
        }

        if ( IsSpatialEntity() )
        {
            m_pRootSpatialComponent->CalculateWorldTransform( false );
        }

        // Systems and Components
        //-------------------------------------------------------------------------

        for ( auto pSystem : m_systems )
        {
            pSystem->Initialize();
        }

        for ( auto pComponent : m_components )
        {
            if ( pComponent->IsInitialized() )
            {
                RegisterComponentWithLocalSystems( pComponent );
                initializationContext.m_componentsToRegister.enqueue( EntityModel::EntityComponentPair( this, pComponent ) );
            }
        }

        for ( auto pSystem : m_systems )
        {
            pSystem->PostComponentRegister();
        }

        // Register for system updates
        if ( !m_systems.IsEmpty() )
        {
            GenerateSystemUpdateList();
            initializationContext.m_registerForEntityUpdate.enqueue( this );
            m_updateRegistrationStatus = UpdateRegistrationStatus::QueuedForRegister;
        }

        // Spatial Attachments
        //-------------------------------------------------------------------------
        
        if ( IsSpatialEntity() )
        {
            RefreshChildSpatialAttachments();
        }

        // Set initialization state
        //-------------------------------------------------------------------------
        // This needs to be done before the initialization of the attached entities since they check this value

        m_status = Status::Initialized;

        // Initialize attached entities
        //-------------------------------------------------------------------------

        for ( auto pAttachedEntity : m_attachedEntities )
        {
            ENGINE_ASSERT( !pAttachedEntity->IsInitialized() );
            if ( pAttachedEntity->IsLoaded() )
            {
                pAttachedEntity->Initialize( initializationContext );
            }
        }
    }

    void Entity::Shutdown( EntityModel::InitializationContext& initializationContext )
    {
        ENGINE_ASSERT( m_status == Status::Initialized );

        Threading::ScopeLock myLock( m_internalStateMutex );

        // If we are attached to another entity, that entity must also have been initialized, else we have a problem in our attachment chains
        if ( HasSpatialParent() )
        {
            ENGINE_ASSERT( m_pParentSpatialEntity->IsInitialized() );
        }

        // Shutdown attached entities
        //-------------------------------------------------------------------------

        for ( auto pAttachedEntity : m_attachedEntities )
        {
            if ( pAttachedEntity->IsInitialized() )
            {
                pAttachedEntity->Shutdown( initializationContext );
            }
        }

        // Spatial Attachments
        //-------------------------------------------------------------------------

        if ( m_isSpatialAttachmentCreated )
        {
            DestroySpatialAttachment( SpatialAttachmentRule::KeepLocalTranform );
        }

        // Systems and Components
        //-------------------------------------------------------------------------

        // Unregister from system updates
        if ( !m_systems.IsEmpty() )
        {
            if ( m_updateRegistrationStatus == UpdateRegistrationStatus::Registered )
            {
                initializationContext.m_unregisterForEntityUpdate.enqueue( this );
                m_updateRegistrationStatus = UpdateRegistrationStatus::QueuedForUnregister;
            }

/*            for ( int8_t i = 0; i < (int8_t) UpdateStage::NumStages; i++ )
            {
                m_systemUpdateLists[i].Clear();
            }*/
        }

        for ( auto pSystem : m_systems )
        {
            pSystem->PreComponentUnregister();
        }

        for ( auto pComponent : m_components )
        {
            if ( pComponent->m_isRegisteredWithWorld )
            {
                initializationContext.m_componentsToUnregister.enqueue( EntityModel::EntityComponentPair( this, pComponent ) );
            }

            if ( pComponent->m_isRegisteredWithEntity )
            {
                UnregisterComponentFromLocalSystems( pComponent );
            }
        }

        for ( auto pSystem : m_systems )
        {
            pSystem->Shutdown();
        }

        //-------------------------------------------------------------------------

        m_status = Status::Loaded;
    }

    //-------------------------------------------------------------------------

    void Entity::UpdateSystems( EntityWorldUpdateContext const& context )
    {
/*        int8 const updateStageIdx = (int8) context.GetUpdateStage();
        for( auto pSystem : m_systemUpdateLists[updateStageIdx] )
        {
            ENGINE_ASSERT( pSystem->GetRequiredUpdatePriorities().IsStageEnabled( (UpdateStage) updateStageIdx ) );
            pSystem->Update( context );
        }*/
    }

    void Entity::RegisterComponentWithLocalSystems( EntityComponent* pComponent )
    {
        ENGINE_ASSERT( pComponent != nullptr && pComponent->IsInitialized() && pComponent->m_entityID == m_ID && !pComponent->m_isRegisteredWithEntity );

        Threading::ScopeLock lock( m_internalStateMutex );

        for ( auto pSystem : m_systems )
        {
            pSystem->RegisterComponent( pComponent );
            pComponent->m_isRegisteredWithEntity = true;
        }
    }

    void Entity::UnregisterComponentFromLocalSystems( EntityComponent* pComponent )
    {
        ENGINE_ASSERT( pComponent != nullptr && pComponent->IsInitialized() && pComponent->m_entityID == m_ID && pComponent->m_isRegisteredWithEntity );

        Threading::ScopeLock lock( m_internalStateMutex );

        for ( auto pSystem : m_systems )
        {
            pSystem->UnregisterComponent( pComponent );
            pComponent->m_isRegisteredWithEntity = false;
        }
    }

    bool Entity::UpdateEntityState( EntityModel::LoadingContext const& loadingContext, EntityModel::InitializationContext& initializationContext )
    {
        Threading::ScopeLock lock( m_internalStateMutex );

        //-------------------------------------------------------------------------
        // Deferred Actions
        //-------------------------------------------------------------------------
        // Note: ORDER OF OPERATIONS MATTERS!

        bool const entityStateChanged = !m_deferredActions.IsEmpty();
        for ( int32 i = 0; i < (int32) m_deferredActions.Count(); i++ )
        {
            EntityInternalStateAction& action = m_deferredActions[i];

            switch ( action.m_type )
            {
                case EntityInternalStateAction::Type::CreateSystem:
                {
                    CreateSystemImmediate( (TypeInfo const*) action.m_ptr );
                    GenerateSystemUpdateList();
                    m_deferredActions.RemoveAt(i );
                    i--;
                }
                break;

                case EntityInternalStateAction::Type::DestroySystem:
                {
                    DestroySystemImmediate( (TypeInfo const*) action.m_ptr );
                    GenerateSystemUpdateList();
                    m_deferredActions.RemoveAt(i );
                    i--;
                }
                break;

                case EntityInternalStateAction::Type::AddComponent:
                {
                    SpatialEntityComponent* pParentComponent = nullptr;

                    // Try to resolve the ID
                    if ( action.m_parentComponentID.IsValid() )
                    {
						Function<bool(EntityComponent* const &)> predicate = [action] ( EntityComponent* const &pComponent) { return pComponent->GetID() == action.m_parentComponentID; };
						int32 const componentIdx = ListExtensions::IndexOf(m_components, predicate);
                        ENGINE_ASSERT( componentIdx != -1 );

                        pParentComponent = TryCast<SpatialEntityComponent>( m_components[componentIdx] );
                        ENGINE_ASSERT( pParentComponent != nullptr );
                    }

                    auto pComponent = (EntityComponent*) action.m_ptr;
                    AddComponentImmediate( pComponent, pParentComponent );
                    pComponent->Load( loadingContext, ResourceRequesterID( m_ID.m_value ) );
                    m_deferredActions.RemoveAt( i );
                    i--;
                }
                break;

                case EntityInternalStateAction::Type::DestroyComponent:
                {
                    auto pComponentToDestroy = (EntityComponent*) action.m_ptr;

                    // Remove spatial components from the hierarchy immediately
                    SpatialEntityComponent* pSpatialComponent = TryCast<SpatialEntityComponent>( pComponentToDestroy );
                    if ( pSpatialComponent != nullptr )
                    {
                        RemoveComponentFromSpatialHierarchy( pSpatialComponent );
                    }

                    // Unregister from local systems
                    if ( pComponentToDestroy->m_isRegisteredWithEntity )
                    {
                        UnregisterComponentFromLocalSystems( pComponentToDestroy );
                    }

                    // Request unregister from global systems
                    if ( pComponentToDestroy->m_isRegisteredWithWorld )
                    {
                        initializationContext.m_componentsToUnregister.enqueue( EntityModel::EntityComponentPair( this, pComponentToDestroy ) );
                        action.m_type = EntityInternalStateAction::Type::WaitForComponentUnregistration;
                    }
                }
                // No Break: Intentional Fall-through here

                case EntityInternalStateAction::Type::WaitForComponentUnregistration:
                {
                    auto pComponentToDestroy = (EntityComponent*) action.m_ptr;

                    // We can destroy the component safely
                    if ( !pComponentToDestroy->m_isRegisteredWithEntity && !pComponentToDestroy->m_isRegisteredWithWorld )
                    {
                        auto pComponent = (EntityComponent*) action.m_ptr;
                        if ( pComponent->IsInitialized() )
                        {
                            pComponent->Shutdown();
                        }

                        pComponent->Unload( loadingContext, ResourceRequesterID( m_ID.m_value ) );
                        DestroyComponentImmediate( pComponent );
                        m_deferredActions.RemoveAt( i );
                        i--;
                    }
                }
                break;

                default:
                ENGINE_UNREACHABLE_CODE();
                break;
            }
        }

        //-------------------------------------------------------------------------
        // Component Loading
        //-------------------------------------------------------------------------

        for ( auto pComponent : m_components )
        {
            if ( pComponent->IsLoading() )
            {
                pComponent->UpdateLoading();

                // If we are still loading, return
                if ( pComponent->IsLoading() )
                {
                    return false;
                }
            }

            // Once we finish loading a component immediately initialize it
            if ( pComponent->IsLoaded() )
            {
                pComponent->Initialize();
                ENGINE_ASSERT( pComponent->IsInitialized() ); // Did you forget to call the parent class initialize?

                if ( auto pSpatialComponent = TryCast<SpatialEntityComponent>( pComponent ) )
                {
                    pSpatialComponent->CalculateWorldTransform( false );
                }

                // If we are already initialized, then register with entity systems
                if ( IsInitialized() )
                {
                    RegisterComponentWithLocalSystems( pComponent );
                    initializationContext.m_componentsToRegister.enqueue( EntityModel::EntityComponentPair( this, pComponent ) );
                }
            }
        }

        //-------------------------------------------------------------------------
        // Entity update registration
        //-------------------------------------------------------------------------

        ENGINE_ASSERT( m_updateRegistrationStatus != UpdateRegistrationStatus::QueuedForRegister && m_updateRegistrationStatus != UpdateRegistrationStatus::QueuedForUnregister );

        bool const shouldBeRegisteredForUpdates = IsInitialized() && !m_systems.IsEmpty() && !HasSpatialParent();

        if ( shouldBeRegisteredForUpdates )
        {
            if ( m_updateRegistrationStatus != UpdateRegistrationStatus::Registered )
            {
                initializationContext.m_registerForEntityUpdate.enqueue( this );
                m_updateRegistrationStatus = UpdateRegistrationStatus::QueuedForRegister;
            }
        }
        else // Doesnt require an update
        {
            if ( m_updateRegistrationStatus == UpdateRegistrationStatus::Registered )
            {
                initializationContext.m_unregisterForEntityUpdate.enqueue( this );
                m_updateRegistrationStatus = UpdateRegistrationStatus::QueuedForUnregister;
            }
        }

        //-------------------------------------------------------------------------

        return m_deferredActions.IsEmpty();
    }

    //-------------------------------------------------------------------------
    // Spatial Parent and Spatial Info
    //-------------------------------------------------------------------------

    SpatialEntityComponent* Entity::FindSocketAttachmentComponent( SpatialEntityComponent* pComponentToSearch, StringID socketID ) const
    {
        ENGINE_ASSERT( pComponentToSearch != nullptr );

        if ( pComponentToSearch->HasSocket( socketID ) )
        {
            return pComponentToSearch;
        }

        for ( auto pChildComponent : pComponentToSearch->m_spatialChildren )
        {
            if ( auto pFoundComponent = FindSocketAttachmentComponent( pChildComponent, socketID ) )
            {
                return pFoundComponent;
            }
        }

        return nullptr;
    }

    void Entity::SetSpatialParent( Entity* pParentEntity, StringID attachmentSocketID, SpatialAttachmentRule attachmentRule )
    {
        ENGINE_ASSERT( IsSpatialEntity() );
        ENGINE_ASSERT( pParentEntity != nullptr && pParentEntity->IsSpatialEntity() );
        ENGINE_ASSERT( pParentEntity->m_mapID == m_mapID );

        // Ensure that we have consistent attachment chains. i.e. cannot have an uninitialized entity in the middle of an initialized chain!
        // This can occur when adding entities to a map and immediately setting spatial parents! Either set spatial parents before adding or after entities are initialized!
        if ( IsInitialized() )
        {
            ENGINE_ASSERT( pParentEntity->IsInitialized() );
        }

        Threading::ScopeLock myLock( m_internalStateMutex );

        //-------------------------------------------------------------------------

        if ( HasSpatialParent() )
        {
            ClearSpatialParent();
        }

        //-------------------------------------------------------------------------

        // Check for circular dependencies
        #ifdef SE_DEVELOPMENT
        Entity const* pAncestorEntity = pParentEntity->GetSpatialParent();
        while ( pAncestorEntity != nullptr )
        {
            ENGINE_ASSERT( pAncestorEntity != this );
            pAncestorEntity = pAncestorEntity->GetSpatialParent();
        }
        #endif

        // Set attachment data
        m_pParentSpatialEntity = pParentEntity;
        m_parentAttachmentSocketID = attachmentSocketID;

        // Add myself to the parent's list of attached entities
        {
            Threading::ScopeLock theirLock( pParentEntity->m_internalStateMutex );
            pParentEntity->m_attachedEntities.Add( this );
        }

        //-------------------------------------------------------------------------

        // If we need to keep our current world position intact, calculate the required local transform offset to do so
        if ( attachmentRule == SpatialAttachmentRule::KeepWorldTransform )
        {
            Transform const originalWorldTransform = GetWorldTransform();
            Transform const parentWorldTransform = m_pParentSpatialEntity->GetAttachmentSocketTransform( m_parentAttachmentSocketID );
            Transform const newLocalTransform = Transform::Delta( parentWorldTransform, originalWorldTransform );

            // Directly set the transforms, if we are initialized then the 'CreateSpatialAttachment' function will update the transform hierarchy
            m_pRootSpatialComponent->m_transform = newLocalTransform;
            m_pRootSpatialComponent->m_worldTransform = newLocalTransform;
        }

        //-------------------------------------------------------------------------

        CreateSpatialAttachment();
    }

    void Entity::ClearSpatialParent( SpatialAttachmentRule attachmentRule )
    {
        ENGINE_ASSERT( IsSpatialEntity() );
        ENGINE_ASSERT( HasSpatialParent() );

        Threading::ScopeLock myLock( m_internalStateMutex );

        //-------------------------------------------------------------------------

        if ( m_isSpatialAttachmentCreated )
        {
            DestroySpatialAttachment( attachmentRule );
        }

        // Remove myself from parent's attached entity list
        int iter = m_pParentSpatialEntity->m_attachedEntities.Find(this);
        ENGINE_ASSERT( iter != INVALID_INDEX );
        m_pParentSpatialEntity->m_attachedEntities.RemoveAt(iter);

        // Clear attachment data
        m_parentAttachmentSocketID = StringID();
        m_pParentSpatialEntity = nullptr;
    }

    void Entity::CreateSpatialAttachment()
    {
        ENGINE_ASSERT( IsSpatialEntity() );
        ENGINE_ASSERT( m_pParentSpatialEntity != nullptr && !m_isSpatialAttachmentCreated );

        Threading::ScopeLock const myLock( m_internalStateMutex );
        Threading::ScopeLock const parentLock( m_pParentSpatialEntity->m_internalStateMutex );

        // Find component to attach to
        //-------------------------------------------------------------------------

        auto pParentEntity = m_pParentSpatialEntity;
        SpatialEntityComponent* pParentRootComponent = pParentEntity->m_pRootSpatialComponent;
        if ( m_parentAttachmentSocketID.IsValid() )
        {
            if ( auto pFoundComponent = pParentEntity->FindSocketAttachmentComponent( m_parentAttachmentSocketID ) )
            {
                pParentRootComponent = pFoundComponent;
            }
            else
            {
                LOG_ENTITY_WARNING_FORMAT( this, "Could not find attachment socket {0}' on entity '{1}' ({2})",
                                    m_parentAttachmentSocketID.ToString(), pParentEntity->GetNameID().ToString(), pParentEntity->GetID().m_value);
            }
        }

        // Perform attachment
        //-------------------------------------------------------------------------

        ENGINE_ASSERT( pParentRootComponent != nullptr );

        // Set component hierarchy values
        m_pRootSpatialComponent->m_pSpatialParent = pParentRootComponent;
        m_pRootSpatialComponent->m_parentAttachmentSocketID = m_parentAttachmentSocketID;
        m_pRootSpatialComponent->CalculateWorldTransform();

        // Add to the list of child components on the component to attach to
        pParentRootComponent->m_spatialChildren.Add( m_pRootSpatialComponent );

        //-------------------------------------------------------------------------

        m_isSpatialAttachmentCreated = true;
    }

    void Entity::DestroySpatialAttachment( SpatialAttachmentRule attachmentRule )
    {
        ENGINE_ASSERT( IsSpatialEntity() );
        ENGINE_ASSERT( m_pParentSpatialEntity != nullptr && m_isSpatialAttachmentCreated );

        Threading::ScopeLock const myLock( m_internalStateMutex );
        Threading::ScopeLock const parentLock( m_pParentSpatialEntity->m_internalStateMutex );

        // Remove from parent component child list
        //-------------------------------------------------------------------------

        Transform const originalWorldTransform = m_pRootSpatialComponent->GetWorldTransform();

        auto pParentComponent = m_pRootSpatialComponent->m_pSpatialParent;
        auto foundIter = pParentComponent->m_spatialChildren.Find(m_pRootSpatialComponent );
        ENGINE_ASSERT( foundIter != INVALID_INDEX );
        pParentComponent->m_spatialChildren.RemoveAt( foundIter );

        // Remove component hierarchy values
        m_pRootSpatialComponent->m_pSpatialParent = nullptr;
        m_pRootSpatialComponent->m_parentAttachmentSocketID = StringID();

        // Keep world position intact
        if ( attachmentRule == SpatialAttachmentRule::KeepWorldTransform )
        {
            m_pRootSpatialComponent->SetLocalTransform( originalWorldTransform );
        }

        //-------------------------------------------------------------------------

        m_isSpatialAttachmentCreated = false;
    }

    void Entity::RefreshChildSpatialAttachments()
    {
        ENGINE_ASSERT( IsSpatialEntity() );
        for ( auto pAttachedEntity: m_attachedEntities )
        {
            // Only refresh active attachments
            if ( pAttachedEntity->m_isSpatialAttachmentCreated )
            {
                pAttachedEntity->DestroySpatialAttachment( SpatialAttachmentRule::KeepLocalTranform );
                pAttachedEntity->CreateSpatialAttachment();
            }
        }
    }

    bool Entity::IsSpatialChildOf( Entity const* pPotentialParent ) const
    {
        ENGINE_ASSERT( pPotentialParent != nullptr );

        auto pActualParent = m_pParentSpatialEntity;
        while ( pActualParent != nullptr )
        {
            if ( pActualParent == pPotentialParent )
            {
                return true;
            }

            pActualParent = pActualParent->m_pParentSpatialEntity;
        }

        return false;
    }

    AABB Entity::GetCombinedWorldBounds() const
    {
        ENGINE_ASSERT( IsSpatialEntity() );

        AABB combinedBounds = m_pRootSpatialComponent->GetWorldBounds().GetAABB();

        for ( auto pComponent : m_components )
        {
            if ( pComponent == m_pRootSpatialComponent )
            {
                continue;
            }

            if ( auto pSC = TryCast<SpatialEntityComponent>( pComponent ) )
            {
                ENGINE_ASSERT( pSC->GetWorldBounds().IsValid() );
                AABB const componentBounds = pSC->GetWorldBounds().GetAABB();
                combinedBounds = AABB::GetCombinedBox( combinedBounds, componentBounds );
            }
        }

        return combinedBounds;
    }

    //-------------------------------------------------------------------------
    // Systems
    //-------------------------------------------------------------------------

    void Entity::GenerateSystemUpdateList()
    {
        Threading::ScopeLock lock( m_internalStateMutex );

        /*for ( int8_t i = 0; i < (int8_t) UpdateStage::NumStages; i++ )
        {
            m_systemUpdateLists[i].Clear();

            for ( auto& pSystem : m_systems )
            {
                if ( pSystem->GetRequiredUpdatePriorities().IsStageEnabled( (UpdateStage) i ) )
                {
                    m_systemUpdateLists[i].Add( pSystem );
                }
            }

            // Sort update list
            Function<bool(EntitySystem* const& , EntitySystem* const&)> comparator = [i] ( EntitySystem* const& pSystemA, EntitySystem* const& pSystemB )
            {
                uint8 const A = pSystemA->GetRequiredUpdatePriorities().GetPriorityForStage( (UpdateStage) i );
                uint8 const B = pSystemB->GetRequiredUpdatePriorities().GetPriorityForStage( (UpdateStage) i );
                return A > B;
            };

             Sorting::QuickSort(m_systemUpdateLists[i], comparator);
        }*/
    }

    void Entity::CreateSystemImmediate( TypeInfo const* pSystemTypeInfo )
    {
        ENGINE_ASSERT( pSystemTypeInfo != nullptr && pSystemTypeInfo->IsDerivedFrom<EntitySystem>() );

        Threading::ScopeLock lock( m_internalStateMutex );

        #ifdef SE_DEVELOPMENT
        // Ensure that we only allow a single system of a specific family
        for ( auto pExistingSystem : m_systems )
        {
            auto const pExistingSystemTypeInfo = pExistingSystem->GetTypeInfo();
            if ( pSystemTypeInfo->IsDerivedFrom( pExistingSystemTypeInfo->id ) || pExistingSystemTypeInfo->IsDerivedFrom( pSystemTypeInfo->id ) )
            {
                ENGINE_HALT();
            }
        }
        #endif

        // Create the new system and add it
        auto pSystem = (EntitySystem*) pSystemTypeInfo->CreateType();
        m_systems.Add( pSystem );

        // If the entity is already initialized, then initialize the system
        if ( IsInitialized() )
        {
            pSystem->Initialize();

            // Register all components with the new system
            for ( auto pComponent : m_components )
            {
                if ( pComponent->m_isRegisteredWithEntity )
                {
                    pSystem->RegisterComponent( pComponent );
                }
            }

            if ( IsInitialized() )
            {
                pSystem->PostComponentRegister();
            }
        }
    }

    void Entity::CreateSystem( TypeInfo const* pSystemTypeInfo )
    {
        ENGINE_ASSERT( pSystemTypeInfo != nullptr );
        ENGINE_ASSERT( pSystemTypeInfo->IsDerivedFrom( EntitySystem::GetStaticTypeID() ) );

        Threading::ScopeLock lock( m_internalStateMutex );

        if ( IsUnloaded() )
        {
            CreateSystemImmediate( pSystemTypeInfo );
        }
        else
        {
            auto& action = m_deferredActions.AddOne();
            action.m_type = EntityInternalStateAction::Type::CreateSystem;
            action.m_ptr = pSystemTypeInfo;
            s_entityInternalStateUpdatedEvent.Execute( this );
        }
    }

    void Entity::DestroySystem( TypeInfo const* pSystemTypeInfo )
    {
        ENGINE_ASSERT( pSystemTypeInfo != nullptr );
        ENGINE_ASSERT( pSystemTypeInfo->IsDerivedFrom( EntitySystem::GetStaticTypeID() ) );

        Threading::ScopeLock lock( m_internalStateMutex );

        if ( IsUnloaded() )
        {
            DestroySystemImmediate( pSystemTypeInfo );
        }
        else
        {
            auto& action = m_deferredActions.AddOne();
            action.m_type = EntityInternalStateAction::Type::DestroySystem;
            action.m_ptr = pSystemTypeInfo;
            s_entityInternalStateUpdatedEvent.Execute( this );
        }
    }

    void Entity::DestroySystem( TypeID systemTypeID )
    {
        TypeInfo const* pSystemTypeInfo = nullptr;
        for ( auto pSystem : m_systems )
        {
            if ( pSystem->GetTypeID() == systemTypeID )
            {
                pSystemTypeInfo = pSystem->GetTypeInfo();
                break;
            }
        }

        DestroySystem( pSystemTypeInfo );
    }

    void Entity::DestroySystemImmediate( TypeInfo const* pSystemTypeInfo )
    {
        ENGINE_ASSERT( pSystemTypeInfo != nullptr && pSystemTypeInfo->IsDerivedFrom<EntitySystem>() );

        Threading::ScopeLock lock( m_internalStateMutex );

		Function<bool(EntitySystem* const &)> predicate = [pSystemTypeInfo] ( EntitySystem* const &pSystem) { return pSystem->GetTypeInfo()->id == pSystemTypeInfo->id; };
		int32 const systemIdx = ListExtensions::IndexOf( m_systems, predicate);
        ENGINE_ASSERT( systemIdx != -1 );
        auto pSystem = m_systems[systemIdx];

        if ( IsInitialized() )
        {
            pSystem->PreComponentUnregister();

            // Unregister all components from the system to remove
            for ( auto pComponent : m_components )
            {
                if ( pComponent->m_isRegisteredWithEntity )
                {
                    pSystem->UnregisterComponent( pComponent );
                }
            }

            pSystem->Shutdown();
        }

        // Destroy the system
        Delete( pSystem );
        m_systems.RemoveAt(systemIdx);
    }
    
    //-------------------------------------------------------------------------
    // Components
    //-------------------------------------------------------------------------

    void Entity::CreateComponent( TypeInfo const* pComponentTypeInfo, ComponentID const& parentSpatialComponentID )
    {
        ENGINE_ASSERT( pComponentTypeInfo != nullptr && pComponentTypeInfo->IsDerivedFrom<EntityComponent>() );
        EntityComponent* pComponent = Cast<EntityComponent>( pComponentTypeInfo->CreateType() );

        #ifdef SE_DEVELOPMENT
        pComponent->m_name = StringID( pComponentTypeInfo->GetFriendlyTypeName() );
        #else
        pComponent->m_name = StringID( pComponentTypeInfo->GetTypeName() );
        #endif

        AddComponent( pComponent, parentSpatialComponentID );
    }

    void Entity::AddComponent( EntityComponent* pComponent, ComponentID const& parentSpatialComponentID )
    {
        ENGINE_ASSERT( pComponent != nullptr && pComponent->GetID().IsValid() );
        ENGINE_ASSERT( !pComponent->m_entityID.IsValid() && pComponent->IsUnloaded() );
        ENGINE_ASSERT( !m_components.Contains((EntityComponent*) pComponent ) );

        //-------------------------------------------------------------------------

        Threading::ScopeLock lock( m_internalStateMutex );

        //-------------------------------------------------------------------------

        SpatialEntityComponent* pSpatialComponent = TryCast<SpatialEntityComponent>( pComponent );
        
        // Parent ID can only be set when adding a spatial component
        if( pSpatialComponent == nullptr )
        {
            ENGINE_ASSERT( !parentSpatialComponentID.IsValid() );
        }

        //-------------------------------------------------------------------------

        #ifdef SE_DEVELOPMENT
        if ( pComponent->IsSingletonComponent() )
        {
            // Ensure we have no components that are derived from or are a parent of the component
            for ( auto pExistingComponent : m_components )
            {
                ENGINE_ASSERT( !pComponent->GetTypeInfo()->IsDerivedFrom( pExistingComponent->GetTypeID() ) );
                ENGINE_ASSERT( !pExistingComponent->GetTypeInfo()->IsDerivedFrom( pComponent->GetTypeID() ) );
            }
        }

        // Ensure unique name
        pComponent->m_name = GenerateUniqueComponentNameID( pComponent, pComponent->m_name );
        #endif

        //-------------------------------------------------------------------------

        if ( IsUnloaded() )
        {
            SpatialEntityComponent* pParentComponent = nullptr;
            if ( parentSpatialComponentID.IsValid() )
            {
                ENGINE_ASSERT( pSpatialComponent != nullptr );
				Function<bool(EntityComponent* const &)> predicate = [parentSpatialComponentID] ( EntityComponent* const &pComponent) { return pComponent->GetID() == parentSpatialComponentID; };
				int32 const componentIdx = ListExtensions::IndexOf( m_components, predicate);
                ENGINE_ASSERT( componentIdx != INVALID_INDEX );

                pParentComponent = TryCast<SpatialEntityComponent>( m_components[componentIdx] );
                ENGINE_ASSERT( pParentComponent != nullptr );
            }

            AddComponentImmediate( pComponent, pParentComponent );
        }
        else // Defer the operation to the next loading phase
        {
            EntityInternalStateAction& action = m_deferredActions.AddOne();
            action.m_type = EntityInternalStateAction::Type::AddComponent;
            action.m_ptr = pComponent;
            action.m_parentComponentID = parentSpatialComponentID;
            s_entityInternalStateUpdatedEvent.Execute( this );
        }
    }

    void Entity::DestroyComponent( ComponentID const& componentID )
    {
        Function<bool(EntityComponent* const &)> predicate = [componentID] ( EntityComponent* const &pComponent) { return pComponent->GetID() == componentID; };
		int32 const componentIdx = ListExtensions::IndexOf( m_components, predicate);

        // If you hit this assert either the component doesnt exist or this entity has still to process deferred actions and the component might be in that list
        // We dont support adding and destroying a component in the same frame, please avoid doing stupid things
        ENGINE_ASSERT( componentIdx != INVALID_INDEX );
        auto pComponent = m_components[componentIdx];

        //-------------------------------------------------------------------------

        Threading::ScopeLock lock( m_internalStateMutex );

        if ( IsUnloaded() )
        {
            // Root removal validation
            if ( pComponent == m_pRootSpatialComponent )
            {
                // You are only allowed to remove the root component if it has a single child or no children. Otherwise we cant fix the spatial hierarchy for this entity.
                ENGINE_ASSERT( m_pRootSpatialComponent->m_spatialChildren.Count() <= 1 );
            }

            DestroyComponentImmediate( pComponent );
        }
        else // Defer the operation to the next loading phase
        {
            auto& action = m_deferredActions.AddOne();
            action.m_type = EntityInternalStateAction::Type::DestroyComponent;
            action.m_ptr = pComponent;
            ENGINE_ASSERT( action.m_ptr != nullptr );
            s_entityInternalStateUpdatedEvent.Execute( this );
        }
    }

    void Entity::AddComponentImmediate( EntityComponent* pComponent, SpatialEntityComponent* pParentSpatialComponent )
    {
        Threading::ScopeLock lock( m_internalStateMutex );
        ENGINE_ASSERT( pComponent != nullptr && pComponent->GetID().IsValid() && !pComponent->m_entityID.IsValid() );
        ENGINE_ASSERT( pComponent->IsUnloaded() && !pComponent->m_isRegisteredWithWorld && !pComponent->m_isRegisteredWithEntity );

        // Update spatial hierarchy
        //-------------------------------------------------------------------------

        SpatialEntityComponent* pSpatialEntityComponent = TryCast<SpatialEntityComponent>( pComponent );
        if ( pSpatialEntityComponent != nullptr )
        {
            // If the parent component is null, attach it to the root (if one exists) by default
            if ( pParentSpatialComponent == nullptr )
            {
                pParentSpatialComponent = m_pRootSpatialComponent;
            }

            // If we have a parent component, update the hierarchy and transform
            if ( pParentSpatialComponent != nullptr )
            {
                pSpatialEntityComponent->m_pSpatialParent = pParentSpatialComponent;
                pParentSpatialComponent->m_spatialChildren.Add( pSpatialEntityComponent );
                pSpatialEntityComponent->CalculateWorldTransform( false );
            }
            else // Make this component the root component
            {
                m_pRootSpatialComponent = pSpatialEntityComponent;
                ENGINE_ASSERT( pSpatialEntityComponent->m_pSpatialParent == nullptr );

                if ( HasSpatialParent() )
                {
                    CreateSpatialAttachment();
                }
            }
        }

        // Add component
        //-------------------------------------------------------------------------

        m_components.Add( pComponent );
        pComponent->m_entityID = m_ID;
    }

    void Entity::DestroyComponentImmediate( EntityComponent* pComponent )
    {
        Threading::ScopeLock lock( m_internalStateMutex );
        ENGINE_ASSERT( pComponent != nullptr );
        ENGINE_ASSERT( pComponent->IsUnloaded() && !pComponent->m_isRegisteredWithWorld && !pComponent->m_isRegisteredWithEntity );

        int32 const componentIdx = m_components.Find(pComponent);
        ENGINE_ASSERT( componentIdx != -1 );

        // Update spatial hierarchy
        //-------------------------------------------------------------------------

        SpatialEntityComponent* pSpatialComponent = TryCast<SpatialEntityComponent>( pComponent );
        if ( pSpatialComponent != nullptr )
        {
            RemoveComponentFromSpatialHierarchy( pSpatialComponent );
        }

        // Remove component
        //-------------------------------------------------------------------------

        m_components.RemoveAt(componentIdx);
        Delete( pComponent );
    }

    void Entity::RemoveComponentFromSpatialHierarchy( SpatialEntityComponent* pSpatialComponent )
    {
        Threading::ScopeLock lock( m_internalStateMutex );
        ENGINE_ASSERT( pSpatialComponent != nullptr );

        if ( pSpatialComponent == m_pRootSpatialComponent )
        {
            int32 const numChildrenForRoot = (int32) m_pRootSpatialComponent->m_spatialChildren.Count();

            // Ensure that we break any spatial attachments before we mess with the root component
            bool const recreateSpatialAttachment = m_isSpatialAttachmentCreated;
            if ( HasSpatialParent() && m_isSpatialAttachmentCreated )
            {
                DestroySpatialAttachment( SpatialAttachmentRule::KeepLocalTranform );
            }

            //-------------------------------------------------------------------------

            // Removal of the root component if it has more than one child is not supported!
            if ( numChildrenForRoot > 1 )
            {
                ENGINE_HALT();
            }
            else if ( numChildrenForRoot == 1 )
            {
                // Replace the root component of this entity with the child of the current root component that we are removing
                pSpatialComponent->m_spatialChildren[0]->m_pSpatialParent = nullptr;
                m_pRootSpatialComponent = pSpatialComponent->m_spatialChildren[0];
                pSpatialComponent->m_spatialChildren.Clear();
            }
            else // No children, so completely remove the root component
            {
                m_pRootSpatialComponent = nullptr;

                // We are no longer a spatial entity so break any attachments
                if ( HasSpatialParent() )
                {
                    ClearSpatialParent();
                }
            }

            //-------------------------------------------------------------------------

            // Should we recreate the spatial attachment
            if ( HasSpatialParent() && recreateSpatialAttachment )
            {
                CreateSpatialAttachment();
            }
        }
        else // 'Fix' spatial hierarchy parenting
        {
            auto pParentComponent = pSpatialComponent->m_pSpatialParent;
            if ( pParentComponent != nullptr ) // Component may already have been removed from the hierarchy due to deferred destruction
            {
                // Remove from parent
				int findIndex = pParentComponent->m_spatialChildren.FindFirst(pSpatialComponent);

				if (findIndex != INVALID_INDEX)
				{
					pParentComponent->m_spatialChildren.RemoveAt(findIndex);
				}

                // Reparent all children to parent component
                for ( auto pChildComponent : pSpatialComponent->m_spatialChildren )
                {
                    pChildComponent->m_pSpatialParent = pParentComponent;
                    pParentComponent->m_spatialChildren.Add( pChildComponent );
                }
            }
            else
            {
                ENGINE_ASSERT( pSpatialComponent->m_spatialChildren.IsEmpty() );
            }
        }
    }

    //-------------------------------------------------------------------------
    // Tool Functions
    //-------------------------------------------------------------------------

    #ifdef SE_DEVELOPMENT
    StringID Entity::GenerateUniqueComponentNameID( EntityComponent* pComponent, StringID desiredNameID ) const
    {
        if ( !desiredNameID.IsValid() )
        {
            desiredNameID = pComponent->GetTypeID().ToStringID();
        }

        //-------------------------------------------------------------------------

        StringView desiredName = desiredNameID.ToString();
        String finalName = desiredName;

        uint32_t counter = 0;
        bool isUniqueName = false;
        while ( !isUniqueName )
        {
            isUniqueName = true;

            for ( auto pExistingComponent : m_components )
            {
                if ( pExistingComponent == pComponent )
                {
                    continue;
                }

                if (StringUtils::Compare(finalName.Get(), pExistingComponent->GetNameID().ToString().Get()) == 0 )
                {
                    isUniqueName = false;
                    break;
                }
            }

            if ( !isUniqueName )
            {
                // Check if the last three characters are a numeric set, if so then increment the value and replace them
                if ( desiredName.Length() > 3 &&
					isdigit( desiredName[desiredName.Length() - 1] ) &&
					isdigit( desiredName[desiredName.Length() - 2] ) &&
					isdigit( desiredName[desiredName.Length() - 3] ) )
                {
                    finalName = String::Format(SE_TEXT("{0}{1}"), desiredName.Substring(0, desiredName.Length() - 3).Get(), counter);
                }
                else // Default name generation
                {
                    finalName = String::Format(SE_TEXT("{0} {1}"), desiredName.Get(), counter);
                }

                counter++;
            }
        }

        //-------------------------------------------------------------------------

        return StringID( finalName.Get() );
    }

    void Entity::RenameComponent( EntityComponent* pComponent, StringID newNameID )
    {
        ENGINE_ASSERT( pComponent != nullptr && pComponent->m_entityID == m_ID );
        pComponent->m_name = GenerateUniqueComponentNameID( pComponent, newNameID );
    }
    #endif
}
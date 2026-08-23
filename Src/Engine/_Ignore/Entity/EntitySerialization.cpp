#include "EntitySerialization.h"
#include "Entity.h"
#include "EntityDescriptors.h"
#include "EntityLog.h"
#include "EntitySystem.h"
#include "EntityMap.h"

#include "Core/TypeSystem/Types.h"
#include "Core/Profiler/Profiler.h"
#include "Core/Types/Collections/Sorting.h"


//-------------------------------------------------------------------------

namespace SE::EntityModel
{
    Entity* Serializer::CreateEntity(SerializedEntityDescriptor const& entityDesc )
    {
        ENGINE_ASSERT( entityDesc.IsValid() );

        auto pEntityTypeInfo = Entity::s_pTypeInfo;
        ENGINE_ASSERT( pEntityTypeInfo != nullptr );

        // Create new entity
        //-------------------------------------------------------------------------

        auto pEntity = reinterpret_cast<Entity*>( pEntityTypeInfo->CreateType() );
        pEntity->m_name = entityDesc.m_name;

        #ifdef SE_DEVELOPMENT
        // Restore entity ID if valid
        if ( entityDesc.m_transientEntityID.IsValid() )
        {
            pEntity->m_ID = entityDesc.m_transientEntityID;
        }
        #endif

        // Create entity components
        //-------------------------------------------------------------------------
        // Component descriptors are sorted during compilation, spatial components are first, followed by regular components

        for ( EntityModel::SerializedComponentDescriptor const& componentDesc : entityDesc.m_components )
        {
            auto pEntityComponent = componentDesc.CreateTypeInstance<EntityComponent>();
            ENGINE_ASSERT( pEntityComponent != nullptr );

            TypeInfo const* pTypeInfo = pEntityComponent->GetTypeInfo();
            ENGINE_ASSERT( pTypeInfo != nullptr );

            // Set IDs and add to component lists
            pEntityComponent->m_name = componentDesc.m_name;
            pEntityComponent->m_entityID = pEntity->m_ID;
            pEntity->m_components.Add( pEntityComponent );

            #ifdef SE_DEVELOPMENT
            // Restore component ID if valid
            if ( componentDesc.m_transientComponentID.IsValid() )
            {
                pEntityComponent->m_ID = componentDesc.m_transientComponentID;
            }
            #endif

            //-------------------------------------------------------------------------

            if ( componentDesc.IsSpatialComponent() )
            {
                // Set parent socket ID
                auto pSpatialEntityComponent = reinterpret_cast<SpatialEntityComponent*>( pEntityComponent );
                pSpatialEntityComponent->m_parentAttachmentSocketID = componentDesc.m_attachmentSocketID;

                // Set as root component
                if ( componentDesc.IsRootComponent() )
                {
                    ENGINE_ASSERT( pEntity->m_pRootSpatialComponent == nullptr );
                    pEntity->m_pRootSpatialComponent = pSpatialEntityComponent;
                }
            }
        }

        // Create component spatial hierarchy
        //-------------------------------------------------------------------------

        for ( int32_t spatialComponentIdx = 0; spatialComponentIdx < entityDesc.m_numSpatialComponents; spatialComponentIdx++ )
        {
            EntityModel::SerializedComponentDescriptor const& spatialComponentDesc = entityDesc.m_components[spatialComponentIdx];
            ENGINE_ASSERT( spatialComponentDesc.IsSpatialComponent() );

            // Skip the root component
            if ( spatialComponentDesc.IsRootComponent() )
            {
                ENGINE_ASSERT( pEntity->GetRootSpatialComponent()->GetNameID() == spatialComponentDesc.m_name );
                continue;
            }

            // Todo: profile this lookup and if it becomes too costly, pre-compute the parent indices and serialize them
            int32_t const parentComponentIdx = entityDesc.FindComponentIndex( spatialComponentDesc.m_spatialParentName );
            ENGINE_ASSERT( parentComponentIdx != -1 );

            auto pParentSpatialComponent = reinterpret_cast<SpatialEntityComponent*>( pEntity->m_components[parentComponentIdx] );
            if ( spatialComponentDesc.m_spatialParentName == pParentSpatialComponent->GetNameID() )
            {
                auto pSpatialComponent = reinterpret_cast<SpatialEntityComponent*>( pEntity->m_components[spatialComponentIdx] );
                pSpatialComponent->m_pSpatialParent = pParentSpatialComponent;

                pParentSpatialComponent->m_spatialChildren.Add( pSpatialComponent );
            }
        }

        // Ensure that all world transforms are up to date!
        //-------------------------------------------------------------------------

        if ( pEntity->IsSpatialEntity() )
        {
            pEntity->GetRootSpatialComponent()->CalculateWorldTransform( false );
        }

        // Create entity systems
        //-------------------------------------------------------------------------

        for ( auto const& systemDesc : entityDesc.m_systems )
        {
            TypeInfo const* pTypeInfo = Types::GetTypeInfo( systemDesc.m_typeID);
            auto pEntitySystem = reinterpret_cast<EntitySystem*>( pTypeInfo->CreateType() );
            ENGINE_ASSERT( pEntitySystem != nullptr );

            pEntity->m_systems.Add( pEntitySystem );
        }

        // Add to collection
        //-------------------------------------------------------------------------

        return pEntity;
    }

	List<Entity*> Serializer::CreateEntities(SerializedEntityCollection const& entityCollection )
    {
        PROFILE_CPU_NAMED( "Instantiate Entity Collection" );

        int32_t const numEntitiesToCreate = (int32_t) entityCollection.m_entityDescriptors.Count();
        List<Entity*> createdEntities;
        createdEntities.Resize( numEntitiesToCreate );

        //-------------------------------------------------------------------------

        // For small number of entities, just create them inline!
        if (numEntitiesToCreate <= 5 )
        {
            for ( auto i = 0; i < numEntitiesToCreate; i++ )
            {
                createdEntities[i] = CreateEntity(entityCollection.m_entityDescriptors[i] );
            }
        }
        else // Go wide and create all entities in parallel
        {
/*            struct EntityCreationTask : public ITaskSet
            {
                EntityCreationTask(List<SerializedEntityDescriptor> const& descriptors, List<Entity*>& createdEntities )
                    : m_descriptors( descriptors )
                    , m_createdEntities( createdEntities )
                {
                    m_SetSize = (uint32) descriptors.Count();
                    m_MinRange = 10;
                }

                virtual void ExecuteRange( TaskSetPartition range, uint32 threadnum ) override final
                {
					PROFILE_CPU_NAMED( "Entity Creation Task" );
                    for ( uint64_t i = range.start; i < range.end; ++i )
                    {
                        m_createdEntities[i] = CreateEntity( m_descriptors[i] );
                    }
                }

            private:

                List<SerializedEntityDescriptor> const&          m_descriptors;
                List<Entity*>&                                   m_createdEntities;
            };

            //-------------------------------------------------------------------------

            // Create all entities in parallel
            EntityCreationTask updateTask(entityCollection.m_entityDescriptors, createdEntities );
            pTaskSystem->ScheduleTask( &updateTask );
            pTaskSystem->WaitForTask( &updateTask );*/
        }

        // Resolve spatial connections
        //-------------------------------------------------------------------------

        {
			PROFILE_CPU_NAMED( "Resolve spatial connections" );

            for ( auto const& entityAttachmentInfo : entityCollection.m_entitySpatialAttachmentInfo )
            {
                ENGINE_ASSERT( entityAttachmentInfo.m_entityIdx != -1 && entityAttachmentInfo.m_parentEntityIdx != -1 );

                auto const& entityDesc = entityCollection.m_entityDescriptors[entityAttachmentInfo.m_entityIdx];
                ENGINE_ASSERT( entityDesc.HasSpatialParent() );

                // The entity collection compiler will guaranteed that entities are always sorted so that parents are created/initialized before their attached entities
                ENGINE_ASSERT( entityAttachmentInfo.m_parentEntityIdx < entityAttachmentInfo.m_entityIdx );

                //-------------------------------------------------------------------------

                Entity* pEntity = createdEntities[entityAttachmentInfo.m_entityIdx];
                ENGINE_ASSERT( pEntity->IsSpatialEntity() );

                Entity* pParentEntity = createdEntities[entityAttachmentInfo.m_parentEntityIdx];
                ENGINE_ASSERT( pParentEntity->IsSpatialEntity() );

                pEntity->SetSpatialParent( pParentEntity, entityDesc.m_attachmentSocketID, Entity::SpatialAttachmentRule::KeepLocalTranform );
            }
        }

        //-------------------------------------------------------------------------

        return createdEntities;
    }

    //-------------------------------------------------------------------------

    #ifdef SE_DEVELOPMENT
    bool Serializer::SerializeEntity(Entity const* pEntity, EntityModel::SerializedEntityDescriptor& outDesc )
    {
        ENGINE_ASSERT( !outDesc.IsValid() );
        outDesc.m_name = pEntity->m_name;

        #ifdef SE_DEVELOPMENT
        outDesc.m_transientEntityID = pEntity->m_ID;
        #endif

        // Get spatial parent
        if ( pEntity->HasSpatialParent() )
        {
            outDesc.m_spatialParentName = pEntity->m_pParentSpatialEntity->GetNameID();
            outDesc.m_attachmentSocketID = pEntity->GetAttachmentSocketID();
        }

        // Components
        //-------------------------------------------------------------------------

        Function<bool(EntityComponent * const&, EntityComponent * const& )> ComponentComparator = [] ( EntityComponent * const&pA, EntityComponent * const&pB )
        {
            auto const pSpatialA = TryCast<SpatialEntityComponent>( pA );
            auto const pSpatialB = TryCast<SpatialEntityComponent>( pB );

            // If both are spatial then provide some arbitrary sort
            if ( pSpatialA != nullptr && pSpatialB != nullptr )
            {
                if ( pSpatialA->IsRootComponent() )
                {
                    return true;
                }
                else if ( pSpatialB->IsRootComponent() )
                {
                    return false;
                }

                return (uintptr_t) pA < (uintptr_t) pB;
            }
            // If neither are spatial then provide some arbitrary sort
            else if ( pSpatialA == nullptr && pSpatialB == nullptr )
            {
                return (uintptr_t) pA < (uintptr_t) pB;
            }
            else // Only one is a spatial component
            {
                return pSpatialA != nullptr;
            }
        };

        List<EntityComponent*> sortedComponents(pEntity->m_components);
        Sorting::QuickSort(sortedComponents, ComponentComparator);

        //-------------------------------------------------------------------------

        List<StringID> entityComponentList;
        for ( auto pComponent : sortedComponents )
        {
            EntityModel::SerializedComponentDescriptor componentDesc;
            componentDesc.m_name = pComponent->GetNameID();

            #ifdef SE_DEVELOPMENT
            componentDesc.m_transientComponentID = pComponent->m_ID;
            #endif

            // Check for unique names
            if ( entityComponentList.Contains(componentDesc.m_name ) )
            {
                // Duplicate name detected!!
                LOG_ENTITY_ERROR_FORMAT( pEntity, "Failed to create entity descriptor, duplicate component name detected: {0} on entity {1}",
					pComponent->GetNameID().ToString(), pEntity->GetNameID().ToString());
                return false;
            }
            else
            {
                entityComponentList.Add( componentDesc.m_name );
            }

            // Spatial info
            auto pSpatialEntityComponent = TryCast<SpatialEntityComponent>( pComponent );
            if ( pSpatialEntityComponent != nullptr )
            {
                if ( !pSpatialEntityComponent->IsRootComponent() )
                {
                    EntityComponent const* pSpatialParentComponent = pEntity->FindComponent( pSpatialEntityComponent->GetSpatialParentID() );
                    componentDesc.m_spatialParentName = pSpatialParentComponent->GetNameID();
                    componentDesc.m_attachmentSocketID = pSpatialEntityComponent->GetAttachmentSocketID();
                }

                componentDesc.m_isSpatialComponent = true;
            }

            // Type descriptor - Properties
            componentDesc.DescribeTypeInstance( pComponent, true );

            // Add component
            outDesc.m_components.Add( componentDesc );
            if ( componentDesc.m_isSpatialComponent )
            {
                outDesc.m_numSpatialComponents++;
            }
        }

        // Systems
        //-------------------------------------------------------------------------

        for ( EntitySystem const* pSystem : pEntity->m_systems )
        {
            EntityModel::SerializedSystemDescriptor systemDesc;
            systemDesc.m_typeID = pSystem->GetTypeID();
            outDesc.m_systems.Add( systemDesc );
        }

        return true;
    }

    bool Serializer::SerializeEntityMap(EntityMap const* pMap, SerializedEntityCollection& outCollection )
    {
        ENGINE_ASSERT( pMap->IsLoaded() );
        ENGINE_ASSERT( pMap->m_entitiesToLoad.IsEmpty() && pMap->m_entitiesToRemove.IsEmpty() );
        ENGINE_ASSERT( pMap->m_entitiesToHotReload.IsEmpty() );

        Dictionary<StringID, StringID> entityNameMap;
        entityNameMap.SetCapacity( pMap->GetNumEntities() );

        List<SerializedEntityDescriptor> entityDescs;
        entityDescs.Resize( pMap->GetNumEntities() );

        for ( auto pEntity : pMap->GetEntities() )
        {
            // Check for unique names - This should never happen but we're paranoid so let's keep the extra validation
            if ( entityNameMap.Find( pEntity->GetNameID() ) != entityNameMap.end() )
            {
                LOG_ENTITY_ERROR_FORMAT( pEntity, "Entity Failed to create entity collection descriptor, duplicate entity name found: {0}",
					pEntity->GetNameID().ToString());
                return false;
            }
            else
            {
                entityNameMap.Add(pEntity->GetNameID(), pEntity->GetNameID());
            }

            //-------------------------------------------------------------------------

            SerializedEntityDescriptor entityDesc;
            if ( !SerializeEntity(pEntity, entityDesc ) )
            {
                return false;
            }

            entityDescs.Add( entityDesc );
        }

        //-------------------------------------------------------------------------

        outCollection.SetCollectionData( std::move( entityDescs ) );
        return true;
    }
    #endif
}
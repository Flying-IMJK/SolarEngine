#include "WorldSystem_EntityCollectionSpawner.h"
#include "Runtime/Entity/Entity.h"
#include "Runtime/Entity/EntityWorldUpdateContext.h"
#include "Runtime/Entity/EntityMap.h"

//-------------------------------------------------------------------------

namespace SGE
{
    void EntityCollectionSpawner::ShutdownSystem()
    {
        ENGINE_ASSERT( m_entityCollectionReferences.empty() );
    }

    void EntityCollectionSpawner::RegisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        if ( auto pEntityCollectionComponent = TryCast<EntityCollectionComponent>( pComponent ) )
        {
            m_entityCollectionReferences.Emplace( pEntityCollectionComponent->GetID(), pEntityCollectionComponent );
            m_collectionsToSpawn.Add( pEntityCollectionComponent );
        }
    }

    void EntityCollectionSpawner::UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        if ( auto pEntityCollectionComponent = TryCast<EntityCollectionComponent>( pComponent ) )
        {
            CollectionRecord* pRecord = m_entityCollectionReferences.FindItem( pEntityCollectionComponent->GetID() );
            ENGINE_ASSERT( pRecord != nullptr );
            m_entitiesToDestroy.Add(pRecord->m_createdEntities);

            m_entityCollectionReferences.Remove( pEntityCollectionComponent->GetID() );
            m_collectionsToSpawn.Remove( pEntityCollectionComponent );
        }
    }

    //-------------------------------------------------------------------------

    void EntityCollectionSpawner::UpdateSystem( EntityWorldUpdateContext const& ctx )
    {
        auto pPersistentMap = ctx.GetPersistentMap();

        //-------------------------------------------------------------------------

        if ( !m_collectionsToSpawn.IsEmpty() )
        {
            for ( auto pCollectionToSpawn : m_collectionsToSpawn )
            {
                if ( pCollectionToSpawn->GetEntityCollectionDesc() != nullptr )
                {
                    CollectionRecord* pRecord = m_entityCollectionReferences.FindItem( pCollectionToSpawn->GetID() );
                    ENGINE_ASSERT( pRecord != nullptr );

                    List<Entity*> createdEntities;
                    pPersistentMap->AddEntityCollection( *pCollectionToSpawn->GetEntityCollectionDesc(),
						pCollectionToSpawn->GetWorldTransform(), &createdEntities );

                    for ( auto pCreatedEntity : createdEntities )
                    {
                        pRecord->m_createdEntities.Add( pCreatedEntity->GetID() );
                    }
                }
            }

            m_collectionsToSpawn.Clear();
        }

        //-------------------------------------------------------------------------

        if ( !m_entitiesToDestroy.IsEmpty() )
        {
            for ( auto const& entityID : m_entitiesToDestroy )
            {
                pPersistentMap->DestroyEntity( entityID );
            }

            m_entitiesToDestroy.Clear();
        }
    }
}
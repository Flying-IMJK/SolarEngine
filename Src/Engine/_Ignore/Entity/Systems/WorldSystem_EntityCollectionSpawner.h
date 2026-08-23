#pragma once

#include "Runtime/Entity/Components/Component_EntityCollection.h"
#include "Runtime/Entity/EntityWorldSystem.h"
#include "Runtime/Entity/IDVector.h"

//-------------------------------------------------------------------------

namespace SGE
{
    class SE_API_RUNTIME EntityCollectionSpawner : public EntityWorldSystem
    {
        struct CollectionRecord
        {
			CollectionRecord(): m_pComponent() {}
            CollectionRecord(EntityCollectionComponent* pComponent): m_pComponent(pComponent) {}

            inline ComponentID GetID() const { return m_pComponent->GetID(); }

        public:

            EntityCollectionComponent*           m_pComponent = nullptr;
            List<EntityID>                       m_createdEntities;
        };

    public:

        ENGINE_ENTITY_WORLD_SYSTEM( EntityCollectionSpawner, RequiresUpdate( UpdateStage::PrePhysics ) );

    private:

        virtual void ShutdownSystem() override final;
        virtual void RegisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;
        virtual void UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;
        virtual void UpdateSystem( EntityWorldUpdateContext const& ctx ) override;

    private:

        TIDVector<ComponentID, CollectionRecord>    m_entityCollectionReferences;
		List<EntityCollectionComponent*>            m_collectionsToSpawn;
        List<EntityID>                              m_entitiesToDestroy;
    };
} 
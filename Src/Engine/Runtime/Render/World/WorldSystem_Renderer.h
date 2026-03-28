#pragma once

#include "Runtime/API.h"
#include "Runtime/Entity/EntityWorldSystem.h"
#include "Runtime/Entity/Components/Component_StaticMesh.h"
#include "Runtime/Render/RenderObject//SkeletalMesh.h"
#include "Runtime/Entity/IDVector.h"

#include "Core/Math/AABBTree.h"
#include "Core/Types/Event.h"
#include "Core/Systems.h"


//-------------------------------------------------------------------------
namespace SE
{
	class SkeletalMeshComponent;
	class DirectionalLightComponent;
	class PointLightComponent;
	class SpotLightComponent;
	class GlobalEnvironmentMapComponent;
	class LocalEnvironmentMapComponent;
}


namespace SE::Render
{
    //-------------------------------------------------------------------------

    class SE_API_RUNTIME RendererWorldSystem final : public EntityWorldSystem
    {
        friend class WorldRenderer;
//        friend class RenderDebugView;

    public:

        ENGINE_ENTITY_WORLD_SYSTEM( RendererWorldSystem, RequiresUpdate( UpdateStage::FrameEnd ), RequiresUpdate( UpdateStage::Paused ) );

    private:

        // Track all instances of a given mesh together - to limit the number of vertex buffer changes
        struct SkeletalMeshGroup
        {
			SkeletalMeshGroup() : m_pMesh() {}
            SkeletalMeshGroup(SkeletalMesh const* pInMesh) : m_pMesh( pInMesh ) { ENGINE_ASSERT( pInMesh != nullptr ); }

            inline uint32 GetID() const { return m_pMesh->GetResourceID().GetPathID(); }

        public:

            SkeletalMesh const*                              m_pMesh = nullptr;
            List<SkeletalMeshComponent*>                     m_components;
        };

    private:

        // Entity System
        //-------------------------------------------------------------------------

        virtual void InitializeSystem( Systems const& systemRegistry ) override final;
        virtual void ShutdownSystem() override final;
        virtual void UpdateSystem( EntityWorldUpdateContext const& ctx ) override final;
        virtual void RegisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;
        virtual void UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent ) override final;

        // Static Meshes
        //-------------------------------------------------------------------------

        void RegisterStaticMeshComponent( Entity const* pEntity, StaticMeshComponent* pMeshComponent );
        void UnregisterStaticMeshComponent( Entity const* pEntity, StaticMeshComponent* pMeshComponent );

        // Skeletal Meshes
        //-------------------------------------------------------------------------

        void RegisterSkeletalMeshComponent( Entity const* pEntity, SkeletalMeshComponent* pMeshComponent );
        void UnregisterSkeletalMeshComponent( Entity const* pEntity, SkeletalMeshComponent* pMeshComponent );

    private:

        // Static meshes
        TIDVector<ComponentID, StaticMeshComponent*>                    m_registeredStaticMeshComponents;
        TIDVector<ComponentID, StaticMeshComponent*>                    m_staticMeshComponents;
        List<StaticMeshComponent const*>                             m_visibleStaticMeshComponents;

        // Skeletal meshes
        TIDVector<ComponentID, SkeletalMeshComponent*>                  m_registeredSkeletalMeshComponents;
        TIDVector<uint32, SkeletalMeshGroup>                          m_skeletalMeshGroups;
        List<SkeletalMeshComponent const*>                           m_visibleSkeletalMeshComponents;

        // Lights
        TIDVector<ComponentID, DirectionalLightComponent*>              m_registeredDirectionLightComponents;
        TIDVector<ComponentID, PointLightComponent*>                    m_registeredPointLightComponents;
        TIDVector<ComponentID, SpotLightComponent*>                     m_registeredSpotLightComponents;
        TIDVector<ComponentID, LocalEnvironmentMapComponent*>           m_registeredLocalEnvironmentMaps;
        TIDVector<ComponentID, GlobalEnvironmentMapComponent*>          m_registeredGlobalEnvironmentMaps;
    };
}
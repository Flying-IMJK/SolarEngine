#include "WorldSystem_Renderer.h"
#include "Runtime/Entity/Entity.h"
#include "Runtime/Entity/EntityWorldUpdateContext.h"
#include "Runtime/Entity/EntityLog.h"
#include "Runtime/Entity/Components/Component_StaticMesh.h"
#include "Runtime/Entity/Components/Component_SkeletalMesh.h"
#include "Runtime/Entity/Components/Component_Lights.h"
#include "Runtime/Entity/Components/Component_EnvironmentMaps.h"
//#include "Runtime/Render/Shaders/EngineShaders.h"
//#include "Runtime/Render/Settings/WorldSettings_Render.h"
//#include "Runtime/Render/RenderCoreResources.h"
//#include "Runtime/Render/RenderViewport.h"
#include "Runtime/Render/Drawing/DebugDrawing.h"
#include "WorldSettings_Render.h"

//-------------------------------------------------------------------------

namespace SE::Render
{
    void RendererWorldSystem::InitializeSystem( Systems const& systemRegistry )
    {}

    void RendererWorldSystem::ShutdownSystem()
    {
        ENGINE_ASSERT( m_registeredStaticMeshComponents.empty() );
        ENGINE_ASSERT( m_registeredSkeletalMeshComponents.empty() );
        ENGINE_ASSERT( m_skeletalMeshGroups.empty() );

        ENGINE_ASSERT( m_registeredDirectionLightComponents.empty() );
        ENGINE_ASSERT( m_registeredPointLightComponents.empty() );
        ENGINE_ASSERT( m_registeredSpotLightComponents.empty() );

        ENGINE_ASSERT( m_registeredLocalEnvironmentMaps.empty() );
        ENGINE_ASSERT( m_registeredGlobalEnvironmentMaps.empty() );
    }

    void RendererWorldSystem::RegisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        // Meshes
        //-------------------------------------------------------------------------

        if ( StaticMeshComponent* pStaticMeshComponent = TryCast<StaticMeshComponent>( pComponent ) )
        {
            RegisterStaticMeshComponent( pEntity, pStaticMeshComponent );
        }
//        else if ( SkeletalMeshComponent* pSkeletalMeshComponent = TryCast<SkeletalMeshComponent>( pComponent ) )
//        {
//            RegisterSkeletalMeshComponent( pEntity, pSkeletalMeshComponent );
//        }
        
        // Lights
        //-------------------------------------------------------------------------

        else if ( auto pLightComponent = TryCast<LightComponent>( pComponent ) )
        {
            if ( DirectionalLightComponent* pDirectionalLightComponent = TryCast<DirectionalLightComponent>( pComponent ) )
            {
                m_registeredDirectionLightComponents.Add( pDirectionalLightComponent );
            }
            else if ( PointLightComponent* pPointLightComponent = TryCast<PointLightComponent>( pComponent ) )
            {
                m_registeredPointLightComponents.Add( pPointLightComponent );
            }
            else if ( SpotLightComponent* pSpotLightComponent = TryCast<SpotLightComponent>( pComponent ) )
            {
                m_registeredSpotLightComponents.Add( pSpotLightComponent );
            }
        }

        // Environment Maps
        //-------------------------------------------------------------------------

        else if ( auto pLocalEnvMapComponent = TryCast<LocalEnvironmentMapComponent>( pComponent ) )
        {
            m_registeredLocalEnvironmentMaps.Add( pLocalEnvMapComponent );
        }
        else if ( auto pGlobalEnvMapComponent = TryCast<GlobalEnvironmentMapComponent>( pComponent ) )
        {
            m_registeredGlobalEnvironmentMaps.Add( pGlobalEnvMapComponent );
        }
    }

    void RendererWorldSystem::UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent )
    {
        // Meshes
        //-------------------------------------------------------------------------

        if ( auto pStaticMeshComponent = TryCast<StaticMeshComponent>( pComponent ) )
        {
            UnregisterStaticMeshComponent( pEntity, pStaticMeshComponent );
        }
        else if ( auto pSkeletalMeshComponent = TryCast<SkeletalMeshComponent>( pComponent ) )
        {
            UnregisterSkeletalMeshComponent( pEntity, pSkeletalMeshComponent );
        }

        // Lights
        //-------------------------------------------------------------------------

        else if ( auto pLightComponent = TryCast<LightComponent>( pComponent ) )
        {
            if ( auto pDirectionalLightComponent = TryCast<DirectionalLightComponent>( pComponent ) )
            {
                m_registeredDirectionLightComponents.Remove( pDirectionalLightComponent->GetID() );
            }
            else if ( auto pPointLightComponent = TryCast<PointLightComponent>( pComponent ) )
            {
                m_registeredPointLightComponents.Remove( pPointLightComponent->GetID() );
            }
            else if ( auto pSpotLightComponent = TryCast<SpotLightComponent>( pComponent ) )
            {
                m_registeredSpotLightComponents.Remove( pSpotLightComponent->GetID() );
            }
        }

        // Environment Maps
        //-------------------------------------------------------------------------

        else if ( auto pLocalEnvMapComponent = TryCast<LocalEnvironmentMapComponent>( pComponent ) )
        {
            m_registeredLocalEnvironmentMaps.Remove( pLocalEnvMapComponent->GetID() );
        }
        else if ( auto pGlobalEnvMapComponent = TryCast<GlobalEnvironmentMapComponent>( pComponent ) )
        {
            m_registeredGlobalEnvironmentMaps.Remove( pGlobalEnvMapComponent->GetID() );
        }
    }

    void RendererWorldSystem::RegisterStaticMeshComponent( Entity const* pEntity, StaticMeshComponent* pMeshComponent )
    {
        m_registeredStaticMeshComponents.Add( pMeshComponent );

        //-------------------------------------------------------------------------

        // Add to appropriate sub-list
        if ( pMeshComponent->HasMeshResourceSet() )
        {
            m_staticMeshComponents.Add( pMeshComponent );
        }
    }

    void RendererWorldSystem::UnregisterStaticMeshComponent( Entity const* pEntity, StaticMeshComponent* pMeshComponent )
    {
        if ( pMeshComponent->HasMeshResourceSet() )
        {
            // Remove from the relevant runtime list
            m_staticMeshComponents.Remove( pMeshComponent->GetID() );
        }

        // Remove record
        m_registeredStaticMeshComponents.Remove( pMeshComponent->GetID() );
    }

    void RendererWorldSystem::RegisterSkeletalMeshComponent( Entity const* pEntity, SkeletalMeshComponent* pMeshComponent )
    {
        m_registeredSkeletalMeshComponents.Add( pMeshComponent );

        // Add to mesh groups
        //-------------------------------------------------------------------------

        if ( pMeshComponent->HasMeshResourceSet() )
        {
            auto pMesh = pMeshComponent->GetMesh();
            ENGINE_ASSERT( pMesh != nullptr && pMesh->IsValid() );
            uint32_t const meshID = pMesh->GetResourceID().GetPathID();

            auto pMeshGroup = m_skeletalMeshGroups.FindOrAdd( meshID, pMesh );
            pMeshGroup->m_components.Add( pMeshComponent );
        }
    }

    void RendererWorldSystem::UnregisterSkeletalMeshComponent( Entity const* pEntity, SkeletalMeshComponent* pMeshComponent )
    {
        // Unregistrations occur at the start of the frame
        // The world might be paused so we might leave an invalid component in this array
        m_visibleSkeletalMeshComponents.Clear();

        // Remove component from mesh group
        if ( pMeshComponent->HasMeshResourceSet() )
        {
            uint32_t const meshID = pMeshComponent->GetMesh()->GetResourceID().GetPathID();
            auto pMeshGroup = m_skeletalMeshGroups.Get( meshID );
            pMeshGroup->m_components.Remove(pMeshComponent);

            // Remove empty groups
            if ( pMeshGroup->m_components.IsEmpty() )
            {
                m_skeletalMeshGroups.Remove( meshID );
            }
        }

        // Remove record
        m_registeredSkeletalMeshComponents.Remove( pMeshComponent->GetID() );
    }

    //-------------------------------------------------------------------------

    void RendererWorldSystem::UpdateSystem( EntityWorldUpdateContext const& ctx )
    {
        if ( ctx.IsWorldPaused())
        {
            return;
        }

        //-------------------------------------------------------------------------
        // Culling
        //-------------------------------------------------------------------------

        AABB const viewBounds;// = ctx.GetViewport()->GetViewVolume().GetAABB();

        m_visibleStaticMeshComponents.Clear();

        for ( auto const& pMeshComponent : m_staticMeshComponents )
        {
//            EE_PROFILE_SCOPE_RENDER( "Static Mesh Cull" );

            if ( pMeshComponent->IsVisible() && viewBounds.Overlaps( pMeshComponent->GetWorldBounds() ) )
            {
                m_visibleStaticMeshComponents.Add( pMeshComponent );
            }
        }

        m_visibleSkeletalMeshComponents.Clear();

        for ( auto const& meshGroup : m_skeletalMeshGroups )
        {
//            EE_PROFILE_SCOPE_RENDER( "Skeletal Mesh Cull" );

            for ( auto pMeshComponent : meshGroup.m_components )
            {
                if ( pMeshComponent->IsVisible() && viewBounds.Overlaps( pMeshComponent->GetWorldBounds() ) )
                {
                    m_visibleSkeletalMeshComponents.Add( pMeshComponent );
                }
            }
        }

        //-------------------------------------------------------------------------
        // Debug
        //-------------------------------------------------------------------------

        #ifdef SE_DEVELOPMENT

        auto const* pRenderSettings = ctx.GetSettings<RenderWorldSettings>();

        Drawing::DrawContext drawCtx = ctx.GetDrawingContext();

        if ( pRenderSettings->m_showStaticMeshBounds )
        {
            for ( auto const& pMeshComponent : m_registeredStaticMeshComponents )
            {
                if ( pMeshComponent->IsVisible() )
                {
                    drawCtx.DrawWireBox( pMeshComponent->GetWorldBounds(), Colors::Cyan.ToFloat4());
                    drawCtx.DrawWireBox( pMeshComponent->GetWorldBounds().GetAABB(), Colors::LimeGreen.ToFloat4() );
                }
                else
                {
                    drawCtx.DrawWireBox( pMeshComponent->GetWorldBounds(), Colors::Cyan.ToFloat4() * Float4(1, 1, 1, 0.2f));
                    drawCtx.DrawWireBox( pMeshComponent->GetWorldBounds().GetAABB(), Colors::LimeGreen.ToFloat4() * Float4(1, 1, 1, 0.2f));
                }
            }
        }

        for ( auto const& pMeshComponent : m_registeredSkeletalMeshComponents )
        {
            if ( pRenderSettings->m_showSkeletalMeshBounds )
            {
                if ( pMeshComponent->IsVisible() )
                {
                    drawCtx.DrawWireBox( pMeshComponent->GetWorldBounds(), Colors::Cyan.ToFloat4());
                    drawCtx.DrawWireBox( pMeshComponent->GetWorldBounds().GetAABB(), Colors::LimeGreen.ToFloat4() );
                }
                else
                {
                    drawCtx.DrawWireBox( pMeshComponent->GetWorldBounds(), Colors::Cyan.ToFloat4() * Float4(1, 1, 1, 0.2f));
                    drawCtx.DrawWireBox( pMeshComponent->GetWorldBounds().GetAABB(), Colors::LimeGreen.ToFloat4() * Float4(1, 1, 1, 0.2f));
                }
            }

            if ( pRenderSettings->m_showSkeletalMeshBones )
            {
                pMeshComponent->DrawPose( drawCtx );
            }

            if ( pRenderSettings->m_showSkeletalMeshBindPoses )
            {
                pMeshComponent->GetMesh()->DrawBindPose( drawCtx, pMeshComponent->GetWorldTransform() );
            }
        }
        #endif
    }
}
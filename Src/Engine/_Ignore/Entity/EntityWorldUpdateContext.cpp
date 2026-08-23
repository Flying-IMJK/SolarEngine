#include "EntityWorldUpdateContext.h"
#include "EntityWorld.h"

//-------------------------------------------------------------------------

namespace SE
{
    EntityWorldUpdateContext::EntityWorldUpdateContext(EntityWorld* pWorld )
        : m_pWorld( pWorld )
        , m_isGameWorld( pWorld->IsGameWorld() )
    {
        ENGINE_ASSERT( m_pWorld != nullptr );

        // Handle paused world
/*        if ( pWorld->IsPaused() )
        {
            if ( pWorld->IsTimeStepRequested() )
            {
                m_DeltaTime = pWorld->GetTimeStepLength();
            }
            else // Set delta time to 0.0f
            {
                m_DeltaTime = 0.0f;
                m_isPaused = true;
            }
        }
        else // Apply world time scale
        {
            m_DeltaTime *= pWorld->GetTimeScale();
        }*/

//        ENGINE_ASSERT( m_DeltaTime >= 0.0f );
    }

    ISettings* EntityWorldUpdateContext::GetSettings( TypeInfo const* pTypeInfo )
    {
        return m_pWorld->GetSettings( pTypeInfo );
    }

    EntityWorldSystem* EntityWorldUpdateContext::GetWorldSystem( uint32_t worldSystemID ) const
    {
        return m_pWorld->GetWorldSystem( worldSystemID );
    }

    EntityModel::EntityMap* EntityWorldUpdateContext::GetPersistentMap() const
    {
        return m_pWorld->GetPersistentMap();
    }

/*    Render::RenderViewport const* EntityWorldUpdateContext::GetViewport() const
    {
        return m_pWorld->GetViewport();
    }*/

    float EntityWorldUpdateContext::GetTimeScale() const
    {
        return m_pWorld->GetTimeScale();
    }

    EntityWorldID const& EntityWorldUpdateContext::GetWorldID() const
    {
        return m_pWorld->GetID();
    }

    #ifdef SE_DEVELOPMENT
    Drawing::DrawContext EntityWorldUpdateContext::GetDrawingContext() const
    {
        return m_pWorld->m_debugDrawingSystem.GetDrawingContext();
    }

    Drawing::DrawingSystem* EntityWorldUpdateContext::GetDebugDrawingSystem() const
    {
        return m_pWorld->GetDebugDrawingSystem();
    }
    #endif
}
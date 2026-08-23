#include "EntitySpatialComponent.h"
#include "EntityLog.h"

//-------------------------------------------------------------------------

namespace SGE
{
    int32 SpatialEntityComponent::GetSpatialHierarchyDepth( bool limitToCurrentEntity ) const
    {
        int32 hierarchyDepth = 0;

        if ( limitToCurrentEntity && IsRootComponent() )
        {
            return hierarchyDepth;
        }

        //-------------------------------------------------------------------------

        SpatialEntityComponent const* pComponent = this;
        while ( pComponent->HasSpatialParent() )
        {
            if ( limitToCurrentEntity && pComponent->GetEntityID() != GetEntityID() )
            {
                break;
            }

            //-------------------------------------------------------------------------

            hierarchyDepth++;
            pComponent = pComponent->m_pSpatialParent;
        }

        return hierarchyDepth;
    }

    bool SpatialEntityComponent::IsSpatialChildOf( SpatialEntityComponent const* pPotentialParent ) const
    {
        ENGINE_ASSERT( pPotentialParent != nullptr );
        ENGINE_ASSERT( pPotentialParent->m_entityID == m_entityID );

        auto pActualParent = m_pSpatialParent;
        while ( pActualParent != nullptr )
        {
            if ( pActualParent == pPotentialParent )
            {
                return true;
            }

            pActualParent = pActualParent->m_pSpatialParent;
        }

        return false;
    }

    Transform SpatialEntityComponent::GetAttachmentSocketTransform( StringID socketID ) const
    {
        Transform socketTransform;

        // If the socket ID is invalid, just return the current transform
        if ( !socketID.IsValid() )
        {
            socketTransform = m_worldTransform;
            return socketTransform;
        }

        //-------------------------------------------------------------------------

        // Try to find the attachment socket transform and if it succeeds return it
        if ( TryFindAttachmentSocketTransform( socketID, socketTransform ) )
        {
            return socketTransform;
        }

        //-------------------------------------------------------------------------

        // Search all children
        for ( auto pChildSpatialComponent : m_spatialChildren )
        {
            auto foundTransform = pChildSpatialComponent->TryGetAttachmentSocketTransform( socketID, socketTransform );
            if ( foundTransform )
            {
                return socketTransform;
            }
        }

        // Log warning only when we are loaded/initialized
        if ( m_status == EntityComponent::Status::Loaded || m_status == EntityComponent::Status::Initialized )
        {
            LOG_ENTITY_WARNING_FORMAT( this, "Failed to find socket {0} on component {1} ({2})", socketID.ToString(), GetNameID().ToString(), GetID().m_value);
        }

        // Fallback to the world transform
        socketTransform = m_worldTransform;
        return socketTransform;
    }

    void SpatialEntityComponent::ApplyOffsetToAllChildren( VectorSIMD const& offset )
    {
        for ( auto pChildSpatialComponent : m_spatialChildren )
        {
            Transform adjustedLocalTransform = pChildSpatialComponent->GetLocalTransform();
            adjustedLocalTransform.AddTranslation( offset );
            pChildSpatialComponent->SetLocalTransform( adjustedLocalTransform );
        }
    }

    bool SpatialEntityComponent::TryGetAttachmentSocketTransform( StringID socketID, Transform& outSocketWorldTransform ) const
    {
        // Try to find the attachment socket transform and if it succeeds return it
        if ( TryFindAttachmentSocketTransform( socketID, outSocketWorldTransform ) )
        {
            return true;
        }

        // Search all children
        for ( auto pChildSpatialComponent : m_spatialChildren )
        {
            auto foundTransform = pChildSpatialComponent->TryGetAttachmentSocketTransform( socketID, outSocketWorldTransform );
            if ( foundTransform )
            {
                return true;
            }
        }

        return false;
    }

    bool SpatialEntityComponent::TryFindAttachmentSocketTransform( StringID socketID, Transform& outSocketWorldTransform ) const
    {
        outSocketWorldTransform = m_worldTransform;
        return false;
    }

    void SpatialEntityComponent::NotifySocketsUpdated()
    {
        for ( auto& pChildComponent : m_spatialChildren )
        {
            pChildComponent->CalculateWorldTransform();
        }
    }

    void SpatialEntityComponent::Initialize()
    {
        EntityComponent::Initialize();
        UpdateBounds();
    }

    //-------------------------------------------------------------------------

    #ifdef SE_DEVELOPMENT
    void SpatialEntityComponent::PostPropertyEdit( PropertyInfo const* pPropertyEdited )
    {
        EntityComponent::PostPropertyEdit( pPropertyEdited );

        // Property edits always refresh the transform since properties could have an effect on bounds/transform
        CalculateWorldTransform();
    }
    #endif
}
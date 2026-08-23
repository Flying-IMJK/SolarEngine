#pragma once

#include "Runtime/Entity/EntitySpatialComponent.h"
#include "Runtime/Entity/EntityDescriptors.h"
#include "Runtime/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace SE
{
    class SE_API_RUNTIME EntityCollectionComponent : public SpatialEntityComponent
    {
		SE_CLASS(EntityCollectionComponent, SpatialEntityComponent)
//        ENGINE_ENTITY_COMPONENT( EntityCollectionComponent );

    public:

        inline EntityCollectionComponent() = default;

        inline EntityModel::SerializedEntityCollection const* GetEntityCollectionDesc() const 
        {
            if ( !m_entityCollectionDesc.IsSet() )
            {
                return nullptr;
            }
            return m_entityCollectionDesc.IsLoaded() ? m_entityCollectionDesc.GetPtr() : nullptr;
        }

    private:

        TResPtr<EntityModel::SerializedEntityCollection> m_entityCollectionDesc;
    };
}
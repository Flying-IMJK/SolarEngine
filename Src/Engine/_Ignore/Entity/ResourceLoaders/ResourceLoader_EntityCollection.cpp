#include "ResourceLoader_EntityCollection.h"
#include "Base/Serialization/BinarySerialization.h"

//-------------------------------------------------------------------------

namespace SE::EntityModel
{
    EntityCollectionLoader::EntityCollectionLoader()
        : m_pTypeRegistry( nullptr )
    {
        m_loadableTypes.push_back( SerializedEntityCollection::GetStaticResourceTypeID() );
        m_loadableTypes.push_back( SerializedEntityMap::GetStaticResourceTypeID() );
    }

    void EntityCollectionLoader::SetTypeRegistryPtr( TypeRegistry const* pTypeRegistry )
    {
        ENGINE_ASSERT( pTypeRegistry != nullptr );
        m_pTypeRegistry = pTypeRegistry;
    }

    bool EntityCollectionLoader::LoadInternal( ResID const& resID, Resource::ResRecord* pResourceRecord, Serialization::BinaryInputArchive& archive ) const
    {
        ENGINE_ASSERT( m_pTypeRegistry != nullptr );

        SerializedEntityCollection* pCollectionDesc = nullptr;

        if ( resID.GetResourceTypeID() == SerializedEntityMap::GetStaticResourceTypeID() )
        {
            auto pMap = EE::New<SerializedEntityMap>();
            archive << *pMap;
            pCollectionDesc = pMap;
        }
        else  if ( resID.GetResourceTypeID() == SerializedEntityCollection::GetStaticResourceTypeID() )
        {
            auto pEC = EE::New<SerializedEntityCollection>();
            archive << *pEC;
            pCollectionDesc = pEC;
        }

        // Set loaded resource
        pResourceRecord->SetResourceData( pCollectionDesc );
        return true;
    }
}
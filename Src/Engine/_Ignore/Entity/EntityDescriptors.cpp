#include "EntityDescriptors.h"

#include "Entity.h"
#include "Core/TypeSystem/Types.h"
#include "Core/Profiler/Profiler.h"
#include "Core/Types/Collections/Sorting.h"

//-------------------------------------------------------------------------

namespace SE::EntityModel
{
    int32 SerializedEntityDescriptor::FindComponentIndex( StringID const& componentName ) const
    {
        ENGINE_ASSERT( componentName.IsValid() );

        int32 const numComponents = (int32) m_components.Count();
        for ( int32 i = 0; i < numComponents; i++ )
        {
            if ( m_components[i].m_name == componentName )
            {
                return i;
            }
        }

        return -1;
    }

    #ifdef SE_DEVELOPMENT
    void SerializedEntityDescriptor::ClearAllSerializedIDs()
    {
        m_transientEntityID.Clear();

        for ( auto& component : m_components )
        {
            component.m_transientComponentID.Clear();
        }
    }
    #endif

    //-------------------------------------------------------------------------

    List<SerializedEntityCollection::SearchResult> SerializedEntityCollection::GetComponentsOfType( Types const& typeRegistry, TypeID typeID, bool allowDerivedTypes )
    {
		List<SearchResult> foundComponents;

        for ( auto& entityDesc : m_entityDescriptors )
        {
            for ( auto& componentDesc : entityDesc.m_components )
            {
                if ( componentDesc.typeID == typeID )
                {
                    auto& result = foundComponents.AddOne();
                    result.m_pEntity = &entityDesc;
                    result.m_pComponent = &componentDesc;
                }
                else if ( allowDerivedTypes )
                {
                    auto pTypeInfo = typeRegistry.GetTypeInfo( componentDesc.typeID );
                    ENGINE_ASSERT( pTypeInfo != nullptr );

                    if ( pTypeInfo->IsDerivedFrom( typeID ) )
                    {
                        auto& result = foundComponents.AddOne();
                        result.m_pEntity = &entityDesc;
                        result.m_pComponent = &componentDesc;
                    }
                }
            }
        }

        //-------------------------------------------------------------------------

        return foundComponents;
    }

    #if ENGINE_DEVELOPMENT_TOOLS
    void SerializedEntityCollection::Clear()
    {
        m_entityDescriptors.Clear();
        m_entityLookupMap.Clear();
        m_entitySpatialAttachmentInfo.Clear();
    }

    void SerializedEntityCollection::SetCollectionData( List<SerializedEntityDescriptor>&& entityDescriptors )
    {
        // Set entity descriptors
        //-------------------------------------------------------------------------

        m_entityDescriptors.Swap( entityDescriptors );
        int32 const numEntities = (int32) m_entityDescriptors.Count();

        // Generate spatial hierarchy depths
        //-------------------------------------------------------------------------

        for ( int32 i = 0; i < numEntities; i++ )
        {
            auto& entityDesc = m_entityDescriptors[i];
            ENGINE_ASSERT( entityDesc.IsValid() );
            entityDesc.m_spatialHierarchyDepth = 0;

            if ( entityDesc.IsSpatialEntity() )
            {
                // Calculate the spatial hierarchy depth
                StringID parentID = entityDesc.m_spatialParentName;
                while ( parentID.IsValid() )
                {
                    entityDesc.m_spatialHierarchyDepth++;

                    int32 const parentIdx = m_entityLookupMap[parentID];
                    ENGINE_ASSERT( parentIdx != -1 );
                    parentID = m_entityDescriptors[parentIdx].m_spatialParentName;
                }
            }
        }

        // Sort entity desc array by depth
        //-------------------------------------------------------------------------

        Function<bool(SerializedEntityDescriptor const&, SerializedEntityDescriptor const&)> SortComparator = [] ( SerializedEntityDescriptor const& entityA, SerializedEntityDescriptor const& entityB )
        {
            ENGINE_ASSERT( entityA.m_spatialHierarchyDepth >= 0 && entityA.m_spatialHierarchyDepth >= 0 );
            return entityA.m_spatialHierarchyDepth < entityB.m_spatialHierarchyDepth;
        };

        Sorting::QuickSort( m_entityDescriptors, SortComparator );

        // Create lookup map
        //-------------------------------------------------------------------------

        m_entityLookupMap.Clear();
        m_entityLookupMap.SetCapacity( numEntities );

        for ( int32 i = 0; i < numEntities; i++ )
        {
            m_entityLookupMap.Add(m_entityDescriptors[i].m_name, i );
        }

        // Generate spatial attachment info
        //-------------------------------------------------------------------------

        m_entitySpatialAttachmentInfo.Clear();
        m_entitySpatialAttachmentInfo.Resize( m_entityDescriptors.Count() );

        for ( int32 i = 0; i < numEntities; i++ )
        {
            auto const& entityDesc = m_entityDescriptors[i];
            if ( !entityDesc.IsSpatialEntity() || !entityDesc.HasSpatialParent() )
            {
                continue;
            }

            //-------------------------------------------------------------------------

            SpatialAttachmentInfo attachmentInfo;
            attachmentInfo.m_entityIdx = i;
            attachmentInfo.m_parentEntityIdx = FindEntityIndex( entityDesc.m_spatialParentName );

            if ( attachmentInfo.m_parentEntityIdx != -1 )
            {
                m_entitySpatialAttachmentInfo.Add( attachmentInfo );
            }
        }
    }

    void SerializedEntityCollection::GetAllReferencedResources( List<ResID>& outReferencedResources ) const
    {
        outReferencedResources.Clear();

        /*TypeID const resourceIDTypeID = CoreTypeRegistry::GetTypeID( TypeIDCore::ResourceID );
        TypeID const resourcePathTypeID = CoreTypeRegistry::GetTypeID( TypeIDCore::ResPath );
        TypeID const resourcePtrTypeID = CoreTypeRegistry::GetTypeID( TypeIDCore::ResourcePtr );
        TypeID const templateResourcePtrTypeID = CoreTypeRegistry::GetTypeID( TypeIDCore::TResourcePtr );*/

        for ( auto const& entityDesc : m_entityDescriptors )
        {
            for ( auto const& componentDesc : entityDesc.m_components )
            {
                for ( auto const& propertyDesc : componentDesc.properties )
                {
/*                    if ( propertyDesc.typeID == resourceIDTypeID || propertyDesc.typeID == resourcePathTypeID || propertyDesc.typeID == resourcePtrTypeID || propertyDesc.typeID == templateResourcePtrTypeID )
                    {
						outReferencedResources.AddUnique(ResID( propertyDesc.stringValue ) );
                    }*/
                }
            }
        }
    }
    #endif
}
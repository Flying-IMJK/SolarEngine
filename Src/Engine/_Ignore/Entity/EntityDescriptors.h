#pragma once
#include "Runtime/API.h"
#include "EntityIDs.h"
#include "Runtime/Resource/IResource.h"
#include "Core/TypeSystem/TypeDescriptors.h"

namespace SE
{
    namespace Reflect { class Types; }
    class Entity;
    class TaskSystem;
}

//-------------------------------------------------------------------------
// Serialized Entity Descriptors
//-------------------------------------------------------------------------
// A custom format for serialized entities
// Very similar to the type descriptor format in the type-system

namespace SE::EntityModel
{
    struct SE_API_RUNTIME SerializedComponentDescriptor : public TypeDescriptor
    {
//        ENGINE_SERIALIZE(ENGINE_SERIALIZE_BASE( TypeDescriptor ), m_spatialParentName, m_attachmentSocketID, m_name, m_isSpatialComponent);

    public:

        inline bool IsValid() const { return TypeDescriptor::IsValid() && m_name.IsValid(); }

        // Spatial Components
        inline bool IsSpatialComponent() const { return m_isSpatialComponent; }
        inline bool IsRootComponent() const { ENGINE_ASSERT( m_isSpatialComponent ); return !m_spatialParentName.IsValid(); }
        inline bool HasSpatialParent() const { ENGINE_ASSERT( m_isSpatialComponent ); return m_spatialParentName.IsValid(); }

    public:

        StringID                                                    m_name;
        StringID                                                    m_spatialParentName;
        StringID                                                    m_attachmentSocketID;
        bool                                                        m_isSpatialComponent = false;

        #ifdef SE_DEVELOPMENT
        ComponentID                                                 m_transientComponentID; // WARNING: this is not serialized, and it is only stored for undo/redo support in the tools
        #endif
    };

    //-------------------------------------------------------------------------

    struct SE_API_RUNTIME SerializedSystemDescriptor
    {
//        ENGINE_SERIALIZE( m_typeID );

    public:

        inline bool IsValid() const { return m_typeID.IsValid(); }

    public:

        TypeID                                          m_typeID;
    };

    //-------------------------------------------------------------------------

    struct SE_API_RUNTIME SerializedEntityDescriptor
    {
//        ENGINE_SERIALIZE( m_name, m_spatialParentName, m_attachmentSocketID, m_systems, m_components, m_numSpatialComponents );

    public:

        inline bool IsValid() const { return m_name.IsValid(); }
        inline bool IsSpatialEntity() const { return m_numSpatialComponents > 0; }
        inline bool HasSpatialParent() const { return m_spatialParentName.IsValid(); }

        int32 FindComponentIndex( StringID const& componentName ) const;

        inline SerializedComponentDescriptor const* FindComponent( StringID const& componentName ) const
        {
            int32 const componentIdx = FindComponentIndex( componentName );
            return ( componentIdx != -1 ) ? &m_components[componentIdx] : nullptr;
        }

        #ifdef SE_DEVELOPMENT
        void ClearAllSerializedIDs();
        #endif

    public:

        StringID                                                    m_name;
        StringID                                                    m_spatialParentName;
        StringID                                                    m_attachmentSocketID;
        int32                                                       m_spatialHierarchyDepth = -1;
        List<SerializedSystemDescriptor>                            m_systems;
		List<SerializedComponentDescriptor>                         m_components; // Ordered list of components: spatial components are first, followed by regular components
        int32                                                       m_numSpatialComponents = 0;

        #ifdef SE_DEVELOPMENT
        EntityID                                                    m_transientEntityID; // WARNING: this is not serialized, and it is only stored for undo/redo support in the tools
        #endif
    };
}

//-------------------------------------------------------------------------
// Entity Collection
//-------------------------------------------------------------------------
// This is a read-only resource that contains a collection of serialized entity descriptors
// We used this to instantiate a collection of entities
//-------------------------------------------------------------------------

namespace SE::EntityModel
{
    class SE_API_RUNTIME SerializedEntityCollection : public IResource
    {
//        ENGINE_RESOURCE("ec", "Entity Collection");
//        ENGINE_SERIALIZE(m_entityDescriptors, m_entityLookupMap, m_entitySpatialAttachmentInfo);

        friend class EntityCollectionLoader;
        friend struct Serializer;

    public:

        struct SearchResult
        {
            SerializedEntityDescriptor*                             m_pEntity = nullptr;
            SerializedComponentDescriptor*                          m_pComponent = nullptr;
        };

    protected:

        struct SpatialAttachmentInfo
        {
//            ENGINE_SERIALIZE( m_entityIdx, m_parentEntityIdx );

            int32                                                   m_entityIdx = -1;
            int32                                                   m_parentEntityIdx = -1;
        };

    public:

        virtual bool IsValid() const override
        {
            if ( m_entityDescriptors.IsEmpty() )
            {
                return true;
            }

            return m_entityDescriptors.Count() == m_entityLookupMap.Count();
        }

        // Entity Access
        //-------------------------------------------------------------------------

        inline int32 GetNumEntityDescriptors() const
        {
            return (int32) m_entityDescriptors.Count();
        }

        inline List<SerializedEntityDescriptor> const& GetEntityDescriptors() const
        {
            return m_entityDescriptors;
        }

        inline List<SpatialAttachmentInfo> const& GetEntitySpatialAttachmentInfo() const
        {
            return m_entitySpatialAttachmentInfo;
        }

        inline SerializedEntityDescriptor const* FindEntityDescriptor( StringID const& entityName ) const
        {
            ENGINE_ASSERT( entityName.IsValid() );

            auto const foundEntityIter = m_entityLookupMap.Find( entityName );
            if ( foundEntityIter != m_entityLookupMap.end() )
            {
                return &m_entityDescriptors[foundEntityIter->Value];
            }
            else
            {
                return nullptr;
            }
        }

        inline int32 FindEntityIndex( StringID const& entityName ) const
        {
            ENGINE_ASSERT( entityName.IsValid() );

            auto const foundEntityIter = m_entityLookupMap.Find( entityName );
            if ( foundEntityIter != m_entityLookupMap.end() )
            {
                return foundEntityIter->Value;
            }
            else
            {
                return -1;
            }
        }

        // Component Access
        //-------------------------------------------------------------------------

		List<SearchResult> GetComponentsOfType( Types const& typeRegistry, TypeID typeID, bool allowDerivedTypes = true );

        inline List<SearchResult> GetComponentsOfType( Types const& typeRegistry, TypeID typeID, bool allowDerivedTypes = true ) const
        {
            return const_cast<SerializedEntityCollection*>( this )->GetComponentsOfType( typeRegistry, typeID, allowDerivedTypes );
        }

        template<typename T>
        inline List<SearchResult> GetComponentsOfType( Types const& typeRegistry, bool allowDerivedTypes = true )
        {
            return GetComponentsOfType( typeRegistry, T::GetStaticTypeID(), allowDerivedTypes );
        }

        template<typename T>
        inline List<SearchResult> GetComponentsOfType( Types const& typeRegistry, bool allowDerivedTypes = true ) const
        {
            return const_cast<SerializedEntityCollection*>( this )->GetComponentsOfType( typeRegistry, T::GetStaticTypeID(), allowDerivedTypes );
        }

        bool HasComponentsOfType( Types const& typeRegistry, TypeID typeID, bool allowDerivedTypes = true ) const
		{
			return !const_cast<SerializedEntityCollection*>( this )->GetComponentsOfType( typeRegistry, typeID, allowDerivedTypes ).IsEmpty();
		}

        template<typename T>
        inline bool HasComponentsOfType( Types const& typeRegistry, bool allowDerivedTypes = true ) const
        {
            return HasComponentsOfType( typeRegistry, T::GetStaticTypeID(), allowDerivedTypes );
        }

        // Collection Creation and Info
        //-------------------------------------------------------------------------

        #ifdef SE_DEVELOPMENT
        void Clear();
        void SetCollectionData( List<SerializedEntityDescriptor>&& entityDescriptors );
        void GetAllReferencedResources( List<ResID>& outReferencedResources ) const;
        #endif

    protected:

		List<SerializedEntityDescriptor>                         m_entityDescriptors;
        Dictionary<StringID, int32>                              m_entityLookupMap;
		List<SpatialAttachmentInfo>                              m_entitySpatialAttachmentInfo;
    };
}

//-------------------------------------------------------------------------
// A compiled entity map template
//-------------------------------------------------------------------------
// This is a read-only resource that contains the serialized entities for a given map
// This is not directly used in the game, instead we create an entity map instance from this map
//-------------------------------------------------------------------------

namespace SE::EntityModel
{
    struct LoadingContext;

    //-------------------------------------------------------------------------

    class SE_API_RUNTIME SerializedEntityMap final : public SerializedEntityCollection
    {
//        ENGINE_RESOURCE("map", "Map");
        friend class EntityCollectionCompiler;
        friend class EntityCollectionLoader;
    };
}
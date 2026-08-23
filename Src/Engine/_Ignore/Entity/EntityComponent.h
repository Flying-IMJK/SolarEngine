#pragma once

#include "Runtime/API.h"
#include "EntityIDs.h"
#include "EntityContexts.h"
#include "Core/TypeSystem/IReflectedType.h"
#include "Runtime/Resource/ResourceRequest.h"

//-------------------------------------------------------------------------

namespace SE
{
    namespace EntityModel
    {
        class EntityMapEditor;
        class EntityCollection;
        class EntityMap;
        struct Serializer;
    }

    //-------------------------------------------------------------------------

    class SE_API_RUNTIME EntityComponent : public IReflectedType
    {
        SE_CLASS( EntityComponent, IReflectedType);

        friend class Entity;
        friend class EntityWorld;
        friend EntityModel::Serializer;
        friend EntityModel::EntityCollection;
        friend EntityModel::EntityMap;

    public:

        enum class Status : uint8
        {
            Unloaded = 0,
            Loading,
            Loaded,
            LoadingFailed,
            Initialized,
        };

    public:

        virtual ~EntityComponent();

        // Get the globally unique transient ID (this is generated at runtime)
        inline ComponentID const& GetID() const { return m_ID; }

        // Get the serialized component name ID, this is unique within the context of an entity
        inline StringID GetNameID() const { return m_name; }

        // Get the ID of the entity that owns this component
        inline EntityID const& GetEntityID() const { return m_entityID; }

        // Status
        inline bool HasLoadingFailed() const { return m_status == Status::LoadingFailed; }
        inline bool IsUnloaded() const { return m_status == Status::Unloaded; }
        inline bool IsLoading() const { return m_status == Status::Loading; }
        inline bool IsLoaded() const { return m_status == Status::Loaded; }
        inline bool IsInitialized() const { return m_status == Status::Initialized; }
        inline Status GetStatus() const { return m_status; }

        // Do we allow multiple components of the same type per entity?
        virtual bool IsSingletonComponent() const { return false; }

    protected:

        EntityComponent() = default;
        EntityComponent( EntityComponent const& ) = default;
        EntityComponent( StringID name ) : m_name( name ) {}

        EntityComponent& operator=( EntityComponent const& rhs ) = default;

        // Request load of all component data - loading takes time
        virtual void Load( EntityModel::LoadingContext const& context, ResourceRequesterID const& requesterID )
		{
			ENGINE_UNIMPLEMENTED_FUNCTION()
		}

        // Request unload of component data, unloading is instant
        virtual void Unload( EntityModel::LoadingContext const& context, ResourceRequesterID const& requesterID )
		{
			ENGINE_UNIMPLEMENTED_FUNCTION()
		}

        // Update loading state, this will check all dependencies
        virtual void UpdateLoading()
		{
			ENGINE_UNIMPLEMENTED_FUNCTION()
		}

        // Called when an component finishes loading all its resources
        // Note: this is only called if the loading succeeds and you are guaranteed all resources to be valid and so should assert on that
        virtual void Initialize() { ENGINE_ASSERT( m_entityID.IsValid() && m_status == Status::Loaded ); m_status = Status::Initialized; }

        // Called just before a component begins unloading
        virtual void Shutdown() { ENGINE_ASSERT( m_entityID.IsValid() && m_status == Status::Initialized ); m_status = Status::Loaded; }

    protected:

        ComponentID                                         m_ID = ComponentID::Generate();                 // The unique ID for this component
        EntityID                                            m_entityID;                                     // The ID of the entity that owns this component
        SE_PROPERTY( "IsToolsReadOnly" : true )
        StringID                                            m_name;                                         // The name of the component
        Status                                              m_status = Status::Unloaded;                    // Component status
        bool                                                m_isRegisteredWithEntity = false;               // Registered with its parent entity's local systems
        bool                                                m_isRegisteredWithWorld = false;                // Registered with the global systems in it's parent world
    };
}

//-------------------------------------------------------------------------

#define ENGINE_ENTITY_COMPONENT( TypeName ) \
        protected:\
        virtual void Load( EntityModel::LoadingContext const& context, ResourceRequesterID const& requesterID ) override;\
        virtual void Unload( EntityModel::LoadingContext const& context, ResourceRequesterID const& requesterID ) override;\
        virtual void UpdateLoading() override;

// Use this macro to create a singleton component (and hierarchy) - Note: All derived types must use the regular registration macro
#define ENGINE_SINGLETON_ENTITY_COMPONENT( TypeName ) \
        SE_CLASS( TypeName );\
        protected:\
        virtual bool IsSingletonComponent() const override final { return true; }\
        virtual void Load( EntityModel::LoadingContext const& context, ResourceRequesterID const& requesterID ) override;\
        virtual void Unload( EntityModel::LoadingContext const& context, ResourceRequesterID const& requesterID ) override;\
        virtual void UpdateLoading() override;
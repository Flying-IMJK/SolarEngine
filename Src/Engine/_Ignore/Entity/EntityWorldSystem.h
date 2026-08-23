#pragma once

#include "EntityIDs.h"
#include "EntityWorldType.h"

#include "Runtime/API.h"
#include "Core/TypeSystem/IReflectedType.h"
#include "Core/Types/Collections/List.h"
#include "Core/Tools/Hash.h"


//-------------------------------------------------------------------------
// World Entity System
//-------------------------------------------------------------------------
// This is a global system that exists once per world and tracks/updates all components of certain types in the world!

namespace SE
{
    class Systems;
    class EntityWorldUpdateContext;
    class Entity;
    class EntityComponent;
    namespace EntityModel { class EntityMap; }

    //-------------------------------------------------------------------------

    class SE_API_RUNTIME EntityWorldSystem : public IReflectedType
    {
        SE_CLASS(EntityWorldSystem, IReflectedType);

        friend class EntityWorld;
        friend EntityModel::EntityMap;

    public:

        virtual uint32 GetSystemID() const = 0;

        // Is this world system in a game world
        bool IsInAGameWorld() const;

        // Is this world system in a tools-only world
        bool IsInAToolsWorld() const;

    protected:

        // Get the required update stages and priorities for this component
//        virtual UpdatePriorityList const& GetRequiredUpdatePriorities() = 0;

        // Called when the system is registered with the world - using explicit "EntitySystem" name to allow for a standalone initialize function
        virtual void InitializeSystem( Systems const& systemRegistry ) {};

        // Called when the system is removed from the world - using explicit "EntitySystem" name to allow for a standalone shutdown function
        virtual void ShutdownSystem() {};

        // System Update - using explicit "EntitySystem" name to allow for a standalone update functions
        virtual void UpdateSystem( EntityWorldUpdateContext const& ctx ) {};

        // Called whenever a new component is activated (i.e. added to the world)
        virtual void RegisterComponent( Entity const* pEntity, EntityComponent* pComponent ) = 0;

        // Called immediately before an component is deactivated
        virtual void UnregisterComponent( Entity const* pEntity, EntityComponent* pComponent ) = 0;

    private:

        EntityWorld* m_pWorld = nullptr;
    };
}

//-------------------------------------------------------------------------

#define ENGINE_ENTITY_WORLD_SYSTEM( Type, ... )\
    SE_CLASS( Type );\
    constexpr static uint32 const s_entitySystemID = Hash::FNV1a::GetHash32( #Type );\
    virtual uint32 GetSystemID() const override final { return Type::s_entitySystemID; }\
    /*static UpdatePriorityList const PriorityList;\
    virtual UpdatePriorityList const& GetRequiredUpdatePriorities() override { static UpdatePriorityList const priorityList = UpdatePriorityList( __VA_ARGS__ ); return priorityList; };\*/

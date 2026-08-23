#pragma once

#include "Runtime/API.h"
#include "EntityWorldType.h"
#include "Runtime/Resource/ResourceRequest.h"
#include "Core/Systems.h"

//-------------------------------------------------------------------------

namespace SE
{
    class UpdateContext;
    class EntityWorld;
    class Systems;
    namespace Reflect { class TypeInfo; }
    namespace Render { class RenderViewport; }

    //-------------------------------------------------------------------------

    class SE_API_RUNTIME EntityWorldManager : public ISystem
    {
        friend class EntityDebugView;

    public:

        ENGINE_SYSTEM( EntityWorldManager );

    public:

        ~EntityWorldManager();

        void Initialize( Systems const& systemsRegistry );
        void Shutdown();

        //-------------------------------------------------------------------------

        // Called at the start of the frame
        void StartFrame();

        // Called at the end of the frame, just before rendering
        void EndFrame();

        // Loading
        //-------------------------------------------------------------------------

        bool IsBusyLoading() const;
        void UpdateLoading();

        // Worlds
        //-------------------------------------------------------------------------

        // Returns the currently active game world, there should only be one at any given time
        EntityWorld* GetGameWorld();
        
        // Returns the currently active game world, there should only be one at any given time
        EntityWorld const* GetGameWorld() const { return const_cast<EntityWorldManager*>( this )->GetGameWorld(); }

        // Create a new entity world - note only a single game world is allowed at any given time
        EntityWorld* CreateWorld( EntityWorldType worldType );

        // Destroy an existing world
        void DestroyWorld( EntityWorld* pWorld );

        // Get all created worlds
        List<EntityWorld*> const& GetWorlds() const { return m_worlds; }

        // Run the world update - updates all entities, systems and camera
        void UpdateWorlds( UpdateContext const& context );

        // Hot Reload
        //-------------------------------------------------------------------------

        #ifdef SE_DEVELOPMENT
        void HotReload_UnloadEntities( List<ResourceRequesterID> const& usersToReload );
        void HotReload_ReloadEntities();
        #endif

    private:

        Systems const*                              m_pSystemsRegistry = nullptr;
        List<EntityWorld*>                               m_worlds;
        List<TypeInfo const*>                m_worldSystemTypeInfos;
    };
}
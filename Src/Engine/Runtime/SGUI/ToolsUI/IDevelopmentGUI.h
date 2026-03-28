#pragma once
#include "Runtime/API.h"
#include "Core/Resource/ResourceRequesterID.h"
#include "Core/Types/Collections/List.h"

//-------------------------------------------------------------------------

namespace SE
{
    class UpdateContext;
}

//-------------------------------------------------------------------------
// Development Tools Framework
//-------------------------------------------------------------------------
// Base class for any runtime/editor development UI tools

#ifdef SE_DEVELOPMENT
namespace SE::SGUI
{
    //-------------------------------------------------------------------------

    class SE_API_RUNTIME IDevelopmentGUI
    {
    public:

        IDevelopmentGUI() = default;
        IDevelopmentGUI( IDevelopmentGUI const& ) = default;
        virtual ~IDevelopmentGUI() = default;

        IDevelopmentGUI& operator=( IDevelopmentGUI const& rhs ) = default;

        virtual void Initialize( UpdateContext const& context) = 0;
        virtual void Shutdown( UpdateContext const& context ) = 0;

        // This is called at the absolute start of the frame before we update the resource system, start updating any entities, etc...
        // Any entity/world/map state changes need to be done via this update!
        virtual void StartFrame( UpdateContext const& context ) {}

        // Optional update run before we update the world at each stage
        virtual void Update( UpdateContext const& context ) {}

        // This is called at the absolute end of the frame just before we kick off rendering. It is generally NOT safe to modify any world/map/entity during this update!!!
        virtual void EndFrame( UpdateContext const& context ) {}

        // Hot Reload Support
        //-------------------------------------------------------------------------

        // Start a hot-reload operation by unloading all resource about to be reloaded
        virtual void HotReload_UnloadResources(List<ResourceRequesterID> const& usersToReload, List<ResID> const& resourcesToBeReloaded ) = 0;

        // Request a load on all unloaded resources
        virtual void HotReload_ReloadResources() = 0;
    };
}
#endif
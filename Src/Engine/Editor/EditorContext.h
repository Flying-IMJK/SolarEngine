#pragma once

#include "Runtime/Core/Types/Strings/String.h"
#include "Editor/API.h"

// #include "Engine/Entity/EntityIDs.h"

//-------------------------------------------------------------------------
namespace SE
{
    class EntityWorld;
    class EntityWorldManager;
    class SystemRegistry;
    class ResourceSystem;
    
    namespace Reflect { class Types; }
    namespace SGUI 
    {
         class ImageCache;
    }
}


namespace SE::Editor
{
    //-------------------------------------------------------------------------

    class SE_API_EDITOR EditorContext
    {
    public:

        virtual ~EditorContext() = default;
        inline bool IsValid() const { return true; }

        String const& GetRawResourceDirectory() const;
		String const& GetCompiledResourceDirectory() const;

        // Resources
        //-------------------------------------------------------------------------

        // Debugging
        //-------------------------------------------------------------------------

        // Try to find a created entity
        //Entity* TryFindEntityInAllWorlds(EntityID ID) const;

        // Get all currently created worlds
        //List<EntityWorld const*> GetAllWorlds() const;

    protected:

//        virtual EntityWorldManager* GetWorldManager() const = 0;

    public:
//		ResourceServerProvider* pResourceProvider;
		// ResourceDatabase * pResourceDatabase = nullptr;
    };
}
#include "EditorContext.h"

// #include "EngineTools/Resource/ResourceDatabase.h"
// #include "Engine/Entity/EntityWorld.h"
// #include "Engine/Entity/EntityWorldManager.h"

//-------------------------------------------------------------------------

namespace SE::Editor
{
    String const& EditorContext::GetRawResourceDirectory() const
    {
		return String::Empty;
//         return pResourceDatabase->GetRawResourceDirectoryEntry()->path;
    }

	String const& EditorContext::GetCompiledResourceDirectory() const
    {
        return String::Empty;//pResourceDatabase->GetCompiledResourceDirectoryPath();
    }

    //-------------------------------------------------------------------------

    // Entity* EditorContext::TryFindEntityInAllWorlds(EntityID ID) const
    // {
    //     Entity* pFoundEntity = nullptr;
    //     for ( EntityWorld const* pWorld : GetWorldManager()->GetWorlds() )
    //     {
    //         pFoundEntity = pWorld->FindEntity( ID );
    //         if ( pFoundEntity != nullptr )
    //         {
    //             break;
    //         }
    //     }

    //     return pFoundEntity;
    // }

    // List<EntityWorld const*> EditorContext::GetAllWorlds() const
    // {
    //     List<EntityWorld const*> debugWorlds;
    //     for ( EntityWorld const* pWorld : GetWorldManager()->GetWorlds() )
    //     {
    //         debugWorlds.emplace_back( pWorld );
    //     }
    //     return debugWorlds;
    // }
}
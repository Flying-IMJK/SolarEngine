#include "EntityWorldSystem.h"
#include "EntityWorld.h"

//-------------------------------------------------------------------------

namespace SE
{
    bool EntityWorldSystem::IsInAGameWorld() const
    {
        return m_pWorld->GetWorldType() == EntityWorldType::Game;
    }

    bool EntityWorldSystem::IsInAToolsWorld() const
    {
        return m_pWorld->GetWorldType() == EntityWorldType::Tools;
    }
}
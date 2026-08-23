#include "EntityComponent.h"

//-------------------------------------------------------------------------

namespace SE
{
    EntityComponent::~EntityComponent()
    {
        ENGINE_ASSERT( m_status == Status::Unloaded );
        ENGINE_ASSERT( m_isRegisteredWithEntity == false && m_isRegisteredWithWorld == false );
    }
}
#pragma once
#include "Runtime/API.h"
#include "Core/Types/Collections/List.h"

//-------------------------------------------------------------------------

namespace SE
{
    class Entity;
    class TaskSystem;
    namespace Reflect { class Types; }
    namespace EntityModel
    {
        class EntityMap;
        struct SerializedEntityDescriptor;
        class SerializedEntityCollection;
        struct SerializedComponentDescriptor;
    }
}

//-------------------------------------------------------------------------

namespace SE::EntityModel
{
    struct SE_API_RUNTIME Serializer
    {
        static Entity* CreateEntity(SerializedEntityDescriptor const& entityDesc );
        static List<Entity*> CreateEntities(SerializedEntityCollection const& entityCollection );

        //-------------------------------------------------------------------------

        #ifdef SE_DEVELOPMENT
        static bool SerializeEntity(Entity const* pEntity, EntityModel::SerializedEntityDescriptor& outDesc );
        static bool SerializeEntityMap(EntityMap const* pMap, SerializedEntityCollection& outCollection );
        #endif
    };
}
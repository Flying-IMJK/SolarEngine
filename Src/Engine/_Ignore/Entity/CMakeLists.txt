
add_subdirectory(Components)

RUNTIME_ADD(
        Entity.h
        Entity.cpp
        EntityComponent.h
        EntityComponent.cpp

        EntityDescriptors.h
        EntityDescriptors.cpp
        EntityIDs.h
        EntityIDs.cpp
        EntityLog.h
        EntityLog.cpp
        EntityMap.h
        EntityMap.cpp
        EntitySerialization.h
        EntitySerialization.cpp
        EntitySpatialComponent.h
        EntitySpatialComponent.cpp
        EntitySystem.h
        EntityWorld.h
        EntityWorld.cpp
        EntityWorldManager.h
        EntityWorldManager.cpp
        EntityWorldSettings.h
        EntityWorldSystem.h
        EntityWorldSystem.cpp
        EntityWorldType.h
        EntityWorldUpdateContext.h
        EntityWorldUpdateContext.cpp

        Systems/WorldSystem_EntityCollectionSpawner.h
        Systems/WorldSystem_EntityCollectionSpawner.cpp

        IDVector.h
)

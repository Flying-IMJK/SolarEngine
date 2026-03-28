
add_subdirectory(Scene)
add_subdirectory(Actors)
add_subdirectory(Prefabs)

RUNTIME_ADD(
    SceneObject.h
    SceneObject.cpp

    Actor.h
    Actor.cpp
    ActorsCache.h
    ActorsCache.cpp
    SceneObjectsFactory.h
    SceneObjectsFactory.cpp
    SceneQuery.h
    SceneQuery.cpp

    Level.h
    Level.cpp

)
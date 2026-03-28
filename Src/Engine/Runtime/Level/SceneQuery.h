#pragma once
#include "Level.h"
#include "Core/Types/Collections/List.h"

namespace SE
{
    class SceneObject;
    class Actor;

    /// <summary>
    /// Helper class to perform scene actions and queries
    /// </summary>
    class SE_API_RUNTIME SceneQuery
    {
    public:
        /// <summary>
        /// Gets all scene objects from the actor into linear list. Appends them (without the given actor).
        /// </summary>
        /// <param name="root">The root actor.</param>
        /// <param name="objects">The objects output.</param>
        static void GetAllSceneObjects(Actor* root, List<SceneObject*>& objects);

        /// <summary>
        /// Gets all scene objects (for serialization) from the actor into linear list. Appends them (including the given actor).
        /// </summary>
        /// <param name="root">The root actor.</param>
        /// <param name="objects">The objects output.</param>
        static void GetAllSerializableSceneObjects(Actor* root, List<SceneObject*>& objects);

        /// <summary>
        /// Gets all actors from the actor into linear list. Appends them (without the given actor).
        /// </summary>
        /// <param name="root">The root actor.</param>
        /// <param name="actors">The actors output.</param>
        static void GetAllActors(Actor* root, List<Actor*>& actors);

        /// <summary>
        /// Gets all actors from the loaded scenes into linear list (without scene actors).
        /// </summary>
        /// <param name="actors">The actors output.</param>
        static void GetAllActors(List<Actor*>& actors);

    public:

        /// <summary>
        /// Execute custom action on actors tree.
        /// </summary>
        /// <param name="action">Actor to call on every actor in the tree. Returns true if keep calling deeper.</param>
        /// <param name="args">Custom arguments for the function</param>
        template<typename... Params>
        static void TreeExecute(Function<bool(Actor*, Params...)>& action, Params... args)
        {
#if SCENE_QUERIES_WITH_LOCK
            ScopeLock lock(Level::ScenesLock);
#endif
            for (int32 i = 0; i < Level::Scenes.Count(); i++)
            {
                Scene* scene = Level::Scenes.Get()[i];
                if (scene != nullptr)
                {
                    scene->TreeExecute<Params...>(action, args...);
                }
            }
        }
    };

} // SE


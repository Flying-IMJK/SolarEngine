#pragma once

#include "Runtime/API.h"
#include "Core/Types/Delegate.h"

namespace SE
{
    class ScriptingObject;
    class SEClass;

    /// <summary>
    /// Global scripting manager.
    ///
    /// Responsibilities:
    ///   - Maintains the global object registry (ID → ScriptingObject*).
    ///   - Provides FindObject() lookup by GUID and optional type filter.
    ///   - Handles ManagedInstanceDeleted callbacks from the C# side.
    ///   - Fires ScriptsReloading / ScriptsReloaded events during hot-reload (editor only).
    ///
    /// </summary>
    class SE_API_RUNTIME Scripting
    {
    public:
        // -----------------------------------------------------------------------
        // Lifecycle
        // -----------------------------------------------------------------------

        /// <summary>
        /// Initializes the scripting system (CLR host, type caches, etc.).
        /// Returns false on success, true on failure.
        /// </summary>
        static bool Initialize();

        /// <summary>
        /// Shuts down the scripting system and releases all managed resources.
        /// </summary>
        static void Shutdown();

        /// <summary>
        /// Registers a ScriptingObject in the global registry.
        /// The object must have a valid, unique ID.
        /// </summary>
        static void RegisterObject(ScriptingObject* obj);

        /// <summary>
        /// Removes a ScriptingObject from the global registry.
        /// </summary>
        static void UnregisterObject(ScriptingObject* obj);

        /// <summary>
        /// Finds a registered ScriptingObject by its unique ID.
        /// Returns nullptr if not found.
        /// </summary>
        static ScriptingObject* TryFindObject(const UID& id);

        /// <summary>
        /// Finds a registered ScriptingObject by its unique ID and optional type filter.
        /// If type is non-null, the object's managed class must be a subclass of type.
        /// Returns nullptr if not found or type doesn't match.
        /// </summary>
        static ScriptingObject* FindObject(const UID& id, SEClass* type = nullptr);

        // -----------------------------------------------------------------------
        // Managed instance callbacks
        // -----------------------------------------------------------------------

        /// <summary>
        /// Called by the C# side when a managed ScriptingObject instance is deleted by the GC.
        /// Forwards to obj->OnManagedInstanceDeleted().
        /// </summary>
        static void OnManagedInstanceDeleted(ScriptingObject* obj);

        /// <summary>
        /// Called when a ScriptingObject's ID changes (updates the registry key).
        /// </summary>
        static void OnObjectIdChanged(ScriptingObject* obj, const UID& oldId);

        // -----------------------------------------------------------------------
        // Events (editor hot-reload)
        // -----------------------------------------------------------------------

        /// <summary>Fired before scripts are reloaded (editor only).</summary>
        static Delegate<> ScriptsReloading;

        /// <summary>Fired after scripts have been reloaded (editor only).</summary>
        static Delegate<> ScriptsReloaded;

#ifdef SE_EDITOR
        /// <summary>Triggers a script hot-reload cycle (editor only).</summary>
        static void ReloadScripts();
#endif
    };

} // namespace SE

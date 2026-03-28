

#include "Scripting.h"
#include "Runtime/Scripting/ScriptingObject.h"
#include "Runtime/Scripting/ManagedCLR/SEClass.h"
#include "Core/Logging/Logging.h"
#include "Core/Types/Collections/Dictionary.h"
#include "Core/Thread/Threading.h"

namespace SE
{
    // =========================================================================
    // Internal state
    // =========================================================================

    namespace
    {
        /// <summary>
        /// Global registry: maps UID → ScriptingObject*.
        /// Protected by s_RegistryMutex for thread-safe access.
        /// </summary>
        Dictionary<uint64, ScriptingObject*> s_ObjectRegistry;

        /// <summary>Mutex protecting s_ObjectRegistry.</summary>
        CriticalSection s_RegistryMutex;

        /// <summary>Converts a UID to a uint64 key for the hash map.</summary>
        inline uint64 UIDToKey(const UID& id)
        {
            // Combine the four 32-bit components into a 64-bit key.
            // Use XOR-shift to reduce collisions.
            uint64 lo = (static_cast<uint64>(id.A) << 32) | id.B;
            uint64 hi = (static_cast<uint64>(id.C) << 32) | id.D;
            return lo ^ (hi * 2654435761ULL);
        }
    }

    // =========================================================================
    // Static member definitions
    // =========================================================================

    Delegate<> Scripting::ScriptsReloading;
    Delegate<> Scripting::ScriptsReloaded;

    // =========================================================================
    // Lifecycle
    // =========================================================================

    bool Scripting::Initialize()
    {
        LOG_INFO("Scripting", "Scripting::Initialize — scripting system starting up.");
        return false; // success
    }

    void Scripting::Shutdown()
    {
        LOG_INFO("Scripting", "Scripting::Shutdown — scripting system shutting down.");


        Threading::ScopeLock lock(s_RegistryMutex);
        if (s_ObjectRegistry.Count() > 0)
        {
            LOG_WARNING("Scripting", "Scripting::Shutdown — {0} object(s) still registered at shutdown.", s_ObjectRegistry.Count());
        }
        s_ObjectRegistry.Clear();
    }

    // =========================================================================
    // Object registry
    // =========================================================================

    void Scripting::RegisterObject(ScriptingObject* obj)
    {
        if (!obj)
            return;

        const UID& id = obj->GetID();
        if (!id.IsValid())
        {
            LOG_WARNING("Scripting", "Scripting::RegisterObject — object has invalid ID, skipping registration.");
            return;
        }

        Threading::ScopeLock lock(s_RegistryMutex);
        uint64 key = UIDToKey(id);

        if (s_ObjectRegistry.ContainsKey(key))
        {
            LOG_WARNING("Scripting", "Scripting::RegisterObject — ID collision detected, overwriting existing entry.");
        }
        s_ObjectRegistry[key] = obj;
    }

    void Scripting::UnregisterObject(ScriptingObject* obj)
    {
        if (!obj)
            return;

        const UID& id = obj->GetID();
        Threading::ScopeLock lock(s_RegistryMutex);
        uint64 key = UIDToKey(id);
        s_ObjectRegistry.Remove(key);
    }

    ScriptingObject* Scripting::TryFindObject(const UID& id)
    {
        if (!id.IsValid())
            return nullptr;

        Threading::ScopeLock lock(s_RegistryMutex);
        uint64 key = UIDToKey(id);
        ScriptingObject* scriptingObject;
        if (s_ObjectRegistry.TryGet(key, scriptingObject))
        {
            return scriptingObject;
        }
        return nullptr;
    }

    ScriptingObject* Scripting::FindObject(const UID& id, SEClass* type)
    {
        ScriptingObject* obj = TryFindObject(id);
        if (!obj)
            return nullptr;

        // If a type filter is provided, check that the object's class is a subclass.
        if (type)
        {
            SEClass* objClass = obj->GetClass();
            if (!objClass || !objClass->IsSubClassOf(type))
            {
                LOG_WARNING("Scripting", "Scripting::FindObject — found object with ID but type doesn't match.");
                return nullptr;
            }
        }

        return obj;
    }

    // =========================================================================
    // Managed instance callbacks
    // =========================================================================

    void Scripting::OnManagedInstanceDeleted(ScriptingObject* obj)
    {
        if (!obj)
            return;
        obj->OnManagedInstanceDeleted();
    }

    void Scripting::OnObjectIdChanged(ScriptingObject* obj, const UID& oldId)
    {
        if (!obj)
        {
            return;
        }

        Threading::ScopeLock lock(s_RegistryMutex);

        // Remove old entry.
        uint64 oldKey = UIDToKey(oldId);
        s_ObjectRegistry.Remove(oldKey);

        // Insert new entry.
        const UID& newId = obj->GetID();
        if (newId.IsValid())
        {
            uint64 newKey = UIDToKey(newId);
            s_ObjectRegistry[newKey] = obj;
        }
    }

    // =========================================================================
    // Editor hot-reload
    // =========================================================================

#ifdef SE_EDITOR
    void Scripting::ReloadScripts()
    {
        LOG_INFO("Scripting", "Scripting::ReloadScripts — beginning script hot-reload.");
        ScriptsReloading();
        // TODO (Task 14): Unload old assembly, load new assembly.
        ScriptsReloaded();
    }
#endif

} // namespace SE

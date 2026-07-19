#pragma once

#include "Runtime/Core/Types/Delegate.h"

#include "Runtime/Core/Scripting/ScriptingType.h"
#include "Runtime/API.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRTypes.h"

namespace SE
{
    namespace Threading
    {
        template<typename T, int32 MaxThreads>
        class ThreadLocal;
    }

    struct ScriptingTypeHandle;
    class BinaryModule;
    class ScriptingObject;

    /// <summary>
    /// Embedded managed scripting runtime service.
    /// </summary>
    class SE_API_RUNTIME Scripting
    {
        friend ScriptingObject;
        friend BinaryModule;
        SCRIPTING_TYPE_NO_SPAWN(Scripting);
    public:

        /// <summary>
        /// Action fired when scripting loads a binary module (eg. with game scripts).
        /// </summary>
        static Delegate<BinaryModule*> BinaryModuleLoaded;

        /// <summary>
        /// Action fired on scripting engine loaded (always main thread).
        /// </summary>
        static Delegate<> ScriptsLoaded;

        /// <summary>
        /// Action fired on scripting engine unloading start (always main thread).
        /// </summary>
        static Delegate<> ScriptsUnload;

        /// <summary>
        /// Action fired on scripting engine reload start (always main thread).
        /// </summary>
        static Delegate<> ScriptsReloading;

        /// <summary>
        /// Action fired on scripting engine reload start (always main thread).
        /// </summary>
        static Delegate<> ScriptsReloaded;

    public:

        /// <summary>
        /// Gets the root domain.
        /// </summary>
        static CLRDomain* GetRootDomain();

        /// <summary>
        /// Gets the scripts domain (it can be the root domain if not using separate domain for scripting).
        /// </summary>
        static CLRDomain* GetScriptsDomain();

    public:

        /// <summary>
        /// Load/Reload scripts now
        /// </summary>
        /// <returns>True if failed or cannot be done, otherwise false</returns>
        static bool Load();

        /// <summary>
        /// Release scripting layer (will destroy internal scripts data)
        /// </summary>
        static void Release();

#if SE_EDITOR
        /// <summary>
        /// Reloads scripts.
        /// </summary>
        /// <param name="canTriggerSceneReload">True if allow to scene scripts reload callback, otherwise it won't be possible.</param>
        static void Reload(bool canTriggerSceneReload = true);
#endif

    public:

        /// <summary>
        /// Gets all registered scripting objects.
        /// </summary>
        /// <remarks>Use with caution due to potentially large memory allocation.</remarks>
        /// <returns>The collection of the objects.</returns>
        static List<ScriptingObject*, HeapAllocation> GetObjects();

        /// <summary>
        /// Finds the class with given fully qualified name within whole assembly.
        /// </summary>
        /// <param name="fullname">The full name of the type eg: System.Int64.</param>
        /// <returns>The SEClass object or null if missing.</returns>
        static CLRClass* FindClass(const StringAnsiView& fullname);

        /// <summary>
        /// Finds the scripting type of the given fullname by searching loaded scripting assemblies.
        /// </summary>
        /// <param name="fullname">The full name of the type eg: System.Int64.</param>
        /// <returns>The scripting type or invalid type if missing.</returns>
        static ScriptingTypeHandle FindScriptingType(const StringAnsiView& fullname);

        /// <summary>
        /// Creates a new instance of the given type object (native construction).
        /// </summary>
        /// <param name="type">The scripting object type class.</param>
        /// <returns>The created object or null if failed.</returns>
        static ScriptingObject* NewObject(const ScriptingTypeHandle& type);

        /// <summary>
        /// Creates a new instance of the given class object (native construction).
        /// </summary>
        /// <param name="type">The Managed type class.</param>
        /// <returns>The created object or null if failed.</returns>
        static ScriptingObject* NewObject(const CLRClass* type);

    public:

        typedef Dictionary<UID, UID, HeapAllocation> IdsMappingTable;

        /// <summary>
        /// The objects lookup identifier mapping used to override the object ids on FindObject call (used by the object references deserialization).
        /// </summary>
        static Threading::ThreadLocal<IdsMappingTable*, PLATFORM_THREADS_LIMIT> ObjectsLookupIdMapping;

        /// <summary>
        /// Finds the object by the given identifier. Searches registered scene objects and optionally assets. Logs warning if fails.
        /// </summary>
        /// <param name="id">The object unique identifier.</param>
        /// <returns>The found object or null if missing.</returns>
        template<typename T>
        FORCE_INLINE static T* FindObject(const UID& id)
        {
            return (T*)FindObject(id, T::GetStaticClass());
        }

        /// <summary>
        /// Finds the object by the given identifier. Searches registered scene objects and optionally assets. Logs warning if fails.
        /// </summary>
        /// <param name="id">The object unique identifier.</param>
        /// <param name="type">The type of the object to find (optional).</param>
        /// <returns>The found object or null if missing.</returns>
        static ScriptingObject* FindObject(UID id, const CLRClass* type = nullptr);

        /// <summary>
        /// Tries to find the object by the given class.
        /// </summary>
        /// <param name="type">The type of the object to find.</param>
        /// <returns>The found object or null if missing.</returns>
        static ScriptingObject* TryFindObject(const CLRClass* type);

        /// <summary>
        /// Tries to find the object by the given identifier.
        /// </summary>
        /// <param name="id">The object unique identifier.</param>
        /// <returns>The found object or null if missing.</returns>
        template<typename T>
        FORCE_INLINE static T* TryFindObject(const UID& id)
        {
            return (T*)TryFindObject(id, T::GetStaticClass());
        }

        /// <summary>
        /// Tries to find the object by the given identifier.
        /// </summary>
        /// <param name="id">The object unique identifier.</param>
        /// <param name="type">The type of the object to find (optional).</param>
        /// <returns>The found object or null if missing.</returns>
        static ScriptingObject* TryFindObject(UID id, const CLRClass* type = nullptr);

        /// <summary>
        /// Finds the object by the given managed instance handle. Searches only registered scene objects.
        /// </summary>
        /// <param name="managedInstance">The managed instance pointer.</param>
        /// <returns>The found object or null if missing.</returns>
        static ScriptingObject* FindObject(const CLRObject* managedInstance);

        /// <summary>
        /// Event called by the internal call on a finalizer thread when the managed objects gets deleted by the GC.
        /// </summary>
        /// <param name="obj">The unmanaged object pointer that was related to the managed object.</param>
        static void OnManagedInstanceDeleted(ScriptingObject* obj);

    public:

        /// <summary>
        /// Returns true if game modules are loaded.
        /// </summary>
        static bool HasGameModulesLoaded();

        /// <summary>
        /// Returns true if every assembly is loaded.
        /// </summary>
        static bool IsEveryAssemblyLoaded();

        /// <summary>
        /// Returns true if given type is from one of the game scripts assemblies.
        /// </summary>
        static bool IsTypeFromGameScripts(const CLRClass* type);

        static void ProcessBuildInfoPath(String& path, const String& projectFolderPath);

    private:

        static bool LoadBinaryModules(const String& path, const String& projectFolderPath);

        // Scripting Object API
        static void RegisterObject(ScriptingObject* obj);
        static void UnregisterObject(ScriptingObject* obj);
        static void OnObjectIdChanged(ScriptingObject* obj, const UID& oldId);
    };
} // namespace SE

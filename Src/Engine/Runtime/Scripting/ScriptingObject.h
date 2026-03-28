#pragma once

#include "Runtime/API.h"
#include "Core/Types/Object.h"
#include "Core/Types/Delegate.h"
#include "Core/Types/UID.h"
#include "Runtime/Scripting/ScriptingTypeHandle.h"
#include "Runtime/Scripting/ManagedCLR/SEGCHandle.h"

namespace SE
{
    class SEObject;
    class SEClass;

    /// <summary>
    /// Parameters used when spawning a ScriptingObject.
    /// </summary>
    struct SE_API_RUNTIME ScriptingObjectSpawnParams
    {
        /// <summary>The unique object ID.</summary>
        UID ID;

        /// <summary>The scripting type handle for this object.</summary>
        ScriptingTypeHandle Type;

        ScriptingObjectSpawnParams(const UID& id, const ScriptingTypeHandle& type)
            : ID(id)
            , Type(type)
        {
        }
    };

    /// <summary>
    /// Base class for C++ objects that have a corresponding C# managed instance.
    /// Bridges C++ object lifetime with C# GC-managed objects via GC handles.
    ///
    /// Lifecycle:
    ///   1. Constructed with SpawnParams (ID + type handle)
    ///   2. CreateManaged() creates the C# instance and stores a strong GC handle
    ///   3. RegisterObject() adds to the global registry
    ///   4. UnregisterObject() removes from the global registry
    ///   5. DestroyManaged() releases the GC handle and notifies C# side
    ///   6. Destructor asserts _gcHandle == 0 (no leaks)
    /// </summary>
    class SE_API_RUNTIME ScriptingObject : public Object
    {
        SE_CLASS(ScriptingObject, Object)

    public:
        typedef ScriptingObjectSpawnParams SpawnParams;

    protected:
        /// <summary>GC handle to the corresponding C# managed instance.</summary>
        GCHandle _gcHandle;

        /// <summary>Scripting type handle identifying the C++ type.</summary>
        ScriptingTypeHandle _type;

        /// <summary>Unique object identifier (shared between C++ and C# sides).</summary>
        UID _id;

    public:
        /// <summary>
        /// Constructs a ScriptingObject with the given spawn parameters.
        /// </summary>
        explicit ScriptingObject(const SpawnParams& params);

        /// <summary>
        /// Destructor — asserts that the GC handle has been released (no leaks).
        /// </summary>
        ~ScriptingObject() override;

    public:
        /// <summary>
        /// Gets the current managed instance (may be null if not yet created or already destroyed).
        /// Does NOT create the managed instance if missing.
        /// </summary>
        SEObject* GetManagedInstance() const;

        /// <summary>
        /// Gets the managed instance, creating it lazily if not yet present.
        /// </summary>
        SEObject* GetOrCreateManagedInstance() const;

        /// <summary>
        /// Returns true if the managed instance is currently alive.
        /// </summary>
        FORCE_INLINE bool HasManagedInstance() const
        {
            return GetManagedInstance() != nullptr;
        }

        /// <summary>
        /// Gets the SEClass descriptor for this object's C# type.
        /// </summary>
        SEClass* GetClass() const;

        /// <summary>
        /// Gets the unique object ID.
        /// </summary>
        FORCE_INLINE const UID& GetID() const { return _id; }

        /// <summary>
        /// Gets the scripting type handle.
        /// </summary>
        FORCE_INLINE const ScriptingTypeHandle& GetTypeHandle() const { return _type; }

    public:
        /// <summary>
        /// Creates the corresponding C# managed instance and establishes the GC handle link.
        /// Returns false on success, true on failure.
        /// </summary>
        virtual bool CreateManaged();

        /// <summary>
        /// Destroys the managed instance: calls OnScriptingDispose() on C# side and frees the GC handle.
        /// </summary>
        virtual void DestroyManaged();

        /// <summary>
        /// Sets the managed instance directly (used when C# creates the object first).
        /// Stores a strong GC handle.
        /// </summary>
        virtual void SetManagedInstance(SEObject* instance);

        /// <summary>
        /// Called when the managed instance is deleted (GC handle becomes invalid).
        /// Base implementation releases the GC handle and unregisters the object.
        /// </summary>
        virtual void OnManagedInstanceDeleted();

        /// <summary>
        /// Called by C# side when the scripting object is being disposed.
        /// Base implementation unregisters and destroys the managed side.
        /// </summary>
        virtual void OnScriptingDispose();

    public:
        /// <summary>
        /// Returns true if this object is registered in the global object registry.
        /// </summary>
        bool IsRegistered() const;

        /// <summary>
        /// Registers this object in the global object registry.
        /// Must not be called if already registered.
        /// </summary>
        void RegisterObject();

        /// <summary>
        /// Unregisters this object from the global object registry.
        /// Must not be called if not registered.
        /// </summary>
        void UnregisterObject();

    public:
        /// <summary>
        /// Checks if a managed type can be cast from one class to another.
        /// </summary>
        static bool CanCast(const SEClass* from, const SEClass* to);

        /// <summary>
        /// Retrieves the C++ ScriptingObject* from a managed C# object's __unmanagedPtr field.
        /// </summary>
        static ScriptingObject* ToNative(SEObject* obj);

        /// <summary>
        /// Gets (or creates) the managed instance for a C++ ScriptingObject.
        /// </summary>
        FORCE_INLINE static SEObject* ToManaged(const ScriptingObject* obj)
        {
            return obj ? obj->GetOrCreateManagedInstance() : nullptr;
        }

    public:
        /// <summary>Event fired when this object is deleted.</summary>
        Delegate<ScriptingObject*> Deleted;

    public:
        // [Object]
        String ToString() const override;
        void OnDeleteObject() override;

    protected:
        /// <summary>
        /// Internal helper: creates the managed instance via SECore::ScriptingObjectHelper.
        /// </summary>
        SEObject* CreateManagedInternal();

    private:
        bool _isRegistered;
    };

    /// <summary>
    /// ScriptingObject variant whose lifetime is controlled by the C# GC.
    /// Uses a weak GC handle so the GC can collect the managed instance.
    /// When the managed instance is collected, OnManagedInstanceDeleted() is called
    /// which deletes this C++ object as well.
    /// </summary>
    class SE_API_RUNTIME ManagedScriptingObject : public ScriptingObject
    {
        SE_CLASS(ManagedScriptingObject, ScriptingObject)

    public:
        explicit ManagedScriptingObject(const SpawnParams& params);

    public:
        // [ScriptingObject]
        void SetManagedInstance(SEObject* instance) override;
        void OnManagedInstanceDeleted() override;
        void OnScriptingDispose() override;
        bool CreateManaged() override;
    };

    /// <summary>
    /// Finds a registered ScriptingObject by its unique ID and optional type filter.
    /// Returns nullptr if not found or type doesn't match.
    /// </summary>
    extern SE_API_RUNTIME ScriptingObject* FindObject(const UID& id, SEClass* type = nullptr);

} // namespace SE

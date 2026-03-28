// ScriptingObject.cpp — C++ base class for objects with C# managed counterparts.
// Implements lifecycle management: creation, GC handle tracking, registration, destruction.
//

#include "ScriptingObject.h"
#include "Scripting.h"
#include "Runtime/Scripting/ManagedCLR/SECore.h"
#include "Runtime/Scripting/ManagedCLR/SEClass.h"
#include "Runtime/Scripting/ManagedCLR/SEObject.h"
#include "Core/Logging/Logging.h"

namespace SE
{
    // =========================================================================
    // ScriptingObject
    // =========================================================================

    ScriptingObject::ScriptingObject(const SpawnParams& params)
        : _gcHandle(GCHandle::Null)
        , _type(params.Type)
        , _id(params.ID)
        , _isRegistered(false)
    {
        // Managed objects must have a valid unique ID.
        ENGINE_ASSERT(_id.IsValid());
    }

    ScriptingObject::~ScriptingObject()
    {
        Deleted(this);

        // Release managed side.
        ScriptingObject::DestroyManaged();

        // Detect GC handle leaks in debug builds.
        ENGINE_ASSERT(!_gcHandle.IsValid());

        // Ensure the object has been unregistered.
        if (IsRegistered())
            UnregisterObject();
    }

    // -------------------------------------------------------------------------
    // Managed instance access
    // -------------------------------------------------------------------------

    SEObject* ScriptingObject::GetManagedInstance() const
    {
        if (!_gcHandle.IsValid())
            return nullptr;
        return SECore::Handle::GetTarget(_gcHandle);
    }

    SEObject* ScriptingObject::GetOrCreateManagedInstance() const
    {
        SEObject* instance = GetManagedInstance();
        if (!instance)
        {
            const_cast<ScriptingObject*>(this)->CreateManaged();
            instance = GetManagedInstance();
        }
        return instance;
    }

    SEClass* ScriptingObject::GetClass() const
    {
        // TODO (Task 7): return _type.GetType().ManagedClass once ScriptingType is fully implemented.
        return nullptr;
    }

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    bool ScriptingObject::CreateManaged()
    {
        SEObject* managedInstance = CreateManagedInternal();
        if (managedInstance != nullptr)
            return true; // failure

        // Store a strong GC handle — C++ controls the lifetime.
        GCHandle handle = SECore::Handle::New(managedInstance, false);
        if (!handle.IsValid())
        {
            LOG_ERROR("Scripting", "ScriptingObject::CreateManaged — failed to create GC handle.");
            return true;
        }

        // Only set if not already set (thread-safety: first writer wins).
        if (_gcHandle.IsValid())
        {
            // Another thread already created the managed instance.
            SEClass* klass = GetClass();
            if (klass)
                SECore::ScriptingObjectHelper::SetInternalValues(klass, managedInstance, nullptr, nullptr);
            SECore::Handle::Free(handle);
            return false; // not a failure — already created
        }

        _gcHandle = handle;

        // Ensure the object is registered.
        if (!IsRegistered())
            RegisterObject();

        return false; // success
    }

    void ScriptingObject::DestroyManaged()
    {
        SEObject* managedInstance = GetManagedInstance();

        // Clear the C++ pointer in the managed instance so it can't call back.
        if (managedInstance)
        {
            SEClass* klass = GetClass();
            if (klass)
                SECore::ScriptingObjectHelper::SetInternalValues(klass, managedInstance, nullptr, nullptr);
        }

        // Free the GC handle.
        if (_gcHandle.IsValid())
        {
            SECore::Handle::Free(_gcHandle);
            _gcHandle = GCHandle::Null;
        }
    }

    void ScriptingObject::SetManagedInstance(SEObject* instance)
    {
        ENGINE_ASSERT(!_gcHandle.IsValid());
        if (!instance)
            return;
        // Strong GC handle — C++ controls lifetime.
        _gcHandle = SECore::Handle::New(instance, false);
    }

    void ScriptingObject::OnManagedInstanceDeleted()
    {
        // Release the GC handle.
        if (_gcHandle.IsValid())
        {
            SECore::Handle::Free(_gcHandle);
            _gcHandle = GCHandle::Null;
        }

        // Unregister from the global registry.
        if (IsRegistered())
            UnregisterObject();
    }

    void ScriptingObject::OnScriptingDispose()
    {
        if (IsRegistered())
            UnregisterObject();
        DestroyManaged();
    }

    // -------------------------------------------------------------------------
    // Registration
    // -------------------------------------------------------------------------

    bool ScriptingObject::IsRegistered() const
    {
        return _isRegistered;
    }

    void ScriptingObject::RegisterObject()
    {
        ENGINE_ASSERT(!_isRegistered);
        _isRegistered = true;
        Scripting::RegisterObject(this);
    }

    void ScriptingObject::UnregisterObject()
    {
        ENGINE_ASSERT(_isRegistered);
        _isRegistered = false;
        Scripting::UnregisterObject(this);
    }

    // -------------------------------------------------------------------------
    // Static helpers
    // -------------------------------------------------------------------------

    bool ScriptingObject::CanCast(const SEClass* from, const SEClass* to)
    {
        if (!from && !to)
            return true;
        if (!from || !to)
            return false;
        return from->IsSubClassOf(to);
    }

    ScriptingObject* ScriptingObject::ToNative(SEObject* obj)
    {
        if (!obj)
            return nullptr;
        // TODO (Task 4): read __unmanagedPtr field via NativeInterop.
        // For now return nullptr as a safe stub.
        return nullptr;
    }

    // -------------------------------------------------------------------------
    // Object overrides
    // -------------------------------------------------------------------------

    String ScriptingObject::ToString() const
    {
        return String::Empty;
    }

    void ScriptingObject::OnDeleteObject()
    {
        DestroyManaged();
        if (IsRegistered())
            UnregisterObject();
        Object::OnDeleteObject();
    }

    // -------------------------------------------------------------------------
    // Internal helpers
    // -------------------------------------------------------------------------

    SEObject* ScriptingObject::CreateManagedInternal()
    {
        SEClass* klass = GetClass();
        if (!klass)
        {
            LOG_WARNING("Scripting", "ScriptingObject::CreateManagedInternal — missing managed class for object.");
            return nullptr;
        }

        SEObject* instance = SECore::ScriptingObjectHelper::CreateScriptingObject(klass, this, &_id);
        if (!instance)
        {
            LOG_WARNING("Scripting", "ScriptingObject::CreateManagedInternal — failed to create managed instance.");
        }
        return instance;
    }

    // =========================================================================
    // ManagedScriptingObject
    // =========================================================================

    ManagedScriptingObject::ManagedScriptingObject(const SpawnParams& params)
        : ScriptingObject(params)
    {
    }

    void ManagedScriptingObject::SetManagedInstance(SEObject* instance)
    {
        ENGINE_ASSERT(!_gcHandle.IsValid());
        if (!instance)
            return;
        // Weak GC handle — C# GC controls lifetime.
        _gcHandle = SECore::Handle::NewWeak(instance);
    }

    void ManagedScriptingObject::OnManagedInstanceDeleted()
    {
        // Base: release handle and unregister.
        ScriptingObject::OnManagedInstanceDeleted();

        // Self-destruct: C# GC collected the managed instance, so delete the C++ object too.
        DeleteObject();
    }

    void ManagedScriptingObject::OnScriptingDispose()
    {
        // Base: unregister and destroy managed side.
        ScriptingObject::OnScriptingDispose();

        // Self-destruct.
        DeleteObject();
    }

    bool ManagedScriptingObject::CreateManaged()
    {
        SEObject* managedInstance = CreateManagedInternal();
        if (!managedInstance)
            return true; // failure

        // Weak GC handle — GC can collect the managed instance.
        GCHandle handle = SECore::Handle::NewWeak(managedInstance);
        if (!handle.IsValid())
        {
            LOG_ERROR("Scripting", "ManagedScriptingObject::CreateManaged — failed to create weak GC handle.");
            return true;
        }

        if (_gcHandle.IsValid())
        {
            // Another thread already created the managed instance.
            SEClass* klass = GetClass();
            if (klass)
                SECore::ScriptingObjectHelper::SetInternalValues(klass, managedInstance, nullptr, nullptr);
            SECore::Handle::Free(handle);
            return false;
        }

        _gcHandle = handle;

        if (!IsRegistered())
            RegisterObject();

        return false; // success
    }

    // =========================================================================
    // Global FindObject
    // =========================================================================

    ScriptingObject* FindObject(const UID& id, SEClass* type)
    {
        return Scripting::FindObject(id, type);
    }

} // namespace SE

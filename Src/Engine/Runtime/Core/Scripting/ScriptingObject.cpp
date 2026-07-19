
#include "ScriptingObject.h"
#include "Scripting.h"
#include "Runtime/Core/Scripting/Binary/BinaryModule.h"
#include "Runtime/Core/Scripting/Binary/ManagedBinaryModule.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRCore.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRClass.h"
#include "Runtime/Core/Scripting/Internal/InternalCalls.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRField.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRUtils.h"
#include "Runtime/Level/Actor.h"

#include "Runtime/Core/Logging/Logging.h"
#include "Runtime/Core/Profiler/ProfilerCPU.h"
#include "Runtime/Core/Thread/ThreadLocal.h"
#include "Runtime/Core/Types/Strings/StringConverter.h"
#include "Runtime/Core/Types/Collections/Dictionary.h"

namespace SE
{
#define ScriptingObject_unmanagedPtr "__unmanagedPtr"
#define ScriptingObject_id "__internalId"

    typedef Pair<ScriptingObject*, ScriptingTypeHandle> ScriptingObjectsInterfaceKey;
    Dictionary<ScriptingObjectsInterfaceKey, void*> ScriptingObjectsInterfaceWrappers;

    // =========================================================================
    // ScriptingObject
    // =========================================================================

    ScriptingObject::ScriptingObject(const SpawnParams& params) : m_GcHandle(0), m_Type(params.Type), m_Id(params.ID)
    {
        // Managed objects must have valid and unique ID
        ENGINE_ASSERT(m_Id.IsValid());
    }

    ScriptingObject::~ScriptingObject()
    {
        Deleted(this);

        // Get rid of managed object
        ScriptingObject::DestroyManaged();
        ENGINE_ASSERT(m_GcHandle == 0);

        // Handle custom scripting objects removing
        /*if (EnumHasAnyFlags(Flags, ObjectFlags::IsCustomScriptingType))
        {
            _type.Module->OnObjectDeleted(this);
        }*/

        // Ensure that object has been already unregistered
        if (IsRegistered())
        {
            UnregisterObject();
        }
    }

    ScriptingObject* ScriptingObject::NewObject(const ScriptingTypeHandle& typeHandle)
    {
        if (!typeHandle)
        {
            return nullptr;
        }
        auto& type = typeHandle.GetType();
        if (type.Type != ScriptingTypes::Script)
        {
            return nullptr;
        }
        const ScriptingObjectSpawnParams params(UID::New(), typeHandle);
        return type.Script.Spawn(params);
    }

    CLRObject* ScriptingObject::GetManagedInstance() const
    {
        const CLRGCHandle handle = Platform::AtomicRead((int64*)&m_GcHandle);

        return handle ? CLRCore::GCHandle::GetTarget(handle) : nullptr;
    }

    CLRObject* ScriptingObject::GetOrCreateManagedInstance() const
    {
        CLRObject* managedInstance = GetManagedInstance();
        if (!managedInstance)
        {
            const_cast<ScriptingObject*>(this)->CreateManaged();
            managedInstance = GetManagedInstance();
        }
        return managedInstance;
    }

    CLRClass* ScriptingObject::GetClass() const
    {
        return m_Type ? m_Type.GetType().ManagedClass : nullptr;
    }

    ScriptingObject* ScriptingObject::FromInterface(void* interfaceObj, const ScriptingTypeHandle& interfaceType)
    {
        if (!interfaceObj || !interfaceType)
            return nullptr;
        PROFILE_CPU();

        // Find the type which implements this interface and has the same vtable as interface object
        // TODO: implement vtableInterface->type hashmap caching in Scripting service to optimize sequential interface casts
        auto& modules = BinaryModule::GetModules();
        for (auto module : modules)
        {
            for (auto& type : module->Types)
            {
                if (type.Type != ScriptingTypes::Script)
                    continue;
                auto interfaceImpl = type.GetInterface(interfaceType);
                if (!interfaceImpl || !interfaceImpl->IsNative)
                    continue;

                // Get vtable for this type
                void* vtable = type.Script.VTable;
                if (!vtable && type.GetDefaultInstance())
                {
                    // Use vtable from default instance of this type
                    vtable = *(void***)type.GetDefaultInstance();
                }

                // Check if object interface vtable matches the type interface vtable value
                ScriptingObject* predictedObj = (ScriptingObject*)((byte*)interfaceObj - interfaceImpl->VTableOffset);
                void* predictedVTable = *(void***)predictedObj;
                if (vtable == predictedVTable)
                {
                    ENGINE_ASSERT(predictedObj->GetScriptType().GetInterface(interfaceType));
                    return predictedObj;
                }

                // Check for case of passing object directly
                predictedObj = (ScriptingObject*)interfaceObj;
                predictedVTable = *(void***)predictedObj;
                if (vtable == predictedVTable)
                {
                    ENGINE_ASSERT(predictedObj->GetScriptType().GetInterface(interfaceType));
                    return predictedObj;
                }
            }
        }

        // Special case for interface wrapper object
        for (const auto& e : ScriptingObjectsInterfaceWrappers)
        {
            if (e.Value == interfaceObj)
                return e.Key.First;
        }

        return nullptr;
    }

    void* ScriptingObject::ToInterface(ScriptingObject* obj, const ScriptingTypeHandle& interfaceType)
    {
        if (!obj || !interfaceType)
            return nullptr;
        const ScriptingType& objectType = obj->GetScriptType();
        const ScriptingType::InterfaceImplementation* interface = objectType.GetInterface(interfaceType);
        void* result = nullptr;
        if (interface && interface->IsNative)
        {
            // Native interface so just offset pointer to the interface vtable start
            result = (byte*)obj + interface->VTableOffset;
        }
        else if (interface)
        {
            // Interface implemented in scripting (eg. C# class inherits C++ interface)
            const ScriptingObjectsInterfaceKey key(obj, interfaceType);
            if (!ScriptingObjectsInterfaceWrappers.TryGet(key, result))
            {
                result = interfaceType.GetType().Interface.GetInterfaceWrapper(obj);
                ScriptingObjectsInterfaceWrappers.Add(key, result);
            }
        }
        return result;
    }

    ScriptingObject* ScriptingObject::ToNative(CLRObject* obj)
    {
        ScriptingObject* ptr = nullptr;
        if (obj)
        {
            static const CLRField* ptrField = CLRCore::Object::GetClass(obj)->GetField(ScriptingObject_unmanagedPtr);
            ptrField->GetValueReference(obj, &ptr);
        }

        return ptr;
    }

    bool ScriptingObject::Is(const ScriptingTypeHandle& type) const
    {
        if (!type)
        {
            return false;
        }

        return m_Type == type || CanCast(GetClass(), type.GetType().ManagedClass);
    }

    void ScriptingObject::ChangeID(const UID& newId)
    {
        ENGINE_ASSERT(newId.IsValid() && newId != m_Id);

        const UID prevId = m_Id;
        m_Id = newId;

        // Update managed instance
        const auto managedInstance = GetManagedInstance();
        const auto monoClass = GetClass();
        if (managedInstance && monoClass)
        {
            const CLRField* monoIdField = monoClass->GetField(ScriptingObject_id);
            if (monoIdField)
                monoIdField->SetValue(managedInstance, &m_Id);
        }

        // Update scripting
        if (IsRegistered())
            Scripting::OnObjectIdChanged(this, prevId);
        m_Type.GetType().Module->OnObjectIdChanged(this, prevId);
    }

    void ScriptingObject::SetManagedInstance(CLRObject* instance)
    {
        ENGINE_ASSERT(m_GcHandle == 0);
        m_GcHandle = (CLRGCHandle)instance;
    }

    void ScriptingObject::OnManagedInstanceDeleted()
    {
        // Release the handle
        if (m_GcHandle)
        {
            CLRCore::GCHandle::Free(m_GcHandle);
            m_GcHandle = 0;
        }

        // Unregister object
        if (IsRegistered())
            UnregisterObject();
    }

    void ScriptingObject::OnScriptingDispose()
    {
        // Delete C# object
        if (IsRegistered())
            UnregisterObject();
        DestroyManaged();
    }

    bool ScriptingObject::CreateManaged()
    {
        CLRObject* managedInstance = CreateManagedInternal();
        if (!managedInstance)
        {
            return false;
        }

        // Prevent from object GC destruction
        auto handle = (CLRGCHandle)managedInstance;
        auto oldHandle = Platform::AtomicCompareExchange((int64*)&m_GcHandle, *(int64*)&handle, 0);
        if (*(uint64*)&oldHandle != 0)
        {
            // Other thread already created the object before
            if (const auto monoClass = GetClass())
            {
                // Reset managed to unmanaged pointer
                CLRCore::ScriptingObject::SetInternalValues(monoClass, managedInstance, nullptr, nullptr);
            }
            CLRCore::GCHandle::Free(handle);
            return false;
        }

        // Ensure to be registered
        if (!IsRegistered())
        {
            RegisterObject();
        }

        return true;
    }

    CLRObject* ScriptingObject::CreateManagedInternal()
    {
        // Get class
        CLRClass* monoClass = GetClass();
        if (monoClass == nullptr)
        {
            LOG_WARNING("Scripting", "Missing managed class for object with id {0}", GetID());
            return nullptr;
        }

        CLRObject* managedInstance = CLRCore::ScriptingObject::CreateScriptingObject(monoClass, this, &m_Id);
        if (managedInstance == nullptr)
        {
            LOG_WARNING("Scripting", "Failed to create new instance of the object of type {0}", String(monoClass->GetFullName()));
        }

        return managedInstance;
    }



    void ScriptingObject::DestroyManaged()
    {
        // Get managed instance
        const auto managedInstance = GetManagedInstance();

        // Reset managed to unmanaged pointer
        if (managedInstance)
        {
            if (const auto monoClass = GetClass())
            {
                CLRCore::ScriptingObject::SetInternalValues(monoClass, managedInstance, nullptr, nullptr);
            }
        }

        // Clear the handle
        if (m_GcHandle)
        {
            CLRCore::GCHandle::Free(m_GcHandle);
            m_GcHandle = 0;
        }
    }

    void ScriptingObject::RegisterObject()
    {
        ENGINE_ASSERT(!IsRegistered());
        // Flags |= ObjectFlags::IsRegistered;
        Scripting::RegisterObject(this);
    }

    void ScriptingObject::UnregisterObject()
    {
        ENGINE_ASSERT(IsRegistered());
        // Flags &= ~ObjectFlags::IsRegistered;
        Scripting::UnregisterObject(this);
    }

    bool ScriptingObject::CanCast(const ScriptingTypeHandle& from, const ScriptingTypeHandle& to)
    {
        if (!from && !to)
            return true;

        if (!(from && to))
        {
            return false;
        }

        return CanCast(from.GetType().ManagedClass, to.GetType().ManagedClass);
    }

    bool ScriptingObject::CanCast(const CLRClass* from, const CLRClass* to)
    {
        if (!from && !to)
        {
            return true;
        }

        if (!(from && to))
        {
            return false;
        }

        return from->IsSubClassOf(to);
    }

    void ScriptingObject::OnDeleteObject()
    {
        // Cleanup managed object
        DestroyManaged();

        // Unregister
        if (IsRegistered())
            UnregisterObject();

        // Base
        Object::OnDeleteObject();
    }

    String ScriptingObject::ToString() const
    {
        return m_Type ? String(m_Type.GetType().Fullname) : String::Empty;
    }

    // =========================================================================
    // ManagedScriptingObject
    // =========================================================================

    ManagedScriptingObject::ManagedScriptingObject(const SpawnParams& params) : ScriptingObject(params)
    {
    }

    void ManagedScriptingObject::SetManagedInstance(CLRObject* instance)
    {
        ENGINE_ASSERT(m_GcHandle == 0);

        m_GcHandle = (CLRGCHandle)instance;
    }

    void ManagedScriptingObject::OnManagedInstanceDeleted()
    {
        // Base
        ScriptingObject::OnManagedInstanceDeleted();

        // Self destruct
        DeleteObject();
    }

    void ManagedScriptingObject::OnScriptingDispose()
    {
        // Base
        ScriptingObject::OnScriptingDispose();

        // Self destruct
        DeleteObject();
    }

    bool ManagedScriptingObject::CreateManaged()
    {
        CLRObject* managedInstance = CreateManagedInternal();
        if (!managedInstance)
            return true;

        // Cache the GC handle to the object (used to track the target object because it can be moved in a memory)
        auto handle = (CLRGCHandle)managedInstance;
        auto oldHandle = Platform::AtomicCompareExchange((int64*)&m_GcHandle, *(int64*)&handle, 0);
        if (*(uint64*)&oldHandle != 0)
        {
            // Other thread already created the object before
            if (const auto monoClass = GetClass())
            {
                // Reset managed to unmanaged pointer
                CLRCore::ScriptingObject::SetInternalValues(monoClass, managedInstance, nullptr, nullptr);
            }
            CLRCore::GCHandle::Free(handle);
            return true;
        }

        // Ensure to be registered
        if (!IsRegistered())
            RegisterObject();

        return false;
    }
    

    DEFINE_INTERNAL_CALL(CLRObject*) ObjectInternal_Create1(CLRTypeObject* type)
    {
        // Peek class for that type (handle generic class cases)
        ENGINE_ASSERT(type != nullptr);
        /*if (!type)
        {
            DebugLog::ThrowArgumentNull("type");
        }*/
        CLRType* mType = type; //INTERNAL_TYPE_OBJECT_GET(type);
        const CLRTypes mTypeType = CLRCore::Type::GetType(mType);
        if (mTypeType == CLRTypes::GenericInst)
        {
            LOG_ERROR("Scripting", "Generic scripts are not supported.");
            return nullptr;
        }
        CLRClass* typeClass = CLRCore::Type::GetClass(mType);
        if (typeClass == nullptr)
        {
            LOG_ERROR("Scripting", "Invalid type.");
            return nullptr;
        }

        // Get the assembly with that class
        const auto module = ManagedBinaryModule::FindModule(typeClass);
        if (module == nullptr)
        {
            LOG_ERROR("Scripting", "Cannot find scripting assembly for type \'{0}\'.", typeClass->GetFullName());
            return nullptr;
        }

        // Try to find the scripting type for this class
        int32 typeIndex;
        if (!module->ClassToTypeIndex.TryGet(typeClass, typeIndex))
        {
            LOG_ERROR("Scripting", "Cannot spawn objects of type \'{0}\'.", typeClass->GetFullName());
            return nullptr;
        }
        const ScriptingType& scriptingType = module->Types[typeIndex];

        // Create unmanaged object
        const ScriptingObjectSpawnParams params(UID::New(), ScriptingTypeHandle(module, typeIndex));
        ScriptingObject* obj = scriptingType.Script.Spawn(params);
        if (obj == nullptr)
        {
            LOG_ERROR("Scripting", "Failed to spawn object of type \'{0}\'.", typeClass->GetFullName());
            return nullptr;
        }

        // Set default name for actors
        if (auto* actor = dynamic_cast<Actor*>(obj))
        {
            actor->SetName(String(typeClass->GetName()));
        }

        // Create managed object
        obj->CreateManaged();
        CLRObject* managedInstance = obj->GetManagedInstance();
        if (managedInstance == nullptr)
        {
            LOG_ERROR("Scripting", "Cannot create managed instance for type \'{0}\'.", typeClass->GetFullName());
            Delete(obj);
        }

        return managedInstance;
    }

    DEFINE_INTERNAL_CALL(CLRObject*) ObjectInternal_Create2(CLRString* typeNameObj)
    {
        // Get typename
        ENGINE_ASSERT(typeNameObj != nullptr);
        
        const StringView typeNameChars = CLRCore::String::GetChars(typeNameObj);
        const StringAsANSI<100> typeNameData(typeNameChars.Get(), typeNameChars.Length());
        const StringAnsiView typeName(typeNameData.Get(), typeNameChars.Length());

        // Try to find the scripting type for this typename
        const ScriptingTypeHandle type = Scripting::FindScriptingType(typeName);
        if (!type)
        {
            LOG_ERROR("Scripting", "Cannot find scripting type for \'{0}\'.", typeName);
            return nullptr;
        }

        // Create unmanaged object
        const ScriptingObjectSpawnParams params(UID::New(), type);
        ScriptingObject* obj = type.GetType().Script.Spawn(params);
        if (obj == nullptr)
        {
            LOG_ERROR("Scripting", "Failed to spawn object of type \'{0}\'.", typeName);
            return nullptr;
        }

        // Create managed object
        obj->CreateManaged();
        CLRObject* managedInstance = obj->GetManagedInstance();
        if (managedInstance == nullptr)
        {
            LOG_ERROR("Scripting", "Cannot create managed instance for type \'{0}\'.", typeName);
            Delete(obj);
        }

        return managedInstance;
    }

    DEFINE_INTERNAL_CALL(void) ObjectInternal_ManagedInstanceCreated(CLRObject* managedInstance, CLRClass* typeClass)
    {
        // Get the assembly with that class
        auto module = ManagedBinaryModule::FindModule(typeClass);
        if (module == nullptr)
        {
            LOG_ERROR("Scripting", "Cannot find scripting assembly for type \'{0}\'.", typeClass->GetFullName());
            return;
        }

        // Try to find the scripting type for this class
        int32 typeIndex;
        if (!module->ClassToTypeIndex.TryGet(typeClass, typeIndex))
        {
            LOG_ERROR("Scripting", "Cannot spawn objects of type \'{0}\'.", typeClass->GetFullName());
            return;
        }
        const ScriptingType& scriptingType = module->Types[typeIndex];

        // Create unmanaged object
        const ScriptingObjectSpawnParams params(UID::New(), ScriptingTypeHandle(module, typeIndex));
        ScriptingObject* obj = scriptingType.Script.Spawn(params);
        if (obj == nullptr)
        {
            LOG_ERROR("Scripting", "Failed to spawn object of type \'{0}\'.", typeClass->GetFullName());
            return;
        }

        // Link created managed instance to the unmanaged object
        obj->SetManagedInstance(managedInstance);

        // Set default name for actors
        if (auto* actor = dynamic_cast<Actor*>(obj))
        {
            actor->SetName(String(typeClass->GetName()));
        }

        CLRClass* monoClass = obj->GetClass();
        const UID id = obj->GetID();
        CLRCore::ScriptingObject::SetInternalValues(monoClass, managedInstance, obj, &id);

        // Register object
        if (!obj->IsRegistered())
        {
            obj->RegisterObject();
        }
    }

    DEFINE_INTERNAL_CALL(void) ObjectInternal_ManagedInstanceDeleted(ScriptingObject* obj)
    {
        Scripting::OnManagedInstanceDeleted(obj);
    }

    DEFINE_INTERNAL_CALL(void) ObjectInternal_Destroy(ScriptingObject* obj, float timeLeft)
    {
        // Use scaled game time for removing actors/scripts by the user (maybe expose it to the api?)
        const bool useGameTime = timeLeft > Math::ZeroTolerance;

        if (obj)
        {
            obj->DeleteObject(timeLeft, useGameTime);
        }
    }

    DEFINE_INTERNAL_CALL(void) ObjectInternal_DestroyNow(ScriptingObject* obj)
    {
        if (obj)
        {
            obj->DeleteObjectNow();
        }
    }

    DEFINE_INTERNAL_CALL(CLRString*) ObjectInternal_GetTypeName(ScriptingObject* obj)
    {
        // INTERNAL_CALL_CHECK_RETURN(obj, nullptr);
        return CLRUtils::ToString(obj->GetType().ToString());
    }

    DEFINE_INTERNAL_CALL(CLRObject*) ObjectInternal_FindObject(UID* id, CLRTypeObject* type)
    {
        if (!id->IsValid())
            return nullptr;
        CLRClass* klass = CLRUtils::GetClass(type);
        ScriptingObject* obj = Scripting::TryFindObject(*id);
        if (!obj)
        {
            if (!klass || klass == ScriptingObject::GetScriptingClass() || klass->IsSubClassOf(Asset::GetScriptingClass()))
            {
                obj = AssetContent::LoadAsync<Asset>(*id);
            }
        }
        if (obj)
        {
            if (klass && !obj->Is(klass))
            {
                LOG_WARNING("Scripting", "Found scripting object with ID={0} of type {1} that doesn't match type {2}.", *id, obj->GetType().ToString(), klass->GetFullName());
                return nullptr;
            }
            return obj->GetOrCreateManagedInstance();
        }
        if (klass)
        {
            LOG_WARNING("Scripting", "Unable to find scripting object with ID={0}. Required type {1}.", *id, klass->GetFullName());
        }
        else
        {
            LOG_WARNING("Scripting", "Unable to find scripting object with ID={0}", *id);
        }
        return nullptr;
    }

    DEFINE_INTERNAL_CALL(CLRObject*) ObjectInternal_TryFindObject(UID* id, CLRTypeObject* type)
    {
        ScriptingObject* obj = Scripting::TryFindObject(*id);
        if (obj && !obj->Is(CLRUtils::GetClass(type)))
        {
            obj = nullptr;
        }
        return obj ? obj->GetOrCreateManagedInstance() : nullptr;
    }

    DEFINE_INTERNAL_CALL(void) ObjectInternal_ChangeID(ScriptingObject* obj, UID* id)
    {
        // INTERNAL_CALL_CHECK(obj);
        obj->ChangeID(*id);
    }

    DEFINE_INTERNAL_CALL(void*) ObjectInternal_GetUnmanagedInterface(ScriptingObject* obj, CLRTypeObject* type)
    {
        if (obj && type)
        {
            CLRClass* typeClass = CLRUtils::GetClass(type);
            const ScriptingTypeHandle interfaceType = ManagedBinaryModule::FindType(typeClass);
            if (interfaceType)
            {
                return ScriptingObject::ToInterface(obj, interfaceType);
            }
        }
        return nullptr;
    }

    DEFINE_INTERNAL_CALL(CLRObject*) ObjectInternal_FromUnmanagedPtr(ScriptingObject* obj)
    {
        CLRObject* result = nullptr;
        if (obj)
            result = obj->GetOrCreateManagedInstance();
        return result;
    }

    DEFINE_INTERNAL_CALL(void) ObjectInternal_MapObjectID(UID* id)
    {
        const auto idsMapping = Scripting::ObjectsLookupIdMapping.Get();
        if (idsMapping && id->IsValid())
            idsMapping->TryGet(*id, *id);
    }

    DEFINE_INTERNAL_CALL(void) ObjectInternal_RemapObjectID(UID* id)
    {
        const auto idsMapping = Scripting::ObjectsLookupIdMapping.Get();
        if (idsMapping && id->IsValid())
            idsMapping->KeyOf(*id, id);
    }

    class ScriptingObjectInternal
    {
    public:
        static void InitRuntime()
        {
            ADD_INTERNAL_CALL("SE.Object::Internal_Create1", &ObjectInternal_Create1);
            ADD_INTERNAL_CALL("SE.Object::Internal_Create2", &ObjectInternal_Create2);
            ADD_INTERNAL_CALL("SE.Object::Internal_ManagedInstanceCreated", &ObjectInternal_ManagedInstanceCreated);
            ADD_INTERNAL_CALL("SE.Object::Internal_ManagedInstanceDeleted", &ObjectInternal_ManagedInstanceDeleted);
            ADD_INTERNAL_CALL("SE.Object::Internal_Destroy", &ObjectInternal_Destroy);
            ADD_INTERNAL_CALL("SE.Object::Internal_DestroyNow", &ObjectInternal_DestroyNow);
            ADD_INTERNAL_CALL("SE.Object::Internal_GetTypeName", &ObjectInternal_GetTypeName);
            ADD_INTERNAL_CALL("SE.Object::Internal_FindObject", &ObjectInternal_FindObject);
            ADD_INTERNAL_CALL("SE.Object::Internal_TryFindObject", &ObjectInternal_TryFindObject);
            ADD_INTERNAL_CALL("SE.Object::Internal_ChangeID", &ObjectInternal_ChangeID);
            ADD_INTERNAL_CALL("SE.Object::Internal_GetUnmanagedInterface", &ObjectInternal_GetUnmanagedInterface);
            ADD_INTERNAL_CALL("SE.Object::FromUnmanagedPtr", &ObjectInternal_FromUnmanagedPtr);
            ADD_INTERNAL_CALL("SE.Object::MapObjectID", &ObjectInternal_MapObjectID);
            ADD_INTERNAL_CALL("SE.Object::RemapObjectID", &ObjectInternal_RemapObjectID);
        }

        static ScriptingObject* Spawn(const ScriptingObjectSpawnParams& params)
        {
            return New<ScriptingObject>(params);
        }
    };

    extern "C" BinaryModule* GetBinaryModuleSERuntime();

    ScriptingTypeInitializer ScriptingObject::TypeInitializer(GetBinaryModuleSERuntime(), StringAnsiView("SE.Object", ARRAY_SIZE("SE.Object") - 1), sizeof(ScriptingObject), &ScriptingObjectInternal::InitRuntime, &ScriptingObjectInternal::Spawn);
} // namespace SE

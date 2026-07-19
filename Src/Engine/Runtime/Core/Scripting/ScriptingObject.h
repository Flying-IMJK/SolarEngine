#pragma once

#include "ScriptingType.h"
#include "Runtime/API.h"
#include "Runtime/Core/Types/Object.h"
#include "Runtime/Core/Types/Delegate.h"
#include "Runtime/Core/Types/UID.h"

namespace SE
{
    /// <summary>
    /// Represents object from unmanaged memory that can use accessed via scripting.
    /// </summary>
    SE_CLASS(Reflect)
    class SE_API_RUNTIME ScriptingObject : public Object
    {
        friend class Scripting;
        friend class BinaryModule;
        friend class ScriptingObjectInternal;
        SE_DEFINE_CLASS(ScriptingObject, Object)
        SCRIPTING_TYPE_NO_SPAWN(ScriptingObject);

    public:
        typedef ScriptingObjectSpawnParams SpawnParams;

    protected:
        CLRGCHandle m_GcHandle;
        ScriptingTypeHandle m_Type;
        UID m_Id;

    public:
        /// <summary>
        /// Initializes a new instance of the <see cref="ScriptingObject"/> class.
        /// </summary>
        /// <param name="params">The object initialization parameters.</param>
        explicit ScriptingObject(const SpawnParams& params);

        /// <summary>
        /// Finalizes an instance of the <see cref="ScriptingObject"/> class.
        /// </summary>
        ~ScriptingObject() override;

    public:
        // Spawns a new objects of the given type.
        static ScriptingObject* NewObject(const ScriptingTypeHandle& typeHandle);

        template<typename T>
        static T* NewObject()
        {
            return (T*)NewObject(T::TypeInitializer);
        }

        template<typename T>
        static T* NewObject(const ScriptingTypeHandle& typeHandle)
        {
            auto obj = NewObject(typeHandle);
            if (obj && !obj->Is<T>())
            {
                Delete(obj);
                obj = nullptr;
            }
            return (T*)obj;
        }

    public:
        /// <summary>
        /// Event fired when object gets deleted.
        /// </summary>
        Delegate<ScriptingObject*> Deleted;

    public:
        /// <summary>
        /// Gets the managed instance object.
        /// </summary>
        CLRObject* GetManagedInstance() const;

        /// <summary>
        /// Gets the managed instance object or creates it if missing.
        /// </summary>
        CLRObject* GetOrCreateManagedInstance() const;

        /// <summary>
        /// Determines whether managed instance is alive.
        /// </summary>
        FORCE_INLINE bool HasManagedInstance() const
        {
            return GetManagedInstance() != nullptr;
        }

        /// <summary>
        /// Gets the unique object ID.
        /// </summary>
        FORCE_INLINE const UID& GetID() const
        {
            return m_Id;
        }

        /// <summary>
        /// Gets the scripting type handle of this object.
        /// </summary>
        FORCE_INLINE const ScriptingTypeHandle& GetTypeHandle() const
        {
            return m_Type;
        }

        /// <summary>
        /// Gets the scripting type of this object.
        /// </summary>
        FORCE_INLINE const ScriptingType& GetScriptType() const
        {
            return m_Type.GetType();
        }

        /// <summary>
        /// Gets the type class of this object.
        /// </summary>
        CLRClass* GetClass() const;

    public:
        // Tries to cast native interface object to scripting object instance. Returns null if fails.
        static ScriptingObject* FromInterface(void* interfaceObj, const ScriptingTypeHandle& interfaceType);

        template<typename T>
        static ScriptingObject* FromInterface(T* interfaceObj)
        {
            return FromInterface(interfaceObj, T::TypeInitializer);
        }

        static void* ToInterface(ScriptingObject* obj, const ScriptingTypeHandle& interfaceType);

        template<typename T>
        static T* ToInterface(ScriptingObject* obj)
        {
            return (T*)ToInterface(obj, T::TypeInitializer);
        }

        static ScriptingObject* ToNative(CLRObject* obj);

        FORCE_INLINE static CLRObject* ToManaged(const ScriptingObject* obj)
        {
            return obj ? obj->GetOrCreateManagedInstance() : nullptr;
        }

        FORCE_INLINE static CLRObject* ToManaged(ScriptingObject* obj)
        {
            return obj ? obj->GetOrCreateManagedInstance() : nullptr;
        }

        /// <summary>
        /// Checks if can cast one scripting object type into another type.
        /// </summary>
        /// <param name="from">The object type for the cast.</param>
        /// <param name="to">The destination type to the cast.</param>
        /// <returns>True if can, otherwise false.</returns>
        static bool CanCast(const ScriptingTypeHandle& from, const ScriptingTypeHandle& to);

        /// <summary>
        /// Checks if can cast one scripting object type into another type.
        /// </summary>
        /// <param name="from">The object class for the cast.</param>
        /// <param name="to">The destination class to the cast.</param>
        /// <returns>True if can, otherwise false.</returns>
        static bool CanCast(const CLRClass* from, const CLRClass* to);

        template<typename T>
        static T* Cast(ScriptingObject* obj)
        {
            return obj && CanCast(obj->GetClass(), T::GetStaticClass()) ? static_cast<T*>(obj) : nullptr;
        }

        bool Is(const ScriptingTypeHandle& type) const;

        bool Is(const CLRClass* type) const
        {
            return CanCast(GetClass(), type);
        }

        template<typename T>
        bool Is() const
        {
            return CanCast(GetClass(), T::GetStaticClass());
        }

    public:
        /// <summary>
        /// Changes the object id (both managed and unmanaged). Warning! Use with caution as object ID is what it identifies it and change might cause issues.
        /// </summary>
        /// <param name="newId">The new ID.</param>
        virtual void ChangeID(const UID& newId);

    public:
        virtual void SetManagedInstance(CLRObject* instance);
        virtual void OnManagedInstanceDeleted();
        virtual void OnScriptingDispose();

        virtual bool CreateManaged();
        virtual void DestroyManaged();

    public:
        /// <summary>
        /// Determines whether this object is registered or not (can be found by the queries and used in a game).
        /// </summary>
        FORCE_INLINE bool IsRegistered() const
        {
            return false;// (Flags & ObjectFlags::IsRegistered) != ObjectFlags::None;
        }

        /// <summary>
        /// Registers the object (cannot be called when objects has been already registered).
        /// </summary>
        void RegisterObject();

        /// <summary>
        /// Unregisters the object (cannot be called when objects has not been already registered).
        /// </summary>
        void UnregisterObject();

    protected:

        /// <summary>
        /// Create a new managed object.
        /// </summary>
        CLRObject* CreateManagedInternal();

    public:
        // [Object]
        void OnDeleteObject() override;
        String ToString() const override;
    };

    /// <summary>
    /// Managed object that uses weak GC handle to track the target object location in memory. 
    /// Can be destroyed by the GC.
    /// Used by the objects that lifetime is controlled by the C# side.
    /// </summary>
    SE_CLASS(Reflect)
    class SE_API_RUNTIME ManagedScriptingObject : public ScriptingObject
    {
        SE_DEFINE_CLASS(ManagedScriptingObject, ScriptingObject)
    public:
        /// <summary>
        /// Initializes a new instance of the <see cref="ManagedScriptingObject"/> class.
        /// </summary>
        /// <param name="params">The object initialization parameters.</param>
        explicit ManagedScriptingObject(const SpawnParams& params);

    public:
        // [ScriptingObject]
        void SetManagedInstance(CLRObject* instance) override;
        void OnManagedInstanceDeleted() override;
        void OnScriptingDispose() override;
        bool CreateManaged() override;
    };

    extern SE_API_RUNTIME class ScriptingObject* FindObject(const UID& id, class MClass* type);
    
} // namespace SE

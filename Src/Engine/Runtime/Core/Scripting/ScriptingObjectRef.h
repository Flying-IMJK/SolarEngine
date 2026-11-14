#pragma once

#include <Runtime/API.h>
#include "Runtime/Core/Serialization/ISerializeModifier.h"
#include <Runtime/Core/Scripting/ScriptingObject.h>
#include <Runtime/Core/Serialization/Serialization.h>

namespace SE
{
    /// <summary>
    /// The scripting object reference.
    /// </summary>
    /// <typeparam name="T">The type of the scripting object.</typeparam>
    class SE_API_RUNTIME ScriptingObjectRefBase
    {
    public:
        typedef Delegate<> EventType;

    protected:
        ScriptingObject* m_Object = nullptr;

    public:
        /// <summary>
        /// Action fired when reference gets changed.
        /// </summary>
        EventType Changed;

    public:
        /// <summary>
        /// Initializes a new instance of the <see cref="ScriptingObjectRefBase"/> class.
        /// </summary>
        ScriptingObjectRefBase() {}

        ScriptingObjectRefBase(ScriptingObjectRefBase&& other) noexcept : m_Object(nullptr)
        {
            OnSet(other.m_Object);
            other.OnSet(nullptr);
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ScriptingObjectRefBase"/> class.
        /// </summary>
        /// <param name="obj">The object to link.</param>
        ScriptingObjectRefBase(ScriptingObject* obj) { OnSet(obj); }

        /// <summary>
        /// Finalizes an instance of the <see cref="ScriptingObjectRef"/> class.
        /// </summary>
        ~ScriptingObjectRefBase()
        {
            ScriptingObject* obj = m_Object;
            if (obj)
            {
                m_Object = nullptr;
                obj->Deleted.Unbind<ScriptingObjectRefBase, &ScriptingObjectRefBase::OnDeleted>(this);
            }
        }

    public:
        /// <summary>
        /// Gets the object ID.
        /// </summary>
        FORCE_INLINE UID GetID() const { return m_Object ? m_Object->GetID() : UID::Empty; }

        /// <summary>
        /// Gets managed instance object (or null if no object linked).
        /// </summary>
        FORCE_INLINE CLRObject* GetManagedInstance() const { return m_Object ? m_Object->GetOrCreateManagedInstance() : nullptr; }

        /// <summary>
        /// Determines whether object is assigned and managed instance of the object is alive.
        /// </summary>
        FORCE_INLINE bool HasManagedInstance() const { return m_Object && m_Object->HasManagedInstance(); }

        /// <summary>
        /// Gets the managed instance object or creates it if missing or null if not assigned.
        /// </summary>
        FORCE_INLINE CLRObject* GetOrCreateManagedInstance() const { return m_Object ? m_Object->GetOrCreateManagedInstance() : nullptr; }

    protected:
        /// <summary>
        /// Sets the object.
        /// </summary>
        /// <param name="object">The object.</param>
        void OnSet(ScriptingObject* object);

        void OnDeleted(ScriptingObject* obj);

        ScriptingObjectRefBase& operator=(ScriptingObjectRefBase&& other) noexcept
        {
            if (this != &other)
            {
                OnSet(other.m_Object);
                other.OnSet(nullptr);
            }
            return *this;
        }
    };

    /// <summary>
    /// The scripting object reference.
    /// </summary>
    SE_CLASS(API(InBuild))
    template<typename T>
    class ScriptingObjectRef : public ScriptingObjectRefBase
    {
    public:
        typedef ScriptingObjectRef<T> Type;

    public:
        /// <summary>
        /// Initializes a new instance of the <see cref="ScriptingObjectRef"/> class.
        /// </summary>
        ScriptingObjectRef() {}

        /// <summary>
        /// Initializes a new instance of the <see cref="ScriptingObjectRef"/> class.
        /// </summary>
        /// <param name="obj">The object to link.</param>
        ScriptingObjectRef(T* obj) : ScriptingObjectRefBase(obj) {}

        /// <summary>
        /// Initializes a new instance of the <see cref="ScriptingObjectRef"/> class.
        /// </summary>
        /// <param name="other">The other property.</param>
        ScriptingObjectRef(const ScriptingObjectRef& other) : ScriptingObjectRefBase(other.m_Object) {}

        ScriptingObjectRef(ScriptingObjectRef&& other) noexcept : ScriptingObjectRefBase(MoveTemp(other)) {}

        /// <summary>
        /// Finalizes an instance of the <see cref="ScriptingObjectRef"/> class.
        /// </summary>
        ~ScriptingObjectRef() {}

    public:
        FORCE_INLINE bool operator==(T* other) const { return m_Object == other; }

        FORCE_INLINE bool operator!=(T* other) const { return m_Object != other; }

        FORCE_INLINE bool operator==(const ScriptingObjectRef& other) const { return m_Object == other._object; }

        FORCE_INLINE bool operator!=(const ScriptingObjectRef& other) const { return m_Object != other._object; }

        FORCE_INLINE ScriptingObjectRef& operator=(T* other)
        {
            OnSet(other);
            return *this;
        }

        ScriptingObjectRef& operator=(const ScriptingObjectRef& other)
        {
            OnSet(other.m_Object);
            return *this;
        }

        ScriptingObjectRef& operator=(ScriptingObjectRef&& other) noexcept
        {
            ScriptingObjectRefBase::operator=(MoveTemp(other));
            return *this;
        }

        FORCE_INLINE ScriptingObjectRef& operator=(const UID& id)
        {
            OnSet(static_cast<ScriptingObject*>(FindObject(id, T::GetScriptingClass())));
            return *this;
        }

        /// <summary>
        /// Implicit conversion to the object.
        /// </summary>
        FORCE_INLINE operator T*() const { return (T*)m_Object; }

        /// <summary>
        /// Implicit conversion to boolean value.
        /// </summary>
        FORCE_INLINE operator bool() const { return m_Object != nullptr; }

        /// <summary>
        /// Object accessor.
        /// </summary>
        FORCE_INLINE T* operator->() const { return (T*)m_Object; }

        /// <summary>
        /// Gets the object pointer.
        /// </summary>
        FORCE_INLINE T* Get() const { return (T*)m_Object; }

        /// <summary>
        /// Gets the object as a given type (static cast).
        /// </summary>
        template<typename U>
        FORCE_INLINE U* As() const
        {
            return static_cast<U*>(m_Object);
        }
    };

    template<typename T>
    uint32 GetHash(const ScriptingObjectRef<T>& key)
    {
        return GetHash(key.GetID());
    }

    namespace Serialization
    {
        template<typename T>
        inline bool ShouldSerialize(const ScriptingObjectRef<T>& v, const void* otherObj)
        {
            return !otherObj || ShouldSerializeRef(v.Get(), ((ScriptingObjectRef<T>*)otherObj)->Get());
        }

        template<typename T>
        inline void Serialize(SerializeContext& context, const ScriptingObjectRef<T>& v)
        {
            context.stream.UUID(v.GetID());
        }

        template<typename T>
        inline void Deserialize(DeserializeContext& context, ScriptingObjectRef<T>& v)
        {
            UID id;
            Deserialize(context, id);
            context.modifier->IdsMapping.TryGet(id, id);
            v = id;
        }

    } // namespace Serialization

} // namespace SE
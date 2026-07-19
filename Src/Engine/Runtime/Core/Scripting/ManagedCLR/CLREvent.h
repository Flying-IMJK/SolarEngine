#pragma once

#include "CLRMethod.h"

namespace SE
{
    /// <summary>
    /// Encapsulates information about a single Mono (managed) event belonging to some managed class. This object also allows you to invoke this event or register other methods to it.
    /// </summary>
    class SE_API_RUNTIME CLREvent
    {
        friend CLRClass;

    protected:
        void* _handle;
        mutable CLRMethod* _addMethod;
        mutable CLRMethod* _removeMethod;
        CLRClass* _parentClass;

        StringAnsi _name;

        mutable int32 _hasCachedAttributes : 1;
        mutable int32 _hasAddMonoMethod : 1;
        mutable int32 _hasRemoveMonoMethod : 1;

        mutable List<CLRObject*> _attributes;

    public:

        CLREvent(CLRClass* parentClass, void* handle, const char* name);

        /// <summary>
        /// Gets the event name.
        /// </summary>
        FORCE_INLINE const StringAnsi& GetName() const
        {
            return _name;
        }

        /// <summary>
        /// Returns the parent class that this method is contained with.
        /// </summary>
        FORCE_INLINE CLRClass* GetParentClass() const
        {
            return _parentClass;
        }

        /// <summary>
        /// Gets the event type class.
        /// </summary>
        CLRType* GetType() const;

        /// <summary>
        /// Gets the event add method.
        /// </summary>
        CLRMethod* GetAddMethod() const;

        /// <summary>
        /// Gets the event remove method.
        /// </summary>
        CLRMethod* GetRemoveMethod() const;

        /// <summary>
        /// Gets event visibility in the class.
        /// </summary>
        FORCE_INLINE CLRVisibility GetVisibility() const
        {
            return GetAddMethod()->GetVisibility();
        }

        /// <summary>
        /// Returns true if event is static.
        /// </summary>
        FORCE_INLINE bool IsStatic() const
        {
            return GetAddMethod()->IsStatic();
        }

    public:
        /// <summary>
        /// Checks if event has an attribute of the specified type.
        /// </summary>
        /// <param name="monoClass">The attribute class to check.</param>
        /// <returns>True if has attribute of that class type, otherwise false.</returns>
        bool HasAttribute(CLRClass* monoClass) const;

        /// <summary>
        /// Checks if event has an attribute of any type.
        /// </summary>
        /// <returns>True if has any custom attribute, otherwise false.</returns>
        bool HasAttribute() const;

        /// <summary>
        /// Returns an instance of an attribute of the specified type. Returns null if the event doesn't have such an attribute.
        /// </summary>
        /// <param name="monoClass">The attribute class to take.</param>
        /// <returns>The attribute object.</returns>
        CLRObject* GetAttribute(CLRClass* monoClass) const;

        /// <summary>
        /// Returns an instance of all attributes connected with given event. Returns null if the event doesn't have any attributes.
        /// </summary>
        /// <returns>The array of attribute objects.</returns>
        const List<CLRObject*>& GetAttributes() const;
    };
}

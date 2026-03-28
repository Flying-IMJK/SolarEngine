#pragma once

#include "SETypes.h"
#include "Runtime/API.h"

namespace SE
{
    /// <summary>
    /// Contains information about a single managed class.
    /// </summary>
    class SE_API_RUNTIME SEClass
    {
        friend SECore;

    private:
        void* m_Handle;
        StringAnsi m_Name;
        StringAnsi m_Namespace;
        uint32 m_Types = 0;
        mutable uint32 m_Size = 0;

        const SEAssembly* m_Assembly;

        StringAnsi m_Fullname;

        mutable List<SEMethod*> m_Methods;
        mutable List<SEField*> m_Fields;
        mutable List<SEProperty*> m_Properties;
        mutable List<SEObject*> m_Attributes;
        mutable List<SEEvent*> m_Events;
        mutable List<SEClass*> m_Interfaces;

        SEVisibility m_Visibility;

        mutable int32 m_HasCachedProperties : 1;
        mutable int32 m_HasCachedFields : 1;
        mutable int32 m_HasCachedMethods : 1;
        mutable int32 m_HasCachedAttributes : 1;
        mutable int32 m_HasCachedEvents : 1;
        mutable int32 m_HasCachedInterfaces : 1;
        int32 m_IsStatic : 1;
        int32 m_IsSealed : 1;
        int32 m_IsAbstract : 1;
        int32 m_IsInterface : 1;
        int32 m_IsValueType : 1;
        int32 m_IsEnum : 1;

    public:
        SEClass(const SEAssembly* parentAssembly, void* handle, const char* name, const char* fullname, const char* namespace_, SETypeAttributes typeAttributes);

        /// <summary>
        /// Finalizes an instance of the <see cref="SEClass"/> class.
        /// </summary>
        ~SEClass();

    public:
        /// <summary>
        /// Gets the parent assembly.
        /// </summary>
        const SEAssembly* GetAssembly() const
        {
            return m_Assembly;
        }

        /// <summary>
        /// Gets the full name of the class (namespace and typename).
        /// </summary>
        FORCE_INLINE const StringAnsi& GetFullName() const
        {
            return m_Fullname;
        }

        /// <summary>
        /// Gets the name of the class.
        /// </summary>
        StringAnsiView GetName() const;

        /// <summary>
        /// Gets the namespace of the class.
        /// </summary>
        StringAnsiView GetNamespace() const;


        FORCE_INLINE void* GetNative() const
        {
            return m_Handle;
        }

        /// <summary>
        /// Gets class visibility
        /// </summary>
        FORCE_INLINE SEVisibility GetVisibility() const
        {
            return m_Visibility;
        }

        /// <summary>
        /// Gets if class is static
        /// </summary>
        FORCE_INLINE bool IsStatic() const
        {
            return m_IsStatic != 0;
        }

        /// <summary>
        /// Gets if class is abstract
        /// </summary>
        FORCE_INLINE bool IsAbstract() const
        {
            return m_IsAbstract != 0;
        }

        /// <summary>
        /// Gets if class is sealed
        /// </summary>
        FORCE_INLINE bool IsSealed() const
        {
            return m_IsSealed != 0;
        }

        /// <summary>
        /// Gets if class is interface
        /// </summary>
        FORCE_INLINE bool IsInterface() const
        {
            return m_IsInterface != 0;
        }

        /// <summary>
        /// Gets if class is value type (eg. enum or structure) but not reference type (eg. class, string, array, interface)
        /// </summary>
        FORCE_INLINE bool IsValueType() const
        {
            return m_IsValueType != 0;
        }

        /// <summary>
        /// Gets if class is enumeration
        /// </summary>
        FORCE_INLINE bool IsEnum() const
        {
            return m_IsEnum != 0;
        }

        /// <summary>
        /// Gets if class is generic
        /// </summary>
        bool IsGeneric() const
        {
            return m_Fullname.FindLast('`') != -1;
        }

        /// <summary>
        /// Gets the class type.
        /// </summary>
        SEType* GetType() const;

        /// <summary>
        /// Returns the base class of this class. Null if this class has no base.
        /// </summary>
        SEClass* GetBaseClass() const;

        /// <summary>
        /// Checks if this class is a sub class of the specified class (including any derived types).
        /// </summary>
        /// <param name="klass">The class.</param>
        /// <param name="checkInterfaces">True if check interfaces, otherwise just base class.</param>
        /// <returns>True if this class is a sub class of the specified class.</returns>
        bool IsSubClassOf(const SEClass* klass, bool checkInterfaces = false) const;

        /// <summary>
        /// Checks if this class implements the specified interface (including any base types).
        /// </summary>
        /// <param name="klass">The interface class.</param>
        /// <returns>True if this class implements the specified interface.</returns>
        bool HasInterface(const SEClass* klass) const;

        /// <summary>
        /// Checks is the provided object instance of this class' type.
        /// </summary>
        /// <param name="object">The object to check.</param>
        /// <returns>True if object  is an instance the this class.</returns>
        bool IsInstanceOfType(SEObject* object) const;

        /// <summary>
        /// Returns the size of an instance of this class, in bytes.
        /// </summary>
        uint32 GetInstanceSize() const;

        /// <summary>
        /// Returns the class of the array type elements.
        /// </summary>
        SEClass* GetElementClass() const;

    public:
        /// <summary>
        /// Returns an object referencing a method with the specified name and number of parameters. Optionally checks the base classes.
        /// </summary>
        /// <param name="name">The method name.</param>
        /// <param name="numParams">The method parameters count.</param>
        /// <param name="checkBaseClasses">True if check base classes when searching for the given method.</param>
        /// <returns>The method or null if failed to find it.</returns>
        SEMethod* FindMethod(const char* name, int32 numParams, bool checkBaseClasses = true) const
        {
            SEMethod* method = GetMethod(name, numParams);
            if (!method && checkBaseClasses)
            {
                SEClass* base = GetBaseClass();
                if (base)
                    method = base->FindMethod(name, numParams, true);
            }
            return method;
        }

        /// <summary>
        /// Returns an object referencing a method with the specified name and number of parameters.
        /// </summary>
        /// <remarks>If the type contains more than one method of the given name and parameters count the returned value can be non-deterministic (one of the matching methods).</remarks>
        /// <param name="name">The method name.</param>
        /// <param name="numParams">The method parameters count.</param>
        /// <returns>The method or null if failed to get it.</returns>
        SEMethod* GetMethod(const char* name, int32 numParams = 0) const;

        /// <summary>
        /// Returns all methods belonging to this class.
        /// </summary>
        /// <remarks>Be aware this will not include the methods of any base classes.</remarks>
        /// <returns>The list of methods.</returns>
        const List<SEMethod*>& GetMethods() const;

        /// <summary>
        /// Returns an object referencing a field with the specified name.
        /// </summary>
        /// <remarks>Does not query base class fields. Returns null if field cannot be found.</remarks>
        /// <param name="name">The field name.</param>
        /// <returns>The field or null if failed.</returns>
        SEField* GetField(const char* name) const;

        /// <summary>
        /// Returns all fields belonging to this class.
        /// </summary>
        /// <remarks>Be aware this will not include the fields of any base classes.</remarks>
        /// <returns>The list of fields.</returns>
        const List<SEField*>& GetFields() const;

        /// <summary>
        /// Returns an object referencing a event with the specified name.
        /// </summary>
        /// <param name="name">The event name.</param>
        /// <returns>The event object.</returns>
        SEEvent* GetEvent(const char* name) const;

        /// <summary>
        /// Returns all events belonging to this class.
        /// </summary>
        /// <returns>The list of events.</returns>
        const List<SEEvent*>& GetEvents() const;

        /// <summary>
        /// Returns an object referencing a property with the specified name.
        /// </summary>
        /// <remarks>Does not query base class properties. Returns null if property cannot be found.</remarks>
        /// <param name="name">The property name.</param>
        /// <returns>The property.</returns>
        SEProperty* GetProperty(const char* name) const;

        /// <summary>
        /// Returns all properties belonging to this class.
        /// </summary>
        /// <remarks>Be aware this will not include the properties of any base classes.</remarks>
        /// <returns>The list of properties.</returns>
        const List<SEProperty*>& GetProperties() const;

        /// <summary>
        /// Returns all interfaces implemented by this class (excluding interfaces from base classes).
        /// </summary>
        /// <remarks>Be aware this will not include the interfaces of any base classes.</remarks>
        /// <returns>The list of interfaces.</returns>
        const List<SEClass*>& GetInterfaces() const;

    public:
        /// <summary>
        /// Creates a new instance of this class and constructs it.
        /// </summary>
        /// <returns>The created managed object.</returns>
        SEObject* CreateInstance() const;

    public:
        /// <summary>
        /// Checks if class has an attribute of the specified type.
        /// </summary>
        /// <param name="monoClass">The attribute class to check.</param>
        /// <returns>True if has attribute of that class type, otherwise false.</returns>
        bool HasAttribute(const SEClass* monoClass) const;

        /// <summary>
        /// Checks if class has an attribute of any type.
        /// </summary>
        /// <returns>True if has any custom attribute, otherwise false.</returns>
        bool HasAttribute() const;

        /// <summary>
        /// Returns an instance of an attribute of the specified type. Returns null if the class doesn't have such an attribute.
        /// </summary>
        /// <param name="monoClass">The attribute class to take.</param>
        /// <returns>The attribute object.</returns>
        SEObject* GetAttribute(const SEClass* monoClass) const;

        /// <summary>
        /// Returns an instance of all attributes connected with given class. Returns null if the class doesn't have any attributes.
        /// </summary>
        /// <returns>The array of attribute objects.</returns>
        const List<SEObject*>& GetAttributes() const;
    };
} // namespace SE

#pragma once

#include "Runtime/App.h"
#include "CLRTypes.h"
#include "Runtime/Core/Types/Strings/String.h"

namespace SE
{
    /// <summary>
    /// Encapsulates information about a single Mono managed property belonging to some managed class.
    /// This object also allows you to set or retrieve values to or from specific instances containing the property.
    /// </summary>
    class SE_API_RUNTIME CLRProperty
    {
        friend CLRClass;

    protected:
        mutable CLRMethod* _getMethod;
        mutable CLRMethod* _setMethod;
        CLRClass* _parentClass;

        StringAnsi _name;

        mutable int32 _hasCachedAttributes : 1;
        mutable int32 _hasSetMethod : 1;
        mutable int32 _hasGetMethod : 1;

        mutable List<CLRObject*> _attributes;

    public:

        CLRProperty(CLRClass* parentClass, const char* name, void* getterHandle, void* setterHandle, CLRMethodAttributes getterAttributes, CLRMethodAttributes setterAttributes);

        /// <summary>
        /// Finalizes an instance of the <see cref="MProperty"/> class.
        /// </summary>
        ~CLRProperty();

    public:
        /// <summary>
        /// Gets the property name.
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
        /// Gets property type class.
        /// </summary>
        CLRType* GetType() const;

        /// <summary>
        /// Gets property get method.
        /// </summary>
        CLRMethod* GetGetMethod() const;

        /// <summary>
        /// Gets property set method.
        /// </summary>
        CLRMethod* GetSetMethod() const;

        /// <summary>
        /// Gets property visibility in the class.
        /// </summary>
        CLRVisibility GetVisibility() const;

        /// <summary>
        /// Returns true if property is static.
        /// </summary>
        bool IsStatic() const;

    public:
        /// <summary>
        /// Retrieves value currently set in the property on the specified object instance. If property is static object instance can be null.
        /// </summary>
        /// <remarks>
        /// Value will be a pointer to raw data type for value types (for example int, float), and a SEObject* for reference types.
        /// </remarks>
        /// <param name="instance">The object of given type to get value from.</param>
        /// <param name="exception">An optional pointer to the exception value to store exception object reference.</param>
        /// <returns>The returned boxed value object.</returns>
        CLRObject* GetValue(CLRObject* instance, CLRObject** exception) const;

        /// <summary>
        /// Sets a value for the property on the specified object instance. If property is static object instance can be null.
        /// </summary>
        /// <remarks>
        /// Value should be a pointer to raw data type for value types (for example int, float), and a SEObject* for reference types.
        /// </remarks>
        /// <param name="instance">Object of given type to set value to.</param>
        /// <param name="exception">An optional pointer to the exception value to store exception object reference.</param>
        /// <param name="value">The value to set of undefined type.</param>
        void SetValue(CLRObject* instance, void* value, CLRObject** exception) const;

    public:
        /// <summary>
        /// Checks if property has an attribute of the specified type.
        /// </summary>
        /// <param name="monoClass">The attribute class to check.</param>
        /// <returns>True if has attribute of that class type, otherwise false.</returns>
        bool HasAttribute(CLRClass* monoClass) const;

        /// <summary>
        /// Checks if property has an attribute of any type.
        /// </summary>
        /// <returns>True if has any custom attribute, otherwise false.</returns>
        bool HasAttribute() const;

        /// <summary>
        /// Returns an instance of an attribute of the specified type. Returns null if the property doesn't have such an attribute.
        /// </summary>
        /// <param name="monoClass">The attribute class to take.</param>
        /// <returns>The attribute object.</returns>
        CLRObject* GetAttribute(CLRClass* monoClass) const;

        /// <summary>
        /// Returns an instance of all attributes connected with given property. Returns null if the property doesn't have any attributes.
        /// </summary>
        /// <returns>The array of attribute objects.</returns>
        const List<CLRObject*>& GetAttributes() const;
    };
}

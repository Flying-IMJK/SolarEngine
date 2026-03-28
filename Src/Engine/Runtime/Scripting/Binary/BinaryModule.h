#pragma once

#include "Runtime/API.h"
#include "Core/Types/Strings/String.h"
#include "Runtime/Scripting/ScriptingType.h"


namespace SE
{
    /// <summary>
    /// The scripting type method metadata for code reflection.
    /// </summary>
    struct ScriptingTypeMethodSignature
    {
        struct Param
        {
            VariantTypeHandle Type;
            bool IsOut;
        };

        StringAnsiView Name;
        VariantTypeHandle ReturnType;
        bool IsStatic;
        List<Param, InlinedAllocation<16>> Params;
    };

    /// <summary>
    /// The scripting type field metadata for code reflection.
    /// </summary>
    struct ScriptingTypeFieldSignature
    {
        StringAnsiView Name;
        VariantTypeHandle ValueType;
        bool IsStatic;
    };

    /// <summary>
    /// The scripting assembly container that holds the scripting types information and metadata.
    /// </summary>
    class SE_API_RUNTIME BinaryModule
    {
    public:

        typedef List<BinaryModule*, InlinedAllocation<32>> BinaryModulesList;

    public:

        /// <summary>
        /// The list with all registered binary modules (including external for plugins and other modules).
        /// </summary>
        static BinaryModulesList& GetModules();

        /// <summary>
        /// Finds the module by name.
        /// </summary>
        /// <param name="name">The module name.</param>
        /// <returns>The found binary module or null if missing.</returns>
        static BinaryModule* GetModule(const StringAnsiView& name);

        // Global scripting locker for cached data.
        static CriticalSection Locker;

    protected:

        /// <summary>
        /// Initializes a new instance of the <see cref="BinaryModule"/> class.
        /// </summary>
        BinaryModule();

    public:

        /// <summary>
        /// Finalizes an instance of the <see cref="BinaryModule"/> class.
        /// </summary>
        virtual ~BinaryModule()
        {
        }

    public:

        /// <summary>
        /// The scripting types collection that exist in this assembly.
        /// </summary>
        List<ScriptingType> Types;

        /// <summary>
        /// The scripting types cache that maps the full typename to the scripting type index. Build after adding the type to the assembly.
        /// </summary>
        Dictionary<StringAnsi, int32> TypeNameToTypeIndex;

    public:

        /// <summary>
        /// Gets the assembly name.
        /// </summary>
        /// <returns>The assembly name.</returns>
        virtual const StringAnsi& GetName() const = 0;

        /// <summary>
        /// Returns true if module is loaded, otherwise false (it might not be loaded yet or failed to load).
        /// </summary>
        virtual bool IsLoaded() const = 0;

        /// <summary>
        /// Tries to find a given scripting type by the full name.
        /// </summary>
        /// <param name="typeName">The full name of the type eg: System.Int64.MaxInt.</param>
        /// <param name="typeIndex">The result type index in Types array of this module. Valid only if method returns true.</param>
        /// <returns>True if found a type, otherwise false.</returns>
        virtual bool FindScriptingType(const StringAnsiView& typeName, int32& typeIndex)
        {
            return TypeNameToTypeIndex.TryGet(typeName, typeIndex);
        }

        /// <summary>
        /// Tries to find a method in a given scripting type by the method name and parameters count.
        /// </summary>
        /// <remarks>If the the type contains more than one method of the given name and parameters count the returned value can be non-deterministic (one of the matching methods).</remarks>
        /// <param name="typeHandle">The type to find method inside it.</param>
        /// <param name="name">The method name.</param>
        /// <param name="numParams">The method parameters count.</param>
        /// <returns>The method or null if failed to get it.</returns>
        virtual void* FindMethod(const ScriptingTypeHandle& typeHandle, const StringAnsiView& name, int32 numParams = 0)
        {
            return nullptr;
        }

        /// <summary>
        /// Tries to find a method in a given scripting type by the method signature.
        /// </summary>
        /// <param name="typeHandle">The type to find method inside it.</param>
        /// <param name="signature">The method signature.</param>
        /// <returns>The method or null if failed to get it.</returns>
        virtual void* FindMethod(const ScriptingTypeHandle& typeHandle, const ScriptingTypeMethodSignature& signature);

        /// <summary>
        /// Invokes a given scripting method.
        /// </summary>
        /// <param name="method">The method.</param>
        /// <param name="instance">The object instance to call it's member method. Unused for static methods.</param>
        /// <param name="paramValues">The method parameters array. For output parameters the method writes the values back to the parameters. Length of this list has to match the method arguments amount.</param>
        /// <param name="result">The output value method returned. Not used for void method.</param>
        /// <returns>True if failed, otherwise false.</returns>
        virtual bool InvokeMethod(void* method, const Variant& instance, Span<Variant> paramValues, Variant& result)
        {
            return true;
        }

        /// <summary>
        /// Gets the scripting type method signature metadata.
        /// </summary>
        /// <param name="method">The method.</param>
        /// <param name="signature">The output method signature info.</param>
        virtual void GetMethodSignature(void* method, ScriptingTypeMethodSignature& signature)
        {
        }

        /// <summary>
        /// Tries to find a field in a given scripting type by the field name.
        /// </summary>
        /// <param name="typeHandle">The type to find field inside it.</param>
        /// <param name="name">The field name.</param>
        /// <returns>The field or null if failed to get it.</returns>
        virtual void* FindField(const ScriptingTypeHandle& typeHandle, const StringAnsiView& name)
        {
            return nullptr;
        }

        /// <summary>
        /// Gets the scripting type field signature metadata.
        /// </summary>
        /// <param name="field">The field.</param>
        /// <param name="fieldSignature">The output field signature info.</param>
        virtual void GetFieldSignature(void* field, ScriptingTypeFieldSignature& fieldSignature)
        {
        }

        /// <summary>
        /// Gets the value of a given scripting field.
        /// </summary>
        /// <param name="field">The field.</param>
        /// <param name="instance">The object instance to get it's member field. Unused for static fields.</param>
        /// <param name="result">The output field value.</param>
        /// <returns>True if failed, otherwise false.</returns>
        virtual bool GetFieldValue(void* field, const Variant& instance, Variant& result)
        {
            return true;
        }

        /// <summary>
        /// Sets the value of a given scripting field.
        /// </summary>
        /// <param name="field">The field.</param>
        /// <param name="instance">The object instance to set it's member field. Unused for static fields.</param>
        /// <param name="value">The field value to assign.</param>
        /// <returns>True if failed, otherwise false.</returns>
        virtual bool SetFieldValue(void* field, const Variant& instance, Variant& value)
        {
            return true;
        }

        /// <summary>
        /// Serializes the scripting object data. Called for objects using IsCustomScriptingType.
        /// </summary>
        /// <param name="stream">The output stream.</param>
        /// <param name="object">The object instance to serialize.</param>
        /// <param name="otherObj">The instance of the object to compare with and serialize only the modified properties. If null, then serialize all properties.</param>
        virtual void SerializeObject(JsonWriter& stream, ScriptingObject* object, const ScriptingObject* otherObj)
        {
        }

        /// <summary>
        /// Deserialize object from the input stream
        /// </summary>
        /// <param name="stream">The input stream.</param>
        /// <param name="object">The object instance to deserialize.</param>
        /// <param name="modifier">The deserialization modifier object. Always valid.</param>
        virtual void DeserializeObject(DeserializeStream& stream, ScriptingObject* object, ISerializeModifier* modifier)
        {
        }

        /// <summary>
        /// Called when object ID gets changed. Can be used to synchronize any cache in the scripting backend.
        /// </summary>
        /// <param name="object">The object instance.</param>
        /// <param name="oldId">The previous object ID.</param>
        virtual void OnObjectIdChanged(ScriptingObject* object, const UID& oldId)
        {
        }

        /// <summary>
        /// Called when object gets removed (inside destructor). Can be used to clear any cached data inside the scripting backend.
        /// </summary>
        /// <param name="object">The object instance.</param>
        virtual void OnObjectDeleted(ScriptingObject* object)
        {
        }

        /// <summary>
        /// Unloads the module (native library and C# assembly and any other scripting data). Unregisters the module.
        /// </summary>
        /// <param name="isReloading">If true module is during reloading and should force release the runtime data. Used for C# assembly to cleanup it's runtime data in Mono (or other scripting runtime).</param>
        virtual void Destroy(bool isReloading);
    };

} // namespace SE

#pragma once

#include "Runtime/API.h"
#include "SETypes.h"
#include "Core/Types/Variable.h"

namespace SE
{
    struct NativeInteropTable;

    /// <summary>
    /// Core CLR host — initializes and manages the .NET Core runtime via nethost/hostfxr.
    ///
    /// Usage:
    ///   bool failed = SECore::LoadEngine("path/to/SE.CSharp.runtimeconfig.json", "path/to/SE.CSharp.dll");
    ///   // ... use scripting ...
    ///   SECore::UnloadEngine();
    ///
    /// Return convention (matches FlaxEngine): false = success, true = failure.
    /// </summary>
    class SE_API_RUNTIME SECore
    {
    public:
        // -----------------------------------------------------------------------
        // Engine lifecycle
        // -----------------------------------------------------------------------
        /// <summary>
        /// Gets the root domain.
        /// </summary>
        static SEDomain* GetRootDomain();

        /// <summary>
        /// Gets the currently active domain.
        /// </summary>
        static SEDomain* GetActiveDomain();

        /// <summary>
        /// Initializes the .NET Core CLR runtime and loads the engine C# assembly.
        /// </summary>
        /// <param name="runtimeConfigPath">Path to SE.CSharp.runtimeconfig.json.</param>
        /// <param name="assemblyPath">Path to SE.CSharp.dll.</param>
        /// <returns>False on success, true on failure.</returns>
        static bool LoadEngine(const char* runtimeConfigPath, const char* assemblyPath);

        /// <summary>
        /// Unloads the CLR runtime and releases all managed resources.
        /// </summary>
        static void UnloadEngine();

        // -----------------------------------------------------------------------
        // GC Handle management
        // -----------------------------------------------------------------------
        struct SE_API_RUNTIME Handle
        {
            /// <summary>Creates a strong (normal or pinned) GC handle.</summary>
            static GCHandle New(SEObject* obj, bool pinned = false);

            /// <summary>Creates a weak GC handle.</summary>
            static GCHandle NewWeak(SEObject* obj, bool trackResurrection = false);

            /// <summary>Retrieves the managed object from a GC handle.</summary>
            static SEObject* GetTarget(const GCHandle& handle);

            /// <summary>Frees a GC handle.</summary>
            static void Free(const GCHandle& handle);
        };

        // -----------------------------------------------------------------------
        // Managed object operations
        // -----------------------------------------------------------------------
        struct SE_API_RUNTIME Object
        {
            /// <summary>Creates a new managed instance of the given class.</summary>
            static SEObject* New(const SEClass* klass);

            static void Init(SEObject* obj);

            /// <summary>Boxes a value type into a managed object.</summary>
            static SEObject* Box(void* value, const SEClass* klass);

            /// <summary>Unboxes a managed object to a raw value pointer.</summary>
            static void* Unbox(SEObject* obj);

            /// <summary>Gets the SEClass of a managed object.</summary>
            static SEClass* GetClass(SEObject* obj);

            static SEString* ToString(SEObject* obj);

            static int32 GetHashCode(SEObject* obj);
        };

        // -----------------------------------------------------------------------
        // String operations
        // -----------------------------------------------------------------------
        struct SE_API_RUNTIME String
        {
            static SEString* GetEmpty(SEDomain* domain = nullptr);
            static SEString* New(const char* str, int32 length, SEDomain* domain = nullptr);
            static SEString* New(const Char* str, int32 length, SEDomain* domain = nullptr);
            static StringView GetChars(SEString* obj);
        };

        /// <summary>
        /// Utilities for C# array management.
        /// </summary>
        struct SE_API_RUNTIME Array
        {
            static SEArray* New(const SEClass* elementKlass, int32 length);
            static SEClass* GetClass(SEClass* elementKlass);
            static SEClass* GetArrayClass(const SEArray* obj);
            static int32 GetLength(const SEArray* obj);
            static void* GetAddress(const SEArray* obj);
            static SEArray* Unbox(SEObject* obj);

            template<typename T>
            FORCE_INLINE static T* GetAddress(const SEArray* obj)
            {
                return (T*)GetAddress(obj);
            }
        };

        /// <summary>
        /// Helper utilities for C# garbage collector.
        /// </summary>
        struct SE_API_RUNTIME GC
        {
            static void Collect();
            static void Collect(int32 generation);
            static void Collect(int32 generation, SEGCCollectionMode collectionMode, bool blocking, bool compacting);
            static int32 MaxGeneration();
            static void WaitForPendingFinalizers();
            static void WriteRef(void* ptr, SEObject* ref);
            static void WriteValue(void* dst, void* src, int32 count, const SEClass* klass);
            static void WriteArrayRef(SEArray* dst, SEObject* ref, int32 index);
            static void WriteArrayRef(SEArray* dst, Span<SEObject*> span);

            static void* AllocateMemory(int32 size, bool coTaskMem = false);
            static void FreeMemory(void* ptr, bool coTaskMem = false);
        };

        /// <summary>
        /// Helper utilities for C# types information.
        /// </summary>
        struct SE_API_RUNTIME Type
        {
            static ::SE::String ToString(SEType* type);
            static SEClass* GetClass(SEType* type);
            static SEType* GetElementType(SEType* type);
            static int32 GetSize(SEType* type);
            static SETypes GetType(SEType* type);
            static bool IsPointer(SEType* type);
            static bool IsReference(SEType* type);
        };

        // -----------------------------------------------------------------------
        // Type cache — frequently used CLR types
        // -----------------------------------------------------------------------
        struct TypeCache
        {
            static SEClass* Void;
            static SEClass* Object;
            static SEClass* Int32;
            static SEClass* String;
            static SEClass* Boolean;
            static SEClass* Single;   // float
            static SEClass* Double;
            static SEClass* Int64;

            /// <summary>Populates the type cache after the engine assembly is loaded.</summary>
            static void Init(SEAssembly* coreAssembly);

            /// <summary>Clears all cached type pointers.</summary>
            static void Clear();
        };

        // -----------------------------------------------------------------------
        // ScriptingObject helpers
        // -----------------------------------------------------------------------
        struct ScriptingObjectHelper
        {
            /// <summary>
            /// Sets the __unmanagedPtr and __internalId fields on a managed ScriptingObject instance.
            /// Called after creating a managed instance to link it back to the C++ object.
            /// </summary>
            static void SetInternalValues(SEClass* klass, SEObject* obj, void* unmanagedPtr, const void* id);

            /// <summary>
            /// Creates a new managed ScriptingObject instance of the given class,
            /// sets its __unmanagedPtr and __internalId fields, and returns the instance.
            /// Returns nullptr on failure.
            /// </summary>
            static SEObject* CreateScriptingObject(SEClass* klass, void* unmanagedPtr, const void* id);
        };
    };

} // namespace SE

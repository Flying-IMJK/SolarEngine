#pragma once

#include "Runtime/API.h"
#include "CLRTypes.h"
#include "Runtime/Core/Types/Variable.h"

namespace SE
{
    struct UID;
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
    class SE_API_RUNTIME CLRCore
    {
    public:
        // -----------------------------------------------------------------------
        // Engine lifecycle
        // -----------------------------------------------------------------------
        /// <summary>
        /// Gets the root domain.
        /// </summary>
        static CLRDomain* GetRootDomain();

        /// <summary>
        /// Gets the currently active domain.
        /// </summary>
        static CLRDomain* GetActiveDomain();

        /// <summary>
        /// Creates an new empty domain.
        /// </summary>
        /// <param name="domainName">The domain name to create.</param>
        /// <returns>The domain object.</returns>
        static CLRDomain* CreateDomain(const StringAnsi& domainName);

        /// <summary>
        /// Unloads the domain.
        /// </summary>
        /// <param name="domainName">The domain name to remove.</param>
        static void UnloadDomain(const StringAnsi& domainName);


        /// <summary>
        /// Initializes the .NET Core CLR runtime and loads the engine C# assembly.
        /// </summary>
        /// <returns>False on success, true on failure.</returns>
        static bool LoadEngine();

        /// <summary>
        /// Unloads the CLR runtime and releases all managed resources.
        /// </summary>
        static void UnloadEngine();

        // -----------------------------------------------------------------------
        // GC Handle management
        // -----------------------------------------------------------------------
        struct SE_API_RUNTIME GCHandle
        {
            /// <summary>Creates a strong (normal or pinned) GC handle.</summary>
            static CLRGCHandle New(CLRObject* obj, bool pinned = false);

            /// <summary>Creates a weak GC handle.</summary>
            static CLRGCHandle NewWeak(CLRObject* obj, bool trackResurrection = false);

            /// <summary>Retrieves the managed object from a GC handle.</summary>
            static CLRObject* GetTarget(const CLRGCHandle& handle);

            /// <summary>Frees a GC handle.</summary>
            static void Free(const CLRGCHandle& handle);
        };

        // -----------------------------------------------------------------------
        // Managed object operations
        // -----------------------------------------------------------------------
        struct SE_API_RUNTIME Object
        {
            /// <summary>Creates a new managed instance of the given class.</summary>
            static CLRObject* New(const CLRClass* klass);

            static void Init(CLRObject* obj);

            /// <summary>Boxes a value type into a managed object.</summary>
            static CLRObject* Box(void* value, const CLRClass* klass);

            /// <summary>Unboxes a managed object to a raw value pointer.</summary>
            static void* Unbox(CLRObject* obj);

            /// <summary>Gets the SEClass of a managed object.</summary>
            static CLRClass* GetClass(CLRObject* obj);

            static CLRString* ToString(CLRObject* obj);

            static int32 GetHashCode(CLRObject* obj);
        };

        // -----------------------------------------------------------------------
        // String operations
        // -----------------------------------------------------------------------
        struct SE_API_RUNTIME String
        {
            static CLRString* GetEmpty(CLRDomain* domain = nullptr);
            static CLRString* New(const char* str, int32 length, CLRDomain* domain = nullptr);
            static CLRString* New(const Char* str, int32 length, CLRDomain* domain = nullptr);
            static StringView GetChars(CLRString* obj);
        };

        /// <summary>
        /// Utilities for C# array management.
        /// </summary>
        struct SE_API_RUNTIME Array
        {
            static CLRArray* New(const CLRClass* elementKlass, int32 length);
            static CLRClass* GetClass(CLRClass* elementKlass);
            static CLRClass* GetArrayClass(const CLRArray* obj);
            static int32 GetLength(const CLRArray* obj);
            static void* GetAddress(const CLRArray* obj);
            static CLRArray* Unbox(CLRObject* obj);

            template<typename T>
            FORCE_INLINE static T* GetAddress(const CLRArray* obj)
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
            static void Collect(int32 generation, CLRGCCollectionMode collectionMode, bool blocking, bool compacting);
            static int32 MaxGeneration();
            static void WaitForPendingFinalizers();
            static void WriteRef(void* ptr, CLRObject* ref);
            static void WriteValue(void* dst, void* src, int32 count, const CLRClass* klass);
            static void WriteArrayRef(CLRArray* dst, CLRObject* ref, int32 index);
            static void WriteArrayRef(CLRArray* dst, Span<CLRObject*> span);

            static void* AllocateMemory(int32 size, bool coTaskMem = false);
            static void FreeMemory(void* ptr, bool coTaskMem = false);
        };

        /// <summary>
        /// Helper utilities for C# types information.
        /// </summary>
        struct SE_API_RUNTIME Type
        {
            static ::SE::String ToString(CLRType* type);
            static CLRClass* GetClass(CLRType* type);
            static CLRType* GetElementType(CLRType* type);
            static int32 GetSize(CLRType* type);
            static CLRTypes GetType(CLRType* type);
            static bool IsPointer(CLRType* type);
            static bool IsReference(CLRType* type);
        };

        /// <summary>
        /// Utilities for ScriptingObject management.
        /// </summary>
        struct SE_API_RUNTIME ScriptingObject
        {
            static void SetInternalValues(CLRClass* klass, CLRObject* object, void* unmanagedPtr, const UID* id);
            static CLRObject* CreateScriptingObject(CLRClass* klass, void* unmanagedPtr, const UID* id);
        };

        // -----------------------------------------------------------------------
        // Type cache — frequently used CLR types
        // -----------------------------------------------------------------------
        struct SE_API_RUNTIME TypeCache
        {
            static CLRClass* Void;
            static CLRClass* Object;
            static CLRClass* Byte;
            static CLRClass* Boolean;
            static CLRClass* SByte;
            static CLRClass* Char;
            static CLRClass* Int16;
            static CLRClass* UInt16;
            static CLRClass* Int32;
            static CLRClass* UInt32;
            static CLRClass* Int64;
            static CLRClass* UInt64;
            static CLRClass* IntPtr;
            static CLRClass* UIntPtr;
            static CLRClass* Single;
            static CLRClass* Double;
            static CLRClass* String;

            // Engine Runtime Type
            static CLRClass* UID;
            static CLRClass* Dictionary;
            static CLRClass* Activator;
            static CLRClass* Type;

            static CLRClass* Vector2;
            static CLRClass* Vector3;
            static CLRClass* Vector4;
            static CLRClass* Color;
            static CLRClass* Transform;
            static CLRClass* Quaternion;
            static CLRClass* Matrix;
            static CLRClass* BoundingBox;
            static CLRClass* BoundingSphere;
            static CLRClass* Rectangle;
            static CLRClass* Ray;

            static CLRClass* Int2;
            static CLRClass* Int3;
            static CLRClass* Int4;
            static CLRClass* Float2;
            static CLRClass* Float3;
            static CLRClass* Float4;
            static CLRClass* Double2;
            static CLRClass* Double3;
            static CLRClass* Double4;


            static CLRClass* CollisionClass;

            static CLRClass* JSON;
            static CLRMethod* Json_Serialize;
            static CLRMethod* Json_SerializeDiff;
            static CLRMethod* Json_Deserialize;

            static CLRClass* ManagedArrayClass;
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
            static void SetInternalValues(CLRClass* klass, CLRObject* obj, void* unmanagedPtr, const void* id);

            /// <summary>
            /// Creates a new managed ScriptingObject instance of the given class,
            /// sets its __unmanagedPtr and __internalId fields, and returns the instance.
            /// Returns nullptr on failure.
            /// </summary>
            static CLRObject* CreateScriptingObject(CLRClass* klass, void* unmanagedPtr, const void* id);
        };
    };

} // namespace SE

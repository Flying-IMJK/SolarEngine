

#include "Core/Types/Variable.h"
#include "Core/Logging/Logging.h"
#include "Core/Types/Collections/Dictionary.h"
#include "Core/Types/Collections/List.h"
#include "Core/Types/Strings/String.h"
#include "Core/Profiler/ProfilerCPU.h"
#include "Core/Platform/File.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Thread/Threading.h"
#include "Core/Types/Stopwatch.h"
#include "Core/Types/Collections/Span.h"

#include "Runtime/Scripting/ManagedCLR/CLRClass.h"
#include "Runtime/Scripting/ManagedCLR/CLRCore.h"
#include "Runtime/Scripting/ManagedCLR/CLRAssembly.h"
#include "Runtime/EngineContext.h"
#include "Runtime/Scripting/Binary/BinaryModule.h"
#include "Runtime/Scripting/Internal/InternalCalls.h"
#include "Runtime/Scripting/ManagedCLR/CLRField.h"
#include "Runtime/Scripting/ManagedCLR/CLRMethod.h"
#include "Runtime/Scripting/ManagedCLR/CLRProperty.h"
#include "Runtime/Scripting/ManagedCLR/CLREvent.h"
#include "Runtime/Scripting/ManagedCLR/CLRDomain.h"
#include "Runtime/Scripting/ManagedCLR/CLRException.h"
#include "Runtime/Scripting/ManagedCLR/CLRUtils.h"

#include "NetCoreInclude.h"

#if PLATFORM_WINDOWS
#include <combaseapi.h>
#undef SetEnvironmentVariable
#undef GetEnvironmentVariable
#undef LoadLibrary
#undef LoadImage
#endif

#if defined(_WIN32)
#define CORECLR_DELEGATE_CALLTYPE __stdcall
#define SE_CORECLR_STRING String
#define SE_CORECLR_TEXT(x) SE_TEXT(x)
#else
#define CORECLR_DELEGATE_CALLTYPE
#define SE_CORECLR_STRING StringAnsi
#define SE_CORECLR_TEXT(x) x
#endif

namespace SE
{
    // Multiple AppDomains are superseded by AssemblyLoadContext in .NET
    extern CLRDomain* m_RootDomain;
    extern CLRDomain* m_ActiveDomain;
    extern List<CLRDomain*, FixedAllocation<4>> m_Domains;

    Dictionary<String, void*> CachedFunctions;
    Dictionary<void*, CLRClass*> CachedClassHandles;
    Dictionary<void*, CLRAssembly*> CachedAssemblyHandles;

    /// <summary>
    /// Returns the function pointer to the managed static method in NativeInterop class.
    /// </summary>
    void* GetStaticMethodPointer(const String& methodName);

    /// <summary>
    /// Calls the managed static method with given parameters.
    /// </summary>
    template<typename RetType, typename... Args>
    RetType CallStaticMethod(void* methodPtr, Args... args)
    {
#if DOTNET_HOST_MONO
        ASSERT_LOW_LAYER(mono_domain_get()); // Ensure that Mono runtime has been attached to this thread
#endif
        typedef RetType (CORECLR_DELEGATE_CALLTYPE* fun)(Args...);
        return ((fun)methodPtr)(args...);
    }

    void RegisterNativeLibrary(const char* moduleName, const Char* modulePath)
    {
        static void* RegisterNativeLibraryPtr = GetStaticMethodPointer(SE_TEXT("RegisterNativeLibrary"));
        CallStaticMethod<void, const char*, const Char*>(RegisterNativeLibraryPtr, moduleName, modulePath);
    }

    bool InitHostfxr();
    void ShutdownHostfxr();

    CLRAssembly* GetAssembly(void* assemblyHandle);
    CLRClass* GetClass(CLRType* typeHandle);
    CLRClass* GetOrCreateClass(CLRType* typeHandle);
    CLRType* GetObjectType(CLRObject* obj);

    void* GetCustomAttribute(const CLRClass* klass, const CLRClass* attributeClass);


    // System.Reflection.TypeAttributes
    enum class CLRTypeAttributes : uint32
    {
        VisibilityMask = 0x00000007,
        NotPublic = 0x00000000,
        Public = 0x00000001,
        NestedPublic = 0x00000002,
        NestedPrivate = 0x00000003,
        NestedFamily = 0x00000004,
        NestedAssembly = 0x00000005,
        NestedFamANDAssem = 0x00000006,
        NestedFamORAssem = 0x00000007,
        LayoutMask = 0x00000018,
        AutoLayout = 0x00000000,
        SequentialLayout = 0x00000008,
        ExplicitLayout = 0x00000010,
        ClassSemanticsMask = 0x00000020,
        Class = 0x00000000,
        Interface = 0x00000020,
        Abstract = 0x00000080,
        Sealed = 0x00000100,
        SpecialName = 0x00000400,
        Import = 0x00001000,
        Serializable = 0x00002000,
        WindowsRuntime = 0x00004000,
        StringFormatMask = 0x00030000,
        AnsiClass = 0x00000000,
        UnicodeClass = 0x00010000,
        AutoClass = 0x00020000,
        CustomFormatClass = 0x00030000,
        CustomFormatMask = 0x00C00000,
        BeforeFieldInit = 0x00100000,
        RTSpecialName = 0x00000800,
        HasSecurity = 0x00040000,
        ReservedMask = 0x00040800,
    };

    // System.Reflection.MethodAttributes
    enum class CLRMethodAttributes : uint32
    {
        MemberAccessMask = 0x0007,
        PrivateScope = 0x0000,
        Private = 0x0001,
        FamANDAssem = 0x0002,
        Assembly = 0x0003,
        Family = 0x0004,
        FamORAssem = 0x0005,
        Public = 0x0006,
        Static = 0x0010,
        Final = 0x0020,
        Virtual = 0x0040,
        HideBySig = 0x0080,
        CheckAccessOnOverride = 0x0200,
        VtableLayoutMask = 0x0100,
        ReuseSlot = 0x0000,
        NewSlot = 0x0100,
        Abstract = 0x0400,
        SpecialName = 0x0800,
        PinvokeImpl = 0x2000,
        UnmanagedExport = 0x0008,
        RTSpecialName = 0x1000,
        HasSecurity = 0x4000,
        RequireSecObject = 0x8000,
        ReservedMask = 0xd000,
    };

    // System.Reflection.FieldAttributes
    enum class CLRFieldAttributes : uint32
    {
        FieldAccessMask = 0x0007,
        PrivateScope = 0x0000,
        Private = 0x0001,
        FamANDAssem = 0x0002,
        Assembly = 0x0003,
        Family = 0x0004,
        FamORAssem = 0x0005,
        Public = 0x0006,
        Static = 0x0010,
        InitOnly = 0x0020,
        Literal = 0x0040,
        NotSerialized = 0x0080,
        SpecialName = 0x0200,
        PinvokeImpl = 0x2000,
        RTSpecialName = 0x0400,
        HasFieldMarshal = 0x1000,
        HasDefault = 0x8000,
        HasFieldRVA = 0x0100,
        ReservedMask = 0x9500,
    };

    SE_ENUM_OPERATORS(CLRTypeAttributes)
    SE_ENUM_OPERATORS(CLRMethodAttributes)
    SE_ENUM_OPERATORS(CLRFieldAttributes)

    // Structures used to pass information from runtime, must match with the structures in managed side
    struct NativeClassDefinitions
    {
        void* typeHandle;
        CLRClass* nativePointer;
        const char* name;
        const char* fullname;
        const char* namespace_;
        CLRTypeAttributes typeAttributes;
    };

    struct NativeMethodDefinitions
    {
        const char* name;
        int numParameters;
        void* handle;
        CLRMethodAttributes methodAttributes;
    };

    struct NativeFieldDefinitions
    {
        const char* name;
        void* fieldHandle;
        void* fieldType;
        int fieldOffset;
        CLRFieldAttributes fieldAttributes;
    };

    struct NativePropertyDefinitions
    {
        const char* name;
        void* getterHandle;
        void* setterHandle;
        CLRMethodAttributes getterAttributes;
        CLRMethodAttributes setterAttributes;
    };

    CLRDomain* CLRCore::CreateDomain(const StringAnsi& domainName)
    {
        return nullptr;
    }

    void CLRCore::UnloadDomain(const StringAnsi& domainName)
    {
    }

    bool CLRCore::LoadEngine()
    {
        PROFILE_CPU();

        // Initialize hostfxr
        if (!InitHostfxr())
        {
            return false;
        }

        // Prepare managed side
        CallStaticMethod<void>(GetStaticMethodPointer(SE_TEXT("Init")));
        ::SE::String libraryPath = Platform::GetExecutableFilePath();

#if PLATFORM_MAC
        // On some platforms all native binaries are side-by-side with the app in a different folder
        if (!FileSystem::FileExists(libraryPath))
        {
            libraryPath = ::SE::String(FileSystem::GetDirectoryName(Platform::GetExecutableFilePath())) / FileSystem::GetFileName(libraryPath);
        }
#endif
#if !PLATFORM_SWITCH
        if (!FileSystem::FileExists(libraryPath))
        {
            LOG_ERROR("Scripting", "Solar Engine native library file is missing ({0})", libraryPath);
        }
#endif
        RegisterNativeLibrary("SERuntime", libraryPath.Get());

        m_RootDomain = New<CLRDomain>("Root");
        m_Domains.Add(m_RootDomain);

        char* buildInfo = CallStaticMethod<char*>(GetStaticMethodPointer(SE_TEXT("GetRuntimeInformation")));
        LOG_INFO("Scripting", ".NET runtime version: {0}", ::SE::String(buildInfo));
        CLRCore::GC::FreeMemory(buildInfo);

        return true;
    }

    void CLRCore::UnloadEngine()
    {
        if (!m_RootDomain)
            return;
        PROFILE_CPU();
        CallStaticMethod<void>(GetStaticMethodPointer(SE_TEXT("Exit")));
        m_Domains.ClearDelete();
        m_RootDomain = nullptr;
        ShutdownHostfxr();
    }

    // -------------------------------------------------------------------------
    // GCHandle namespace
    // -------------------------------------------------------------------------

    CLRGCHandle CLRCore::GCHandle::New(CLRObject* obj, bool pinned)
    {
        ENGINE_ASSERT(obj);
        static void* NewGCHandlePtr = GetStaticMethodPointer(SE_TEXT("NewGCHandle"));
        return (CLRGCHandle)CallStaticMethod<void*, void*, bool>(NewGCHandlePtr, obj, pinned);
    }

    CLRGCHandle CLRCore::GCHandle::NewWeak(CLRObject* obj, bool trackResurrection)
    {
        ENGINE_ASSERT(obj);
        static void* NewGCHandleWeakPtr = GetStaticMethodPointer(SE_TEXT("NewGCHandleWeak"));
        return (CLRGCHandle)CallStaticMethod<void*, void*, bool>(NewGCHandleWeakPtr, obj, trackResurrection);
    }

    CLRObject* CLRCore::GCHandle::GetTarget(const CLRGCHandle& handle)
    {
        return (CLRObject*)(void*)handle;
    }

    void CLRCore::GCHandle::Free(const CLRGCHandle& handle)
    {
        static void* FreeGCHandlePtr = GetStaticMethodPointer(SE_TEXT("FreeGCHandle"));
        CallStaticMethod<void, void*>(FreeGCHandlePtr, (void*)handle);
    }


    CLRObject* CLRCore::Object::Box(void* value, const CLRClass* klass)
    {
        static void* BoxValuePtr = GetStaticMethodPointer(SE_TEXT("BoxValue"));
        return (CLRObject*)CallStaticMethod<void*, void*, void*>(BoxValuePtr, klass->m_Handle, value);
    }

    void* CLRCore::Object::Unbox(CLRObject* obj)
    {
        static void* UnboxValuePtr = GetStaticMethodPointer(SE_TEXT("UnboxValue"));
        return CallStaticMethod<void*, void*>(UnboxValuePtr, obj);
    }

    CLRObject* CLRCore::Object::New(const CLRClass* klass)
    {
        static void* NewObjectPtr = GetStaticMethodPointer(SE_TEXT("NewObject"));
        return (CLRObject*)CallStaticMethod<void*, void*>(NewObjectPtr, klass->m_Handle);
    }

    void CLRCore::Object::Init(CLRObject* obj)
    {
        static void* ObjectInitPtr = GetStaticMethodPointer(SE_TEXT("ObjectInit"));
        CallStaticMethod<void, void*>(ObjectInitPtr, obj);
    }

    CLRClass* CLRCore::Object::GetClass(CLRObject* obj)
    {
        ENGINE_ASSERT(obj);
        static void* GetObjectClassPtr = GetStaticMethodPointer(SE_TEXT("GetObjectClass"));
        return (CLRClass*)CallStaticMethod<CLRClass*, void*>(GetObjectClassPtr, obj);
    }

    CLRString* CLRCore::Object::ToString(CLRObject* obj)
    {
        static void* GetObjectStringPtr = GetStaticMethodPointer(SE_TEXT("GetObjectString"));
        return (CLRString*)CallStaticMethod<void*, void*>(GetObjectStringPtr, obj);
    }

    int32 CLRCore::Object::GetHashCode(CLRObject* obj)
    {
        static void* GetObjectStringPtr = GetStaticMethodPointer(SE_TEXT("GetObjectHashCode"));
        return CallStaticMethod<int32, void*>(GetObjectStringPtr, obj);
    }


    CLRString* CLRCore::String::GetEmpty(CLRDomain* domain)
    {
        static void* GetStringEmptyPtr = GetStaticMethodPointer(SE_TEXT("GetStringEmpty"));
        return (CLRString*)CallStaticMethod<void*>(GetStringEmptyPtr);
    }

    CLRString* CLRCore::String::New(const char* str, int32 length, CLRDomain* domain)
    {
        static void* NewStringUTF8Ptr = GetStaticMethodPointer(SE_TEXT("NewStringUTF8"));
        return (CLRString*)CallStaticMethod<void*, const char*, int>(NewStringUTF8Ptr, str, length);
    }

    CLRString* CLRCore::String::New(const Char* str, int32 length, CLRDomain* domain)
    {
        static void* NewStringUTF16Ptr = GetStaticMethodPointer(SE_TEXT("NewStringUTF16"));
        return (CLRString*)CallStaticMethod<void*, const Char*, int>(NewStringUTF16Ptr, str, length);
    }

    StringView CLRCore::String::GetChars(CLRString* obj)
    {
        int32 length = 0;
        static void* GetStringPointerPtr = GetStaticMethodPointer(SE_TEXT("GetStringPointer"));
        const Char* chars = CallStaticMethod<const Char*, void*, int*>(GetStringPointerPtr, obj, &length);
        return StringView(chars, length);
    }


    CLRArray* CLRCore::Array::New(const CLRClass* elementKlass, int32 length)
    {
        static void* NewArrayPtr = GetStaticMethodPointer(SE_TEXT("NewArray"));
        return (CLRArray*)CallStaticMethod<void*, void*, long long>(NewArrayPtr, elementKlass->m_Handle, length);
    }

    CLRClass* CLRCore::Array::GetClass(CLRClass* elementKlass)
    {
        static void* GetArrayTypeFromElementTypePtr = GetStaticMethodPointer(SE_TEXT("GetArrayTypeFromElementType"));
        CLRType* typeHandle = (CLRType*)CallStaticMethod<void*, void*>(GetArrayTypeFromElementTypePtr, elementKlass->m_Handle);
        return GetOrCreateClass(typeHandle);
    }

    CLRClass* CLRCore::Array::GetArrayClass(const CLRArray* obj)
    {
        static void* GetArrayTypeFromWrappedArrayPtr = GetStaticMethodPointer(SE_TEXT("GetArrayTypeFromWrappedArray"));
        CLRType* typeHandle = (CLRType*)CallStaticMethod<void*, void*>(GetArrayTypeFromWrappedArrayPtr, (void*)obj);
        return GetOrCreateClass(typeHandle);
    }

    int32 CLRCore::Array::GetLength(const CLRArray* obj)
    {
        static void* GetArrayLengthPtr = GetStaticMethodPointer(SE_TEXT("GetArrayLength"));
        return CallStaticMethod<int, void*>(GetArrayLengthPtr, (void*)obj);
    }

    void* CLRCore::Array::GetAddress(const CLRArray* obj)
    {
        static void* GetArrayPointerPtr = GetStaticMethodPointer(SE_TEXT("GetArrayPointer"));
        return CallStaticMethod<void*, void*>(GetArrayPointerPtr, (void*)obj);
    }

    CLRArray* CLRCore::Array::Unbox(CLRObject* obj)
    {
        static void* GetArrayPtr = GetStaticMethodPointer(SE_TEXT("GetArray"));
        return (CLRArray*)CallStaticMethod<void*, void*>(GetArrayPtr, (void*)obj);
    }

    
    
    void CLRCore::GC::Collect()
    {
        PROFILE_CPU();
        static void* GCCollectPtr = GetStaticMethodPointer(SE_TEXT("GCCollect"));
        CallStaticMethod<void, int, int, bool, bool>(GCCollectPtr, MaxGeneration(), (int)CLRGCCollectionMode::Default, true, false);
    }

    void CLRCore::GC::Collect(int32 generation)
    {
        PROFILE_CPU();
        static void* GCCollectPtr = GetStaticMethodPointer(SE_TEXT("GCCollect"));
        CallStaticMethod<void, int, int, bool, bool>(GCCollectPtr, generation, (int)CLRGCCollectionMode::Default, true, false);
    }

    void CLRCore::GC::Collect(int32 generation, CLRGCCollectionMode collectionMode, bool blocking, bool compacting)
    {
        PROFILE_CPU();
        static void* GCCollectPtr = GetStaticMethodPointer(SE_TEXT("GCCollect"));
        CallStaticMethod<void, int, int, bool, bool>(GCCollectPtr, generation, (int)collectionMode, blocking, compacting);
    }

    int32 CLRCore::GC::MaxGeneration()
    {
        static int32 maxGeneration = CallStaticMethod<int32>(GetStaticMethodPointer(SE_TEXT("GCMaxGeneration")));
        return maxGeneration;
    }

    void CLRCore::GC::WaitForPendingFinalizers()
    {
        PROFILE_CPU();
        static void* GCWaitForPendingFinalizersPtr = GetStaticMethodPointer(SE_TEXT("GCWaitForPendingFinalizers"));
        CallStaticMethod<void>(GCWaitForPendingFinalizersPtr);
    }

    void CLRCore::GC::WriteRef(void* ptr, CLRObject* ref)
    {
        *(void**)ptr = ref;
    }

    void CLRCore::GC::WriteValue(void* dst, void* src, int32 count, const CLRClass* klass)
    {
        const int32 size = klass->GetInstanceSize();
        memcpy(dst, src, count * size);
    }

    void CLRCore::GC::WriteArrayRef(CLRArray* dst, CLRObject* ref, int32 index)
    {
        static void* WriteArrayReferencePtr = GetStaticMethodPointer(SE_TEXT("WriteArrayReference"));
        CallStaticMethod<void, void*, void*, int32>(WriteArrayReferencePtr, dst, ref, index);
    }

    void CLRCore::GC::WriteArrayRef(CLRArray* dst, Span<CLRObject*> refs)
    {
        static void* WriteArrayReferencesPtr = GetStaticMethodPointer(SE_TEXT("WriteArrayReferences"));
        CallStaticMethod<void, void*, void*, int32>(WriteArrayReferencesPtr, dst, refs.Get(), refs.Length());
    }

    void* CLRCore::GC::AllocateMemory(int32 size, bool coTaskMem)
    {
        static void* AllocMemoryPtr = GetStaticMethodPointer(SE_TEXT("AllocMemory"));
        return CallStaticMethod<void*, int, bool>(AllocMemoryPtr, size, (int)coTaskMem);
    }

    void CLRCore::GC::FreeMemory(void* ptr, bool coTaskMem)
    {
        if (!ptr)
            return;
        static void* FreeMemoryPtr = GetStaticMethodPointer(SE_TEXT("FreeMemory"));
        CallStaticMethod<void, void*, bool>(FreeMemoryPtr, ptr, coTaskMem);
    }


    ::SE::String CLRCore::Type::ToString(CLRType* type)
    {
        CLRClass* klass = GetOrCreateClass(type);
        return ::SE::String(klass->GetFullName());
    }

    CLRClass* CLRCore::Type::GetClass(CLRType* type)
    {
        static void* GetTypeClassPtr = GetStaticMethodPointer(SE_TEXT("GetTypeClass"));
        return CallStaticMethod<CLRClass*, void*>(GetTypeClassPtr, type);
    }

    CLRType* CLRCore::Type::GetElementType(CLRType* type)
    {
        static void* GetElementClassPtr = GetStaticMethodPointer(SE_TEXT("GetElementClass"));
        return (CLRType*)CallStaticMethod<void*, void*>(GetElementClassPtr, type);
    }

    int32 CLRCore::Type::GetSize(CLRType* type)
    {
        return GetOrCreateClass(type)->GetInstanceSize();
    }

    CLRTypes CLRCore::Type::GetType(CLRType* type)
    {
        CLRClass* klass = GetOrCreateClass(type);
        if (klass->m_Types == 0)
        {
            static void* GetTypeMTypesEnumPtr = GetStaticMethodPointer(SE_TEXT("GetTypeMTypesEnum"));
            klass->m_Types = CallStaticMethod<uint32, void*>(GetTypeMTypesEnumPtr, klass->m_Handle);
        }
        return (CLRTypes)klass->m_Types;
    }

    bool CLRCore::Type::IsPointer(CLRType* type)
    {
        static void* GetTypeIsPointerPtr = GetStaticMethodPointer(SE_TEXT("GetTypeIsPointer"));
        return CallStaticMethod<bool, void*>(GetTypeIsPointerPtr, type);
    }

    bool CLRCore::Type::IsReference(CLRType* type)
    {
        static void* GetTypeIsReferencePtr = GetStaticMethodPointer(SE_TEXT("GetTypeIsReference"));
        return CallStaticMethod<bool, void*>(GetTypeIsReferencePtr, type);
    }

    void CLRCore::ScriptingObject::SetInternalValues(CLRClass* klass, CLRObject* object, void* unmanagedPtr, const UID* id)
    {
#if PLATFORM_DESKTOP
        static void* ScriptingObjectSetInternalValuesPtr = GetStaticMethodPointer(SE_TEXT("ScriptingObjectSetInternalValues"));
        CallStaticMethod<void, CLRObject*, void*, const UID*>(ScriptingObjectSetInternalValuesPtr, object, unmanagedPtr, id);
#else
        const SEField* monoUnmanagedPtrField = klass->GetField("__unmanagedPtr");
        if (monoUnmanagedPtrField)
            monoUnmanagedPtrField->SetValue(object, &unmanagedPtr);
        const SEField* monoIdField = klass->GetField("__internalId");
        if (id != nullptr && monoIdField)
            monoIdField->SetValue(object, (void*)id);
#endif
    }

    CLRObject* CLRCore::ScriptingObject::CreateScriptingObject(CLRClass* klass, void* unmanagedPtr, const UID* id)
    {
#if PLATFORM_DESKTOP
        static void* ScriptingObjectSetInternalValuesPtr = GetStaticMethodPointer(SE_TEXT("ScriptingObjectCreate"));
        return CallStaticMethod<CLRObject*, void*, void*, const UID*>(ScriptingObjectSetInternalValuesPtr, klass->m_Handle, unmanagedPtr, id);
#else
        SEObject* object = SECore::Object::New(klass);
        if (object)
        {
            SECore::ScriptingObject::SetInternalValues(klass, object, unmanagedPtr, id);
            SECore::Object::Init(object);
        }
        return object;
#endif
    }

    CLRClass::CLRClass(const CLRAssembly* parentAssembly, void* handle, const char* name, const char* fullname, const char* namespace_, CLRTypeAttributes attributes)
        : m_Handle(handle)
        , m_Name(name)
        , m_Namespace(namespace_)
        , m_Assembly(parentAssembly)
        , m_Fullname(fullname)
        , m_HasCachedProperties(false)
        , m_HasCachedFields(false)
        , m_HasCachedMethods(false)
        , m_HasCachedAttributes(false)
        , m_HasCachedEvents(false)
        , m_HasCachedInterfaces(false)
    {
        ENGINE_ASSERT(handle != nullptr);
        switch (attributes & CLRTypeAttributes::VisibilityMask)
        {
        case CLRTypeAttributes::NotPublic:
        case CLRTypeAttributes::NestedPrivate:
            m_Visibility = CLRVisibility::Private;
            break;
        case CLRTypeAttributes::Public:
        case CLRTypeAttributes::NestedPublic:
            m_Visibility = CLRVisibility::Public;
            break;
        case CLRTypeAttributes::NestedFamily:
        case CLRTypeAttributes::NestedAssembly:
            m_Visibility = CLRVisibility::Internal;
            break;
        case CLRTypeAttributes::NestedFamORAssem:
            m_Visibility = CLRVisibility::ProtectedInternal;
            break;
        case CLRTypeAttributes::NestedFamANDAssem:
            m_Visibility = CLRVisibility::PrivateProtected;
            break;
        default:
            ENGINE_UNREACHABLE_CODE();
        }

        const CLRTypeAttributes staticClassFlags = CLRTypeAttributes::Abstract | CLRTypeAttributes::Sealed;
        m_IsStatic = (attributes & staticClassFlags) == staticClassFlags;
        m_IsSealed = !m_IsStatic && (attributes & CLRTypeAttributes::Sealed) == CLRTypeAttributes::Sealed;
        m_IsAbstract = !m_IsStatic && (attributes & CLRTypeAttributes::Abstract) == CLRTypeAttributes::Abstract;
        m_IsInterface = (attributes & CLRTypeAttributes::ClassSemanticsMask) == CLRTypeAttributes::Interface;

        // TODO: pass type info from C# side at once (pack into flags with attributes)

        static void* TypeIsValueTypePtr = GetStaticMethodPointer(SE_TEXT("TypeIsValueType"));
        m_IsValueType = CallStaticMethod<bool, void*>(TypeIsValueTypePtr, handle);

        static void* TypeIsEnumPtr = GetStaticMethodPointer(SE_TEXT("TypeIsEnum"));
        m_IsEnum = CallStaticMethod<bool, void*>(TypeIsEnumPtr, handle);

        CachedClassHandles[handle] = this;
    }


    CLRClass::~CLRClass()
    {
        m_Methods.ClearDelete();
        m_Fields.ClearDelete();
        m_Properties.ClearDelete();
        m_Events.ClearDelete();

        CachedClassHandles.Remove(m_Handle);
    }

    StringAnsiView CLRClass::GetName() const
    {
        return m_Name;
    }

    StringAnsiView CLRClass::GetNamespace() const
    {
        return m_Namespace;
    }

    CLRType* CLRClass::GetType() const
    {
        return (CLRType*)m_Handle;
    }

    CLRClass* CLRClass::GetBaseClass() const
    {
        static void* GetClassParentPtr = GetStaticMethodPointer(SE_TEXT("GetClassParent"));
        return CallStaticMethod<CLRClass*, void*>(GetClassParentPtr, m_Handle);
    }

    bool CLRClass::IsSubClassOf(const CLRClass* klass, bool checkInterfaces) const
    {
        static void* TypeIsSubclassOfPtr = GetStaticMethodPointer(SE_TEXT("TypeIsSubclassOf"));
        return klass && CallStaticMethod<bool, void*, void*, bool>(TypeIsSubclassOfPtr, m_Handle, klass->m_Handle, checkInterfaces);
    }

    bool CLRClass::HasInterface(const CLRClass* klass) const
    {
        static void* TypeIsAssignableFrom = GetStaticMethodPointer(SE_TEXT("TypeIsAssignableFrom"));
        return klass && CallStaticMethod<bool, void*, void*>(TypeIsAssignableFrom, klass->m_Handle, m_Handle);
    }

    bool CLRClass::IsInstanceOfType(CLRObject* object) const
    {
        if (object == nullptr)
            return false;
        CLRClass* objectClass = CLRCore::Object::GetClass(object);
        return IsSubClassOf(objectClass, false);
    }

    uint32 CLRClass::GetInstanceSize() const
    {
        if (m_Size != 0)
            return m_Size;
        static void* NativeSizeOfPtr = GetStaticMethodPointer(SE_TEXT("NativeSizeOf"));
        m_Size = CallStaticMethod<int, void*>(NativeSizeOfPtr, m_Handle);
        return m_Size;
    }

    CLRClass* CLRClass::GetElementClass() const
    {
        static void* GetElementClassPtr = GetStaticMethodPointer(SE_TEXT("GetElementClass"));
        return CallStaticMethod<CLRClass*, void*>(GetElementClassPtr, m_Handle);
    }

    CLRMethod* CLRClass::GetMethod(const char* name, int32 numParams) const
    {
        GetMethods();
        for (int32 i = 0; i < m_Methods.Count(); i++)
        {
            if (m_Methods[i]->GetParametersCount() == numParams && m_Methods[i]->GetName() == name)
                return m_Methods[i];
        }
        return nullptr;
    }

    const List<CLRMethod*>& CLRClass::GetMethods() const
    {
        if (m_HasCachedMethods)
            return m_Methods;
        Threading::ScopeLock lock(BinaryModule::Locker);
        if (m_HasCachedMethods)
            return m_Methods;

        NativeMethodDefinitions* methods;
        int methodsCount;
        static void* GetClassMethodsPtr = GetStaticMethodPointer(SE_TEXT("GetClassMethods"));
        CallStaticMethod<void, void*, NativeMethodDefinitions**, int*>(GetClassMethodsPtr, m_Handle, &methods, &methodsCount);
        for (int32 i = 0; i < methodsCount; i++)
        {
            NativeMethodDefinitions& definition = methods[i];
            CLRMethod* method = New<CLRMethod>(const_cast<CLRClass*>(this), StringAnsi(definition.name), definition.handle, definition.numParameters, definition.methodAttributes);
            m_Methods.Add(method);
            CLRCore::GC::FreeMemory((void*)definition.name);
        }
        CLRCore::GC::FreeMemory(methods);

        m_HasCachedMethods = true;
        return m_Methods;
    }

    CLRField* CLRClass::GetField(const char* name) const
    {
        GetFields();
        for (int32 i = 0; i < m_Fields.Count(); i++)
        {
            if (m_Fields[i]->GetName() == name)
                return m_Fields[i];
        }
        return nullptr;
    }

    const List<CLRField*>& CLRClass::GetFields() const
    {
        if (m_HasCachedFields)
        {
            return m_Fields;
        }
        Threading::ScopeLock lock(BinaryModule::Locker);
        if (m_HasCachedFields)
            return m_Fields;

        NativeFieldDefinitions* fields;
        int numFields;
        static void* GetClassFieldsPtr = GetStaticMethodPointer(SE_TEXT("GetClassFields"));
        CallStaticMethod<void, void*, NativeFieldDefinitions**, int*>(GetClassFieldsPtr, m_Handle, &fields, &numFields);
        for (int32 i = 0; i < numFields; i++)
        {
            NativeFieldDefinitions& definition = fields[i];
            CLRField* field = New<CLRField>(const_cast<CLRClass*>(this), definition.fieldHandle, definition.name, definition.fieldType, definition.fieldOffset, definition.fieldAttributes);
            m_Fields.Add(field);
            CLRCore::GC::FreeMemory((void*)definition.name);
        }
        CLRCore::GC::FreeMemory(fields);

        m_HasCachedFields = true;
        return m_Fields;
    }

    const List<CLREvent*>& CLRClass::GetEvents() const
    {
        if (m_HasCachedEvents)
            return m_Events;

        // TODO: implement CLREvent in .NET

        m_HasCachedEvents = true;
        return m_Events;
    }

    CLRProperty* CLRClass::GetProperty(const char* name) const
    {
        GetProperties();
        for (int32 i = 0; i < m_Properties.Count(); i++)
        {
            if (m_Properties[i]->GetName() == name)
                return m_Properties[i];
        }
        return nullptr;
    }

    const List<CLRProperty*>& CLRClass::GetProperties() const
    {
        if (m_HasCachedProperties)
            return m_Properties;
        Threading::ScopeLock lock(BinaryModule::Locker);
        if (m_HasCachedProperties)
            return m_Properties;

        NativePropertyDefinitions* foundProperties;
        int numProperties;
        static void* GetClassPropertiesPtr = GetStaticMethodPointer(SE_TEXT("GetClassProperties"));
        CallStaticMethod<void, void*, NativePropertyDefinitions**, int*>(GetClassPropertiesPtr, m_Handle, &foundProperties, &numProperties);
        for (int i = 0; i < numProperties; i++)
        {
            const NativePropertyDefinitions& definition = foundProperties[i];
            CLRProperty* property = New<CLRProperty>(const_cast<CLRClass*>(this), definition.name, definition.getterHandle, definition.setterHandle, definition.getterAttributes, definition.setterAttributes);
            m_Properties.Add(property);
            CLRCore::GC::FreeMemory((void*)definition.name);
        }
        CLRCore::GC::FreeMemory(foundProperties);

        m_HasCachedProperties = true;
        return m_Properties;
    }

    const List<CLRClass*>& CLRClass::GetInterfaces() const
    {
        if (m_HasCachedInterfaces)
        {
            return m_Interfaces;
        }
        Threading::ScopeLock lock(BinaryModule::Locker);
        if (m_HasCachedInterfaces)
            return m_Interfaces;

        CLRType** foundInterfaceTypes;
        int numInterfaces;
        static void* GetClassInterfacesPtr = GetStaticMethodPointer(SE_TEXT("GetClassInterfaces"));
        CallStaticMethod<void, void*, CLRType***, int*>(GetClassInterfacesPtr, m_Handle, &foundInterfaceTypes, &numInterfaces);
        for (int32 i = 0; i < numInterfaces; i++)
        {
            CLRClass* interfaceClass = GetOrCreateClass(foundInterfaceTypes[i]);
            m_Interfaces.Add(interfaceClass);
        }
        CLRCore::GC::FreeMemory(foundInterfaceTypes);

        m_HasCachedInterfaces = true;
        return m_Interfaces;
    }

    bool CLRClass::HasAttribute(const CLRClass* monoClass) const
    {
        return GetCustomAttribute(this, monoClass) != nullptr;
    }

    bool CLRClass::HasAttribute() const
    {
        return !GetAttributes().IsEmpty();
    }

    CLRObject* CLRClass::GetAttribute(const CLRClass* monoClass) const
    {
        return (CLRObject*)GetCustomAttribute(this, monoClass);
    }

    const List<CLRObject*>& CLRClass::GetAttributes() const
    {
        if (m_HasCachedAttributes)
        {
            return m_Attributes;
        }
        Threading::ScopeLock lock(BinaryModule::Locker);
        if (m_HasCachedAttributes)
        {
            return m_Attributes;
        }

        CLRObject** attributes;
        int numAttributes;
        static void* GetClassAttributesPtr = GetStaticMethodPointer(SE_TEXT("GetClassAttributes"));
        CallStaticMethod<void, void*, CLRObject***, int*>(GetClassAttributesPtr, m_Handle, &attributes, &numAttributes);
        m_Attributes.Set(attributes, numAttributes);
        CLRCore::GC::FreeMemory(attributes);

        m_HasCachedAttributes = true;
        return m_Attributes;
    }

    CLREvent::CLREvent(CLRClass* parentClass, void* handle, const char* name)
        : _handle(handle)
        , _addMethod(nullptr)
        , _removeMethod(nullptr)
        , _parentClass(parentClass)
        , _name(name)
        , _hasCachedAttributes(false)
        , _hasAddMonoMethod(true)
        , _hasRemoveMonoMethod(true)
    {
    }

    CLRMethod* CLREvent::GetAddMethod() const
    {
        return nullptr; // TODO: implement CLREvent in .NET
    }

    CLRMethod* CLREvent::GetRemoveMethod() const
    {
        return nullptr; // TODO: implement CLREvent in .NET
    }

    bool CLREvent::HasAttribute(CLRClass* monoClass) const
    {
        return false; // TODO: implement CLREvent in .NET
    }

    bool CLREvent::HasAttribute() const
    {
        return false; // TODO: implement CLREvent in .NET
    }

    CLRObject* CLREvent::GetAttribute(CLRClass* monoClass) const
    {
        return nullptr; // TODO: implement CLREvent in .NET
    }

    const List<CLRObject*>& CLREvent::GetAttributes() const
    {
        if (_hasCachedAttributes)
        {
            return _attributes;
        }
        _hasCachedAttributes = true;

        // TODO: implement CLREvent in .NET
        return _attributes;
    }



    CLRField::CLRField(CLRClass* parentClass, void* handle, const char* name, void* type, int fieldOffset, CLRFieldAttributes attributes)
        : _handle(handle)
        , _type(type)
        , _fieldOffset(fieldOffset)
        , _parentClass(parentClass)
        , _name(name)
        , _hasCachedAttributes(false)
    {
        switch (attributes & CLRFieldAttributes::FieldAccessMask)
        {
        case CLRFieldAttributes::Private:
            _visibility = CLRVisibility::Private;
            break;
        case CLRFieldAttributes::FamANDAssem:
            _visibility = CLRVisibility::PrivateProtected;
            break;
        case CLRFieldAttributes::Assembly:
            _visibility = CLRVisibility::Internal;
            break;
        case CLRFieldAttributes::Family:
            _visibility = CLRVisibility::Protected;
            break;
        case CLRFieldAttributes::FamORAssem:
            _visibility = CLRVisibility::ProtectedInternal;
            break;
        case CLRFieldAttributes::Public:
            _visibility = CLRVisibility::Public;
            break;
        default:
            ENGINE_UNREACHABLE_CODE();
        }
        _isStatic = (attributes & CLRFieldAttributes::Static) == CLRFieldAttributes::Static;
    }

    CLRType* CLRField::GetType() const
    {
        return (CLRType*)_type;
    }

    int32 CLRField::GetOffset() const
    {
        return _fieldOffset;
    }

    void CLRField::GetValue(CLRObject* instance, void* result) const
    {
        static void* FieldGetValuePtr = GetStaticMethodPointer(TEXT("FieldGetValue"));
        CallStaticMethod<void, void*, void*, void*>(FieldGetValuePtr, instance, _handle, result);
    }

    void CLRField::GetValueReference(CLRObject* instance, void* result) const
    {
        static void* FieldGetValueReferencePtr = GetStaticMethodPointer(TEXT("FieldGetValueReference"));
        CallStaticMethod<void, void*, void*, int, void*>(FieldGetValueReferencePtr, instance, _handle, _fieldOffset, result);
    }

    CLRObject* CLRField::GetValueBoxed(CLRObject* instance) const
    {
        static void* FieldGetValueBoxedPtr = GetStaticMethodPointer(TEXT("FieldGetValueBoxed"));
        return CallStaticMethod<CLRObject*, void*, void*>(FieldGetValueBoxedPtr, instance, _handle);
    }

    void CLRField::SetValue(CLRObject* instance, void* value) const
    {
        static void* FieldSetValuePtr = GetStaticMethodPointer(TEXT("FieldSetValue"));
        CallStaticMethod<void, void*, void*, void*>(FieldSetValuePtr, instance, _handle, value);
    }

    bool CLRField::HasAttribute(CLRClass* monoClass) const
    {
        // TODO: implement CLRField attributes in .NET
        return false;
    }

    bool CLRField::HasAttribute() const
    {
        // TODO: implement CLRField attributes in .NET
        return false;
    }

    CLRObject* CLRField::GetAttribute(CLRClass* monoClass) const
    {
        // TODO: implement CLRField attributes in .NET
        return nullptr;
    }

    const List<CLRObject*>& CLRField::GetAttributes() const
    {
        if (_hasCachedAttributes)
            return _attributes;
        _hasCachedAttributes = true;

        // TODO: implement CLRField attributes in .NET
        return _attributes;
    }


    
    CLRProperty::CLRProperty(CLRClass* parentClass, const char* name, void* getterHandle, void* setterHandle, CLRMethodAttributes getterAttributes, CLRMethodAttributes setterAttributes)
        : _parentClass(parentClass)
        , _name(name)
        , _hasCachedAttributes(false)
    {
        _hasGetMethod = getterHandle != nullptr;
        if (_hasGetMethod)
        {
            _getMethod = New<CLRMethod>(parentClass, StringAnsi("get_" + _name), getterHandle, 1, getterAttributes);
        }
        else
        {
            _getMethod = nullptr;
        }
        _hasSetMethod = setterHandle != nullptr;
        if (_hasSetMethod)
        {
            _setMethod = New<CLRMethod>(parentClass, StringAnsi("set_" + _name), setterHandle, 1, setterAttributes);
        }
        else
        {
            _setMethod = nullptr;
        }
    }

    CLRProperty::~CLRProperty()
    {
        if (_getMethod)
            Delete(_getMethod);
        if (_setMethod)
            Delete(_setMethod);
    }

    CLRMethod* CLRProperty::GetGetMethod() const
    {
        return _getMethod;
    }

    CLRMethod* CLRProperty::GetSetMethod() const
    {
        return _setMethod;
    }

    CLRObject* CLRProperty::GetValue(CLRObject* instance, CLRObject** exception) const
    {
        if (_getMethod == nullptr)
        {
            return nullptr;
        }

        return _getMethod->Invoke(instance, nullptr, exception);
    }

    void CLRProperty::SetValue(CLRObject* instance, void* value, CLRObject** exception) const
    {
        ENGINE_ASSERT(_setMethod);
        void* params[1];
        params[0] = value;
        _setMethod->Invoke(instance, params, exception);
    }

    bool CLRProperty::HasAttribute(CLRClass* monoClass) const
    {
        // TODO: implement CLRProperty attributes in .NET
        return false;
    }

    bool CLRProperty::HasAttribute() const
    {
        // TODO: implement CLRProperty attributes in .NET
        return false;
    }

    CLRObject* CLRProperty::GetAttribute(CLRClass* monoClass) const
    {
        // TODO: implement CLRProperty attributes in .NET
        return nullptr;
    }

    const List<CLRObject*>& CLRProperty::GetAttributes() const
    {
        if (_hasCachedAttributes)
            return _attributes;
        _hasCachedAttributes = true;

        // TODO: implement CLRProperty attributes in .NET
        return _attributes;
    }


    CLRMethod::CLRMethod(CLRClass* parentClass, StringAnsi&& name, void* handle, int32 paramsCount, CLRMethodAttributes attributes)
        : _handle(handle)
        , _paramsCount(paramsCount)
        , _parentClass(parentClass)
        , _name(MoveTemp(name))
        , _hasCachedAttributes(false)
        , _hasCachedSignature(false)
    {
        switch (attributes & CLRMethodAttributes::MemberAccessMask)
        {
        case CLRMethodAttributes::Private:
            _visibility = CLRVisibility::Private;
            break;
        case CLRMethodAttributes::FamANDAssem:
            _visibility = CLRVisibility::PrivateProtected;
            break;
        case CLRMethodAttributes::Assembly:
            _visibility = CLRVisibility::Internal;
            break;
        case CLRMethodAttributes::Family:
            _visibility = CLRVisibility::Protected;
            break;
        case CLRMethodAttributes::FamORAssem:
            _visibility = CLRVisibility::ProtectedInternal;
            break;
        case CLRMethodAttributes::Public:
            _visibility = CLRVisibility::Public;
            break;
        default:
            ENGINE_UNREACHABLE_CODE();
        }
        _isStatic = (attributes & CLRMethodAttributes::Static) == CLRMethodAttributes::Static;

#if COMPILE_WITH_PROFILER
        const StringAnsi& className = parentClass->GetFullName();
        ProfilerName.Resize(className.Length() + 2 + _name.Length());
        Platform::MemoryCopy(ProfilerName.Get(), className.Get(), className.Length());
        ProfilerName.Get()[className.Length()] = ':';
        ProfilerName.Get()[className.Length() + 1] = ':';
        Platform::MemoryCopy(ProfilerName.Get() + className.Length() + 2, _name.Get(), _name.Length());
        ProfilerData.name = ProfilerName.Get();
        ProfilerData.function = _name.Get();
        ProfilerData.file = nullptr;
        ProfilerData.line = 0;
        ProfilerData.color = 0;
#endif
    }

    void CLRMethod::CacheSignature() const
    {
        Threading::ScopeLock lock(BinaryModule::Locker);
        if (_hasCachedSignature)
            return;

        static void* GetMethodReturnTypePtr = GetStaticMethodPointer(TEXT("GetMethodReturnType"));
        static void* GetMethodParameterTypesPtr = GetStaticMethodPointer(TEXT("GetMethodParameterTypes"));
        _returnType = CallStaticMethod<void*, void*>(GetMethodReturnTypePtr, _handle);
        if (_paramsCount != 0)
        {
            void** parameterTypeHandles;
            CallStaticMethod<void, void*, void***>(GetMethodParameterTypesPtr, _handle, &parameterTypeHandles);
            _parameterTypes.Set(parameterTypeHandles, _paramsCount);
            CLRCore::GC::FreeMemory(parameterTypeHandles);
        }

        _hasCachedSignature = true;
    }

    CLRObject* CLRMethod::Invoke(void* instance, void** params, CLRObject** exception) const
    {
        // PROFILE_CPU_SRC_LOC(ProfilerData);
        static void* InvokeMethodPtr = GetStaticMethodPointer(TEXT("InvokeMethod"));
        return (CLRObject*)CallStaticMethod<void*, void*, void*, void*, void*>(InvokeMethodPtr, instance, _handle, params, exception);
    }

    CLRObject* CLRMethod::InvokeVirtual(CLRObject* instance, void** params, CLRObject** exception) const
    {
        return Invoke(instance, params, exception);
    }

#if !USE_MONO_AOT

    void* CLRMethod::GetThunk()
    {
        if (!_cachedThunk)
        {
            static void* GetThunkPtr = GetStaticMethodPointer(TEXT("GetThunk"));
            _cachedThunk = CallStaticMethod<void*, void*>(GetThunkPtr, _handle);
#if !BUILD_RELEASE
            if (!_cachedThunk)
                LOG_ERROR("Scripting", "Failed to get C# method thunk for {0}::{1}", _parentClass->GetFullName(), _name);
#endif
        }
        return _cachedThunk;
    }

#endif

    CLRMethod* CLRMethod::InflateGeneric() const
    {
        // This seams to be unused on .NET (Mono required inflating generic class of the script)
        return const_cast<CLRMethod*>(this);
    }

    CLRType* CLRMethod::GetReturnType() const
    {
        if (!_hasCachedSignature)
            CacheSignature();
        return (CLRType*)_returnType;
    }

    int32 CLRMethod::GetParametersCount() const
    {
        return _paramsCount;
    }

    CLRType* CLRMethod::GetParameterType(int32 paramIdx) const
    {
        if (!_hasCachedSignature)
            CacheSignature();
        ASSERT_LOW_LAYER(paramIdx >= 0 && paramIdx < _paramsCount);
        return (CLRType*)_parameterTypes.Get()[paramIdx];
    }

    bool CLRMethod::GetParameterIsOut(int32 paramIdx) const
    {
        if (!_hasCachedSignature)
            CacheSignature();
        ASSERT_LOW_LAYER(paramIdx >= 0 && paramIdx < _paramsCount);
        // TODO: cache GetParameterIsOut maybe?
        static void* GetMethodParameterIsOutPtr = GetStaticMethodPointer(TEXT("GetMethodParameterIsOut"));
        return CallStaticMethod<bool, void*, int>(GetMethodParameterIsOutPtr, _handle, paramIdx);
    }

    bool CLRMethod::HasAttribute(CLRClass* monoClass) const
    {
        // TODO: implement CLRMethod attributes in .NET
        return false;
    }

    bool CLRMethod::HasAttribute() const
    {
        // TODO: implement CLRMethod attributes in .NET
        return false;
    }

    CLRObject* CLRMethod::GetAttribute(CLRClass* monoClass) const
    {
        // TODO: implement CLRMethod attributes in .NET
        return nullptr;
    }

    const List<CLRObject*>& CLRMethod::GetAttributes() const
    {
        if (_hasCachedAttributes)
            return _attributes;
        _hasCachedAttributes = true;

        // TODO: implement CLRMethod attributes in .NET
        return _attributes;
    }

    CLRException::CLRException(CLRObject* exception)
        : InnerException(nullptr)
    {
        ENGINE_ASSERT(exception);
        CLRClass* exceptionClass = CLRCore::Object::GetClass(exception);

        CLRProperty* exceptionMsgProp = exceptionClass->GetProperty("Message");
        CLRMethod* exceptionMsgGetter = exceptionMsgProp->GetGetMethod();
        CLRString* exceptionMsg = (CLRString*)exceptionMsgGetter->Invoke(exception, nullptr, nullptr);
        Message = CLRUtils::ToString(exceptionMsg);

        CLRProperty* exceptionStackProp = exceptionClass->GetProperty("StackTrace");
        CLRMethod* exceptionStackGetter = exceptionStackProp->GetGetMethod();
        CLRString* exceptionStackTrace = (CLRString*)exceptionStackGetter->Invoke(exception, nullptr, nullptr);
        StackTrace = CLRUtils::ToString(exceptionStackTrace);

        CLRProperty* innerExceptionProp = exceptionClass->GetProperty("InnerException");
        CLRMethod* innerExceptionGetter = innerExceptionProp->GetGetMethod();
        CLRObject* innerException = (CLRObject*)innerExceptionGetter->Invoke(exception, nullptr, nullptr);
        if (innerException)
        {
            InnerException = New<CLRException>(innerException);
        }
    }

    CLRException::~CLRException()
    {
        if (InnerException)
        {
            Delete(InnerException);
        }
    }

    const CLRAssembly::ClassesDictionary& CLRAssembly::GetClasses() const
    {
        if (m_HasCachedClasses || !IsLoaded())
            return m_Classes;
        PROFILE_CPU();
        Stopwatch stopwatch;

#if TRACY_ENABLE
        ZoneText(*m_Name, m_Name.Length());
#endif
        Threading::ScopeLock lock(BinaryModule::Locker);
        if (m_HasCachedClasses)
            return m_Classes;

        ENGINE_ASSERT(m_Classes.IsEmpty());

        NativeClassDefinitions* managedClasses;
        int classCount;
        static void* GetManagedClassesPtr = GetStaticMethodPointer(SE_TEXT("GetManagedClasses"));
        CallStaticMethod<void, void*, NativeClassDefinitions**, int*>(GetManagedClassesPtr, m_Handle, &managedClasses, &classCount);
        m_Classes.EnsureCapacity(classCount);
        for (int32 i = 0; i < classCount; i++)
        {
            NativeClassDefinitions& managedClass = managedClasses[i];

            // Create class object
            CLRClass* klass = New<CLRClass>(this, managedClass.typeHandle, managedClass.name, managedClass.fullname, managedClass.namespace_, managedClass.typeAttributes);
            m_Classes.Add(klass->GetFullName(), klass);

            managedClass.nativePointer = klass;

            CLRCore::GC::FreeMemory((void*)managedClasses[i].name);
            CLRCore::GC::FreeMemory((void*)managedClasses[i].fullname);
            CLRCore::GC::FreeMemory((void*)managedClasses[i].namespace_);
        }

        static void* RegisterManagedClassNativePointersPtr = GetStaticMethodPointer(SE_TEXT("RegisterManagedClassNativePointers"));
        CallStaticMethod<void, NativeClassDefinitions**, int>(RegisterManagedClassNativePointersPtr, &managedClasses, classCount);

        CLRCore::GC::FreeMemory(managedClasses);

        stopwatch.Stop();
        LOG_INFO("Scripting", "Caching classes for assembly {0} took {1}ms", m_Name, stopwatch.GetMilliseconds());


#if 0
        for (auto i = _classes.Begin(); i.IsNotEnd(); ++i)
            LOG(Info, "Class: {0}", String(i->Value->GetFullName()));
#endif

        m_HasCachedClasses = true;
        return m_Classes;
    }

    void GetAssemblyName(void* assemblyHandle, StringAnsi& name, StringAnsi& fullname)
    {
        static void* GetAssemblyNamePtr = GetStaticMethodPointer(SE_TEXT("GetAssemblyName"));
        const char* name_;
        const char* fullname_;
        CallStaticMethod<void, void*, const char**, const char**>(GetAssemblyNamePtr, assemblyHandle, &name_, &fullname_);
        name = name_;
        fullname = fullname_;
        CLRCore::GC::FreeMemory((void*)name_);
        CLRCore::GC::FreeMemory((void*)fullname_);
    }

    DEFINE_INTERNAL_CALL(void) NativeInterop_CreateClass(NativeClassDefinitions* managedClass, void* assemblyHandle)
    {
        Threading::ScopeLock lock(BinaryModule::Locker);
        CLRAssembly* assembly = GetAssembly(assemblyHandle);
        if (assembly == nullptr)
        {
            StringAnsi assemblyName;
            StringAnsi assemblyFullName;
            GetAssemblyName(assemblyHandle, assemblyName, assemblyFullName);
            assembly = New<CLRAssembly>(nullptr, assemblyName, assemblyFullName, assemblyHandle);
            CachedAssemblyHandles.Add(assemblyHandle, assembly);
        }

        CLRClass* klass = New<CLRClass>(assembly, managedClass->typeHandle, managedClass->name, managedClass->fullname, managedClass->namespace_, managedClass->typeAttributes);
        if (assembly != nullptr)
        {
            auto& classes = const_cast<CLRAssembly::ClassesDictionary&>(assembly->GetClasses());
            CLRClass* oldKlass;
            if (classes.TryGet(klass->GetFullName(), oldKlass))
            {
                LOG_WARNING("Scripting", "Class '{0}' was already added to assembly '{1}'", String(klass->GetFullName()), String(assembly->GetName()));
                Delete(klass);
                klass = oldKlass;
            }
            else
            {
                classes.Add(klass->GetFullName(), klass);
            }
        }
        managedClass->nativePointer = klass;
    }

    bool CLRAssembly::LoadCorlib()
    {
        if (IsLoaded())
        {
            return true;
        }

        PROFILE_CPU();
#if TRACY_ENABLE
        const StringAnsiView name("Corlib");
        ZoneText(*name, name.Length());
#endif

        // Ensure to be unloaded
        Unload();

        // Start
        Stopwatch stopwatch;
        OnLoading();

        // Load
        {
            static void* GetAssemblyByNamePtr = GetStaticMethodPointer(SE_TEXT("GetAssemblyByName"));
            m_Handle = CallStaticMethod<void*, const char*>(GetAssemblyByNamePtr, "System.Private.CoreLib");
            GetAssemblyName(m_Handle, m_Name, m_Fullname);
        }
        if (m_Handle == nullptr)
        {
            OnLoadFailed();
            return false;
        }
        m_HasCachedClasses = false;
        CachedAssemblyHandles.Add(m_Handle, this);

        // End
        OnLoaded(stopwatch);
        return true;
    }

    bool CLRAssembly::LoadImage(const String& assemblyPath, const StringView& nativePath)
    {
        // TODO: Use new hostfxr delegate load_assembly_bytes? (.NET 8+)
        // Open .Net assembly
        static void* LoadAssemblyImagePtr = GetStaticMethodPointer(SE_TEXT("LoadAssemblyImage"));
        m_Handle = CallStaticMethod<void*, const Char*>(LoadAssemblyImagePtr, assemblyPath.Get());
        if (m_Handle == nullptr)
        {
            LOG_ERROR("Scripting", ".NET assembly image is invalid at {0}", assemblyPath);
            return false;
        }
        GetAssemblyName(m_Handle, m_Name, m_Fullname);
        CachedAssemblyHandles.Add(m_Handle, this);

        // Provide new path of hot-reloaded native library path for managed DllImport
        if (nativePath.HasChars())
        {
            StringAnsi nativeName = m_Name.EndsWith(".CSharp") ? StringAnsi(m_Name.Get(), m_Name.Length() - 7) : StringAnsi(m_Name);
            RegisterNativeLibrary(nativeName.Get(), nativePath.Get());
        }
#if SE_EDITOR
        // Register the editor module location for Assembly resolver
        else
        {
            RegisterNativeLibrary(m_Name.Get(), assemblyPath.Get());
        }
#endif

        m_HasCachedClasses = false;
        m_AssemblyPath = assemblyPath;
        return true;
    }

    bool CLRAssembly::UnloadImage(bool isReloading)
    {
        if (m_Handle && isReloading)
        {
            LOG_INFO("Scripting", "Unloading managed assembly \'{0}\' (is reloading)", m_Name);

            static void* CloseAssemblyPtr = GetStaticMethodPointer(SE_TEXT("CloseAssembly"));
            CallStaticMethod<void, const void*>(CloseAssemblyPtr, m_Handle);

            CachedAssemblyHandles.Remove(m_Handle);
            m_Handle = nullptr;
        }
        return false;
    }

    bool CLRAssembly::ResolveMissingFile(String& assemblyPath) const
    {
#if DOTNET_HOST_MONO
        // Fallback to AOT-ed assembly location
        assemblyPath = Globals::BinariesFolder / SE_TEXT("Dotnet") / StringUtils::GetFileName(assemblyPath);
        return !FileSystem::FileExists(assemblyPath);
#endif
        return true;
    }

    bool CLRDomain::SetCurrentDomain(bool force)
    {
        m_ActiveDomain = this;
        return true;
    }

    void CLRDomain::Dispatch() const
    {
    }

    CLRAssembly* GetAssembly(void* assemblyHandle)
    {
        Threading::ScopeLock lock(BinaryModule::Locker);
        CLRAssembly* assembly;
        if (CachedAssemblyHandles.TryGet(assemblyHandle, assembly))
        {
            return assembly;
        }
        return nullptr;
    }


    CLRClass* GetClass(CLRType* typeHandle)
    {
        Threading::ScopeLock lock(BinaryModule::Locker);
        CLRClass* klass = nullptr;
        CachedClassHandles.TryGet(typeHandle, klass);
        return nullptr;
    }

    CLRClass* GetOrCreateClass(CLRType* typeHandle)
    {
        if (!typeHandle)
        {
            return nullptr;
        }
        Threading::ScopeLock lock(BinaryModule::Locker);
        CLRClass* klass;
        if (!CachedClassHandles.TryGet(typeHandle, klass))
        {
            NativeClassDefinitions classInfo;
            void* assemblyHandle;
            static void* GetManagedClassFromTypePtr = GetStaticMethodPointer(TEXT("GetManagedClassFromType"));
            CallStaticMethod<void, void*, void*>(GetManagedClassFromTypePtr, typeHandle, &classInfo, &assemblyHandle);
            CLRAssembly* assembly = GetAssembly(assemblyHandle);
            klass = New<CLRClass>(assembly, classInfo.typeHandle, classInfo.name, classInfo.fullname, classInfo.namespace_, classInfo.typeAttributes);
            if (assembly != nullptr)
            {
                auto& classes = const_cast<CLRAssembly::ClassesDictionary&>(assembly->GetClasses());
                if (classes.ContainsKey(klass->GetFullName()))
                {
                    LOG_WARNING("Scripting", "Class '{0}' was already added to assembly '{1}'", klass->GetFullName(), assembly->GetName());
                }
                classes[klass->GetFullName()] = klass;
            }

            if (typeHandle != classInfo.typeHandle)
                CallStaticMethod<void, void*, void*>(GetManagedClassFromTypePtr, typeHandle, &classInfo);

            CLRCore::GC::FreeMemory((void*)classInfo.name);
            CLRCore::GC::FreeMemory((void*)classInfo.fullname);
            CLRCore::GC::FreeMemory((void*)classInfo.namespace_);
        }
        ENGINE_ASSERT(klass != nullptr);
        return klass;
    }

    CLRType* GetObjectType(CLRObject* obj)
    {
        static void* GetObjectTypePtr = GetStaticMethodPointer(TEXT("GetObjectType"));
        void* typeHandle = CallStaticMethod<void*, void*>(GetObjectTypePtr, obj);
        return (CLRType*)typeHandle;
    }

    void* GetCustomAttribute(const CLRClass* klass, const CLRClass* attributeClass)
    {
        const List<CLRObject*>& attributes = klass->GetAttributes();
        for (CLRObject* attr : attributes)
        {
            CLRClass* attrClass = CLRCore::Object::GetClass(attr);
            if (attrClass == attributeClass)
            {
                return attr;
            }
        }
        return nullptr;
    }

    
    
    const char_t* NativeInteropTypeName = SE_CORECLR_TEXT("SE.Interop.NativeInterop, SERuntimeCSharp");
    hostfxr_initialize_for_runtime_config_fn hostfxr_initialize_for_runtime_config;
    hostfxr_initialize_for_dotnet_command_line_fn hostfxr_initialize_for_dotnet_command_line;
    hostfxr_get_runtime_delegate_fn hostfxr_get_runtime_delegate;
    hostfxr_close_fn hostfxr_close;
    load_assembly_and_get_function_pointer_fn load_assembly_and_get_function_pointer;
    get_function_pointer_fn get_function_pointer;
    hostfxr_set_error_writer_fn hostfxr_set_error_writer;
    hostfxr_get_dotnet_environment_info_result_fn hostfxr_get_dotnet_environment_info_result;
    hostfxr_run_app_fn hostfxr_run_app;

    bool InitHostfxr()
    {
        const String csharpLibraryPath = EngineContext::BinariesFolder / SE_TEXT("SERuntimeCSharp.dll");
        const String csharpRuntimeConfigPath = EngineContext::BinariesFolder / SE_TEXT("SERuntimeCSharp.runtimeconfig.json");
        if (!FileSystem::FileExists(csharpLibraryPath))
        {
            LOG_FATAL("Scripting", "Failed to initialize .NET runtime, missing file: {0}", csharpLibraryPath);
        }
        if (!FileSystem::FileExists(csharpRuntimeConfigPath))
        {
            LOG_FATAL("Scripting", "Failed to initialize .NET runtime, missing file: {0}", csharpRuntimeConfigPath);
        }
        const SE_CORECLR_STRING& libraryPath = SE_CORECLR_STRING(csharpLibraryPath);

        // Get path to hostfxr library
        get_hostfxr_parameters get_hostfxr_params;
        get_hostfxr_params.size = sizeof(get_hostfxr_parameters);
        get_hostfxr_params.assembly_path = libraryPath.Get();
#if PLATFORM_MAC
        ::String macOSDotnetRoot = SE_TEXT("/usr/local/share/dotnet");
#if defined(__x86_64) || defined(__x86_64__) || defined(__amd64__) || defined(_M_X64)
        // When emulating x64 on arm
        const ::String dotnetRootEmulated = macOSDotnetRoot / SE_TEXT("x64");
        if (FileSystem::FileExists(dotnetRootEmulated / SE_TEXT("dotnet"))) {
            macOSDotnetRoot = dotnetRootEmulated;
        }
#endif
        const FLAX_CORECLR_STRING& finalDotnetRootPath = FLAX_CORECLR_STRING(macOSDotnetRoot);
        get_hostfxr_params.dotnet_root = finalDotnetRootPath.Get();
#else
        get_hostfxr_params.dotnet_root = nullptr;
#endif
        SE_CORECLR_STRING dotnetRoot;
        String dotnetRootEnvVar;
        if (!Platform::GetEnvironmentVariable(SE_TEXT("DOTNET_ROOT"), dotnetRootEnvVar) && FileSystem::DirectoryExists(dotnetRootEnvVar))
        {
            dotnetRoot = SE_CORECLR_STRING(dotnetRootEnvVar);
            get_hostfxr_params.dotnet_root = dotnetRoot.Get();
        }
#if !SE_EDITOR
        const String bundledDotnetPath = EngineContext::ProjectFolder / SE_TEXT("Dotnet");
        if (FileSystem::DirectoryExists(bundledDotnetPath))
        {
            dotnetRoot = SE_CORECLR_STRING(bundledDotnetPath);
#if PLATFORM_WINDOWS_FAMILY
            dotnetRoot.Replace('/', '\\');
#endif
            get_hostfxr_params.dotnet_root = dotnetRoot.Get();
        }
#endif
        char_t hostfxrPath[1024];
        size_t hostfxrPathSize = sizeof(hostfxrPath) / sizeof(char_t);
        int rc = get_hostfxr_path(hostfxrPath, &hostfxrPathSize, &get_hostfxr_params);
        if (rc != 0)
        {
            LOG_ERROR("Scripting", "Failed to find hostfxr: {0:x} ({1})", (unsigned int)rc, String(get_hostfxr_params.dotnet_root));

            // Warn user about missing .Net
#if PLATFORM_DESKTOP
            // Platform::OpenUrl(SE_TEXT("https://dotnet.microsoft.com/en-us/download/dotnet/8.0"));
#endif

#if SE_EDITOR
            LOG_FATAL("Scripting", "Missing .NET 8 or later SDK installation required to run Flax Editor.");
#else
            LOG_FATAL("Scripting", "Missing .NET 8 or later Runtime installation required to run this application.");
#endif
            return false;
        }

        String path(hostfxrPath);
        LOG_INFO("Scripting", "Found hostfxr in {0}", path);

        // Get API from hostfxr library
        void* hostfxr = Platform::LoadLibrary(path.Get());
        if (hostfxr == nullptr)
        {
            if (FileSystem::FileExists(path))
            {
                LOG_FATAL("Scripting", "Failed to load hostfxr library, possible platform/architecture mismatch with the library. See log for more information. ({0})", path);
            }
            else
            {
                LOG_FATAL("Scripting", "Failed to load hostfxr library ({0})", path);
            }
            return false;
        }
        hostfxr_initialize_for_runtime_config = (hostfxr_initialize_for_runtime_config_fn)Platform::GetProcAddress(hostfxr, "hostfxr_initialize_for_runtime_config");
        hostfxr_initialize_for_dotnet_command_line = (hostfxr_initialize_for_dotnet_command_line_fn)Platform::GetProcAddress(hostfxr, "hostfxr_initialize_for_dotnet_command_line");
        hostfxr_get_runtime_delegate = (hostfxr_get_runtime_delegate_fn)Platform::GetProcAddress(hostfxr, "hostfxr_get_runtime_delegate");
        hostfxr_close = (hostfxr_close_fn)Platform::GetProcAddress(hostfxr, "hostfxr_close");
        hostfxr_set_error_writer = (hostfxr_set_error_writer_fn)Platform::GetProcAddress(hostfxr, "hostfxr_set_error_writer");
        hostfxr_get_dotnet_environment_info_result = (hostfxr_get_dotnet_environment_info_result_fn)Platform::GetProcAddress(hostfxr, "hostfxr_get_dotnet_environment_info_result");
        hostfxr_run_app = (hostfxr_run_app_fn)Platform::GetProcAddress(hostfxr, "hostfxr_run_app");
        if (!hostfxr_get_runtime_delegate || !hostfxr_run_app)
        {
            LOG_FATAL("Scripting", "Failed to setup hostfxr API ({0})", path);
            return false;
        }

        // TODO: Implement support for picking RC/beta updates of .NET runtime
        // Uncomment for enabling support for upcoming .NET major release candidates
#if 0
        String dotnetRollForwardPr;
        if (Platform::GetEnvironmentVariable(SE_TEXT("DOTNET_ROLL_FORWARD_TO_PRERELEASE"), dotnetRollForwardPr))
            Platform::SetEnvironmentVariable(SE_TEXT("DOTNET_ROLL_FORWARD_TO_PRERELEASE"), SE_TEXT("1"));
#endif

        // Initialize hosting component
        const char_t* argv[1] = { libraryPath.Get() };
        /*hostfxr_initialize_parameters init_params;
        init_params.size = sizeof(hostfxr_initialize_parameters);
        init_params.host_path = libraryPath.Get();
        path = String(FileSystem::GetParentDirectory(path)) / SE_TEXT("/../../../");
        FileSystem::PathRemoveRelativeParts(path);
        dotnetRoot = SE_CORECLR_STRING(path);
        init_params.dotnet_root = dotnetRoot.Get();*/
        hostfxr_initialize_parameters init_params;
        init_params.size = sizeof(hostfxr_initialize_parameters);
        init_params.host_path = nullptr;
        init_params.dotnet_root = nullptr;
        hostfxr_handle handle = nullptr;
        rc = hostfxr_initialize_for_dotnet_command_line(ARRAY_SIZE(argv), argv, &init_params, &handle);
        if (rc != 0 || handle == nullptr)
        {
            hostfxr_close(handle);
            if (rc == 0x80008096) // FrameworkMissingFailure
            {
                String platformStr;
                switch (PLATFORM_TYPE)
                {
                case PlatformType::Windows:
                case PlatformType::UWP:
                    platformStr = PLATFORM_64BITS ? "Windows x64" : "Windows x86";
                    break;
                case PlatformType::Linux:
                    platformStr = PLATFORM_ARCH_ARM64 ? "Linux Arm64" : PLATFORM_ARCH_ARM ? "Linux Arm32" : PLATFORM_64BITS ? "Linux x64" : "Linux x86";
                    break;
                case PlatformType::Mac:
                    platformStr = PLATFORM_ARCH_ARM || PLATFORM_ARCH_ARM64 ? "macOS Arm64" : PLATFORM_64BITS ? "macOS x64" : "macOS x86";
                    break;
                default:;
                    platformStr = "";
                }
                LOG_FATAL("Scripting", "Failed to resolve compatible .NET runtime version in '{0}'. Make sure the correct platform version for runtime is installed ({1})", platformStr, String(init_params.dotnet_root));
            }
            else
                LOG_FATAL("Scripting", "Failed to initialize hostfxr: {0:x} ({1})", (unsigned int)rc, String(init_params.dotnet_root));
            return false;
        }

        void* pget_function_pointer = nullptr;
        rc = hostfxr_get_runtime_delegate(handle, hdt_get_function_pointer, &pget_function_pointer);
        if (rc != 0 || pget_function_pointer == nullptr)
        {
            hostfxr_close(handle);
            LOG_FATAL("Scripting", "Failed to get runtime delegate hdt_get_function_pointer: 0x{0:x}", (unsigned int)rc);
            return false;
        }

        hostfxr_close(handle);
        // load_assembly_and_get_function_pointer = (load_assembly_and_get_function_pointer_fn)pget_function_pointer;
        get_function_pointer = (get_function_pointer_fn)pget_function_pointer;
        return true;
    }

    void ShutdownHostfxr()
    {
    }

    void* GetStaticMethodPointer(const String& methodName)
    {
        void* fun;
        if (CachedFunctions.TryGet(methodName, fun))
        {
            return fun;
        }
        PROFILE_CPU();
        const int rc = get_function_pointer(NativeInteropTypeName,
            methodName.Get(),
            UNMANAGEDCALLERSONLY_METHOD,
            nullptr,
            nullptr,
            &fun);
        if (rc != 0)
        {
            LOG_FATAL("Scripting", "Failed to get unmanaged function pointer for method '{0}': 0x{1:x}", methodName, (unsigned int)rc);
        }
        CachedFunctions.Add(methodName, fun);
        return fun;
    }
}
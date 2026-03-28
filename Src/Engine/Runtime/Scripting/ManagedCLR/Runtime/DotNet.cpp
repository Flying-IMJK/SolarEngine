

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

#include "Runtime/Scripting/ManagedCLR/SEClass.h"
#include "Runtime/Scripting/ManagedCLR/SECore.h"
#include "Runtime/Scripting/ManagedCLR/SEAssembly.h"
#include "Runtime/ThirdParty/netCore/NetCoreInclude.h"
#include "Runtime/EngineContext.h"
#include "Runtime/Scripting/Binary/BinaryModule.h"

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
    extern SEDomain* MRootDomain;
    extern SEDomain* MActiveDomain;
    extern List<SEDomain*, FixedAllocation<4>> MDomains;

    Dictionary<String, void*> CachedFunctions;
    Dictionary<void*, SEClass*> CachedClassHandles;
    Dictionary<void*, SEAssembly*> CachedAssemblyHandles;

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

    SEAssembly* GetAssembly(void* assemblyHandle);
    SEClass* GetClass(SEType* typeHandle);
    SEClass* GetOrCreateClass(SEType* typeHandle);
    SEType* GetObjectType(SEObject* obj);

    void* GetCustomAttribute(const SEClass* klass, const SEClass* attributeClass);


    // System.Reflection.TypeAttributes
    enum class MTypeAttributes : uint32
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
    enum class MMethodAttributes : uint32
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
    enum class MFieldAttributes : uint32
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

    bool SECore::LoadEngine(const char* runtimeConfigPath, const char* assemblyPath)
    {
        PROFILE_CPU();

        // Initialize hostfxr
        if (InitHostfxr())
            return true;

        // Prepare managed side
        CallStaticMethod<void>(GetStaticMethodPointer(SE_TEXT("Init")));
#ifdef MCORE_MAIN_MODULE_NAME
        // MCORE_MAIN_MODULE_NAME define is injected by Scripting.Build.cs on platforms that use separate shared library for engine symbols
        ::SE::String flaxLibraryPath(Platform::GetMainDirectory() / SE_TEXT(MACRO_TO_STR(MCORE_MAIN_MODULE_NAME)));
#else
        ::SE::String flaxLibraryPath = Platform::GetExecutableFilePath();
#endif

#if PLATFORM_MAC
        // On some platforms all native binaries are side-by-side with the app in a different folder
        if (!FileSystem::FileExists(flaxLibraryPath))
        {
            flaxLibraryPath = ::String(StringUtils::GetDirectoryName(Platform::GetExecutableFilePath())) / StringUtils::GetFileName(flaxLibraryPath);
        }
#endif
#if !PLATFORM_SWITCH
        if (!FileSystem::FileExists(flaxLibraryPath))
        {
            LOG_ERROR("Scripting", "Flax Engine native library file is missing ({0})", flaxLibraryPath);
        }
#endif
        RegisterNativeLibrary("FlaxEngine", flaxLibraryPath.Get());

        MRootDomain = New<SEDomain>("Root");
        MDomains.Add(MRootDomain);

        char* buildInfo = CallStaticMethod<char*>(GetStaticMethodPointer(SE_TEXT("GetRuntimeInformation")));
        LOG_INFO("Scripting", ".NET runtime version: {0}", ::SE::String(buildInfo));
        SECore::GC::FreeMemory(buildInfo);

        return false;
    }

    void SECore::UnloadEngine()
    {
        if (!MRootDomain)
            return;
        PROFILE_CPU();
        CallStaticMethod<void>(GetStaticMethodPointer(SE_TEXT("Exit")));
        MDomains.ClearDelete();
        MRootDomain = nullptr;
        ShutdownHostfxr();
    }

    // -------------------------------------------------------------------------
    // GCHandle namespace
    // -------------------------------------------------------------------------

    GCHandle SECore::Handle::New(SEObject* obj, bool pinned)
    {
        ASSERT(obj);
        static void* NewGCHandlePtr = GetStaticMethodPointer(SE_TEXT("NewGCHandle"));
        return (GCHandle)CallStaticMethod<void*, void*, bool>(NewGCHandlePtr, obj, pinned);
    }

    GCHandle SECore::Handle::NewWeak(SEObject* obj, bool trackResurrection)
    {
        ASSERT(obj);
        static void* NewGCHandleWeakPtr = GetStaticMethodPointer(SE_TEXT("NewGCHandleWeak"));
        return (GCHandle)CallStaticMethod<void*, void*, bool>(NewGCHandleWeakPtr, obj, trackResurrection);
    }

    SEObject* SECore::Handle::GetTarget(const GCHandle& handle)
    {
        return (SEObject*)(void*)handle;
    }

    void SECore::Handle::Free(const GCHandle& handle)
    {
        static void* FreeGCHandlePtr = GetStaticMethodPointer(SE_TEXT("FreeGCHandle"));
        CallStaticMethod<void, void*>(FreeGCHandlePtr, (void*)handle);
    }


    SEObject* SECore::Object::Box(void* value, const SEClass* klass)
    {
        static void* BoxValuePtr = GetStaticMethodPointer(SE_TEXT("BoxValue"));
        return (SEObject*)CallStaticMethod<void*, void*, void*>(BoxValuePtr, klass->m_Handle, value);
    }

    void* SECore::Object::Unbox(SEObject* obj)
    {
        static void* UnboxValuePtr = GetStaticMethodPointer(SE_TEXT("UnboxValue"));
        return CallStaticMethod<void*, void*>(UnboxValuePtr, obj);
    }

    SEObject* SECore::Object::New(const SEClass* klass)
    {
        static void* NewObjectPtr = GetStaticMethodPointer(SE_TEXT("NewObject"));
        return (SEObject*)CallStaticMethod<void*, void*>(NewObjectPtr, klass->m_Handle);
    }

    void SECore::Object::Init(SEObject* obj)
    {
        static void* ObjectInitPtr = GetStaticMethodPointer(SE_TEXT("ObjectInit"));
        CallStaticMethod<void, void*>(ObjectInitPtr, obj);
    }

    SEClass* SECore::Object::GetClass(SEObject* obj)
    {
        ASSERT(obj);
        static void* GetObjectClassPtr = GetStaticMethodPointer(SE_TEXT("GetObjectClass"));
        return (SEClass*)CallStaticMethod<SEClass*, void*>(GetObjectClassPtr, obj);
    }

    SEString* SECore::Object::ToString(SEObject* obj)
    {
        static void* GetObjectStringPtr = GetStaticMethodPointer(SE_TEXT("GetObjectString"));
        return (SEString*)CallStaticMethod<void*, void*>(GetObjectStringPtr, obj);
    }

    int32 SECore::Object::GetHashCode(SEObject* obj)
    {
        static void* GetObjectStringPtr = GetStaticMethodPointer(SE_TEXT("GetObjectHashCode"));
        return CallStaticMethod<int32, void*>(GetObjectStringPtr, obj);
    }


    SEString* SECore::String::GetEmpty(SEDomain* domain)
    {
        static void* GetStringEmptyPtr = GetStaticMethodPointer(SE_TEXT("GetStringEmpty"));
        return (SEString*)CallStaticMethod<void*>(GetStringEmptyPtr);
    }

    SEString* SECore::String::New(const char* str, int32 length, SEDomain* domain)
    {
        static void* NewStringUTF8Ptr = GetStaticMethodPointer(SE_TEXT("NewStringUTF8"));
        return (SEString*)CallStaticMethod<void*, const char*, int>(NewStringUTF8Ptr, str, length);
    }

    SEString* SECore::String::New(const Char* str, int32 length, SEDomain* domain)
    {
        static void* NewStringUTF16Ptr = GetStaticMethodPointer(SE_TEXT("NewStringUTF16"));
        return (SEString*)CallStaticMethod<void*, const Char*, int>(NewStringUTF16Ptr, str, length);
    }

    StringView SECore::String::GetChars(SEString* obj)
    {
        int32 length = 0;
        static void* GetStringPointerPtr = GetStaticMethodPointer(SE_TEXT("GetStringPointer"));
        const Char* chars = CallStaticMethod<const Char*, void*, int*>(GetStringPointerPtr, obj, &length);
        return StringView(chars, length);
    }


    SEArray* SECore::Array::New(const SEClass* elementKlass, int32 length)
    {
        static void* NewArrayPtr = GetStaticMethodPointer(SE_TEXT("NewArray"));
        return (SEArray*)CallStaticMethod<void*, void*, long long>(NewArrayPtr, elementKlass->m_Handle, length);
    }

    SEClass* SECore::Array::GetClass(SEClass* elementKlass)
    {
        static void* GetArrayTypeFromElementTypePtr = GetStaticMethodPointer(SE_TEXT("GetArrayTypeFromElementType"));
        SEType* typeHandle = (SEType*)CallStaticMethod<void*, void*>(GetArrayTypeFromElementTypePtr, elementKlass->m_Handle);
        return GetOrCreateClass(typeHandle);
    }

    SEClass* SECore::Array::GetArrayClass(const SEArray* obj)
    {
        static void* GetArrayTypeFromWrappedArrayPtr = GetStaticMethodPointer(SE_TEXT("GetArrayTypeFromWrappedArray"));
        SEType* typeHandle = (SEType*)CallStaticMethod<void*, void*>(GetArrayTypeFromWrappedArrayPtr, (void*)obj);
        return GetOrCreateClass(typeHandle);
    }

    int32 SECore::Array::GetLength(const SEArray* obj)
    {
        static void* GetArrayLengthPtr = GetStaticMethodPointer(SE_TEXT("GetArrayLength"));
        return CallStaticMethod<int, void*>(GetArrayLengthPtr, (void*)obj);
    }

    void* SECore::Array::GetAddress(const SEArray* obj)
    {
        static void* GetArrayPointerPtr = GetStaticMethodPointer(SE_TEXT("GetArrayPointer"));
        return CallStaticMethod<void*, void*>(GetArrayPointerPtr, (void*)obj);
    }

    SEArray* SECore::Array::Unbox(SEObject* obj)
    {
        static void* GetArrayPtr = GetStaticMethodPointer(SE_TEXT("GetArray"));
        return (SEArray*)CallStaticMethod<void*, void*>(GetArrayPtr, (void*)obj);
    }

    
    
    void SECore::GC::Collect()
    {
        PROFILE_CPU();
        static void* GCCollectPtr = GetStaticMethodPointer(SE_TEXT("GCCollect"));
        CallStaticMethod<void, int, int, bool, bool>(GCCollectPtr, MaxGeneration(), (int)SEGCCollectionMode::Default, true, false);
    }

    void SECore::GC::Collect(int32 generation)
    {
        PROFILE_CPU();
        static void* GCCollectPtr = GetStaticMethodPointer(SE_TEXT("GCCollect"));
        CallStaticMethod<void, int, int, bool, bool>(GCCollectPtr, generation, (int)SEGCCollectionMode::Default, true, false);
    }

    void SECore::GC::Collect(int32 generation, SEGCCollectionMode collectionMode, bool blocking, bool compacting)
    {
        PROFILE_CPU();
        static void* GCCollectPtr = GetStaticMethodPointer(SE_TEXT("GCCollect"));
        CallStaticMethod<void, int, int, bool, bool>(GCCollectPtr, generation, (int)collectionMode, blocking, compacting);
    }

    int32 SECore::GC::MaxGeneration()
    {
        static int32 maxGeneration = CallStaticMethod<int32>(GetStaticMethodPointer(SE_TEXT("GCMaxGeneration")));
        return maxGeneration;
    }

    void SECore::GC::WaitForPendingFinalizers()
    {
        PROFILE_CPU();
        static void* GCWaitForPendingFinalizersPtr = GetStaticMethodPointer(SE_TEXT("GCWaitForPendingFinalizers"));
        CallStaticMethod<void>(GCWaitForPendingFinalizersPtr);
    }

    void SECore::GC::WriteRef(void* ptr, SEObject* ref)
    {
        *(void**)ptr = ref;
    }

    void SECore::GC::WriteValue(void* dst, void* src, int32 count, const SEClass* klass)
    {
        const int32 size = klass->GetInstanceSize();
        memcpy(dst, src, count * size);
    }

    void SECore::GC::WriteArrayRef(SEArray* dst, SEObject* ref, int32 index)
    {
        static void* WriteArrayReferencePtr = GetStaticMethodPointer(SE_TEXT("WriteArrayReference"));
        CallStaticMethod<void, void*, void*, int32>(WriteArrayReferencePtr, dst, ref, index);
    }

    void SECore::GC::WriteArrayRef(SEArray* dst, Span<SEObject*> refs)
    {
        static void* WriteArrayReferencesPtr = GetStaticMethodPointer(SE_TEXT("WriteArrayReferences"));
        CallStaticMethod<void, void*, void*, int32>(WriteArrayReferencesPtr, dst, refs.Get(), refs.Length());
    }

    void* SECore::GC::AllocateMemory(int32 size, bool coTaskMem)
    {
        static void* AllocMemoryPtr = GetStaticMethodPointer(SE_TEXT("AllocMemory"));
        return CallStaticMethod<void*, int, bool>(AllocMemoryPtr, size, coTaskMem);
    }

    void SECore::GC::FreeMemory(void* ptr, bool coTaskMem)
    {
        if (!ptr)
            return;
        static void* FreeMemoryPtr = GetStaticMethodPointer(SE_TEXT("FreeMemory"));
        CallStaticMethod<void, void*, bool>(FreeMemoryPtr, ptr, coTaskMem);
    }


    ::SE::String SECore::Type::ToString(SEType* type)
    {
        SEClass* klass = GetOrCreateClass(type);
        return ::SE::String(klass->GetFullName());
    }

    SEClass* SECore::Type::GetClass(SEType* type)
    {
        static void* GetTypeClassPtr = GetStaticMethodPointer(SE_TEXT("GetTypeClass"));
        return CallStaticMethod<SEClass*, void*>(GetTypeClassPtr, type);
    }

    SEType* SECore::Type::GetElementType(SEType* type)
    {
        static void* GetElementClassPtr = GetStaticMethodPointer(SE_TEXT("GetElementClass"));
        return (SEType*)CallStaticMethod<void*, void*>(GetElementClassPtr, type);
    }

    int32 SECore::Type::GetSize(SEType* type)
    {
        return GetOrCreateClass(type)->GetInstanceSize();
    }

    SETypes SECore::Type::GetType(SEType* type)
    {
        SEClass* klass = GetOrCreateClass(type);
        if (klass->m_Types == 0)
        {
            static void* GetTypeMTypesEnumPtr = GetStaticMethodPointer(SE_TEXT("GetTypeMTypesEnum"));
            klass->m_Types = CallStaticMethod<uint32, void*>(GetTypeMTypesEnumPtr, klass->m_Handle);
        }
        return (SETypes)klass->m_Types;
    }

    bool SECore::Type::IsPointer(SEType* type)
    {
        static void* GetTypeIsPointerPtr = GetStaticMethodPointer(SE_TEXT("GetTypeIsPointer"));
        return CallStaticMethod<bool, void*>(GetTypeIsPointerPtr, type);
    }

    bool SECore::Type::IsReference(SEType* type)
    {
        static void* GetTypeIsReferencePtr = GetStaticMethodPointer(SE_TEXT("GetTypeIsReference"));
        return CallStaticMethod<bool, void*>(GetTypeIsReferencePtr, type);
    }


const SEAssembly::ClassesDictionary& SEAssembly::GetClasses() const
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
        SEClass* klass = New<SEClass>(this, managedClass.typeHandle, managedClass.name, managedClass.fullname, managedClass.namespace_, managedClass.typeAttributes);
        m_Classes.Add(klass->GetFullName(), klass);

        managedClass.nativePointer = klass;

        SECore::GC::FreeMemory((void*)managedClasses[i].name);
        SECore::GC::FreeMemory((void*)managedClasses[i].fullname);
        SECore::GC::FreeMemory((void*)managedClasses[i].namespace_);
    }

    static void* RegisterManagedClassNativePointersPtr = GetStaticMethodPointer(SE_TEXT("RegisterManagedClassNativePointers"));
    CallStaticMethod<void, NativeClassDefinitions**, int>(RegisterManagedClassNativePointersPtr, &managedClasses, classCount);

    SECore::GC::FreeMemory(managedClasses);

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
    SECore::GC::FreeMemory((void*)name_);
    SECore::GC::FreeMemory((void*)fullname_);
}

DEFINE_INTERNAL_CALL(void) NativeInterop_CreateClass(NativeClassDefinitions* managedClass, void* assemblyHandle)
    {
        ScopeLock lock(BinaryModule::Locker);
        SEAssembly* assembly = GetAssembly(assemblyHandle);
        if (assembly == nullptr)
        {
            StringAnsi assemblyName;
            StringAnsi assemblyFullName;
            GetAssemblyName(assemblyHandle, assemblyName, assemblyFullName);
            assembly = New<SEAssembly>(nullptr, assemblyName, assemblyFullName, assemblyHandle);
            CachedAssemblyHandles.Add(assemblyHandle, assembly);
        }

        MClass* klass = New<MClass>(assembly, managedClass->typeHandle, managedClass->name, managedClass->fullname, managedClass->namespace_, managedClass->typeAttributes);
        if (assembly != nullptr)
        {
            auto& classes = const_cast<SEAssembly::ClassesDictionary&>(assembly->GetClasses());
            MClass* oldKlass;
            if (classes.TryGet(klass->GetFullName(), oldKlass))
            {
                LOG(Warning, "Class '{0}' was already added to assembly '{1}'", String(klass->GetFullName()), String(assembly->GetName()));
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

    bool SEAssembly::LoadCorlib()
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

    bool SEAssembly::LoadImage(const String& assemblyPath, const StringView& nativePath)
    {
        // TODO: Use new hostfxr delegate load_assembly_bytes? (.NET 8+)
        // Open .Net assembly
        static void* LoadAssemblyImagePtr = GetStaticMethodPointer(SE_TEXT("LoadAssemblyImage"));
        m_Handle = CallStaticMethod<void*, const Char*>(LoadAssemblyImagePtr, assemblyPath.Get());
        if (m_Handle == nullptr)
        {
            Log::CLRInnerException(SE_TEXT(".NET assembly image is invalid at ") + assemblyPath);
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
#if USE_EDITOR
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

    bool SEAssembly::UnloadImage(bool isReloading)
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

    bool SEAssembly::ResolveMissingFile(String& assemblyPath) const
    {
#if DOTNET_HOST_MONO
        // Fallback to AOT-ed assembly location
        assemblyPath = Globals::BinariesFolder / SE_TEXT("Dotnet") / StringUtils::GetFileName(assemblyPath);
        return !FileSystem::FileExists(assemblyPath);
#endif
        return true;
    }

    
    const char_t* NativeInteropTypeName = SE_CORECLR_TEXT("FlaxEngine.Interop.NativeInterop, FlaxEngine.CSharp");
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
        const ::SE::String csharpLibraryPath = EngineContext::BinariesFolder / SE_TEXT("FlaxEngine.CSharp.dll");
        const ::SE::String csharpRuntimeConfigPath = EngineContext::BinariesFolder / SE_TEXT("FlaxEngine.CSharp.runtimeconfig.json");
        if (!FileSystem::FileExists(csharpLibraryPath))
            LOG_FATAL("Scripting", "Failed to initialize .NET runtime, missing file: {0}", csharpLibraryPath);
        if (!FileSystem::FileExists(csharpRuntimeConfigPath))
            LOG_FATAL("Scripting", "Failed to initialize .NET runtime, missing file: {0}", csharpRuntimeConfigPath);
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
#if !USE_EDITOR
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

#if USE_EDITOR
            LOG_FATAL("Scripting", "Missing .NET 8 or later SDK installation required to run Flax Editor.");
#else
            LOG_FATAL("Scripting", "Missing .NET 8 or later Runtime installation required to run this application.");
#endif
            return true;
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
            return true;
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
            return true;
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
        hostfxr_initialize_parameters init_params;
        init_params.size = sizeof(hostfxr_initialize_parameters);
        init_params.host_path = libraryPath.Get();
        path = String(FileSystem::GetDirectoryName(path)) / SE_TEXT("/../../../");
        FileSystem::PathRemoveRelativeParts(path);
        dotnetRoot = SE_CORECLR_STRING(path);
        init_params.dotnet_root = dotnetRoot.Get();
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
            return true;
        }

        void* pget_function_pointer = nullptr;
        rc = hostfxr_get_runtime_delegate(handle, hdt_get_function_pointer, &pget_function_pointer);
        if (rc != 0 || pget_function_pointer == nullptr)
        {
            hostfxr_close(handle);
            LOG_FATAL("Scripting", "Failed to get runtime delegate hdt_get_function_pointer: 0x{0:x}", (unsigned int)rc);
            return true;
        }

        hostfxr_close(handle);
        get_function_pointer = (get_function_pointer_fn)pget_function_pointer;
        return false;
    }

    void ShutdownHostfxr()
    {
    }

    void* GetStaticMethodPointer(const String& methodName)
    {
        void* fun;
        if (CachedFunctions.TryGet(methodName, fun))
            return fun;
        PROFILE_CPU();
        const int rc = get_function_pointer(NativeInteropTypeName, SE_CORECLR_STRING(methodName).Get(), UNMANAGEDCALLERSONLY_METHOD, nullptr, nullptr, &fun);
        if (rc != 0)
            LOG_FATAL("Scripting", "Failed to get unmanaged function pointer for method '{0}': 0x{1:x}", methodName, (unsigned int)rc);
        CachedFunctions.Add(methodName, fun);
        return fun;
    }
}
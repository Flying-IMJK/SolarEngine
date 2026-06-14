

#include "Scripting.h"
#include "Events.h"
#include "Binary/BinaryModule.h"
#include "Binary/ManagedBinaryModule.h"
#include "Binary/NativeBinaryModule.h"
#include "Core/Systems.h"
#include "ManagedCLR/CLRCore.h"

#include "Runtime/Scripting/ScriptingObject.h"
#include "Runtime/Scripting/ManagedCLR/CLRClass.h"
#include "Runtime/EngineContext.h"
#include "Runtime/Level/Level.h"
#include "ManagedCLR/CLRDomain.h"
#include "ManagedCLR/CLRException.h"
#include "ManagedCLR/CLRMethod.h"
#include "Runtime/Render/RenderTask.h"

#include "Core/Logging/Logging.h"
#include "Core/Types/Collections/Dictionary.h"
#include "Core/Thread/Threading.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Platform/File.h"
#include "Core/Profiler/ProfilerCPU.h"
#include "Core/Types/Stopwatch.h"
#include "Core/Serialization/JsonTools.h"
#include "Core/Thread/ThreadLocal.h"


namespace SE
{
    extern void RegisterInternalCalls();
    extern "C" BinaryModule* GetBinaryModuleSEEditor();
    
    class ScriptingSystem : public ISystem
    {
        ENGINE_SYSTEM(ScriptingSystem)
public:
        ScriptingSystem() : ISystem(SE_TEXT("Scripting"), -20)
        {
        }

        bool OnInit() override;
        void OnUpdate() override;
        void OnLateUpdate() override;
        void OnBeforeExit() override;
        void OnDispose() override;
    };

    ENGINE_SYSTEM_REGISTER(ScriptingSystem)

    namespace
    {
        CLRDomain* m_RootDomain = nullptr;
        CLRDomain* m_ScriptsDomain = nullptr;
        CriticalSection m_ObjectsLocker;

        Dictionary<UID, ScriptingObject*> m_ObjectsDictionary(1024 * 16);

        Dictionary<StringAnsi, BinaryModule*, InlinedAllocation<64>> _nonNativeModules;

        bool _isEngineAssemblyLoaded = false;
        bool _hasGameModulesLoaded = false;

        CLRMethod* _method_Update = nullptr;
        CLRMethod* _method_LateUpdate = nullptr;
        CLRMethod* _method_FixedUpdate = nullptr;
        CLRMethod* _method_LateFixedUpdate = nullptr;
        CLRMethod* _method_Draw = nullptr;
        CLRMethod* _method_Exit = nullptr;

        void ReleaseObjects(bool gameOnly)
        {
            // Flush objects already enqueued objects to delete
            // ObjectsRemovalService::Flush();

            // Give GC a try to cleanup old user objects and the other mess
            CLRCore::GC::Collect();
            CLRCore::GC::WaitForPendingFinalizers();

            // Destroy objects from game assemblies (eg. not released objects that might crash if persist in memory after reload)
            const auto solarModule = GetBinaryModuleSERuntime();
            m_ObjectsLocker.Lock();
            for (auto i = m_ObjectsDictionary.begin(); i.IsNotEnd(); ++i)
            {
                auto obj = i->Value;
                if (gameOnly && obj->GetTypeHandle().Module == solarModule)
                    continue;

#if USE_OBJECTS_DISPOSE_CRASHES_DEBUGGING
                LOG(Info, "[OnScriptingDispose] obj = 0x{0:x}, {1}", (uint64)obj.Ptr, String(obj.TypeName));
#endif
                obj->OnScriptingDispose();
            }
            m_ObjectsLocker.Unlock();

            // Release assets sourced from game assemblies
            List<Asset*> assets = AssetContent::GetAssets();
            for (auto asset : assets)
            {
                if (asset->GetTypeHandle().Module == solarModule)
                {
                    continue;
                }

                asset->DeleteObject();
            }
            // ObjectsRemovalService::Flush();
        }
    }

    Dictionary<Pair<ScriptingTypeHandle, StringView>, void(*)(ScriptingObject*, void*, bool)> ScriptingEvents::EventsTable;
    Delegate<ScriptingObject*, Span<Variant>, ScriptingTypeHandle, StringView> ScriptingEvents::Event;

    bool InitEngine();

    // Assembly events
    void OnEngineLoaded(CLRAssembly* assembly);
    void OnEngineUnloading(CLRAssembly* assembly);

    Delegate<BinaryModule*> Scripting::BinaryModuleLoaded;
    Action Scripting::ScriptsLoaded;
    Action Scripting::ScriptsUnload;
    Action Scripting::ScriptsReloading;
    Action Scripting::ScriptsReloaded;
    Threading::ThreadLocal<Scripting::IdsMappingTable*, PLATFORM_THREADS_LIMIT> Scripting::ObjectsLookupIdMapping;


    CLRDomain* Scripting::GetRootDomain()
    {
        return m_RootDomain;
    }

    CLRDomain* Scripting::GetScriptsDomain()
    {
        return m_ScriptsDomain;
    }

    class ScriptingInternal
    {
    public:
        static void InitRuntime()
        {
            // Scripting API
            // ADD_INTERNAL_CALL("FlaxEngine.Scripting::HasGameModulesLoaded", &ScriptingInternal_HasGameModulesLoaded);
            // ADD_INTERNAL_CALL("FlaxEngine.Scripting::IsTypeFromGameScripts", &ScriptingInternal_IsTypeFromGameScripts);
            // ADD_INTERNAL_CALL("FlaxEngine.Scripting::FlushRemovedObjects", &ScriptingInternal_FlushRemovedObjects);
            //
            // // Profiler API
            // ADD_INTERNAL_CALL("FlaxEngine.Profiler::BeginEvent", &ProfilerInternal_BeginEvent);
            // ADD_INTERNAL_CALL("FlaxEngine.Profiler::EndEvent", &ProfilerInternal_EndEvent);
            // ADD_INTERNAL_CALL("FlaxEngine.Profiler::BeginEventGPU", &ProfilerInternal_BeginEventGPU);
            // ADD_INTERNAL_CALL("FlaxEngine.Profiler::EndEventGPU", &ProfilerInternal_EndEventGPU);
        }
    };

    IMPLEMENT_SCRIPTING_TYPE_NO_SPAWN(Scripting, SERuntime, "SE.Scripting", nullptr, nullptr);

    void Scripting::ProcessBuildInfoPath(String& path, const String& projectFolderPath)
    {
        if (path.IsEmpty())
            return;
        if (path.StartsWith(SE_TEXT("$(EnginePath)")))
            path = EngineContext::StartupFolder / path.Substring(14);
        else if (path.StartsWith(SE_TEXT("$(ProjectPath)")))
            path = projectFolderPath / path.Substring(14);
        else if (FileSystem::IsRelative(path))
            path = projectFolderPath / path;
    }

    bool Scripting::LoadBinaryModules(const String& path, const String& projectFolderPath)
    {
        PROFILE_CPU_NAMED("LoadBinaryModules");
        LOG_INFO("Scripting", "Loading binary modules from build info file {0}", path);

        // Read file contents
        List<byte> fileData;
        if (!File::ReadAllBytes(path, fileData))
        {
            LOG_ERROR("Scripting", "Failed to read file contents.");
            return false;
        }

        // Parse Json data
        Json::Document document;
        {
            PROFILE_CPU_NAMED("Json.Parse");
            document.Parse((char*)fileData.Get(), fileData.Count());
        }
        if (document.HasParseError())
        {
            LOG_ERROR("Scripting", "Failed to file contents.");
            return false;
        }

        // TODO: validate Name, Platform, Architecture, Configuration from file

        // Load all references
        auto referencesMember = document.FindMember("References");
        if (referencesMember != document.MemberEnd() && referencesMember->value.IsArray())
        {
            auto& referencesArray = referencesMember->value;
            for (rapidjson::SizeType i = 0; i < referencesArray.Size(); i++)
            {
                auto& reference = referencesArray[i];
                String referenceProjectPath = JsonTools::GetString(reference, "ProjectPath", String::Empty);
                if (referenceProjectPath == SE_TEXT("$(EnginePath)/Flax.flaxproj"))
                    continue; // Skip reference to engine
                String referencePath = JsonTools::GetString(reference, "Path", String::Empty);
                if (referenceProjectPath.IsEmpty() || referencePath.IsEmpty())
                {
                    LOG_ERROR("Scripting", "Empty reference.");
                    return false;
                }

                ProcessBuildInfoPath(referenceProjectPath, projectFolderPath);
                ProcessBuildInfoPath(referencePath, projectFolderPath);

                String referenceProjectFolderPath = FileSystem::GetDirectoryName(referenceProjectPath);

                if (LoadBinaryModules(referencePath, referenceProjectFolderPath))
                {
                    LOG_ERROR("Scripting", "Failed to load reference.");
                    return false;
                }
            }
        }

        // Load all binary modules
        auto binaryModulesMember = document.FindMember("BinaryModules");
        if (binaryModulesMember != document.MemberEnd() && binaryModulesMember->value.IsArray())
        {
            auto& binaryModulesArray = binaryModulesMember->value;
            for (rapidjson::SizeType i = 0; i < binaryModulesArray.Size(); i++)
            {
                auto& binaryModule = binaryModulesArray[i];
                const auto nameMember = binaryModule.FindMember("Name");
                if (nameMember == binaryModule.MemberEnd())
                {
                    LOG_ERROR("Scripting", "Failed to process file.");
                    return false;
                }
                String name = nameMember->value.GetText();
                StringAnsi nameAnsi(nameMember->value.GetString(), nameMember->value.GetStringLength());
                String nativePath = JsonTools::GetString(binaryModule, "NativePath", String::Empty);
                String managedPath = JsonTools::GetString(binaryModule, "ManagedPath", String::Empty);
                ProcessBuildInfoPath(nativePath, projectFolderPath);
                ProcessBuildInfoPath(managedPath, projectFolderPath);
                LOG_INFO("Scripting", "Loading binary module {0}", name);

                // Check if that module has been already registered
                BinaryModule* module = BinaryModule::GetModule(nameAnsi);
                if (!module)
                {
                    _nonNativeModules.TryGet(nameAnsi, module);
                }
                if (!module)
                {
                    // C++
                    if (nativePath.HasChars())
                    {
                        // Check if this module has been statically linked
                        auto& staticallyLinkedBinaryModules = StaticallyLinkedBinaryModuleInitializer::GetStaticallyLinkedBinaryModules();
                        for (auto getter : staticallyLinkedBinaryModules)
                        {
                            module = getter();
                            if (module && module->GetName() == nameAnsi)
                                break;
                            module = nullptr;
                        }

                        if (!module)
                        {
                            // Load library
                            Stopwatch stopwatch;
#if PLATFORM_ANDROID || PLATFORM_MAC
                            // On some platforms all native binaries are side-by-side with the app in a different folder
                            if (!FileSystem::FileExists(nativePath))
                            {
                                nativePath = String(StringUtils::GetDirectoryName(Platform::GetExecutableFilePath())) / StringUtils::GetFileName(nativePath);
                            }
#elif PLATFORM_IOS
                            // iOS uses Frameworks folder with native binaries
                            if (!FileSystem::FileExists(nativePath))
                            {
                                nativePath = Globals::ProjectFolder / SE_TEXT("Frameworks") / StringUtils::GetFileName(nativePath);
                            }
#endif
                            auto library = Platform::LoadLibrary(nativePath.Get());
                            if (!library)
                            {
                                LOG_ERROR("Scripting", "Failed to load library '{0}' for binary module {1}.", nativePath, name);
                                return true;
                            }
                            char getBinaryFuncName[512];
                            StringAnsiView getBinaryFuncNamePrefix("GetBinaryModule");
                            ENGINE_ASSERT(getBinaryFuncNamePrefix.Length() + nameAnsi.Length() < ARRAY_SIZE(getBinaryFuncName));
                            Platform::MemoryCopy(getBinaryFuncName, getBinaryFuncNamePrefix.Get(), getBinaryFuncNamePrefix.Length());
                            Platform::MemoryCopy(getBinaryFuncName + getBinaryFuncNamePrefix.Length(), nameAnsi.Get(), nameAnsi.Length());
                            *(getBinaryFuncName + getBinaryFuncNamePrefix.Length() + nameAnsi.Length()) = '\0';
                            const auto getBinaryFunc = (GetBinaryModuleFunc)Platform::GetProcAddress(library, getBinaryFuncName);
                            if (!getBinaryFunc)
                            {
                                Platform::FreeLibrary(library);
                                LOG_ERROR("Scripting", "Failed to setup library '{0}' for binary module {1}.", nativePath, name);
                                return true;
                            }
                            stopwatch.Stop();
                            LOG_INFO("Scripting", "Module {0} loaded in {1}ms", name, stopwatch.GetMilliseconds());

                            // Get the binary module
                            module = getBinaryFunc();
                            if (!module)
                            {
                                Platform::FreeLibrary(library);
                                LOG_ERROR("Scripting", "Failed to get binary module {0}.", name);
                                return true;
                            }
                            ((NativeBinaryModule*)module)->Library = library;
                        }
                    }
                    else
                    {
                        // Create module if native library is not used
                        module = New<ManagedBinaryModule>(nameAnsi);
                        _nonNativeModules.Add(nameAnsi, module);
                    }
                }

                // C#
                if (managedPath.HasChars() && !((ManagedBinaryModule*)module)->Assembly->IsLoaded())
                {
                    if (((ManagedBinaryModule*)module)->Assembly->Load(managedPath, nativePath))
                    {
                        LOG_ERROR("Scripting", "Failed to load C# assembly '{0}' for binary module {1}.", managedPath, name);
                        return false;
                    }
                }

                BinaryModuleLoaded(module);
            }
        }

        return true;
    }

    bool Scripting::Load()
    {
        PROFILE_CPU();
        // Note: this action can be called from main thread (due to Mono problems with assemblies actions from other threads)
        ENGINE_ASSERT(Threading::IsMainThread());
        Threading::ScopeLock lock(BinaryModule::Locker);
        
        // Load C# core assembly
        ManagedBinaryModule* corlib = GetBinaryModuleCorlib();
        if (!corlib->Assembly->LoadCorlib())
        {
            LOG_ERROR("Scripting", "Failed to load corlib C# assembly.");
            return false;
        }

        // Initialize C# corelib types
        {
            const auto& corlibClasses = corlib->Assembly->GetClasses();
            bool gotAll = true;
#define CACHE_CORLIB_CLASS(var, name) gotAll &= corlibClasses.TryGet(StringAnsiView(name), CLRCore::TypeCache::var)
            CACHE_CORLIB_CLASS(Void, "System.Void");
            CACHE_CORLIB_CLASS(Object, "System.Object");
            CACHE_CORLIB_CLASS(Byte, "System.Byte");
            CACHE_CORLIB_CLASS(Boolean, "System.Boolean");
            CACHE_CORLIB_CLASS(SByte, "System.SByte");
            CACHE_CORLIB_CLASS(Char, "System.Char");
            CACHE_CORLIB_CLASS(Int16, "System.Int16");
            CACHE_CORLIB_CLASS(UInt16, "System.UInt16");
            CACHE_CORLIB_CLASS(Int32, "System.Int32");
            CACHE_CORLIB_CLASS(UInt32, "System.UInt32");
            CACHE_CORLIB_CLASS(Int64, "System.Int64");
            CACHE_CORLIB_CLASS(UInt64, "System.UInt64");
            CACHE_CORLIB_CLASS(IntPtr, "System.IntPtr");
            CACHE_CORLIB_CLASS(UIntPtr, "System.UIntPtr");
            CACHE_CORLIB_CLASS(Single, "System.Single");
            CACHE_CORLIB_CLASS(Double, "System.Double");
            CACHE_CORLIB_CLASS(String, "System.String");
#undef CACHE_CORLIB_CLASS
            if (!gotAll)
            {
                LOG_ERROR("Scripting", "Failed to load corlib C# assembly.");
                for (const auto& e : corlibClasses)
                {
                    LOG_INFO("Scripting", "Class: {0}", String(e.Value->GetFullName()));
                }
                return false;
            }
        }


        // Load Runtime
        bool needLoadEngine = false;
        const String solarRuntimePath = EngineContext::BinariesFolder / SE_TEXT("SERuntimeCSharp.dll");
        auto* engineModule = (NativeBinaryModule*)GetBinaryModuleSERuntime();
        if (!engineModule->Assembly->IsLoaded())
        {
            if (!engineModule->Assembly->Load(solarRuntimePath))
            {
                LOG_ERROR("Scripting", "Failed to load SolarEngine C# assembly.");
                return false;
            }
            needLoadEngine = true;

            // Insert type aliases for vector types that don't exist in C++ but are just typedef (properly redirect them to actual types)
            // TODO: add support for automatic typedef aliases setup for scripting module to properly lookup type from the alias typename
#if USE_LARGE_WORLDS
            engineModule->TypeNameToTypeIndex["SE.Vector2"] = engineModule->TypeNameToTypeIndex["SE.Double2"];
            engineModule->TypeNameToTypeIndex["SE.Vector3"] = engineModule->TypeNameToTypeIndex["SE.Double3"];
            engineModule->TypeNameToTypeIndex["SE.Vector4"] = engineModule->TypeNameToTypeIndex["SE.Double4"];
#else
            engineModule->TypeNameToTypeIndex["SE.Vector2"] = engineModule->TypeNameToTypeIndex["SE.Float2"];
            engineModule->TypeNameToTypeIndex["SE.Vector3"] = engineModule->TypeNameToTypeIndex["SE.Float3"];
            engineModule->TypeNameToTypeIndex["SE.Vector4"] = engineModule->TypeNameToTypeIndex["SE.Float4"];
#endif

            engineModule->ClassToTypeIndex[engineModule->Assembly->GetClass("SE.Vector2")] = engineModule->TypeNameToTypeIndex["SE.Vector2"];
            engineModule->ClassToTypeIndex[engineModule->Assembly->GetClass("SE.Vector3")] = engineModule->TypeNameToTypeIndex["SE.Vector3"];
            engineModule->ClassToTypeIndex[engineModule->Assembly->GetClass("SE.Vector4")] = engineModule->TypeNameToTypeIndex["SE.Vector4"];
        }
#if SE_EDITOR

        const String solarEditorPath = EngineContext::BinariesFolder / SE_TEXT("SEEditorCSharp.dll");
        auto* editorEngineModule = (NativeBinaryModule*)GetBinaryModuleSEEditor();
        if (!editorEngineModule->Assembly->IsLoaded() && !editorEngineModule->Assembly->Load(solarEditorPath))
        {
            LOG_ERROR("Scripting", "Failed to load SolarEngine C# Editor assembly.");
            return false;
        }
#endif


        if (needLoadEngine)
        {
            OnEngineLoaded(engineModule->Assembly);
        }


#ifdef SE_EDITOR
        // Skip loading game modules in Editor on startup - Editor loads them later during splash screen (eg. after first compilation)
        static bool SkipFirstLoad = true;
        if (SkipFirstLoad)
        {
            SkipFirstLoad = false;
            return true;
        }

        // Flax.Build outputs the <target>.Build.json with binary modules to use for game scripting
        const Char *target = SE_TEXT(""), *platform = SE_TEXT(""), *architecture = SE_TEXT(""), *configuration = SE_TEXT("");
        /*ScriptsBuilder::GetBinariesConfiguration(target, platform, architecture, configuration);
        if (StringUtils::Length(target) == 0)
        {
            LOG_INFO("Scripting", "Missing EditorTarget in project. Not using game script modules.");
            _hasGameModulesLoaded = true;
            return false;
        }*/
        const String targetBuildInfo = EngineContext::ProjectFolder / SE_TEXT("Binaries") / target / platform / architecture / configuration / target + SE_TEXT(".Build.json");

        // Call compilation if game target build info is missing
        /*if (!FileSystem::FileExists(targetBuildInfo))
        {
            LOG_INFO("Scripting", "Missing target build info ({0})", targetBuildInfo);
            if (LastBinariesLoadTriggeredCompilation)
                return false;
            LastBinariesLoadTriggeredCompilation = true;
            ScriptsBuilder::Compile();
            return false;
        }*/
#else
        const String targetBuildInfo = EngineContext::BinariesFolder / SE_TEXT("Game.Build.json");
#endif

        // Load game binary modules
        if (!LoadBinaryModules(targetBuildInfo, EngineContext::ProjectFolder))
        {
            LOG_ERROR("Scripting", "Failed to load Game assemblies.");
            return false;
        }
        _hasGameModulesLoaded = true;

        // End
        ScriptsLoaded();
        return true;
    }

    void Scripting::Release()
    {
        PROFILE_CPU();
        // Note: this action can be called from main thread (due to Mono problems with assemblies actions from other threads)
        ENGINE_ASSERT(Threading::IsMainThread());

        // Fire event
        ScriptsUnload();

        // Release managed objects instances for persistent objects (assets etc.)
        ReleaseObjects(false);

        auto* flaxEngineModule = (NativeBinaryModule*)GetBinaryModuleSERuntime();
        OnEngineUnloading(flaxEngineModule->Assembly);

        // Unload assemblies (from back to front)
        {
            LOG_INFO("Scripting", "Unloading binary modules");
            auto modules = BinaryModule::GetModules();
            for (int32 i = modules.Count() - 1; i >= 0; i--)
            {
                auto module = modules[i];
                module->Destroy(false);
            }
            _nonNativeModules.ClearDelete();
            _hasGameModulesLoaded = false;
        }

        // Cleanup
        CLRCore::GC::Collect();
        CLRCore::GC::WaitForPendingFinalizers();

        // Flush objects
        // ObjectsRemovalService::Flush();

        // Switch domain
        auto rootDomain = CLRCore::GetRootDomain();
        if (rootDomain)
        {
            if (!rootDomain->SetCurrentDomain(false))
            {
                LOG_ERROR("Scripting", "Failed to set current domain to root");
            }
        }

#if !USE_SCRIPTING_SINGLE_DOMAIN
        CLRCore::UnloadDomain("Scripts Domain");
#endif
    }

#if SE_EDITOR

    void Scripting::Reload(bool canTriggerSceneReload)
    {
        // By default we allow to call it only from the main thread and when no scene is loaded.
        // Otherwise call scene manager to perform clear scripts reload.
        // It will call this method back on main thread without scenes loaded, see SceneActionType::ReloadScripts.
        if (!Threading::IsMainThread() || Level::IsAnySceneLoaded())
        {
            if (canTriggerSceneReload)
            {
                // Call scene to reload scripts
                // Level::ReloadScriptsAsync();
            }
            else
            {
                LOG_WARNING("Scripting", "Cannot reload scene on scripting reload. Flag is not set.");
            }
            return;
        }

        PROFILE_CPU();

        // Ideally we would call Release and Load but this would also reload Editor objects which we want avoid
        // Editor is also referencing assets and other managed objects so we should force reload everything.
        // However Reload is called when no scene is loaded.

        // Faster path - if no game assembly loaded yet
        if (!_hasGameModulesLoaded)
        {
            // Just load missing assemblies
            Load();
            return;
        }

        LOG_INFO("Scripting", "Start user scripts reload");
        ScriptsReloading();

        // Destroy objects from game assemblies (eg. not released objects that might crash if persist in memory after reload)
        ReleaseObjects(true);

        // Unload all game modules
        LOG_INFO("Scripting", "Unloading game binary modules");
        auto modules = BinaryModule::GetModules();
        for (int32 i = modules.Count() - 1; i >= 0; i--)
        {
            BinaryModule* module = modules[i];
            if (module == GetBinaryModuleCorlib() || module == GetBinaryModuleSERuntime())
                continue;

            module->Destroy(true);
        }
        modules.Clear();
        _nonNativeModules.ClearDelete();
        _hasGameModulesLoaded = false;

        // Release and create a new assembly load context for user assemblies
        // CLRCore::ReloadScriptingAssemblyLoadContext();

        // Give GC a try to cleanup old user objects and the other mess
        CLRCore::GC::Collect();
        CLRCore::GC::WaitForPendingFinalizers();

        // Load all game modules
        if (Load())
        {
            LOG_ERROR("Scripting", "User assemblies reload failed.");
        }

        ScriptsReloaded();
        LOG_INFO("Scripting", "End user scripts reload");
    }

#endif

    List<ScriptingObject*, HeapAllocation> Scripting::GetObjects()
    {
        List<ScriptingObject*> objects;
        m_ObjectsLocker.Lock();
        m_ObjectsDictionary.GetValues(objects);
        m_ObjectsLocker.Unlock();
        return objects;
    }

    CLRClass* Scripting::FindClass(const StringAnsiView& fullname)
    {
        if (fullname.IsEmpty())
            return nullptr;
        PROFILE_CPU();
        auto& modules = BinaryModule::GetModules();
        for (auto module : modules)
        {
            auto managedModule = dynamic_cast<ManagedBinaryModule*>(module);
            if (managedModule && managedModule->Assembly->IsLoaded())
            {
                CLRClass* result = managedModule->Assembly->GetClass(fullname);
                if (result != nullptr)
                    return result;
            }
        }
        return nullptr;
    }

    ScriptingTypeHandle Scripting::FindScriptingType(const StringAnsiView& fullname)
    {
        if (fullname.IsEmpty())
            return ScriptingTypeHandle();
        PROFILE_CPU();
        auto& modules = BinaryModule::GetModules();
        for (auto module : modules)
        {
            int32 typeIndex;
            if (module->FindScriptingType(fullname, typeIndex))
            {
                return ScriptingTypeHandle(module, typeIndex);
            }
        }
        return ScriptingTypeHandle();
    }

    ScriptingObject* Scripting::NewObject(const ScriptingTypeHandle& type)
    {
        if (!type)
        {
            LOG_ERROR("Scripting", "Invalid type.");
            return nullptr;
        }
        const ScriptingType& scriptingType = type.GetType();

        // Create unmanaged object
        const ScriptingObjectSpawnParams params(UID::New(), type);
        ScriptingObject* obj = scriptingType.Script.Spawn(params);
        if (obj == nullptr)
            LOG_ERROR("Scripting", "Failed to spawn object of type \'{0}\'.", scriptingType.ToString());
        return obj;
    }

    ScriptingObject* Scripting::NewObject(const CLRClass* type)
    {
        if (type == nullptr)
        {
            LOG_ERROR("Scripting", "Invalid type.");
            return nullptr;
        }

        // Get the assembly with that class
        auto module = ManagedBinaryModule::FindModule(type);
        if (module == nullptr)
        {
            LOG_ERROR("Scripting", "Cannot find scripting assembly for type \'{0}\'.", String(type->GetFullName()));
            return nullptr;
        }

        // Try to find the scripting type for this class
        int32 typeIndex;
        if (!module->ClassToTypeIndex.TryGet(type, typeIndex))
        {
            LOG_ERROR("Scripting", "Cannot spawn objects of type \'{0}\'.", String(type->GetFullName()));
            return nullptr;
        }
        const ScriptingType& scriptingType = module->Types[typeIndex];

        // Create unmanaged object
        const ScriptingObjectSpawnParams params(UID::New(), ScriptingTypeHandle(module, typeIndex));
        ScriptingObject* obj = scriptingType.Script.Spawn(params);
        if (obj == nullptr)
            LOG_ERROR("Scripting", "Failed to spawn object of type \'{0}\'.", scriptingType.ToString());
        return obj;


    }

    ScriptingObject* FindObject(const UID& id, CLRClass* type)
    {
        return Scripting::FindObject(id, type);
    }
    
    ScriptingObject* Scripting::FindObject(UID id, const CLRClass* type)
    {
        if (!id.IsValid())
        {
            return nullptr;
        }
        PROFILE_CPU();

        // Try to map object id
        /*const auto idsMapping = ObjectsLookupIdMapping.Get();
        if (idsMapping)
        {
            idsMapping->TryGet(id, id);
        }*/

        // Try to find it
#if USE_OBJECTS_DISPOSE_CRASHES_DEBUGGING
        ScriptingObjectData data;
        _objectsLocker.Lock();
        _objectsDictionary.TryGet(id, data);
        _objectsLocker.Unlock();
        auto result = data.Ptr;
#else
        ScriptingObject* result = nullptr;
        m_ObjectsLocker.Lock();
        m_ObjectsDictionary.TryGet(id, result);
        m_ObjectsLocker.Unlock();
#endif
        if (result)
        {
            // Check type
            if (!type || result->Is(type))
                return result;
            LOG_WARNING("Scripting", "Found scripting object with ID={0} of type {1} that doesn't match type {2}.", id, result->GetScriptType().Fullname, type->GetFullName());
            return nullptr;
        }

        // Check if object can be an asset and try to load it
        if (!type)
        {
            result = AssetContent::LoadAsync<Asset>(id);
            if (!result)
                LOG_WARNING("Scripting", "Unable to find scripting object with ID={0}", id);
            return result;
        }
        if (type == ScriptingObject::GetScriptingClass() || type->IsSubClassOf(Asset::GetScriptingClass()))
        {
            /*Asset* asset = AssetContent::LoadAsync(id, type);
            if (asset)
            {
                return asset;
            }*/
        }

        LOG_WARNING("Scripting", "Unable to find scripting object with ID={0}. Required type {1}.", id, String(type->GetFullName()));
        return nullptr;
    }

    ScriptingObject* Scripting::TryFindObject(UID id, const CLRClass* type)
    {
        if (!id.IsValid())
        {
            return nullptr;
        }
        PROFILE_CPU();

        // Try to map object id
        /*const auto idsMapping = ObjectsLookupIdMapping.Get();
        if (idsMapping)
        {
            idsMapping->TryGet(id, id);
        }*/

        // Try to find it
#if USE_OBJECTS_DISPOSE_CRASHES_DEBUGGING
        ScriptingObjectData data;
        _objectsLocker.Lock();
        _objectsDictionary.TryGet(id, data);
        _objectsLocker.Unlock();
        auto result = data.Ptr;
#else
        ScriptingObject* result = nullptr;
        m_ObjectsLocker.Lock();
        m_ObjectsDictionary.TryGet(id, result);
        m_ObjectsLocker.Unlock();
#endif

        // Check type
        if (result && type && !result->Is(type))
        {
            result = nullptr;
        }

        return result;
    }

    ScriptingObject* Scripting::TryFindObject(const CLRClass* type)
    {
        if (type == nullptr)
            return nullptr;
        Threading::ScopeLock lock(m_ObjectsLocker);
        for (auto i = m_ObjectsDictionary.begin(); i.IsNotEnd(); ++i)
        {
            const auto obj = i->Value;
            if (obj->GetClass() == type)
                return obj;
        }
        return nullptr;
    }

    ScriptingObject* Scripting::FindObject(const CLRObject* managedInstance)
    {
        if (managedInstance == nullptr)
            return nullptr;
        PROFILE_CPU();

        // TODO: optimize it by reading the unmanagedPtr or _internalId from managed Object property

        Threading::ScopeLock lock(m_ObjectsLocker);

        for (auto i = m_ObjectsDictionary.begin(); i.IsNotEnd(); ++i)
        {
            const auto obj = i->Value;
            if (obj->GetManagedInstance() == managedInstance)
                return obj;
        }
        return nullptr;
    }

    void Scripting::OnManagedInstanceDeleted(ScriptingObject* obj)
    {
        PROFILE_CPU();
        ENGINE_ASSERT(obj != nullptr);

        // Validate if object still exists
        m_ObjectsLocker.Lock();
        if (m_ObjectsDictionary.ContainsValue(obj))
        {
#if USE_OBJECTS_DISPOSE_CRASHES_DEBUGGING
            LOG_INFO("Scripting", "[OnManagedInstanceDeleted] obj = 0x{0:x}, {1}", (uint64)obj, String(ScriptingObjectData(obj).TypeName));
#endif
            obj->OnManagedInstanceDeleted();
        }
        else
        {
            //LOG_WARNING("Scripting", "Object finalization called for already removed object (address={0:x})", (uint64)obj);
        }
        m_ObjectsLocker.Unlock();
    }

    bool Scripting::HasGameModulesLoaded()
    {
        return _hasGameModulesLoaded;
    }

    bool Scripting::IsEveryAssemblyLoaded()
    {
        auto& modules = BinaryModule::GetModules();
        for (BinaryModule* module : modules)
        {
            if (!module->IsLoaded())
                return false;
        }
        return true;
    }

    bool Scripting::IsTypeFromGameScripts(const CLRClass* type)
    {
        const auto binaryModule = ManagedBinaryModule::GetModule(type ? type->GetAssembly() : nullptr);
        return binaryModule && binaryModule != GetBinaryModuleCorlib() && binaryModule != GetBinaryModuleSERuntime();
    }

    void Scripting::RegisterObject(ScriptingObject* obj)
    {
        const UID id = obj->GetID();
        Threading::ScopeLock lock(m_ObjectsLocker);

        ENGINE_ASSERT(!m_ObjectsDictionary.ContainsValue(obj));

#if USE_OBJECTS_DISPOSE_CRASHES_DEBUGGING
        ScriptingObjectData other;
        if (_objectsDictionary.TryGet(id, other))
#else
        ScriptingObject* other;
        if (m_ObjectsDictionary.TryGet(id, other))
#endif
        {
            // Something went wrong...
            CLRClass* clrClass = obj->GetClass();
            CLRClass* otherClrClass = other->GetClass();
            LOG_ERROR("Scripting", "Objects registry already contains object with ID={0} (type '{3}')! Trying to register object {1} (type '{2}').", id,
                obj->ToString(), clrClass->GetFullName(), otherClrClass->GetFullName());
        }

#if USE_OBJECTS_DISPOSE_CRASHES_DEBUGGING
        LOG_INFO("Scripting", "[RegisterObject] obj = 0x{0:x}, {1}", (uint64)obj, String(ScriptingObjectData(obj).TypeName));
#endif
        m_ObjectsDictionary[id] = obj;
    }

    void Scripting::UnregisterObject(ScriptingObject* obj)
    {
        Threading::ScopeLock lock(m_ObjectsLocker);

        //ENGINE_ASSERT(!obj->_id.IsValid() || _objectsDictionary.ContainsValue(obj));

#if USE_OBJECTS_DISPOSE_CRASHES_DEBUGGING
        LOG_INFO("Scripting", "[UnregisterObject] obj = 0x{0:x}, {1}", (uint64)obj, String(ScriptingObjectData(obj).TypeName));
#endif
        m_ObjectsDictionary.Remove(obj->GetID());
    }

    void Scripting::OnObjectIdChanged(ScriptingObject* obj, const UID& oldId)
    {
        Threading::ScopeLock lock(m_ObjectsLocker);

        ENGINE_ASSERT(obj && oldId.IsValid());
        ENGINE_ASSERT(obj->GetID() != oldId);
        ENGINE_ASSERT(m_ObjectsDictionary.ContainsKey(oldId));
        //ENGINE_ASSERT(_objectsDictionary.ContainsValue(obj));
        ENGINE_ASSERT(!m_ObjectsDictionary.ContainsKey(obj->GetID()));

        m_ObjectsDictionary.Remove(oldId);
        m_ObjectsDictionary.Add(obj->GetID(), obj);
    }

    bool ScriptingSystem::OnInit()
    {
        Stopwatch stopwatch;

        // Initialize managed runtime
        if (!CLRCore::LoadEngine())
        {
            LOG_FATAL("Scripting", "C# runtime initialization failed.");
            return false;
        }

        // Cache root domain
        m_RootDomain = CLRCore::GetRootDomain();

        // Use single root domain
        auto domain = m_RootDomain;

        domain->SetCurrentDomain(true);
        m_ScriptsDomain = domain;

        // Add internal calls
        RegisterInternalCalls();

        // Load assemblies
        if (!Scripting::Load())
        {
            LOG_FATAL("Scripting", "Scripting Engine initialization failed.");
            return false;
        }

        stopwatch.Stop();
        LOG_INFO("Scripting", "Scripting Engine initializated! (time: {0}ms)", stopwatch.GetMilliseconds());
        return true;
    }

    void ScriptingSystem::OnUpdate()
    {

    }

    void ScriptingSystem::OnLateUpdate()
    {

    }

    void ScriptingSystem::OnBeforeExit()
    {

    }

    void ScriptingSystem::OnDispose()
    {

    }

    bool StdTypesContainerGather();
    void StdTypesContainerClear();

    bool InitEngine()
    {
        // Cache common types
        if (!StdTypesContainerGather())
        {
            return false;
        }

        // Init C# class library
        {
            auto scriptingClass = Scripting::GetScriptingClass();
            ENGINE_ASSERT(scriptingClass);
            const auto initMethod = scriptingClass->GetMethod("Init");
            ENGINE_ASSERT(initMethod);
            CLRObject* exception = nullptr;
            initMethod->Invoke(nullptr, nullptr, &exception);
            if (exception)
            {
                CLRException ex(exception);
                ex.Log(Log::Severity::Fatal, SE_TEXT("SolarEngine.Scripting.Init"));
                return false;
            }
        }

        // TODO: move it somewhere to game instance class or similar
        MainRenderTask::Instance = New<MainRenderTask>();

        return true;
    }

    void OnEngineLoaded(CLRAssembly* assembly)
    {
        if (InitEngine())
        {
            LOG_FATAL("Scripting", "Failed to initialize Solar Engine runtime.");
        }

        // Set flag
        _isEngineAssemblyLoaded = true;
    }

    void OnEngineUnloading(CLRAssembly* assembly)
    {
        // Clear flag
        _isEngineAssemblyLoaded = false;

        // Clear cached methods
        _method_Update = nullptr;
        _method_LateUpdate = nullptr;
        _method_FixedUpdate = nullptr;
        _method_Exit = nullptr;

        StdTypesContainerClear();
    }

    void StdTypesContainerClear()
    {
        CLRCore::TypeCache::UID = nullptr;
        CLRCore::TypeCache::Dictionary = nullptr;
        CLRCore::TypeCache::Activator = nullptr;
        CLRCore::TypeCache::Type = nullptr;

        CLRCore::TypeCache::Vector2 = nullptr;
        CLRCore::TypeCache::Vector3 = nullptr;
        CLRCore::TypeCache::Vector4 = nullptr;
        CLRCore::TypeCache::Color = nullptr;
        CLRCore::TypeCache::Transform = nullptr;
        CLRCore::TypeCache::Quaternion = nullptr;
        CLRCore::TypeCache::Matrix = nullptr;
        CLRCore::TypeCache::BoundingBox = nullptr;
        CLRCore::TypeCache::BoundingSphere = nullptr;
        CLRCore::TypeCache::Rectangle = nullptr;
        CLRCore::TypeCache::Ray = nullptr;

        CLRCore::TypeCache::CollisionClass = nullptr;

        CLRCore::TypeCache::JSON = nullptr;

        // GET_METHOD(Json_Serialize, JSON, "Serialize", 2);
        // GET_METHOD(Json_SerializeDiff, JSON, "SerializeDiff", 3);
        // GET_METHOD(Json_Deserialize, JSON, "Deserialize", 3);

        CLRCore::TypeCache::ManagedArrayClass = nullptr;

        /*#if SE_EDITOR
                GET_CLASS(FlaxEngine, ExecuteInEditModeAttribute, "SolarEngine.ExecuteInEditModeAttribute");
        #endif*/
    }

    bool StdTypesContainerGather()
    {
        #define GET_CLASS(assembly, type, typeName) \
        type = ((ManagedBinaryModule*)CONCAT_MACROS(GetBinaryModule, assembly)())->Assembly->GetClass(typeName);    \
        if (type == nullptr)                                                                                        \
        {                                                                                                           \
            LOG_ERROR("Scripting", "Missing managed type: \'{0}\'", StringAnsiView(typeName));                      \
            return false;                                                                                            \
        }

        #define GET_METHOD(type, klass, typeName, paramsCount)  \
        type = klass->GetMethod(typeName, paramsCount);         \
        if (type == nullptr)                                    \
        {                                                       \
            LOG_ERROR("Scripting", "Missing managed type: \'{0}\'", StringAnsiView(typeName));  \
            return false;                                                                        \
        }

        GET_CLASS(Corlib, CLRCore::TypeCache::UID, "System.Guid");
        GET_CLASS(Corlib, CLRCore::TypeCache::Dictionary, "System.Collections.Generic.Dictionary`2");
        GET_CLASS(Corlib, CLRCore::TypeCache::Activator, "System.Activator");
        GET_CLASS(Corlib, CLRCore::TypeCache::Type, "System.Type");

        GET_CLASS(SERuntime, CLRCore::TypeCache::Vector2, "SE.Vector2");
        GET_CLASS(SERuntime, CLRCore::TypeCache::Vector3, "SE.Vector3");
        GET_CLASS(SERuntime, CLRCore::TypeCache::Vector4, "SE.Vector4");
        GET_CLASS(SERuntime, CLRCore::TypeCache::Color, "SE.Color");
        GET_CLASS(SERuntime, CLRCore::TypeCache::Transform, "SE.Transform");
        GET_CLASS(SERuntime, CLRCore::TypeCache::Quaternion, "SE.Quaternion");
        GET_CLASS(SERuntime, CLRCore::TypeCache::Matrix, "SE.Matrix");
        GET_CLASS(SERuntime, CLRCore::TypeCache::BoundingBox, "SE.BoundingBox");
        GET_CLASS(SERuntime, CLRCore::TypeCache::BoundingSphere, "SE.BoundingSphere");
        GET_CLASS(SERuntime, CLRCore::TypeCache::Rectangle, "SE.Rectangle");
        GET_CLASS(SERuntime, CLRCore::TypeCache::Ray, "SE.Ray");

        GET_CLASS(SERuntime, CLRCore::TypeCache::CollisionClass, "SE.Collision");

        GET_CLASS(SERuntime, CLRCore::TypeCache::JSON, "SE.Json.JsonSerializer");

        // GET_METHOD(Json_Serialize, JSON, "Serialize", 2);
        // GET_METHOD(Json_SerializeDiff, JSON, "SerializeDiff", 3);
        // GET_METHOD(Json_Deserialize, JSON, "Deserialize", 3);

        GET_CLASS(SERuntime, CLRCore::TypeCache::ManagedArrayClass, "SE.Interop.ManagedArray");

/*#if SE_EDITOR
        GET_CLASS(FlaxEngine, ExecuteInEditModeAttribute, "SolarEngine.ExecuteInEditModeAttribute");
#endif*/

#undef GET_CLASS
#undef GET_METHOD
        return true;
    }
} // namespace SE

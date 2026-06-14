
#include "CLRCore.h"
#include "CLRClass.h"
#include "CLRAssembly.h"
#include "CLRDomain.h"
#include "CLREvent.h"
#include "CLRException.h"
#include "CLRProperty.h"
#include "Core/Logging/Logging.h"
#include "Core/Logging/Exceptions/FileNotFoundException.h"
#include "Core/Logging/Exceptions/InvalidOperationException.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Profiler/ProfilerCPU.h"
#include "Core/Types/Stopwatch.h"

namespace SE
{
    List<CLRDomain*, FixedAllocation<4>> m_Domains;
    CLRDomain* m_RootDomain = nullptr;
    CLRDomain* m_ActiveDomain = nullptr;

    // TypeCache statics
    CLRClass* CLRCore::TypeCache::Void = nullptr;
    CLRClass* CLRCore::TypeCache::Object = nullptr;
    CLRClass* CLRCore::TypeCache::Byte = nullptr;
    CLRClass* CLRCore::TypeCache::Boolean = nullptr;
    CLRClass* CLRCore::TypeCache::SByte = nullptr;
    CLRClass* CLRCore::TypeCache::Char = nullptr;
    CLRClass* CLRCore::TypeCache::Int16 = nullptr;
    CLRClass* CLRCore::TypeCache::UInt16 = nullptr;
    CLRClass* CLRCore::TypeCache::Int32 = nullptr;
    CLRClass* CLRCore::TypeCache::UInt32 = nullptr;
    CLRClass* CLRCore::TypeCache::Int64 = nullptr;
    CLRClass* CLRCore::TypeCache::UInt64 = nullptr;
    CLRClass* CLRCore::TypeCache::IntPtr = nullptr;
    CLRClass* CLRCore::TypeCache::UIntPtr = nullptr;
    CLRClass* CLRCore::TypeCache::Single = nullptr;
    CLRClass* CLRCore::TypeCache::Double = nullptr;
    CLRClass* CLRCore::TypeCache::String = nullptr;

    CLRClass* CLRCore::TypeCache::UID = nullptr;
    CLRClass* CLRCore::TypeCache::Dictionary = nullptr;
    CLRClass* CLRCore::TypeCache::Activator = nullptr;
    CLRClass* CLRCore::TypeCache::Type = nullptr;

    CLRClass* CLRCore::TypeCache::Vector2 = nullptr;
    CLRClass* CLRCore::TypeCache::Vector3 = nullptr;
    CLRClass* CLRCore::TypeCache::Vector4 = nullptr;
    CLRClass* CLRCore::TypeCache::Color = nullptr;
    CLRClass* CLRCore::TypeCache::Transform = nullptr;
    CLRClass* CLRCore::TypeCache::Quaternion = nullptr;
    CLRClass* CLRCore::TypeCache::Matrix = nullptr;
    CLRClass* CLRCore::TypeCache::BoundingBox = nullptr;
    CLRClass* CLRCore::TypeCache::BoundingSphere = nullptr;
    CLRClass* CLRCore::TypeCache::Rectangle = nullptr;
    CLRClass* CLRCore::TypeCache::Ray = nullptr;

    CLRClass* CLRCore::TypeCache::Int2 = nullptr;
    CLRClass* CLRCore::TypeCache::Int3 = nullptr;
    CLRClass* CLRCore::TypeCache::Int4 = nullptr;
    CLRClass* CLRCore::TypeCache::Float2 = nullptr;
    CLRClass* CLRCore::TypeCache::Float3 = nullptr;
    CLRClass* CLRCore::TypeCache::Float4 = nullptr;
    CLRClass* CLRCore::TypeCache::Double2 = nullptr;
    CLRClass* CLRCore::TypeCache::Double3 = nullptr;
    CLRClass* CLRCore::TypeCache::Double4 = nullptr;

    CLRClass* CLRCore::TypeCache::CollisionClass = nullptr;

    CLRClass* CLRCore::TypeCache::JSON = nullptr;
    CLRMethod* CLRCore::TypeCache::Json_Serialize = nullptr;
    CLRMethod* CLRCore::TypeCache::Json_SerializeDiff = nullptr;
    CLRMethod* CLRCore::TypeCache::Json_Deserialize = nullptr;

    CLRClass* CLRCore::TypeCache::ManagedArrayClass = nullptr;

    CLRAssembly::CLRAssembly(CLRDomain* domain, const StringAnsiView& name)
        : m_Domain(domain)
        , m_IsLoaded(false)
        , m_IsLoading(false)
        , m_HasCachedClasses(false)
        , m_ReloadCount(0)
        , m_Name(name)
    {
    }

    CLRAssembly::CLRAssembly(CLRDomain* domain, const StringAnsiView& name, const StringAnsiView& fullname, void* handle)
        : m_Handle(handle)
        , m_Fullname(fullname)
        , m_Domain(domain)
        , m_IsLoaded(false)
        , m_IsLoading(false)
        , m_HasCachedClasses(false)
        , m_ReloadCount(0)
        , m_Name(name)
    {
    }

    CLRAssembly::~CLRAssembly()
    {
        Unload();
    }

    String CLRAssembly::ToString() const
    {
        return m_Name.ToString();
    }

    bool CLRAssembly::Load(const String& assemblyPath, const StringView& nativePath)
    {
        if (IsLoaded())
        {
            return true;
        }
        PROFILE_CPU();
        ZoneText(*assemblyPath, assemblyPath.Length());
        Stopwatch stopwatch;

        const String* pathPtr = &assemblyPath;
        String path;
        if (!FileSystem::FileExists(assemblyPath))
        {
            path = assemblyPath;
            pathPtr = &path;
            if (!ResolveMissingFile(path))
            {
                Log::FileNotFoundException ex(assemblyPath);
                return false;
            }
        }

        OnLoading();

        if (!LoadImage(*pathPtr, nativePath))
        {
            OnLoadFailed();
            return false;
        }

        OnLoaded(stopwatch);
        return true;
    }

    void CLRAssembly::Unload(bool isReloading)
    {
        if (!IsLoaded())
            return;
        PROFILE_CPU();

        Unloading(this);

        // Close runtime
        UnloadImage(isReloading);

        // Cleanup
        m_DebugData.Resize(0);
        m_AssemblyPath.Clear();
        m_IsLoading = false;
        m_IsLoaded = false;
        m_HasCachedClasses = false;
        m_Classes.ClearDelete();

        Unloaded(this);
    }

    CLRClass* CLRAssembly::GetClass(const StringAnsiView& fullname) const
    {
        // Check state
        if (!IsLoaded())
        {
            Log::InvalidOperationException(SE_TEXT("SEAssembly was not yet loaded or loading was in progress"));
            return nullptr;
        }

        StringAnsiView key(fullname);

        // Special case for reference
        if (fullname[fullname.Length() - 1] == '&')
            key = StringAnsiView(key.Get(), key.Length() - 1);

        // Find class by name
        const auto& classes = GetClasses();
        CLRClass* result = nullptr;
        classes.TryGet(key, result);

#if 0
        if (!result)
        {
            LOG(Warning, "Failed to find class {0} in assembly {1}. Classes:", String(fullname), ToString());
            for (auto i = classes.Begin(); i.IsNotEnd(); ++i)
            {
                LOG(Warning, " - {0}", String(i->Key));
            }
        }
#endif
        return result;
    }

    void CLRAssembly::OnLoading()
    {
        Loading(this);

        m_IsLoading = true;

        // Pick a domain
        if (m_Domain == nullptr)
            m_Domain = CLRCore::GetActiveDomain();
    }

    void CLRAssembly::OnLoaded(Stopwatch& stopwatch)
    {
        // Register in domain
        m_Domain->_assemblies[m_Name] = this;

        m_IsLoaded = true;
        m_IsLoading = false;

        stopwatch.Stop();
        LOG_INFO("Scripting", "Assembly {0} loaded in {1}ms", String(m_Name), stopwatch.GetMilliseconds());

        // Pre-cache classes
        GetClasses();

        Loaded(this);
    }

    void CLRAssembly::OnLoadFailed()
    {
        m_IsLoading = false;

        LoadFailed(this);
    }



    CLREvent* CLRClass::GetEvent(const char* name) const
    {
        GetEvents();
        for (int32 i = 0; i < m_Events.Count(); i++)
        {
            if (m_Events[i]->GetName() == name)
                return m_Events[i];
        }
        return nullptr;
    }

    CLRObject* CLRClass::CreateInstance() const
    {
        CLRObject* obj = CLRCore::Object::New(this);
        if (!IsValueType())
            CLRCore::Object::Init(obj);
        return obj;
    }



    CLRType* CLREvent::GetType() const
    {
        if (GetAddMethod() != nullptr)
            return GetAddMethod()->GetReturnType();
        if (GetRemoveMethod() != nullptr)
            return GetRemoveMethod()->GetReturnType();
        return nullptr;
    }



    void CLRException::Log(const Log::Severity type, const Char* target)
    {
        // Log inner exceptions chain
        CLRException* inner = InnerException;
        while (inner)
        {
            const Char* stackTrace = inner->StackTrace.HasChars() ? *inner->StackTrace : SE_TEXT("<empty>");
            LOG_WARNING("Scripting", "Inner exception. {0}\nStack strace:\n{1}\n", inner->Message, stackTrace);
            inner = inner->InnerException;
        }

        // Send stack trace only to log file
        const Char* stackTrace = StackTrace.HasChars() ? *StackTrace : SE_TEXT("<empty>");
        const String info = target && *target ? String::Format(SE_TEXT("Exception has been thrown during {0}."), target) : SE_TEXT("Exception has been thrown.");
        LOG_WARNING("Scripting", "{0} {1}\nStack strace:\n{2}", info, Message, stackTrace);
        Log::AddEntry(type, SE_TEXT("Scripting"), "", 0, SE_TEXT("{0}\n{1}"), info, Message);
    }


    CLRType* CLRProperty::GetType() const
    {
        if (GetGetMethod() != nullptr)
            return GetGetMethod()->GetReturnType();
        return GetSetMethod()->GetReturnType();
    }

    CLRVisibility CLRProperty::GetVisibility() const
    {
        if (GetGetMethod() && GetSetMethod())
        {
            return static_cast<CLRVisibility>(
                Math::Max(
                    static_cast<int>(GetGetMethod()->GetVisibility()),
                    static_cast<int>(GetSetMethod()->GetVisibility())
                ));
        }
        if (GetGetMethod())
        {
            return GetGetMethod()->GetVisibility();
        }
        return GetSetMethod()->GetVisibility();
    }

    bool CLRProperty::IsStatic() const
    {
        if (GetGetMethod())
        {
            return GetGetMethod()->IsStatic();
        }
        if (GetSetMethod())
        {
            return GetSetMethod()->IsStatic();
        }
        return false;
    }

    CLRDomain* CLRCore::GetRootDomain()
    {
        return m_RootDomain;
    }

    CLRDomain* CLRCore::GetActiveDomain()
    {
        return m_ActiveDomain;
    }
} // namespace SE

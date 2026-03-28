
#include "SECore.h"
#include "SEClass.h"
#include "SEAssembly.h"
#include "SEDomain.h"
#include "SEEvent.h"
#include "SEException.h"
#include "SEProperty.h"
#include "Core/Logging/Logging.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Profiler/ProfilerCPU.h"
#include "Core/Types/Stopwatch.h"

namespace SE
{
    SEDomain* m_RootDomain = nullptr;
    SEDomain* m_ActiveDomain = nullptr;

    // TypeCache statics
    SEClass* SECore::TypeCache::Void    = nullptr;
    SEClass* SECore::TypeCache::Object  = nullptr;
    SEClass* SECore::TypeCache::Int32   = nullptr;
    SEClass* SECore::TypeCache::String  = nullptr;
    SEClass* SECore::TypeCache::Boolean = nullptr;
    SEClass* SECore::TypeCache::Single  = nullptr;
    SEClass* SECore::TypeCache::Double  = nullptr;
    SEClass* SECore::TypeCache::Int64   = nullptr;

    SEAssembly::SEAssembly(SEDomain* domain, const StringAnsiView& name)
        : m_Domain(domain)
        , m_IsLoaded(false)
        , m_IsLoading(false)
        , m_HasCachedClasses(false)
        , m_ReloadCount(0)
        , m_Name(name)
    {
    }

    SEAssembly::SEAssembly(SEDomain* domain, const StringAnsiView& name, const StringAnsiView& fullname, void* handle)
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

    SEAssembly::~SEAssembly()
    {
        Unload();
    }

    String SEAssembly::ToString() const
    {
        return m_Name.ToString();
    }

    bool SEAssembly::Load(const String& assemblyPath, const StringView& nativePath)
    {
        if (IsLoaded())
            return false;
        PROFILE_CPU();
        ZoneText(*assemblyPath, assemblyPath.Length());
        Stopwatch stopwatch;

        const String* pathPtr = &assemblyPath;
        String path;
        if (!FileSystem::FileExists(assemblyPath))
        {
            path = assemblyPath;
            pathPtr = &path;
            if (ResolveMissingFile(path))
            {
                Log::FileNotFoundException ex(assemblyPath);
                return true;
            }
        }

        OnLoading();

        if (LoadImage(*pathPtr, nativePath))
        {
            OnLoadFailed();
            return true;
        }

        OnLoaded(stopwatch);
        return false;
    }

    void SEAssembly::Unload(bool isReloading)
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

    SEClass* SEAssembly::GetClass(const StringAnsiView& fullname) const
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
        SEClass* result = nullptr;
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

    void SEAssembly::OnLoading()
    {
        Loading(this);

        m_IsLoading = true;

        // Pick a domain
        if (m_Domain == nullptr)
            m_Domain = SECore::GetActiveDomain();
    }

    void SEAssembly::OnLoaded(Stopwatch& stopwatch)
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

    void SEAssembly::OnLoadFailed()
    {
        m_IsLoading = false;

        LoadFailed(this);
    }



    SEEvent* SEClass::GetEvent(const char* name) const
    {
        GetEvents();
        for (int32 i = 0; i < m_Events.Count(); i++)
        {
            if (m_Events[i]->GetName() == name)
                return m_Events[i];
        }
        return nullptr;
    }

    SEObject* SEClass::CreateInstance() const
    {
        SEObject* obj = SECore::Object::New(this);
        if (!IsValueType())
            SECore::Object::Init(obj);
        return obj;
    }



    SEType* SEEvent::GetType() const
    {
        if (GetAddMethod() != nullptr)
            return GetAddMethod()->GetReturnType();
        if (GetRemoveMethod() != nullptr)
            return GetRemoveMethod()->GetReturnType();
        return nullptr;
    }



    void SEException::Log(const Log::Severity type, const Char* target)
    {
        // Log inner exceptions chain
        SEException* inner = InnerException;
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


    SEType* SEProperty::GetType() const
    {
        if (GetGetMethod() != nullptr)
            return GetGetMethod()->GetReturnType();
        return GetSetMethod()->GetReturnType();
    }

    SEVisibility SEProperty::GetVisibility() const
    {
        if (GetGetMethod() && GetSetMethod())
        {
            return static_cast<SEVisibility>(
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

    bool SEProperty::IsStatic() const
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

    SEDomain* SECore::GetRootDomain()
    {
        return m_RootDomain;
    }

    SEDomain* SECore::GetActiveDomain()
    {
        return m_ActiveDomain;
    }
} // namespace SE

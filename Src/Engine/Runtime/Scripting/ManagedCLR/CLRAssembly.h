#pragma once

#include "CLRTypes.h"
#include "Runtime/API.h"
#include "Core/Types/Delegate.h"
#include "Core/Types/Strings/String.h"
#include "Core/Types/Collections/List.h"
#include "Core/Types/Collections/Dictionary.h"
#include "Core/Platform/CriticalSection.h"

namespace SE
{
    /// <summary>
    /// Load and management unit for a C# assembly (.dll).
    /// Provides Load/Unload lifecycle and type lookup by fully-qualified name.
    /// </summary>
    class SE_API_RUNTIME CLRAssembly
    {
        friend CLRDomain;
        friend Scripting;

    public:
        typedef Dictionary<StringAnsi, CLRClass*> ClassesDictionary;

    private:
        void* m_Handle = nullptr;
        StringAnsi m_Fullname;
        CLRDomain* m_Domain;

        int32 m_IsLoaded : 1;
        int32 m_IsLoading : 1;
        mutable int32 m_HasCachedClasses : 1;

        mutable ClassesDictionary m_Classes;

        int32 m_ReloadCount;
        StringAnsi m_Name;
        String m_AssemblyPath;

        List<byte> m_DebugData;

    public:
        /// <summary>
        /// Initializes a new instance of the <see cref="SEAssembly"/> class.
        /// </summary>
        /// <param name="domain">The assembly domain.</param>
        /// <param name="name">The assembly name.</param>
        CLRAssembly(CLRDomain* domain, const StringAnsiView& name);

        /// <summary>
        /// Initializes a new instance of the <see cref="SEAssembly"/> class.
        /// </summary>
        /// <param name="domain">The assembly domain.</param>
        /// <param name="name">The assembly name.</param>
        /// <param name="fullname">The assembly full name.</param>
        /// <param name="handle">The managed handle of the assembly.</param>
        CLRAssembly(CLRDomain* domain, const StringAnsiView& name, const StringAnsiView& fullname, void* handle);

        /// <summary>
        /// Finalizes an instance of the <see cref="SEAssembly"/> class.
        /// </summary>
        ~CLRAssembly();

    public:
        /// <summary>
        /// Managed assembly actions delegate type.
        /// </summary>
        typedef Delegate<CLRAssembly*> AssemblyDelegate;

        /// <summary>
        /// Action fired when assembly starts loading.
        /// </summary>
        AssemblyDelegate Loading;

        /// <summary>
        /// Action fired when assembly gets loaded.
        /// </summary>
        AssemblyDelegate Loaded;

        /// <summary>
        /// Action fired when assembly loading fails.
        /// </summary>
        AssemblyDelegate LoadFailed;

        /// <summary>
        /// Action fired when assembly start unloading.
        /// </summary>
        AssemblyDelegate Unloading;

        /// <summary>
        /// Action fired when assembly gets unloaded.
        /// </summary>
        AssemblyDelegate Unloaded;

    public:
        /// <summary>
        /// Returns true if assembly is during loading state.
        /// </summary>
        FORCE_INLINE bool IsLoading() const
        {
            return m_IsLoading != 0;
        }

        /// <summary>
        /// Returns true if assembly has been loaded.
        /// </summary>
        FORCE_INLINE bool IsLoaded() const
        {
            return m_IsLoaded != 0;
        }

        /// <summary>
        /// Gets the assembly name.
        /// </summary>
        FORCE_INLINE const StringAnsi& GetName() const
        {
            return m_Name;
        }

        /// <summary>
        /// Gets the assembly name as string.
        /// </summary>
        String ToString() const;

        /// <summary>
        /// Gets the assembly path.
        /// </summary>
        /// <remarks>
        /// If assembly was made from scratch (empty), path will return null.
        /// </remarks>
        FORCE_INLINE const String& GetAssemblyPath() const
        {
            return m_AssemblyPath;
        }

        /// <summary>
        /// Gets the parent domain.
        /// </summary>
        FORCE_INLINE CLRDomain* GetDomain() const
        {
            return m_Domain;
        }


        FORCE_INLINE void* GetHandle() const
        {
            return m_Handle;
        }

    public:
        /// <summary>
        /// Loads assembly for domain.
        /// </summary>
        /// <param name="assemblyPath">The assembly path.</param>
        /// <param name="nativePath">The optional path to the native code assembly (eg. if C# assembly contains bindings).</param>
        /// <returns>True if cannot load, otherwise false</returns>
        bool Load(const String& assemblyPath, const StringView& nativePath = StringView::Empty);

        /// <summary>
        /// Cleanup data. Caller must ensure not to use any types from this assembly after it has been unloaded.
        /// </summary>
        /// <param name="isReloading">If true assembly is during reloading and should force release the runtime data.</param>
        void Unload(bool isReloading = false);

    public:
        /// <summary>
        /// Attempts to find a managed class with the specified namespace and name in this assembly. Returns null if one cannot be found.
        /// </summary>
        /// <param name="typeName">The type name.</param>
        /// <returns>The class object or null if failed to find it.</returns>
        CLRClass* GetClass(const StringAnsiView& typeName) const;

        /// <summary>
        /// Gets the classes lookup cache. Performs full initialization if not cached. The result cache contains all classes from the assembly.
        /// </summary>
        const ClassesDictionary& GetClasses() const;

    private:
        bool LoadCorlib();
        bool LoadImage(const String& assemblyPath, const StringView& nativePath);
        bool UnloadImage(bool isReloading);
        void OnLoading();
        void OnLoaded(struct Stopwatch& stopwatch);
        void OnLoadFailed();
        bool ResolveMissingFile(String& assemblyPath) const;
    };
} // namespace SE

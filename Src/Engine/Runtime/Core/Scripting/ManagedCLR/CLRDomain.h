#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/Core/Types/Collections/Dictionary.h"

namespace SE
{
    class CLRAssembly;
    class CLRCore;


    /// <summary>
    /// Domain separates multiple processes within one executed CLR environment.
    /// </summary>
    /// <remarks>
    /// At once you can execute methods, get instances etc, only from on Domain at the time.
    /// If you want to execute any code that given domain contains, you need to switch context, and dispatch current thread to CLR environment.
    /// </remarks>
    class SE_API_RUNTIME CLRDomain
    {
        friend CLRCore;
        friend CLRAssembly;

    public:
        typedef Dictionary<StringAnsi, CLRAssembly*> AssembliesDictionary;

    private:
        StringAnsi _domainName;
        AssembliesDictionary _assemblies;

    public:
        CLRDomain(const StringAnsi& domainName)
            : _domainName(domainName)
        {
        }

    public:
        /// <summary>
        /// Gets current domain name
        /// </summary>
        FORCE_INLINE const StringAnsi& GetName() const
        {
            return _domainName;
        }

        /// <summary>
        /// Gets the current domain assemblies.
        /// </summary>
        FORCE_INLINE const AssembliesDictionary& GetAssemblies() const
        {
            return _assemblies;
        }

    public:
        /// <summary>
        /// Attaches current CLR domain calls to the current thread.
        /// </summary>
        void Dispatch() const;

        /// <summary>
        /// Sets currently using domain.
        /// </summary>
        /// <returns>True if succeed in settings, false if failed.</returns>
        bool SetCurrentDomain(bool force = false);
    };
} // namespace SE

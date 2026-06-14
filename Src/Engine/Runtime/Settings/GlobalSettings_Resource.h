#pragma once

#include "Core/Math/Math.h"
#include "Settings.h"

//-------------------------------------------------------------------------

namespace SE
{
    class SE_API_RUNTIME ResourceGlobalSettings : public GlobalSettings
    {
        SE_DEFINE_CLASS(ResourceGlobalSettings, GlobalSettings);

        // Paths
        //-------------------------------------------------------------------------
        // Default paths are relative to the working directory of the process
        constexpr static Char const * const s_defaultRawResourcePath = SE_TEXT("Data/");
        constexpr static Char const * const s_defaultPackagedBuildName = SE_TEXT("x64_Shipping");

        // Resource Compiler
        //-------------------------------------------------------------------------
        constexpr static Char const * const s_defaultCompiledResourceDirectoryName = SE_TEXT("CompiledData");
        constexpr static Char const * const s_defaultCompiledResourceDatabaseName = SE_TEXT("CompiledData.db");

        // Resource Server
        //-------------------------------------------------------------------------

        constexpr static Char const * const s_defaultResourceServerExecutableName = SE_TEXT("ResourceServer.exe");
        constexpr static Char const * const s_defaultResourceServerAddress = SE_TEXT("127.0.0.1");
        constexpr static uint16 const s_defaultResourceServerPort = 5556;

    public:
        virtual bool LoadSettings(IniFile const& ini ) override;
        virtual bool SaveSettings(IniFile& ini ) const override;

    private:

        bool TryGeneratePaths();

    public:
        ResourceGlobalSettings();

        // Settings
        //-------------------------------------------------------------------------

        String                  m_compiledResourceDirectoryName = String(s_defaultCompiledResourceDirectoryName);

        #ifdef SE_DEVELOPMENT
        String                  m_packagedBuildName = String(s_defaultPackagedBuildName);
        String                  m_compiledDBName = String(s_defaultCompiledResourceDatabaseName);
        String                  m_resourceServerExeName = String(s_defaultResourceServerExecutableName);
        String                  m_resourceServerNetworkAddress = String(s_defaultResourceServerAddress);
        uint16	                m_resourceServerPort = 5556;
        #endif

        // Derived Paths
        //-------------------------------------------------------------------------

		String        m_compiledResourcePath;

        #ifdef SE_DEVELOPMENT
		String        m_rawResourcePath;
		String        m_packagedBuildCompiledResourcePath;
		String        m_compiledResourceDatabasePath;
		String        m_resourceServerExecutablePath;
        #endif
    };
}
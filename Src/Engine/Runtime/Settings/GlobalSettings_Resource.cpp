#include "GlobalSettings_Resource.h"
#include "Runtime/Core/IniFile.h"
#include "Runtime/Core/Platform/FileSystem.h"

#include "Runtime/EngineContext.h"
//-------------------------------------------------------------------------

namespace SE
{
    ResourceGlobalSettings::ResourceGlobalSettings()
    {
    }

    bool ResourceGlobalSettings::LoadSettings(IniFile const &ini)
    {
        // Settings
        //-------------------------------------------------------------------------

        m_compiledResourceDirectoryName = ini.GetStringOrDefault("Resource:CompiledResourceDirectoryName", s_defaultCompiledResourceDirectoryName);

#if SE_DEVELOPMENT
        {
            m_packagedBuildName = ini.GetStringOrDefault("Resource:PackagedBuildName", s_defaultPackagedBuildName);
            m_compiledDBName = ini.GetStringOrDefault("Resource:CompiledResourceDatabaseName", s_defaultCompiledResourceDatabaseName);
            m_resourceServerExeName = ini.GetStringOrDefault("Resource:ResourceServerExecutable", s_defaultResourceServerExecutableName);
            m_resourceServerNetworkAddress = ini.GetStringOrDefault("Resource:ResourceServerAddress", s_defaultResourceServerAddress);
            m_resourceServerPort = (uint16)ini.GetUIntOrDefault("Resource:ResourceServerPort", s_defaultResourceServerPort);
        }
#endif

        //-------------------------------------------------------------------------

        return TryGeneratePaths();
    }

    bool ResourceGlobalSettings::TryGeneratePaths()
    {
        // Compiled Resource Path
        //-------------------------------------------------------------------------

        m_compiledResourcePath = EngineContext::ProjectCacheFolder + SE_TEXT("/") + m_compiledResourceDirectoryName.Get();
		m_compiledResourcePath = FileSystem::ConvertAbsolutePathToRelative(EngineContext::ProjectFolder, m_compiledResourcePath);

		if (m_compiledResourcePath.IsEmpty())
        {
            LOG_ERROR("Resource", "Resource Settings Invalid compiled data path: {0}", m_compiledResourcePath);
            return false;
        }

//        m_compiledResourcePath.MakeIntoDirectoryPath();

        //-------------------------------------------------------------------------

#if SE_DEVELOPMENT
        {
            // Raw Resource Path
            //-------------------------------------------------------------------------

//            m_rawResourcePath.MakeIntoDirectoryPath();

            // Packaged Build Path
            //-------------------------------------------------------------------------

//            m_packagedBuildCompiledResourcePath = m_compiledResourcePath.GetParentDirectory().GetParentDirectory();
            m_packagedBuildCompiledResourcePath += m_packagedBuildName.Get();
            m_packagedBuildCompiledResourcePath += m_compiledResourceDirectoryName.Get();

            if (m_packagedBuildCompiledResourcePath.IsEmpty())
            {
                LOG_ERROR("Resource", "Resource Settings Invalid source data path: {0}", m_rawResourcePath);
                return false;
            }

//            m_packagedBuildCompiledResourcePath.MakeIntoDirectoryPath();

            // Compiled Resource DB
            //-------------------------------------------------------------------------

            m_compiledResourceDatabasePath = m_compiledResourcePath + SE_TEXT("/") + m_compiledDBName;

            if (m_compiledResourceDatabasePath.IsEmpty())
            {
                LOG_ERROR("Resource", "Resource Settings Invalid compiled resource database path: {0}", m_compiledResourceDatabasePath);
                return false;
            }

            // Resource Server Executable
            //-------------------------------------------------------------------------

            m_resourceServerExecutablePath = m_compiledResourcePath + m_resourceServerExeName.Get();

            if (m_resourceServerExecutablePath.IsEmpty())
            {
                LOG_ERROR("Resource", "Resource Settings Invalid resource server path: {0}", m_resourceServerExecutablePath);
                return false;
            }
        }
#endif

        return true;
    }

    bool ResourceGlobalSettings::SaveSettings(IniFile &ini) const
    {
        ini.CreateSection("Resource");
        ini.SetString("Resource:CompiledResourceDirectoryName", m_compiledResourceDirectoryName);

#if SE_DEVELOPMENT
        ini.SetString("Resource:PackagedBuildName", m_packagedBuildName);
        ini.SetString("Resource:CompiledResourceDatabaseName", m_compiledDBName);
        ini.SetString("Resource:ResourceServerExecutable", m_resourceServerExeName);
        ini.SetString("Resource:ResourceServerAddress", m_resourceServerNetworkAddress);
        ini.SetUInt("Resource:ResourceServerPort", m_resourceServerPort);
#endif

        return true;
    }
}
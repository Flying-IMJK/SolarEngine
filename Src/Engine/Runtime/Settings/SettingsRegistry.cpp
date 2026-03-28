#include "SettingsRegistry.h"
#include "Core/TypeSystem/Types.h"
#include "Core/Utilities/IniFile.h"
#include "Core/Memory/Memory.h"
#include "Core/Types/Collections/ListExtensions.h"

//-------------------------------------------------------------------------

namespace SE
{
    ISettings *SettingsRegistry::Group::GetSettings(TypeCompositeInfo const *pTypeInfo)
    {
        ENGINE_ASSERT(pTypeInfo != nullptr);
        ENGINE_ASSERT(pTypeInfo->IsDerivedFrom(Typeof<ISettings>()));
        ENGINE_ASSERT(!pTypeInfo->IsDerivedFrom(Typeof<GlobalSettings>()));

        for (auto pSettings : m_settings)
        {
            if (pSettings->GetType() == pTypeInfo->id)
            {
                return pSettings;
            }
        }

        return nullptr;
    }

    //-------------------------------------------------------------------------

    bool SettingsRegistry::Initialize(String const &iniFilePath)
    {
        ENGINE_ASSERT(m_globalSettings.IsEmpty());

        List<TypeCompositeInfo const *> settingsTypes = Types::GetAllDerivedTypes(Typeof<GlobalSettings>(), false, false, false);
        for (auto pTypeInfo : settingsTypes)
        {
            m_globalSettings.Add(TypeCast<GlobalSettings>(pTypeInfo->CreateType()));
        }

        //-------------------------------------------------------------------------

        ENGINE_ASSERT(iniFilePath != String::Empty);
        m_iniFilePath = iniFilePath;

        // If the ini file doesnt exist, create one with the default settings!
        IniFile iniFile(iniFilePath);
        if (!iniFile.IsValid())
        {
#ifdef SE_DEVELOPMENT
            if (!SaveGlobalSettingsToIniFile())
            {
                LOG_FATAL("Settings", "Registry Failed to generate default INI file: {0}", iniFilePath);
                return false;
            }
#endif

            // Try to load the newly created ini file
            iniFile = IniFile(iniFilePath);
            if (!iniFile.IsValid())
            {
                LOG_FATAL("Settings", "Registry Failed to load settings from INI file: {0}", iniFilePath);
                return false;
            }
        }

        //-------------------------------------------------------------------------

        for (auto pGlobalSettings : m_globalSettings)
        {
            if (!pGlobalSettings->LoadSettings(iniFile))
            {
                return false;
            }
        }

        return true;
    }

    void SettingsRegistry::Shutdown()
    {
        m_iniFilePath.Clear();

        //-------------------------------------------------------------------------

        for (auto pSettings : m_globalSettings)
        {
            Delete(pSettings);
        }

        m_globalSettings.Clear();
    }

#ifdef SE_DEVELOPMENT
    bool SettingsRegistry::SaveGlobalSettingsToIniFile()
    {
        IniFile iniFile;

        for (auto pGlobalSettings : m_globalSettings)
        {
            if (!pGlobalSettings->SaveSettings(iniFile))
            {
                return false;
            }
        }

        return iniFile.SaveToFile(m_iniFilePath);
    }
#endif

    //-------------------------------------------------------------------------

    ISettings *SettingsRegistry::CreateSettings(uint64 groupID, TypeCompositeInfo const *pTypeInfo)
    {
        ENGINE_ASSERT(pTypeInfo != nullptr);
        ENGINE_ASSERT(pTypeInfo->IsDerivedFrom(Typeof<ISettings>()));
        ENGINE_ASSERT(!pTypeInfo->IsDerivedFrom(Typeof<GlobalSettings>()));

        auto pGroup = FindGroup(groupID);
        ENGINE_ASSERT(pGroup != nullptr);
        ENGINE_ASSERT(pGroup->GetSettings(pTypeInfo) == nullptr);

        ISettings *pSettings = TypeCast<ISettings>(pTypeInfo->CreateType());
		pGroup->m_settings.Add(pSettings);
        return pSettings;
    }

    void SettingsRegistry::CreateGroup(uint64_t groupID, String const &friendlyName)
    {
        Group *pGroup = FindGroup(groupID);
        ENGINE_ASSERT(pGroup == nullptr);
        m_groups.Add(Group(groupID, friendlyName));
    }

    void SettingsRegistry::DestroyGroup(uint64_t groupID)
    {
		Function<bool(const Group &)> call([groupID](const Group& group){
		  return group.m_groupID == groupID;
		});

		int index = ListExtensions::IndexOf(m_groups, call);

		if (index != INVALID_INDEX)
		{
			Group& group = m_groups.At(index);
			for (auto pSettings : group.m_settings)
			{
				Delete(pSettings);
			}

			m_groups.RemoveAt(index);
			return;
		}

        ENGINE_UNREACHABLE_CODE();
    }

    SettingsRegistry::Group *SettingsRegistry::FindGroup(uint64 groupID)
    {
        for (auto &group : m_groups)
        {
            if (group.m_groupID == groupID)
            {
                return &group;
            }
        }

        return nullptr;
    }
}
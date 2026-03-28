#pragma once
#include "Core/TypeSystem/IType.h"
#include "Core/Systems.h"
#include "Settings.h"

//-------------------------------------------------------------------------

namespace SE
{
    class Console;
}
namespace SE
{
    class TypeCompositeInfo;
}

//-------------------------------------------------------------------------

namespace SE
{
    class SE_API_RUNTIME SettingsRegistry : public ISystem
    {
        friend Console;

    public:
        ENGINE_SYSTEM(SettingsRegistry);

    private:
        struct Group
        {
            Group() = default;
            Group(uint64 ID, String const &name = SE_TEXT("")) : m_groupID(ID), m_name(name) {}

            template <typename T>
            T *GetSettings()
            {
                static_assert(std::is_base_of<ISettings, T>::value, "T must be derived from ISettings");
                static_assert(!std::is_base_of<GlobalSettings, T>::value, "T is not allowed to be derived from GlobalSettings");

                for (auto pSettings : m_settings)
                {
                    T *pT = TryCast<T>(pSettings);
                    if (pT != nullptr)
                    {
                        return pT;
                    }
                }

                return nullptr;
            }

            template <typename T>
            T const *GetSettings() const
            {
                static_assert(std::is_base_of<ISettings, T>::value, "T must be derived from ISettings");
                static_assert(!std::is_base_of<GlobalSettings, T>::value, "T is not allowed to be derived from GlobalSettings");
                return const_cast<Group *>(this)->GetSettings<T>();
            }

            ISettings *GetSettings(TypeCompositeInfo const *pTypeInfo);

            ISettings const *GetSettings(TypeCompositeInfo const *pTypeInfo) const { return const_cast<Group *>(this)->GetSettings(pTypeInfo); }

        public:
            uint64 m_groupID = 0xFFFFFFFF;
            String m_name;
			List<ISettings *> m_settings;
        };

    public:
        bool Initialize(String const &iniFilePath);
        void Shutdown();

#ifdef SE_DEVELOPMENT
        bool SaveGlobalSettingsToIniFile();
#endif

        // Global Settings
        //-------------------------------------------------------------------------

        // Get the settings object of the specified type
        template <typename T>
        T *GetGlobalSettings()
        {
            static_assert(std::is_base_of<GlobalSettings, T>::value, "T must be derived from GlobalSettings");

            for (auto pSettings : m_globalSettings)
            {
                T *pT = TryCast<T>(pSettings);
                if (pT != nullptr)
                {
                    return pT;
                }
            }

            return nullptr;
        }

        // Get the settings object of the specified type
        template <typename T>
        T const *GetGlobalSettings() const
        {
            static_assert(std::is_base_of<GlobalSettings, T>::value, "T must be derived from GlobalSettings");
            return const_cast<SettingsRegistry *>(this)->GetGlobalSettings<T>();
        }

        // Dynamic Settings
        //-------------------------------------------------------------------------
        // You can create settings object at runtime and group them via ID
        // You are only allowed a single settings object of a given type per group

        template <typename T>
        T *CreateSettings(uint64 groupID)
        {
            static_assert(std::is_base_of<ISettings, T>::value, "T must be derived from ISettings");
            static_assert(!std::is_base_of<GlobalSettings, T>::value, "T is not allowed to be derived from GlobalSettings");

            auto pGroup = FindOrCreateGroup(groupID);
            ENGINE_ASSERT(pGroup != nullptr);
			ENGINE_ASSERT(pGroup->GetSettings<T>() == nullptr);

            T *pSettings = pGroup->m_settings.emplace_back(New<T>());
            return pSettings;
        }

        ISettings *CreateSettings(uint64 groupID, TypeCompositeInfo const *pTypeInfo);

        // Get the settings object of the specified type
        template <typename T>
        T *GetSettings(uint64 groupID)
        {
            static_assert(std::is_base_of<ISettings, T>::value, "T must be derived from ISettings");
            static_assert(!std::is_base_of<GlobalSettings, T>::value, "T is not allowed to be derived from GlobalSettings");

            Group *pGroup = FindGroup(groupID);
            return (pGroup != nullptr) ? pGroup->GetSettings<T>() : nullptr;
        }

        // Get the settings object of the specified type
        template <typename T>
        T const *GetSettings(uint64 groupID) const
        {
            static_assert(std::is_base_of<ISettings, T>::value, "T must be derived from ISettings");
            static_assert(!std::is_base_of<GlobalSettings, T>::value, "T is not allowed to be derived from GlobalSettings");
            return const_cast<SettingsRegistry *>(this)->GetSettings<T>(groupID);
        }

        // Get the settings object of the specified type
        ISettings *GetSettings(uint64 groupID, TypeCompositeInfo const *pTypeInfo)
        {
            Group *pGroup = FindGroup(groupID);
            return (pGroup != nullptr) ? pGroup->GetSettings(pTypeInfo) : nullptr;
        }

        // Get the settings object of the specified type
        ISettings const *GetSettings(uint64 groupID, TypeCompositeInfo const *pTypeInfo) const { return const_cast<SettingsRegistry *>(this)->GetSettings(groupID, pTypeInfo); }

        // Create a group with a specific ID
        void CreateGroup(uint64 groupID, String const &friendlyName = SE_TEXT(""));

        // Destroy a group and all settings for it
        void DestroyGroup(uint64 groupID);

    private:
        Group *FindGroup(uint64 groupID);

    private:
        String m_iniFilePath;
		List<GlobalSettings *> m_globalSettings;
		List<Group> m_groups;
    };
}
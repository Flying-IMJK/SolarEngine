#pragma once

#include "Runtime/Core/Logging/Logging.h"

namespace SE
{
    template<typename T, typename PT = T>
    class AutoSingleton
    {
        NON_COPYABLE(AutoSingleton)
    public:
        static PT& Instance()
        {
            static T singleObj;
            return singleObj;
        }

    protected:
        AutoSingleton() = default;

        virtual ~AutoSingleton() = default;
    };

    template<typename T, typename PT = T>
    class ManualSingleton
    {
        NON_COPYABLE(ManualSingleton)
    public:
        static PT* Instance()
        {
            ENGINE_ASSERT(!m_IsDestroyed);

            if (m_IsDestroyed)
                return NULL;

            if (!m_ObjectPtr)
            {
                m_ObjectPtr = new T();
            }
            return m_ObjectPtr;
        }

        static bool IsBeing() { return m_ObjectPtr != nullptr; }

        static PT* GetInstance()
        {
            ENGINE_ASSERT(m_ObjectPtr);
            return m_ObjectPtr;
        }

        static void Destroy()
        {
            if (m_ObjectPtr)
            {
                delete (m_ObjectPtr);
                m_ObjectPtr   = NULL;
                m_IsDestroyed = true;
            }
        }

    protected:
        static bool m_IsDestroyed;
        static PT*  m_ObjectPtr;

    protected:
        ManualSingleton() = default;

        virtual ~ManualSingleton() = default;
    };

    template<typename T, typename PT>
    PT* ManualSingleton<T, PT>::m_ObjectPtr = NULL;
    template<typename T, typename PT>
    bool ManualSingleton<T, PT>::m_IsDestroyed = false;
}
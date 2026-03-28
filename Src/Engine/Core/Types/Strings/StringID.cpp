#include "Core/Types/Strings/StringID.h"
#include "Core/Memory/Memory.h"
#include "Core/Thread/Threading.h"
#include "Core/Types/Strings/String.h"
#include "Core/Types/Collections/Dictionary.h"

//-------------------------------------------------------------------------

namespace SE
{
    class StringID_CustomAllocator
    {
    public:
        StringID_CustomAllocator(const Char *pName = SE_TEXT("SE")) {}
        StringID_CustomAllocator(const StringID_CustomAllocator &x) {}
        StringID_CustomAllocator(const StringID_CustomAllocator &x, const Char *pName) {}
        StringID_CustomAllocator &operator=(const StringID_CustomAllocator &x) { return *this; }
        const Char *get_name() const { return SE_TEXT("StringID"); }
        void set_name(const char *pName) {}

        void *allocate(size_t n, int flags = 0)
        {
            return allocate(n, 16, 0, flags);
        }

        void *allocate(size_t n, size_t alignment, size_t offset, int flags = 0)
        {
            size_t adjustedAlignment = (alignment > 16) ? alignment : 16;

            void *p = NewArray<char>(n + adjustedAlignment + 16); // new char[n + adjustedAlignment + ENGINE_PLATFORM_PTR_SIZE];
            void *pPlusPointerSize = (void *)((uintptr_t)p + 16);
            void *pAligned = (void *)(((uintptr_t)pPlusPointerSize + adjustedAlignment - 1) & ~(adjustedAlignment - 1));

            void **pStoredPtr = (void **)pAligned - 1;
            ENGINE_ASSERT(pStoredPtr >= p);
            *(pStoredPtr) = p;

            ENGINE_ASSERT(((size_t)pAligned & ~(alignment - 1)) == (size_t)pAligned);
            return pAligned;
        }

        void deallocate(void *p, size_t n)
        {
            if (p != nullptr)
            {
				Char* pOriginalAllocation = (Char*)(*((void **)p - 1));
                // DeleteArray<Char>(pOriginalAllocation);
            }
        }
    };

    inline bool operator==(const StringID_CustomAllocator &, const StringID_CustomAllocator &) { return true; }
    inline bool operator!=(const StringID_CustomAllocator &, const StringID_CustomAllocator &) { return false; }

    //-------------------------------------------------------------------------
	Dictionary<uint32, String> g_stringCache;
	CriticalSection g_stringCacheMutex;

    // TODO StringID Debug 信息
    // Natvis/Debugger info to print out human-readable strings
    // StringID::DebuggerInfo g_debuggerInfo;
    // SE::StringID::DebuggerInfo const *StringID::s_pDebuggerInfo = &g_debuggerInfo;

    //-------------------------------------------------------------------------

    StringID StringID::Invalid = StringID(0u);

    StringID::StringID(Char const *pStr)
    {
        if (pStr != nullptr && StringUtils::Length(pStr) > 0)
        {
            m_ID = GetHash(pStr);

            Threading::ScopeLock lock(g_stringCacheMutex);
            auto iter = g_stringCache.Find(m_ID);
            
            if (iter == g_stringCache.end())
            {
                g_stringCache[m_ID] = String(pStr);
            }
        }
    }

    StringID::StringID(String const &str) : StringID(str.Get())
    {
    }

	StringView StringID::ToString() const
    {
        if (m_ID == 0)
        {
            return String::Empty;
        }

        {
            // Get cached string
            Threading::ScopeLock lock(g_stringCacheMutex);
            auto iter = g_stringCache.Find(m_ID);
            if (iter != g_stringCache.end())
            {
                return iter->Value;
            }
        }

        // ID likely directly created via uint32_t
        return String::Empty;
    }

    const Char* GetStringIDToString(StringID id)
    {
        {
            // Get cached string
            Threading::ScopeLock lock(g_stringCacheMutex);
            auto iter = g_stringCache.Find(id);
            if (iter != g_stringCache.end())
            {
                return iter->Value.Get();
            }
        }
        return String::Empty.Get();
    }
}

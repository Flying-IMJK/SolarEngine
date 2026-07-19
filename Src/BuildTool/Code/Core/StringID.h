#pragma once

#include "String.h"

namespace SE::BuildTool
{
    class StringID
    {
    public:
        static const StringID Invalid;

        StringID() = default;
        explicit StringID(uint64 value) : m_value(value) {}
        StringID(std::string_view text) : m_value(Hash(text)), m_cache(text.data(), text.size()) {}
        StringID(const char* text) : StringID(std::string_view(text ? text : "")) {}

        bool operator==(const StringID& rhs) const { return m_value == rhs.m_value; }
        bool operator!=(const StringID& rhs) const { return m_value != rhs.m_value; }
        bool operator==(uint32 rhs) const { return (uint32)m_value == rhs; }
        bool operator!=(uint32 rhs) const { return !(*this == rhs); }
        bool operator==(uint64 rhs) const { return m_value == rhs; }
        bool operator!=(uint64 rhs) const { return !(*this == rhs); }
        operator uint64() const { return m_value; }
        uint32 ToUint() const { return (uint32)m_value; }

        std::string ToString() const { return m_cache; }
        const char* Get() const
        {
            return m_cache.c_str();
        }

    private:
        static uint64 Hash(std::string_view text)
        {
            return std::hash<std::string_view>()(text);
        }

        uint64 m_value = 0;
        std::string m_cache;
    };

    inline const StringID StringID::Invalid = StringID(0ull);
    using TypeID = StringID;
}

namespace std
{
    template<>
    struct hash<SE::BuildTool::StringID>
    {
        size_t operator()(const SE::BuildTool::StringID& value) const noexcept
        {
            return std::hash<uint64_t>()((uint64_t)value);
        }
    };
}

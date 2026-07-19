#pragma once

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdarg>
#include <cstdint>
#include <functional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#define ENGINE_ASSERT(x) assert(x)
#define ENGINE_UNREACHABLE_CODE() assert(false)
#define LOG_ERROR(category, message) ((void)0)

namespace SE::BuildTool
{
    using Char = char;
    using uint8 = uint8_t;
    using int32 = int32_t;
    using uint32 = uint32_t;
    using int64 = int64_t;
    using uint64 = uint64_t;

    constexpr int INVALID_INDEX = -1;

    template<typename Enum>
    class EnumFlags
    {
    public:
        void Set(uint32 value) { m_value = value; }
        void SetFlag(Enum flag) { m_value |= (uint32)flag; }
        void Flag(Enum flag, bool enabled)
        {
            if (enabled)
                m_value |= (uint32)flag;
            else
                m_value &= ~(uint32)flag;
        }
        bool IsFlag(Enum flag) const { return (m_value & (uint32)flag) != 0; }
        uint32 Get() const { return m_value; }
        operator uint32() const { return m_value; }

    private:
        uint32 m_value = 0;
    };

    template<typename T>
    using Function = std::function<T>;


}

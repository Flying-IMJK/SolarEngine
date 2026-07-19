#pragma once

#include <deque>

namespace SE
{
    template<typename T, class alloc = std::allocator<T>>
    using Queue = std::deque<T, alloc>;
}
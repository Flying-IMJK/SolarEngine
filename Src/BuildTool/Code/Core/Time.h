#pragma once

#include <chrono>

namespace SE::BuildTool
{
    class Milliseconds
    {
    public:
        Milliseconds() = default;
        Milliseconds(double value) : m_value(value) {}

        Milliseconds& operator+=(double value)
        {
            m_value += value;
            return *this;
        }

        double ToSeconds() const { return m_value; }
        operator double() const { return m_value; }

    private:
        double m_value = 0.0;
    };

    class PlatformClock
    {
    public:
        using clock = std::chrono::steady_clock;
        using time_point = clock::time_point;

        static time_point Now()
        {
            return clock::now();
        }
    };

    template<typename Clock>
    class ScopedTimer
    {
    public:
        explicit ScopedTimer(Milliseconds& elapsed)
            : m_elapsed(elapsed)
            , m_start(Clock::Now())
        {}

        ~ScopedTimer()
        {
            const auto end = Clock::Now();
            const auto delta = std::chrono::duration<double, std::milli>(end - m_start).count();
            m_elapsed += delta;
        }

    private:
        Milliseconds& m_elapsed;
        typename Clock::time_point m_start;
    };
}

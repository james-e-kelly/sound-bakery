#pragma once

#include "sound_bakery/pch.h"

namespace sbk::core
{
    template <typename T>
    class SB_CLASS property
    {
        static_assert(std::is_arithmetic<T>::value);

    public:
        using property_changed_delegate = MulticastDelegate<T, T>;

        property() : m_value(T()), m_min(0), m_max(1) {}
        property(T value) : m_value(value), m_min(value), m_max(value + 1) {}
        property(T value, T min, T max) : m_value(value), m_min(min), m_max(max)
        {
            assert(value >= min);
            assert(value <= max);
            assert(min < max);
        }

        property(const property& other)
            : m_value(other.m_value), m_min(other.m_min), m_max(other.m_max), m_delegate(other.m_delegate)
        {
        }

        property(property&& other) = default;
        ~property() = default;

        property& operator=(const property& other)
        {
            if (this != &other)
            {
                m_value = other.m_value;
                m_min      = other.m_min;
                m_max      = other.m_max;
                m_delegate = other.m_delegate;
            }

            return *this;
        }

        property& operator=(property&& other) = default;

        auto set(T value) -> bool
        {
            if (value != m_value)
            {
                if (value >= m_min && value <= m_max)
                {
                    m_delegate.Broadcast(m_value, value);
                    m_value = value;
                    return true;
                }
            }
            return false;
        }

        [[nodiscard]] auto get() const -> T { return m_value; }
        [[nodiscard]] auto get_min() const -> T { return m_min; }
        [[nodiscard]] auto get_max() const -> T{ return m_max; }
        [[nodiscard]] auto get_min_max_pair() const -> std::pair<T, T> { return std::pair<T, T>(m_min, m_max); }
        [[nodiscard]] auto get_delegate() -> property_changed_delegate& { return m_delegate; }

    private:
        T m_value;
        T m_min;
        T m_max;
        property_changed_delegate m_delegate;
    };

    using int_property   = property<int32_t>;
    using id_property    = property<sbk_id>;
    using float_property = property<float>;
}  // namespace sbk::core
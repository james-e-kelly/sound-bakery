#pragma once

#include "sound_bakery/pch.h"

#include "sound_bakery/core/core_fwd.h"

namespace sbk::core
{
    template <arithmetic T>
    class SB_CLASS property
    {
    public:
        using property_changed_delegate = MulticastDelegate<T, T>;

        property() : m_value(T()), m_min(0), m_max(1) {}
        property(T value) : m_value(value), m_min(value), m_max(value + 1) {}
        property(T value, T min, T max) : m_value(value), m_min(min), m_max(max)
        {
            BOOST_ASSERT(value >= min);
            BOOST_ASSERT(value <= max);
            BOOST_ASSERT(min < max);
        }

        property(const property& other)
            : m_value(other.m_value), m_min(other.m_min), m_max(other.m_max), m_delegate(other.m_delegate)
        {
        }

        property(property&& other) = default;
        ~property()                = default;

        auto operator=(const property& other) -> property&
        {
            if (this != &other)
            {
                m_value    = other.m_value;
                m_min      = other.m_min;
                m_max      = other.m_max;
                m_delegate = other.m_delegate;
            }

            return *this;
        }

        auto operator=(property&& other) -> property& = default;

        auto set(T value) -> bool
        {
            if (value != m_value)
            {
                if (value >= m_min && value <= m_max)
                {
                    const T oldValue = m_value;
                    m_value = value;
                    m_delegate.Broadcast(oldValue, value);
                    return true;
                }
            }
            return false;
        }

        /**
         * @brief Set the min value and clamp the property's value to fit.
         * @tody Add the ability for the user to choose between clamping and scaling the value
         */
        auto set_min(T value) -> void
        {
            m_min          = value;
            T clampedValue = std::clamp(m_value, m_min, m_max);
            set(clampedValue);
        }

        /**
         * @brief Set the min value and clamp the property's value to fit.
         * @tody Add the ability for the user to choose between clamping and scaling the value
         */
        auto set_max(T value) -> void
        {
            m_max          = value;
            T clampedValue = std::clamp(m_value, m_min, m_max);
            set(clampedValue);
        }

        [[nodiscard]] auto get() const -> T { return m_value; }
        [[nodiscard]] auto get_min() const -> T { return m_min; }
        [[nodiscard]] auto get_max() const -> T { return m_max; }
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
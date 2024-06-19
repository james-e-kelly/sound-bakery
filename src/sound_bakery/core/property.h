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

        property() : m_property(T()), m_min(0), m_max(1) {}

        property(T value) : m_property(value), m_min(value), m_max(value + 1) {}

        property(T value, T min, T max) : m_property(value), m_min(min), m_max(max)
        {
            assert(value >= min);
            assert(value <= max);
            assert(min < max);
        }

        /**
         * @brief Copy constructor.
         */
        property(const property& other)
            : m_property(other.m_property), m_min(other.m_min), m_max(other.m_max), m_delegate(other.m_delegate)
        {
        }

        /**
         * @brief Move constructor.
         * @param other
         */
        property(property&& other) = default;

        ~property() = default;

        /**
         * @brief Assignment operator.
         *
         * Copys all values from other to this Property, including the delegate.
         * @param other Property to copy from.
         * @return
         */
        property& operator=(const property& other)
        {
            if (this != &other)
            {
                m_property = other.m_property;
                m_min      = other.m_min;
                m_max      = other.m_max;
                m_delegate = other.m_delegate;
            }

            return *this;
        }

        property& operator=(property&& other) = default;

        void set(T value)
        {
            if (value != m_property)
            {
                if (value >= m_min && value <= m_max)
                {
                    m_delegate.Broadcast(m_property, value);
                    m_property = value;
                }
            }
        }

        [[nodiscard]] T get() const { return m_property; }
        property_changed_delegate& get_delegate() { return m_delegate; }
        [[nodiscard]] T get_min() const { return m_min; }
        [[nodiscard]] T get_max() const { return m_max; }
        [[nodiscard]] std::pair<T, T> get_min_max_pair() const { return std::pair<T, T>(m_min, m_max); }

    private:
        T m_property;
        T m_min;
        T m_max;
        property_changed_delegate m_delegate;
    };

    using int_property   = property<int32_t>;
    using id_property    = property<sbk_id>;
    using float_property = property<float>;
}  // namespace sbk::core
#pragma once

#include "Delegates.h"
#include "sound_bakery/sound_bakery_internal.h"

namespace sbk::core
{
    template <typename T>
    class SB_CLASS Property
    {
        static_assert(std::is_arithmetic<T>::value);

    public:
        using PropertyChangedDelegate = MulticastDelegate<T, T>;

    public:
        Property() : m_property(T()), m_min(0), m_max(1) {}

        Property(T value) : m_property(value), m_min(value), m_max(value + 1) {}

        Property(T value, T min, T max) : m_property(value), m_min(min), m_max(max)
        {
            assert(value >= min);
            assert(value <= max);
            assert(min < max);
        }

        /**
         * @brief Copy constructor.
         */
        Property(const Property& other)
            : m_property(other.m_property), m_min(other.m_min), m_max(other.m_max), m_delegate(other.m_delegate)
        {
        }

        /**
         * @brief Move constructor.
         * @param other
         */
        Property(Property&& other) = default;

    public:
        /**
         * @brief Assignment operator.
         *
         * Copys all values from other to this Property, including the delegate.
         * @param other Property to copy from.
         * @return
         */
        Property& operator=(const Property& other)
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

        Property& operator=(Property&& other) = default;

    public:
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

        PropertyChangedDelegate& getDelegate() { return m_delegate; }

        [[nodiscard]] T getMin() const { return m_min; }

        [[nodiscard]] T getMax() const { return m_max; }

        [[nodiscard]] std::pair<T, T> getMinMaxPair() const { return std::pair<T, T>(m_min, m_max); }

    private:
        T m_property;
        T m_min;
        T m_max;
        PropertyChangedDelegate m_delegate;
    };

    using IntProperty   = Property<int32_t>;
    using IdProperty    = Property<sbk_id>;
    using FloatProperty = Property<float>;
}  // namespace SB::Core
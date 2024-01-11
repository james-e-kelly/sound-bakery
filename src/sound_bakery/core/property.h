#pragma once

#include "Delegates.h"

namespace SB::Core
{
    template<typename T>
    class Property
    {
        static_assert(std::is_arithmetic<T>::value);

    public:
        using PropertyChangedDelegate = MulticastDelegate<T, T>;

    public:
        Property()
            : m_property(T()), m_min(0), m_max(1)
        {}

        Property(T value)
            : m_property(value), m_min(value), m_max(value + 1)
        {}

        Property(T value, T min, T max)
            : m_property(value), m_min(min), m_max(max)
        {
            assert(value >= min);
            assert(value <= max);
            assert(min < max);
        }

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

        [[nodiscard]] T get() const
        {
            return m_property;
        }

        PropertyChangedDelegate& getDelegate()
        {
            return m_delegate;
        }

        [[nodiscard]] T getMin() const
        {
            return m_min;
        }

        [[nodiscard]] T getMax() const
        {
            return m_max;
        }

        [[nodiscard]] std::pair<T,T> getMinMaxPair() const
        {
            return std::pair<T, T>(m_min, m_max);
        }

    protected:
        T m_property;
        T m_min;
        T m_max;
        PropertyChangedDelegate m_delegate;
    };

    using IntProperty = Property<int32_t>;
    using IdProperty = Property<SB_ID>;
    using FloatProperty = Property<float>;
}
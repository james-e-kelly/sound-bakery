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
            : m_property(T())
        {}

        Property(T value)
            : m_property(value)
        {}

    public:
        void set(T value)
        {
            if (value != m_property)
            {
                m_delegate.Broadcast(m_property, value);
                m_property = value;
            }
        }

        T get() const
        {
            return m_property;
        }

        PropertyChangedDelegate& getDelegate()
        {
            return m_delegate;
        }

    protected:
        T m_property;
        PropertyChangedDelegate m_delegate;
    };

    using IntProperty = Property<int32_t>;
    using FloatProperty = Property<float>;
}
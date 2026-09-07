#pragma once

#include "sound_bakery/pch.h"

#include "sound_bakery/core/core_fwd.h"

namespace sbk::engine
{
    class modulator;
}

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

        property(const property& other) = default;
        property(property&& other) noexcept = default;
        ~property() = default;
        property& operator=(property&& other) noexcept = default;

        auto operator=(const property& other) -> property&
        {
            if (this != &other)
            {
                m_min = other.m_min;
                m_max = other.m_max;
                set(other.m_value);

                if constexpr (std::is_floating_point_v<T>)
                {
                    m_random = other.m_random;
                }
            }

            return *this;
        }

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
         * @todo Add the ability for the user to choose between clamping and scaling the value
         */
        auto set_min(T value) -> void
        {
            m_min          = value;
            T clampedValue = std::clamp(m_value, m_min, m_max);
            set(clampedValue);
        }

        /**
         * @brief Set the min value and clamp the property's value to fit.
         * @todo Add the ability for the user to choose between clamping and scaling the value
         */
        auto set_max(T value) -> void
        {
            m_max          = value;
            T clampedValue = std::clamp(m_value, m_min, m_max);
            set(clampedValue);
        }

        auto set_random_min_offset(float min) -> void
            requires std::floating_point<T>
        {
            m_random.m_randomMinOffset = min;

        }

        auto set_random_max_offset(float max) -> void
            requires std::floating_point<T>
        {
            m_random.m_randomMaxOffset = max;
        }

        auto set_random_enabled(bool enabled) -> void
            requires std::floating_point<T>
        {
            m_random.m_enableRandomOffsets = enabled;
        }

        /**
         * @brief Get the random offset of this property, if randomness is enabled.
         * 
         * Because callers might want randomness on every call, every frame, or every session,
         * the property class does not have a "get_with_random" function that gives a new random value every call.
         * Instead, callers can use code like the example below to get the random value from the offset:
         * @code
         *  std::mt19937 rng;
         *  float_property prop;
         *  float value = prop.get() + prop.get_random_offset(rng);
         * @endcode
         * 
         * The user is then free to store the offset. 
         * The @ref voice class does this have clean playback when properties modulate.
         * 
         * Like Wwise, Sound Bakery allows users to enable and disable randomness.
         * A min or max value of 0 does not mean "no randomness". It's just the default values.
         * This is mainly important for serialization. At runtime, zero offsets results in no randomness.
         * 
         * @param rng   Random number generator passed by the caller. The property itself does not generate randomness. This lets randomness to be generated per-system, per-gameobject, or per-voice.
         */
        [[nodiscard]] auto get_random_offset(std::mt19937& rng) const -> float
            requires std::floating_point<T>
        {
            if (m_random.m_enableRandomOffsets)
            {
                std::uniform_real_distribution<float> dist(m_random.m_randomMinOffset, m_random.m_randomMaxOffset);
                return dist(rng);
            }
            return {};
        }

        [[nodiscard]] auto get_random_enabled() const -> bool
            requires std::floating_point<T>
        {
            return m_random.m_enableRandomOffsets;
        }

        [[nodiscard]] auto get_random_min_offset() const -> float
            requires std::floating_point<T>
        {
            return m_random.m_randomMinOffset;
        }

        [[nodiscard]] auto get_random_max_offset() const -> float
            requires std::floating_point<T>
        {
            return m_random.m_randomMaxOffset;
        }

        /**
         * @brief Get the raw value of the property with no randomness.
         */
        [[nodiscard]] auto get() const -> T  { return m_value; }
        [[nodiscard]] auto get_min() const -> T { return m_min; }
        [[nodiscard]] auto get_max() const -> T { return m_max; }
        [[nodiscard]] auto get_min_max_pair() const -> std::pair<T, T> { return std::pair<T, T>(m_min, m_max); }
        [[nodiscard]] auto get_delegate() -> property_changed_delegate& { return m_delegate; }

    private:
        T m_value;
        T m_min;
        T m_max;
        property_changed_delegate m_delegate;

        struct empty {};

        struct random_data
        {
            bool    m_enableRandomOffsets{};
            float   m_randomMinOffset{};
            float   m_randomMaxOffset{};
        };

        [[no_unique_address]] std::conditional_t<std::floating_point<T>, random_data, empty> m_random{};
    };

    using float_property = property<float>;
    using int_property   = property<int32_t>;
    using id_property    = property<sbk_id>;
}  // namespace sbk::core
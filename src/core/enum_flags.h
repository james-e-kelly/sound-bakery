#pragma once

#include <type_traits>

#define BIT(x) (1u << (x))

/**
 * @brief Defines |, &, ^, ~, |=, &=, ^= for a scoped enum so it can be used as a set of flags.
 *
 * Invoke this in the same namespace as the enum, immediately after its definition, e.g.
 *
 *     enum class object_flags : std::uint32_t
 *     {
 *         none    = BIT(0),
 *         loading = BIT(1)
 *     };
 *     DEFINE_ENUM_FLAG_OPERATORS(object_flags)
 */
#define DEFINE_ENUM_FLAG_OPERATORS(EnumType)                                                              \
    constexpr EnumType operator|(EnumType lhs, EnumType rhs)                                              \
    {                                                                                                      \
        return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(lhs) |                 \
                                      static_cast<std::underlying_type_t<EnumType>>(rhs));                 \
    }                                                                                                      \
    constexpr EnumType operator&(EnumType lhs, EnumType rhs)                                              \
    {                                                                                                      \
        return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(lhs) &                 \
                                      static_cast<std::underlying_type_t<EnumType>>(rhs));                 \
    }                                                                                                      \
    constexpr EnumType operator^(EnumType lhs, EnumType rhs)                                              \
    {                                                                                                      \
        return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(lhs) ^                 \
                                      static_cast<std::underlying_type_t<EnumType>>(rhs));                 \
    }                                                                                                      \
    constexpr EnumType operator~(EnumType flags)                                                           \
    {                                                                                                      \
        return static_cast<EnumType>(~static_cast<std::underlying_type_t<EnumType>>(flags));              \
    }                                                                                                      \
    constexpr EnumType& operator|=(EnumType& lhs, EnumType rhs)                                            \
    {                                                                                                      \
        return lhs = lhs | rhs;                                                                            \
    }                                                                                                      \
    constexpr EnumType& operator&=(EnumType& lhs, EnumType rhs)                                            \
    {                                                                                                      \
        return lhs = lhs & rhs;                                                                            \
    }                                                                                                      \
    constexpr EnumType& operator^=(EnumType& lhs, EnumType rhs)                                            \
    {                                                                                                      \
        return lhs = lhs ^ rhs;                                                                            \
    }

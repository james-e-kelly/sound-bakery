#pragma once

#include "sound_bakery/reflection/reflection.h" //< Must be included here and not individual files. @todo Investigate rttr strangeness when included in multiple files

#include "Delegates.h"
#include "concurrencpp/concurrencpp.h"
#include "sound_bakery_internal.h"
#include "spdlog/async.h"
#include "spdlog/fmt/fmt.h"
#include "spdlog/spdlog.h"
#include "tl/expected.hpp"
#include "tracy/Tracy.hpp"
#include "tracy/TracyC.h"

#define BOOST_SPIRIT_DEBUG

#include <atomic>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/assert.hpp>
#include <boost/serialization/utility.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <new>
#include <optional>
#include <random>
#include <set>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <source_location>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

/**
 * @def Registers this type's get_parent classes (if any) and marks its private members visible to reflection.
 */
#define REGISTER_REFLECTION(T, ...)      \
                                         \
public:                                  \
    [[nodiscard]] static auto type() -> rttr::type; \
    RTTR_ENABLE(__VA_ARGS__)             \
    RTTR_REGISTRATION_FRIEND             \
    friend auto sbk::reflection::register_reflection_types() -> void;

/**
 * @def Defines the static function so it is compiled into the SoundBakery library and not the consuming application.
 */
#define DEFINE_REFLECTION(T) \
    auto T::type() -> rttr::type { return rttr::type::get<T>(); }

// Compiled out entirely when logging is disabled (SBK_CONFIG_ENABLE_LOGGING comes from
// cmake/setup_build_config.cmake: on in Debug/Profile, off in Release by default).
// Error-path logging via sbk::log_error/make_error is not gated - failures stay
// visible in every configuration.
// Each macro takes a fmt-style format string plus its arguments, matching the error
// macros in error/result.h, e.g. SBK_INFO("Could not find {}", objectID). The format
// string is a compile-time literal, so fmt checks the placeholders against the argument
// types at compile time. To log a runtime string that may itself contain braces, pass it
// as an argument rather than as the format string: SBK_INFO("{}", runtimeMessage).
//
// fmt::format returns a temporary std::string; its c_str() stays valid for the full
// expression, which is all sbk_log needs (it logs synchronously before returning).
#if SBK_CONFIG_ENABLE_LOGGING
    #define SBK_DEBUG(...) sbk_log(MA_LOG_LEVEL_DEBUG, ::fmt::format(__VA_ARGS__).c_str())
    #define SBK_INFO(...)  sbk_log(MA_LOG_LEVEL_INFO, ::fmt::format(__VA_ARGS__).c_str())
    #define SBK_WARN(...)  sbk_log(MA_LOG_LEVEL_WARNING, ::fmt::format(__VA_ARGS__).c_str())
    #define SBK_ERROR(...) sbk_log(MA_LOG_LEVEL_ERROR, ::fmt::format(__VA_ARGS__).c_str())
#else
    #define SBK_DEBUG(...) ((void)0)
    #define SBK_INFO(...)  ((void)0)
    #define SBK_WARN(...)  ((void)0)
    #define SBK_ERROR(...) ((void)0)
#endif
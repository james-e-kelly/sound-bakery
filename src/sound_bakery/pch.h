#pragma once

#include "Delegates.h"
#include "concurrencpp/concurrencpp.h"
#include "sound_bakery/core/core_fwd.h"
#include "sound_bakery/reflection/reflection.h"
#include "sound_bakery_internal.h"
#include "spdlog/async.h"
#include "spdlog/spdlog.h"
#include "tracy/Tracy.hpp"
#include "tracy/TracyC.h"
#include "ztd/out_ptr.hpp"

#define BOOST_SPIRIT_DEBUG

#include <atomic>
#include <boost/assert.hpp>
#include <boost/serialization/utility.hpp>
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
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "yaml-cpp/yaml.h"

/**
 * @def Registers this type's get_parent classes (if any) and marks its private members visible to reflection.
 */
#define REGISTER_REFLECTION(T, ...) \
                                    \
public:                             \
    static rttr::type type();       \
    RTTR_ENABLE(__VA_ARGS__)        \
    RTTR_REGISTRATION_FRIEND        \
    friend void sbk::reflection::registerReflectionTypes();

/**
 * @def Defines the static function so it is compiled into the SoundBakery library and not the consuming application.
 */
#define DEFINE_REFLECTION(T) \
    rttr::type T::type() { return rttr::type::get<T>(); }

#define SBK_INFO(message)                               \
    if (spdlog::default_logger_raw())                   \
    {                                                   \
        SPDLOG_INFO(message);                           \
    }                                                   \
    TracyMessageC(message, sizeof(message), 0xffffff);

#define SBK_WARN(message)                               \
    if (spdlog::default_logger_raw()) \
    {                                 \
        SPDLOG_WARN(message);         \
    }                                                   \
    TracyMessageC(message, sizeof(message), 0xff4500);

#define SBK_ERROR(message)                              \
    if (spdlog::default_logger_raw()) \
    {                                 \
        SPDLOG_ERROR(message);         \
    }                                                   \
    TracyMessageC(message, sizeof(message), 0xff0000);

#define SBK_CRITICAL(message)                           \
    if (spdlog::default_logger_raw()) \
    {                                 \
        SPDLOG_CRITICAL(message);         \
    }                                                   \
    TracyMessageC(message, sizeof(message), 0x8b0000);
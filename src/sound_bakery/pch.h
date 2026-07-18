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
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/range/algorithm.hpp>
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

#define SBK_INFO(message) sbk_log(MA_LOG_LEVEL_INFO, message);              
#define SBK_WARN(message) sbk_log(MA_LOG_LEVEL_WARNING, message);              
#define SBK_ERROR(message) sbk_log(MA_LOG_LEVEL_ERROR, message);             
#define SBK_CRITICAL(message) sbk_log(MA_LOG_LEVEL_ERROR, message);               
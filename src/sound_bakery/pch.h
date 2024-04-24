#pragma once

#include "sound_bakery/core/core_fwd.h"
#include "sound_bakery/reflection/reflection.h"
#include "sound_bakery_internal.h"

#include <atomic>
#include <cassert>
#include <iostream>
#include <memory>
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
 * @def Registers this type's parent classes (if any) and marks its private members visible to reflection.
 */
#define REGISTER_REFLECTION(T, ...) \
                                    \
public:                             \
    static rttr::type type();       \
    RTTR_ENABLE(__VA_ARGS__)        \
    RTTR_REGISTRATION_FRIEND        \
    friend void SB::Reflection::registerReflectionTypes();

/**
 * @def Defines the static function so it is compiled into the SoundBakery library and not the consuming application.
 */
#define DEFINE_REFLECTION(T) \
    rttr::type T::type() { return rttr::type::get<T>(); }
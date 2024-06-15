#pragma once

#include "sound_bakery/core/core_fwd.h"

#include <rttr/registration_friend>
#include <rttr/type>

namespace sbk::reflection
{
    template <typename Target_Type, typename Source_Type>
    Target_Type cast(Source_Type object)
    {
        static_assert(rttr::detail::pointer_count<Target_Type>::value == 1, "Target type must be a pointer");
        static_assert(rttr::detail::pointer_count<Source_Type>::value == 1, "Source argument must be a pointer");
        static_assert(rttr::detail::has_get_type_func<Source_Type>::value,
                      "Class has not type defined - please use the macro RTTR_ENABLE().");

        using Return_Type = rttr::detail::remove_pointer_t<Target_Type>;
        using Arg_Type    = rttr::detail::remove_pointer_t<Source_Type>;

        static_assert((std::is_volatile<Arg_Type>::value && std::is_volatile<Return_Type>::value) ||
                          (!std::is_volatile<Arg_Type>::value && std::is_volatile<Return_Type>::value) ||
                          (!std::is_volatile<Arg_Type>::value && !std::is_volatile<Return_Type>::value),
                      "Return type must have volatile qualifier");

        static_assert((std::is_const<Arg_Type>::value && std::is_const<Return_Type>::value) ||
                          (!std::is_const<Arg_Type>::value && std::is_const<Return_Type>::value) ||
                          (!std::is_const<Arg_Type>::value && !std::is_const<Return_Type>::value),
                      "Return type must have const qualifier");

        using source_type_no_cv = typename std::remove_cv<typename std::remove_pointer<Source_Type>::type>::type;
        return static_cast<Target_Type>(rttr::type::apply_offset(const_cast<source_type_no_cv*>(object)->get_ptr(),
                                                                 const_cast<source_type_no_cv*>(object)->get_type(),
                                                                 Arg_Type::type()));
    }

    void registerReflectionTypes();
    void unregisterReflectionTypes();
}  // namespace SB::Reflection
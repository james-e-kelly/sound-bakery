#pragma once

#include <type_traits>

namespace sbk
{
    namespace core
    {
        class object;
        class database_object;

        template <typename T>
        class database_ptr;

        /**
         * @brief Restricts a template parameter to arithmetic types (integral or floating-point).
         */
        template <typename T>
        concept arithmetic = std::is_arithmetic_v<T>;

        template <arithmetic T>
        class property;
    }  // namespace core
}  // namespace sbk
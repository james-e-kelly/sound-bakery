#pragma once

#include "gluten/data/data_source.h"

namespace gluten
{
    /**
     * @brief Inherit this class to add helper functions on top of the data sources.
     */
    template <typename T>
    struct data_controller
    {
        auto get_data() -> T*
        {
            return m_data.get_data();
        }

        auto get_const_data() const -> const T*
        {
            return m_data.get_data();
        }

        auto operator->() -> T*
        {
            return get_data();
        }

        auto operator->() const -> const T*
        {
            return get_const_data();
        }

    private:
        data_source<T> m_data;
    };
}  // namespace gluten
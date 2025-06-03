#pragma once

#include "gluten/data/data_source.h"

namespace gluten
{
    /**
     * @brief Inherit this class to add helper functions on top of the data sources.
     */
    template<typename T>
    class data_controller
    {
    public:
        auto get_data() -> T*
        {
            return m_data.get_data();
        }

    private:
        data_source<T> m_data;
    };
}
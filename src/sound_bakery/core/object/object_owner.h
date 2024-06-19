#pragma once

#include "sound_bakery/core/database/database.h"
#include "sound_bakery/core/object/object_tracker.h"

namespace sbk::core
{
    /**
     * @brief Creates, owns and tracks objects.
     *
     * This is the central location for object creation.
     */
    class SB_CLASS object_owner : protected database, protected object_tracker
    {
    public:
        using database::get_all;
        using database::try_find;
        using object_tracker::get_objects_of_category;
        using object_tracker::get_objects_of_type;

        std::shared_ptr<object> create_runtime_object(const rttr::type& type);
        std::shared_ptr<database_object> create_database_object(const rttr::type& type);

        template <typename T>
        std::shared_ptr<T> create_runtime_object();

        template <typename T>
        std::shared_ptr<T> create_database_object();

        void destroy_all() { m_objects.clear(); }

    private:
        std::vector<std::shared_ptr<object>> m_objects;
    };

    template <typename T>
    std::shared_ptr<T> object_owner::create_runtime_object()
    {
        if (!rttr::type::get<T>().is_derived_from(rttr::type::get<sbk::core::object>()))
        {
            SPDLOG_ERROR("Cannot create object. Objects must derive from the base object type");
            return {};
        }

        if (std::shared_ptr<object> object = create_runtime_object(rttr::type::get<T>()))
        {
            if (std::shared_ptr<T> castedObject = std::static_pointer_cast<T>(object))
            {
                return castedObject;
            }
        }

        return {};
    }

    template <typename T>
    std::shared_ptr<T> object_owner::create_database_object()
    {
        if (!rttr::type::get<T>().is_derived_from(rttr::type::get<sbk::core::database_object>()))
        {
            SPDLOG_ERROR("Cannot create object. Database objects must derive from the base database object type");
            return {};
        }

        if (std::shared_ptr<database_object> object = create_database_object(rttr::type::get<T>()))
        {
            if (std::shared_ptr<T> castedObject = std::static_pointer_cast<T>(object))
            {
                return castedObject;
            }
        }

        return {};
    }
}  // namespace sbk::core

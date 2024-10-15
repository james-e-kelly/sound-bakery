#pragma once

#include "sound_bakery/pch.h"

namespace sbk::core
{
    /**
     * @brief Creates, owns and tracks objects.
     *
     * This is the central location for object creation.
     */
    class SB_CLASS object_owner
    {
    public:
        std::shared_ptr<object> create_runtime_object(const rttr::type& type);
        std::shared_ptr<object> load_object(YAML::Node& node);

        /**
         * @param addToDatabase to automatically track the object. If set to false, the user is responsible for adding
         * the object to the database.
         */
        std::shared_ptr<database_object> create_database_object(const rttr::type& type, bool addToDatabase = true);

        template <typename T>
        std::shared_ptr<T> create_runtime_object();

        template <typename T>
        std::shared_ptr<T> create_database_object();

        void remove_object(const std::shared_ptr<object>& object);
        void destroy_all() { m_objects.clear(); }

        std::vector<std::shared_ptr<object>>& get_objects() { return m_objects; }
        const std::vector<std::shared_ptr<object>>& get_objects() const { return m_objects; }

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

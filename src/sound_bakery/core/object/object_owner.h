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
        [[nodiscard]] auto create_runtime_object(const rttr::type& type) -> std::shared_ptr<object>;
        [[nodiscard]] auto load_object(YAML::Node& node) -> std::shared_ptr<object>;

        /**
         * @param addToDatabase to automatically track the object. If set to false, the user is responsible for adding
         * the object to the database.
         */
        [[nodiscard]] auto create_database_object(const rttr::type& type,
                                                  bool addToDatabase = true) -> std::shared_ptr<database_object>;

        template <typename T>
        [[nodiscard]] auto create_runtime_object() -> std::shared_ptr<T>;

        template <typename T>
        [[nodiscard]] auto create_database_object() -> std::shared_ptr<T>;

        auto remove_object(const std::shared_ptr<object>& object) -> void;

        auto destroy_all() -> void;

        [[nodiscard]] auto get_objects() -> std::vector<std::shared_ptr<object>>&;
        [[nodiscard]] auto get_objects() const -> const std::vector<std::shared_ptr<object>>&;

    private:
        std::vector<std::shared_ptr<object>> m_objects;
    };

#include "object_owner.inl"
}  // namespace sbk::core

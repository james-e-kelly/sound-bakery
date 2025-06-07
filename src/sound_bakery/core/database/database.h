#pragma once

#include "sound_bakery/core/database/database_object.h"

namespace sbk::core
{
    /**
     * @brief Runtime lookup of objects, using their ID or name.
     *
     * The database holds weak pointers to objects.
     *
     * Another object should own the shared_ptr objects to the database_object.
     */
    class SB_CLASS database
    {
    public:
        auto add_object_to_database(std::shared_ptr<database_object> object) -> concurrencpp::result<void>;
        auto add_object_to_database(sbk_id id, database_name name) -> concurrencpp::result<void>;
        auto remove_object_from_database(sbk_id objectID) -> concurrencpp::result<void>;

        [[nodiscard]] auto try_find_database_object(sbk_id objectID) const -> concurrencpp::result<std::weak_ptr<database_object>>;
        [[nodiscard]] auto try_find_database_object(database_name name) const -> concurrencpp::result<std::weak_ptr<database_object>>;
        [[nodiscard]] auto get_all_database_objects() const -> concurrencpp::result<std::vector<std::weak_ptr<database_object>>>;
        [[nodiscard]] auto get_all_database_names() const -> concurrencpp::result <std::vector<database_name>>;
        [[nodiscard]] auto get_database_object_count() const -> concurrencpp::result <size_t>;
        [[nodiscard]] auto get_database_object_at(size_t index) const -> concurrencpp::result<std::weak_ptr<database_object>>;

        auto clear_database() -> concurrencpp::result<void>;

    private:
        static auto create_new_id() -> sbk_id;
        static auto create_new_name(const rttr::type& type) -> std::string;

        auto update_id(sbk_id oldID, sbk_id newID) -> void;
        auto update_database_name(const database_name& oldName, const database_name& newName) -> void;
        auto on_object_destroyed(object* object) -> void;

        std::unordered_map<sbk_id, std::weak_ptr<database_object>> m_idToPointerMap;
        std::unordered_map<database_name, sbk_id> m_nameToIdMap;
    };
}  // namespace sbk::core
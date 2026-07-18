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
        auto add_object_to_database(const std::shared_ptr<database_object>& object) -> sbk::result<void>;
        auto remove_object_from_database(sbk_id objectID) -> sbk::result<void>;

        /**
         * @brief Adds an entry to the name -> ID lookup. Allows functions to find the ID of an object from its name before the live object exists.
         */
        auto assign_name_to_id(sbk_id id, const database_name& name) -> sbk::result<void>;

        [[nodiscard]] auto try_find_database_object(sbk_id objectID) const -> std::weak_ptr<database_object>;
        [[nodiscard]] auto try_find_database_object(const database_name& name) const -> std::weak_ptr<database_object>;
        [[nodiscard]] auto get_all_database_objects() const -> std::vector<std::weak_ptr<database_object>>;
        [[nodiscard]] auto get_all_database_names() const -> std::vector<database_name>;
        [[nodiscard]] auto get_database_object_count() const -> size_t; //< Returns the number of "live IDs -> Objects" in the database. Ignores "name -> ID" as it's possible they are not real objects yet (soundbank hasn't loaded it yet)
        [[nodiscard]] auto get_database_object_at(size_t index) const -> std::weak_ptr<database_object>;

        auto clear_database() noexcept -> void;

    private:
        [[nodiscard]] static auto create_new_id() -> sbk_id;
        [[nodiscard]] static auto create_new_name(const rttr::type& type) -> std::string;

        auto work_till_name_is_unique(const std::shared_ptr<database_object>& object) -> sbk::result<void>;

        auto update_id(sbk_id oldID, sbk_id newID) -> void;
        auto update_database_name(const database_name& oldName, const database_name& newName) -> void;
        auto on_object_destroyed(object* object) -> void;

        std::unordered_map<sbk_id, std::weak_ptr<database_object>> m_idToPointerMap;
        std::unordered_map<database_name, sbk_id> m_nameToIdMap;
    };
}  // namespace sbk::core
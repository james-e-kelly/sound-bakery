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
        auto add_object_to_database(const std::shared_ptr<database_object>& object) -> void;
        auto add_object_to_database(sbk_id id, const database_name& name) -> void;
        auto remove_object_from_database(sbk_id objectID) -> void;

        [[nodiscard]] auto try_find_database_object(sbk_id objectID) const -> std::weak_ptr<database_object>;
        [[nodiscard]] auto try_find_database_object(const database_name& name) const -> std::weak_ptr<database_object>;
        [[nodiscard]] auto get_all_database_objects() const -> std::vector<std::weak_ptr<database_object>>;
        [[nodiscard]] auto get_all_database_names() const -> std::vector<database_name>;

        void clear_database() noexcept;

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
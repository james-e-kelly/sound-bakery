#pragma once

#include "sound_bakery/core/database/database_object.h"

namespace sbk::core
{
    /**
     * @brief Runtime lookup of objects, using their ID or name.
     *
     * Identity is the ID. The ID -> object map is the single source of truth; the database holds
     * weak pointers, and another object owns the shared_ptr to each database_object.
     *
     * An object's name is a *derived* value - database_object::get_database_name walks the parent
     * graph on demand - so it is never stored per-object here and reparenting costs no bookkeeping.
     * Name -> ID is a decoupled index populated explicitly via assign_name_to_id (soundbanks bake a
     * flat table for the immutable runtime graph). Name lookups check that table first and, for the
     * mutable editor graph, fall back to resolving against the live objects.
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
        [[nodiscard]] auto get_database_object_name(sbk_id objectID) const -> database_name;
        [[nodiscard]] auto get_database_object_count() const -> size_t;  //< Returns the number of "live IDs -> Objects" in the database
        [[nodiscard]] auto get_database_object_at(size_t index) const -> std::weak_ptr<database_object>;

        auto clear_database() noexcept -> void;

    private:
        [[nodiscard]] static auto create_new_id() -> sbk_id;
        [[nodiscard]] static auto create_new_name(const rttr::type& type) -> std::string;

        /**
         * @brief Finds an object using its name for the editor.
         * 
         * During runtime, names and IDs are loaded by banks. For the editor, nodes can be reparented and renamed.
         * Instead of keeping an accurate map of names -> IDs, we just resolve the same with a slow lookup.
         * This reduces the code needed for keeping any map updated.
         */
        [[nodiscard]] auto resolve_name_in_graph(const database_name& name) const -> std::weak_ptr<database_object>;

        auto update_id(sbk_id oldID, sbk_id newID) -> void;

        std::unordered_map<sbk_id, std::weak_ptr<database_object>> m_idToPointerMap;
        std::unordered_map<database_name, sbk_id> m_nameToIdMap;    //< Main runtime map to lookup IDs from names. In the editor, this can be a little stale, but the editor is generally working off of IDs anyway
    };
}  // namespace sbk::core
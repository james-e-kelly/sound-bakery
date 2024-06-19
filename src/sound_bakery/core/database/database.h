#pragma once

#include "sound_bakery/core/database/database_object.h"
#include "sound_bakery/core/object/object_tracker.h"

namespace sbk::core
{
    /**
     * @brief Runtime lookup of objects, using their ID or name.
     *
     * The database holds weak pointers to objects.
     *
     * Another object should own the shared_ptr objects to the database_object.
     */
    class SB_CLASS database : protected object_tracker
    {
    public:
        void add_object_to_database(const std::shared_ptr<database_object>& object);
        void remove_object_from_database(sbk_id objectID);

        [[nodiscard]] std::weak_ptr<database_object> try_find(sbk_id objectID) const;
        [[nodiscard]] std::weak_ptr<database_object> try_find(std::string_view name) const;

        [[nodiscard]] std::vector<std::weak_ptr<database_object>> get_all() const;
        using object_tracker::getObjectsOfCategory;
        using object_tracker::getObjectsOfType;

        void clear_database() noexcept;

    private:
        static sbk_id create_new_id();

        void update_id(sbk_id oldID, sbk_id newID);
        void update_name(std::string_view oldName, std::string_view newName);
        void on_object_destroyed(object* object);

        std::unordered_map<sbk_id, std::weak_ptr<database_object>> m_idToPointerMap;
        std::unordered_map<std::string, sbk_id> m_nameToIdMap;
    };
}  // namespace sbk::core
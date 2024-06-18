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
        void add_or_update_id(sbk_id oldID, database_object* object, sbk_id newID);
        void add_or_update_name(std::string_view oldName, database_object* object, std::string_view newName);

        void remove(sbk_id objectID);
        void remove(database_object* object);

        [[nodiscard]] std::weak_ptr<database_object> try_find(sbk_id objectID) const;
        [[nodiscard]] std::weak_ptr<database_object> try_find(std::string_view name) const;

        [[nodiscard]] std::vector<std::weak_ptr<database_object>> get_all() const;

        void clear() noexcept;

    private:
        static sbk_id create_new_id();

        std::unordered_map<sbk_id, std::weak_ptr<database_object>> m_idToPointerMap;
        std::unordered_map<std::string, sbk_id> m_nameToIdMap;
    };
}  // namespace sbk::core
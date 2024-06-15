#pragma once

#include "sound_bakery/core/database/database_object.h"

namespace sbk::core
{
    /**
     * @brief Runtime lookup of objects, using their ID or name
     */
    class SB_CLASS database
    {
    public:
        void add_or_update_id(sbk_id oldID, sbk_id newID, database_object* object)
        {
            assert(object != nullptr && "object was nullptr");

            if (newID == SB_INVALID_ID)
            {
                newID              = create_new_id();
                object->m_objectID = newID;  // privately set the ID without recursion
            }

            assert(newID != 0 && "New ID cannot be 0!");

            if (auto iter = m_idToPointerMap.find(oldID); iter != m_idToPointerMap.end())
            {
                assert(iter->second.get() == object && "Provided object does not match the one in the database");

                m_idToPointerMap.erase(iter);
            }

            m_idToPointerMap[newID].reset(object);
        }

        void add_or_update_name(std::string_view oldName, std::string_view newName, database_object* object)
        {
            assert(object != nullptr && "object was nullptr");

            if (!oldName.empty())
            {
                if (auto iter = m_nameToIdMap.find(oldName.data()); iter != m_nameToIdMap.end())
                {
                    m_nameToIdMap.erase(iter);
                }
            }

            if (!newName.empty())
            {
                m_nameToIdMap.emplace(newName, object->get_database_id());
            }
        }

        void remove(sbk_id id)
        {
            if (id)
            {
                if (m_idToPointerMap.contains(id))
                {
                    m_idToPointerMap.erase(id);
                }
            }
        }

        void remove(database_object* object)
        {
            assert(object != nullptr && "object was nullptr");

            if (object)
            {
                m_nameToIdMap.erase(object->get_database_name().data());
                m_idToPointerMap.erase(object->get_database_id());
            }
        }

        /**
         * @brief Removes the entry and gives ownership to the caller
         * @param object
         */
        std::shared_ptr<database_object> remove_unsafe(database_object* object)
        {
            assert(object != nullptr);

            std::shared_ptr<database_object> result;

            m_nameToIdMap.erase(object->get_database_name().data());
            m_idToPointerMap[object->get_database_id()].swap(result);
            m_idToPointerMap.erase(object->get_database_id());

            return result;
        }

        database_object* try_find(sbk_id id) const
        {
            database_object* result = nullptr;

            if (auto iter = m_idToPointerMap.find(id); iter != m_idToPointerMap.end())
            {
                result = iter->second.get();
            }

            return result;
        }

        database_object* try_find(std::string_view name) const
        {
            database_object* result = nullptr;

            if (auto iter = m_nameToIdMap.find(name.data()); iter != m_nameToIdMap.end())
            {
                result = try_find(iter->second);
            }

            return result;
        }

        std::weak_ptr<database_object> try_find_weak(sbk_id id) const
        {
            std::weak_ptr<database_object> result;

            if (auto iter = m_idToPointerMap.find(id); iter != m_idToPointerMap.end())
            {
                result = iter->second;
            }

            return result;
        }

        std::weak_ptr<database_object> try_find_weak(std::string_view name) const
        {
            std::weak_ptr<database_object> result;

            if (auto iter = m_nameToIdMap.find(name.data()); iter != m_nameToIdMap.end())
            {
                result = try_find_weak(iter->second);
            }

            return result;
        }

        std::vector<std::weak_ptr<database_object>> get_all() const
        {
            std::vector<std::weak_ptr<database_object>> result;
            result.reserve(m_idToPointerMap.size());

            for (auto& i : m_idToPointerMap)
            {
                result.push_back(i.second);
            }

            return result;
        }

        void clear() noexcept
        {
            m_idToPointerMap.clear();
            m_nameToIdMap.clear();
        }

    private:
        static sbk_id create_new_id();

        std::unordered_map<sbk_id, std::shared_ptr<database_object>> m_idToPointerMap;
        std::unordered_map<std::string, sbk_id> m_nameToIdMap;
    };
}  // namespace SB::Core
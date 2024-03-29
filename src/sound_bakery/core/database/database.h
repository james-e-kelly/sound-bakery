#pragma once

#include "sound_bakery/core/database/database_object.h"

namespace SB::Core
{
    /**
     * @brief Runtime lookup of objects, using their ID or name
     */
    class SB_CLASS Database
    {
    public:
        void addOrUpdateID(SB_ID oldID, SB_ID newID, DatabaseObject* object)
        {
            assert(object != nullptr && "Object was nullptr");

            if (newID == SB_INVALID_ID)
            {
                newID              = createNewID();
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

        void addOrUpdateName(std::string_view oldName, std::string_view newName, DatabaseObject* object)
        {
            assert(object != nullptr && "Object was nullptr");

            if (!oldName.empty())
            {
                if (auto iter = m_nameToIdMap.find(oldName.data()); iter != m_nameToIdMap.end())
                {
                    m_nameToIdMap.erase(iter);
                }
            }

            if (!newName.empty())
            {
                m_nameToIdMap.emplace(newName, object->getDatabaseID());
            }
        }

        void remove(SB_ID id)
        {
            if (id)
            {
                if (m_idToPointerMap.contains(id))
                {
                    m_idToPointerMap.erase(id);
                }
            }
        }

        void remove(DatabaseObject* object)
        {
            assert(object != nullptr && "Object was nullptr");

            if (object)
            {
                m_nameToIdMap.erase(object->getDatabaseName().data());
                m_idToPointerMap.erase(object->getDatabaseID());
            }
        }

        /**
         * @brief Removes the entry and gives ownership to the caller
         * @param object
         */
        std::shared_ptr<DatabaseObject> removeUnsafe(DatabaseObject* object)
        {
            assert(object != nullptr);

            std::shared_ptr<DatabaseObject> result;

            m_nameToIdMap.erase(object->getDatabaseName().data());
            m_idToPointerMap[object->getDatabaseID()].swap(result);
            m_idToPointerMap.erase(object->getDatabaseID());

            return result;
        }

        DatabaseObject* tryFind(SB_ID id) const
        {
            DatabaseObject* result = nullptr;

            if (auto iter = m_idToPointerMap.find(id); iter != m_idToPointerMap.end())
            {
                result = iter->second.get();
            }

            return result;
        }

        DatabaseObject* tryFind(std::string_view name) const
        {
            DatabaseObject* result = nullptr;

            if (auto iter = m_nameToIdMap.find(name.data()); iter != m_nameToIdMap.end())
            {
                result = tryFind(iter->second);
            }

            return result;
        }

        std::weak_ptr<DatabaseObject> tryFindWeak(SB_ID id) const
        {
            std::weak_ptr<DatabaseObject> result;

            if (auto iter = m_idToPointerMap.find(id); iter != m_idToPointerMap.end())
            {
                result = iter->second;
            }

            return result;
        }

        std::weak_ptr<DatabaseObject> tryFindWeak(std::string_view name) const
        {
            std::weak_ptr<DatabaseObject> result;

            if (auto iter = m_nameToIdMap.find(name.data()); iter != m_nameToIdMap.end())
            {
                result = tryFindWeak(iter->second);
            }

            return result;
        }

        std::vector<std::weak_ptr<DatabaseObject>> getAll() const
        {
            std::vector<std::weak_ptr<DatabaseObject>> result;
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
        static SB_ID createNewID();

    private:
        std::unordered_map<SB_ID, std::shared_ptr<DatabaseObject>> m_idToPointerMap;
        std::unordered_map<std::string, SB_ID> m_nameToIdMap;
    };
}  // namespace SB::Core
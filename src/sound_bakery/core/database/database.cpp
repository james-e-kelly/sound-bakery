#include "database.h"

void sbk::core::database::add_or_update_id(sbk_id oldID, database_object* object, sbk_id newID)
{
    if (object != nullptr)
    {
        if (newID == SB_INVALID_ID)
        {
            newID              = create_new_id();
            object->m_objectID = newID;  // privately set the ID without recursion
        }

        if (auto iter = m_idToPointerMap.find(oldID); iter != m_idToPointerMap.end())
        {
            m_idToPointerMap.erase(iter);
        }

        m_idToPointerMap[newID] = object;
    }
}

void sbk::core::database::add_or_update_name(std::string_view oldName,
                                             database_object* object,
                                             std::string_view newName)
{
    if (object != nullptr)
    {
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
}

void sbk::core::database::remove(sbk_id objectID)
{
    if (objectID != SB_INVALID_ID)
    {
        if (m_idToPointerMap.contains(objectID))
        {
            m_idToPointerMap.erase(objectID);
        }
    }
}

void sbk::core::database::remove(database_object* object)
{
    if (object != nullptr)
    {
        m_nameToIdMap.erase(object->get_database_name().data());
        m_idToPointerMap.erase(object->get_database_id());
    }
}

std::weak_ptr<sbk::core::database_object> sbk::core::database::try_find(sbk_id objectID) const
{
    std::weak_ptr<sbk::core::database_object> result;

    if (auto iter = m_idToPointerMap.find(objectID); iter != m_idToPointerMap.end())
    {
        result = iter->second;
    }

    return result;
}

std::weak_ptr<sbk::core::database_object> sbk::core::database::try_find(std::string_view name) const
{
    std::weak_ptr<sbk::core::database_object> result;

    if (auto iter = m_nameToIdMap.find(name.data()); iter != m_nameToIdMap.end())
    {
        result = try_find(iter->second);
    }

    return result;
}

std::vector<std::weak_ptr<sbk::core::database_object>> sbk::core::database::get_all() const
{
    std::vector<std::weak_ptr<sbk::core::database_object>> result;
    result.reserve(m_idToPointerMap.size());

    for (const auto& i : m_idToPointerMap)
    {
        result.push_back(i.second);
    }

    return result;
}

void sbk::core::database::clear() noexcept
{
    m_idToPointerMap.clear();
    m_nameToIdMap.clear();
}

sbk_id sbk::core::database::create_new_id()
{
    static std::random_device s_randomDevice;
    static std::mt19937_64 s_engine(s_randomDevice());
    static std::uniform_int_distribution<sbk_id> s_uniformDistribution;

    return s_uniformDistribution(s_engine);
}

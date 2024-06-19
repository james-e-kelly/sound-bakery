#include "database.h"

void sbk::core::database::add_object_to_database(const std::shared_ptr<database_object>& object)
{
    if (!object)
    {
        SPDLOG_ERROR("Cannot add object to database. Object was null");
        return;
    }

    sbk_id objectID              = object->get_database_id();
    const std::string objectName = std::string(object->get_database_name());

    if (objectID == SB_INVALID_ID)
    {
        objectID           = create_new_id();
        object->m_objectID = objectID;  // calling the function would trigger the callbacks
    }

    if (auto iter = m_idToPointerMap.find(objectID); iter != m_idToPointerMap.end())
    {
        SPDLOG_ERROR("Cannot add object to database. Object ID already mapped");
        return;
    }

    if (auto iter = m_nameToIdMap.find(objectName); iter != m_nameToIdMap.end())
    {
        SPDLOG_ERROR("Cannot add object to database. Object name already mapped");
        return;
    }

    m_idToPointerMap[objectID] = object;
    m_nameToIdMap[objectName]  = objectID;

    object->get_on_destroy().AddRaw(this, &sbk::core::database::on_object_destroyed);
    object->get_on_update_id().AddRaw(this, &sbk::core::database::update_id);
    object->get_on_update_name().AddRaw(this, &sbk::core::database::update_name);
}

void sbk::core::database::remove_object_from_database(sbk_id objectID)
{
    if (objectID == SB_INVALID_ID)
    {
        SPDLOG_ERROR("Cannot remove object from database. Object ID is 0!");
        return;
    }

    if (const auto idIter = m_idToPointerMap.find(objectID); idIter != m_idToPointerMap.end())
    {
        if (const std::shared_ptr<sbk::core::database_object> object = idIter->second.lock())
        {
            if (auto nameIter = m_nameToIdMap.find(std::string(object->get_database_name()));
                nameIter != m_nameToIdMap.end())
            {
                m_nameToIdMap.erase(nameIter);
            }

            object->get_on_destroy().RemoveObject(this);
            object->get_on_update_id().RemoveObject(this);
            object->get_on_update_name().RemoveObject(this);
        }

        m_idToPointerMap.erase(idIter);
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

void sbk::core::database::clear_database() noexcept
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

void sbk::core::database::update_id(sbk_id oldID, sbk_id newID)
{
    if (oldID == SB_INVALID_ID)
    {
        SPDLOG_ERROR("Cannot update database ID. Old ID is invalid");
        return;
    }

    if (newID == SB_INVALID_ID)
    {
        SPDLOG_ERROR("Cannot update database ID. New ID is invalid");
        return;
    }

    if (const auto iter = m_idToPointerMap.find(oldID); iter != m_idToPointerMap.end())
    {
        const std::weak_ptr<sbk::core::database_object> object = iter->second;

        m_idToPointerMap.erase(iter);
        m_idToPointerMap[newID] = object;
    }
}

void sbk::core::database::update_name(std::string_view oldName, std::string_view newName)
{
    if (oldName.empty())
    {
        SPDLOG_ERROR("Cannot update database name. Object name is invalid");
        return;
    }

    if (newName.empty())
    {
        SPDLOG_ERROR("Cannot update database name. New name is invalid");
        return;
    }

    if (auto iter = m_nameToIdMap.find(std::string(oldName)); iter != m_nameToIdMap.end())
    {
        m_nameToIdMap.erase(iter);
        m_nameToIdMap[std::string(newName)] = iter->second;
    }
}

void sbk::core::database::on_object_destroyed(object* object)
{
    if (object != nullptr)
    {
        if (database_object* databaseObject = object->try_convert_object<database_object>())
        {
            remove_object_from_database(databaseObject->get_database_id());
        }
    }
}
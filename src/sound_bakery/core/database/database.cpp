#include "database.h"

#include "sound_bakery/util/type_helper.h"

auto sbk::core::database::add_object_to_database(const std::shared_ptr<database_object>& object) -> void
{
    if (!object)
    {
        SPDLOG_ERROR("Cannot add object to database. Object was null");
        return;
    }

    sbk_id objectID        = object->get_database_id();
    std::string objectName = std::string(object->get_database_name());

    if (objectID == SB_INVALID_ID)
    {
        objectID           = create_new_id();
        object->m_objectID = objectID;  // calling the function would trigger the callbacks
    }

    if (objectName.empty())
    {
        objectName           = create_new_name(object->get_type());
        object->m_objectName = objectName;
    }

    if (auto iter = m_idToPointerMap.find(objectID); iter != m_idToPointerMap.end())
    {
        SPDLOG_ERROR("Cannot add object to database. Object ID already mapped");
        return;
    }

    if (auto iter = m_nameToIdMap.find(objectName); iter != m_nameToIdMap.end())
    {
        SPDLOG_ERROR("Cannot add object to database. Object name already mapped");
        m_idToPointerMap.erase(objectID);
        return;
    }

    m_idToPointerMap[objectID] = object;
    m_nameToIdMap[objectName]  = objectID;

    object->get_on_destroy().AddRaw(this, &sbk::core::database::on_object_destroyed);
    object->get_on_update_id().AddRaw(this, &sbk::core::database::update_id);
    object->get_on_update_name().AddRaw(this, &sbk::core::database::update_name);
}

auto sbk::core::database::remove_object_from_database(sbk_id objectID) -> void
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

auto sbk::core::database::try_find(sbk_id objectID) const -> std::weak_ptr<sbk::core::database_object>
{
    std::weak_ptr<sbk::core::database_object> result;

    if (auto iter = m_idToPointerMap.find(objectID); iter != m_idToPointerMap.end())
    {
        result = iter->second;
    }

    return result;
}

auto sbk::core::database::try_find(std::string_view name) const -> std::weak_ptr<sbk::core::database_object>
{
    std::weak_ptr<sbk::core::database_object> result;

    if (auto iter = m_nameToIdMap.find(name.data()); iter != m_nameToIdMap.end())
    {
        result = try_find(iter->second);
    }

    return result;
}

auto sbk::core::database::get_all() const -> std::vector<std::weak_ptr<sbk::core::database_object>>
{
    std::vector<std::weak_ptr<sbk::core::database_object>> result;
    result.reserve(m_idToPointerMap.size());

    for (const auto& i : m_idToPointerMap)
    {
        result.push_back(i.second);
    }

    return result;
}

auto sbk::core::database::clear_database() noexcept -> void
{
    m_idToPointerMap.clear();
    m_nameToIdMap.clear();
}

auto sbk::core::database::create_new_id() -> sbk_id
{
    static std::random_device s_randomDevice;
    static std::mt19937_64 s_engine(s_randomDevice());
    static std::uniform_int_distribution<sbk_id> s_uniformDistribution;

    return s_uniformDistribution(s_engine);
}

auto sbk::core::database::create_new_name(const rttr::type& type) -> std::string
{
    static std::atomic<int> serialNumberGenerator = 0;

    const std::string typeName =
        type.is_valid() ? sbk::util::type_helper::get_display_name_from_type(type).data() : "Object";

    return fmt::format("{} {}", typeName, serialNumberGenerator.fetch_add(1));
}

auto sbk::core::database::update_id(sbk_id oldID, sbk_id newID) -> void
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

auto sbk::core::database::update_name(std::string_view oldName, std::string_view newName) -> void
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
        m_nameToIdMap[std::string(newName)] = iter->second;
        m_nameToIdMap.erase(iter);
    }
}

auto sbk::core::database::on_object_destroyed(object* object) -> void
{
    if (object != nullptr)
    {
        if (database_object* databaseObject = object->try_convert_object<database_object>())
        {
            remove_object_from_database(databaseObject->get_database_id());
        }
    }
}
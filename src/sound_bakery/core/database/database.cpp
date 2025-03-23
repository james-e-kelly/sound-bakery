#include "database.h"

#include "sound_bakery/util/type_helper.h"

auto sbk::core::database::add_object_to_database(const std::shared_ptr<database_object>& object) -> void
{
    if (!object)
    {
        SBK_ERROR("Cannot add object to database. Object was null");
        return;
    }

    sbk_id objectID        = object->get_database_id();
    database_name objectName = object->get_database_name();

    if (objectID == SBK_INVALID_ID)
    {
        objectID           = create_new_id();
        object->m_objectID = objectID;  // calling the function would trigger the callbacks so set directly
    }

    BOOST_ASSERT_MSG(objectName.valid(), "Database names must be valid");

    if (auto iter = m_idToPointerMap.find(objectID); iter != m_idToPointerMap.end())
    {
        SBK_ERROR("Cannot add object to database. Object ID already mapped");
        return;
    }

    std::unordered_map<database_name, sbk_id>::iterator iter = m_nameToIdMap.find(objectName);

    while (iter != m_nameToIdMap.end())
    {
        object->m_objectName = create_new_name(object->get_type());
        objectName           = object->get_database_name();
        iter = m_nameToIdMap.find(objectName);
    }

    m_idToPointerMap[objectID] = object;
    m_nameToIdMap[objectName]  = objectID;

    object->get_on_destroy().AddRaw(this, &sbk::core::database::on_object_destroyed);
    object->get_on_update_id().AddRaw(this, &sbk::core::database::update_id);
    object->get_on_update_database_name().AddRaw(this, &sbk::core::database::update_database_name);
}

auto sbk::core::database::add_object_to_database(sbk_id id, const database_name& name) -> void
{
    if (id == SBK_INVALID_ID)
    {
        SBK_ERROR("Cannot add invalid ID to database");
        return;
    }

    if (!name.valid())
    {
        SBK_ERROR("Cannot add invalid name to database");
        return;
    }

    if (auto iter = m_idToPointerMap.find(id); iter != m_idToPointerMap.end())
    {
        SBK_WARN("Cannot add object to database. Object ID already mapped");
        return;
    }

    if (auto iter = m_nameToIdMap.find(name); iter != m_nameToIdMap.end())
    {
        SBK_WARN("Cannot add object to database. Object name already mapped");
        m_idToPointerMap.erase(id);
        return;
    }

    m_nameToIdMap[name] = id;
}

auto sbk::core::database::remove_object_from_database(sbk_id objectID) -> void
{
    if (objectID == SBK_INVALID_ID)
    {
        SBK_ERROR("Cannot remove object from database. Object ID is 0!");
        return;
    }

    if (const auto idIter = m_idToPointerMap.find(objectID); idIter != m_idToPointerMap.end())
    {
        if (const std::shared_ptr<sbk::core::database_object> object = idIter->second.lock())
        {
            if (auto nameIter = m_nameToIdMap.find(object->get_database_name());
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

auto sbk::core::database::try_find(const database_name& name) const -> std::weak_ptr<sbk::core::database_object>
{
    std::weak_ptr<sbk::core::database_object> result;

    if (auto iter = m_nameToIdMap.find(name); iter != m_nameToIdMap.end())
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

    BOOST_ASSERT(type.is_valid());
    const std::string typeName =
        type.is_valid() ? sbk::util::type_helper::get_display_name_from_type(type).data() : "Object";

    return fmt::format("{}_{}", typeName, serialNumberGenerator.fetch_add(1));
}

auto sbk::core::database::update_id(sbk_id oldID, sbk_id newID) -> void
{
    if (oldID == SBK_INVALID_ID)
    {
        SBK_ERROR("Cannot update database ID. Old ID is invalid");
        return;
    }

    if (newID == SBK_INVALID_ID)
    {
        SBK_ERROR("Cannot update database ID. New ID is invalid");
        return;
    }

    if (const auto iter = m_idToPointerMap.find(oldID); iter != m_idToPointerMap.end())
    {
        const std::weak_ptr<sbk::core::database_object> object = iter->second;

        m_idToPointerMap.erase(iter);
        m_idToPointerMap[newID] = object;
    }
}

auto sbk::core::database::update_database_name(const database_name& oldName, const database_name& newName) -> void
{
    if (!oldName.valid())
    {
        SBK_ERROR("Cannot update database name. Object name is invalid");
        return;
    }

    if (!newName.valid())
    {
        SBK_ERROR("Cannot update database name. New name is invalid");
        return;
    }

    if (const auto iter = m_nameToIdMap.find(oldName); iter != m_nameToIdMap.end())
    {
        const sbk_id id = iter->second;
        m_nameToIdMap.erase(iter);
        m_nameToIdMap[newName] = id;
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
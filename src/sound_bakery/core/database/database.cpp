 #include "database.h"

#include "sound_bakery/util/type_helper.h"

auto sbk::core::database::add_object_to_database(const std::shared_ptr<database_object>& object) -> sbk::result<void>
{
    SBK_CHECK_MSG(object, SBK_ERR_INVALID_PARAMETER, "Cannot add a null object to the database");

    sbk_id objectID = object->get_database_id();

    if (objectID == SBK_INVALID_ID)
    {
        objectID           = create_new_id();
        object->m_objectID = objectID;  // calling the function would trigger the callbacks so set directly
    }

    SBK_TRYV(work_till_name_is_unique(object));

    auto idIter = m_idToPointerMap.find(objectID);
    SBK_CHECK_MSG(idIter == m_idToPointerMap.end(), SBK_ERR_BAKERY_OBJECT_EXISTS, "Adding an object to the database should only happen once. There is already an object with this ID");

    m_idToPointerMap[objectID]                  = object;
    m_nameToIdMap[object->get_database_name()]  = objectID;

    object->get_on_destroy().AddRaw(this, &sbk::core::database::on_object_destroyed);
    object->get_on_update_id().AddRaw(this, &sbk::core::database::update_id);
    object->get_on_update_database_name().AddRaw(this, &sbk::core::database::update_database_name);

    return sbk::ok();
}

auto sbk::core::database::assign_name_to_id(sbk_id id, const database_name& name) -> sbk::result<void>
{
    SBK_CHECK(id != SBK_INVALID_ID, SBK_ERR_INVALID_PARAMETER);
    SBK_CHECK(name.valid(), SBK_ERR_INVALID_PARAMETER);

    auto nameIter = m_nameToIdMap.find(name);
    SBK_CHECK_MSG(nameIter == m_nameToIdMap.end(), SBK_ERR_BAKERY_OBJECT_EXISTS, "Assigning a name to an ID should only happen once");

    m_nameToIdMap[name] = id;
    
    return sbk::ok();
}

auto sbk::core::database::remove_object_from_database(sbk_id objectID) -> sbk::result<void>
{
    SBK_CHECK(objectID != SBK_INVALID_ID, SBK_ERR_INVALID_PARAMETER);

    auto nameIter       = m_nameToIdMap.end();
    bool objectExpired  = false;

    if (const auto idIter = m_idToPointerMap.find(objectID); idIter != m_idToPointerMap.end())
    {
        if (const std::shared_ptr<sbk::core::database_object> object = idIter->second.lock())
        {
            nameIter = m_nameToIdMap.find(object->get_database_name());

            object->get_on_destroy().RemoveObject(this);
            object->get_on_update_id().RemoveObject(this);
            object->get_on_update_name().RemoveObject(this);
        }
        else
        {
            objectExpired = true;
        }

        m_idToPointerMap.erase(idIter);
    }

    if (nameIter != m_nameToIdMap.end())
    {
        m_nameToIdMap.erase(nameIter);
    }
    else if (objectExpired)
    {
        SBK_WARN("Could not find {} in the database. Doing slow iteration to ensure any names that point to {} are removed", static_cast<const char*>(objectName), objectID);

        // Removing objects is meant to happen before their destruction
        // It should be very unlikely any code removes an ID alone
        // However, in the rare chance it happens, do a slow search for the ID in the name map
        for (nameIter = m_nameToIdMap.begin(); nameIter != m_nameToIdMap.end();)
        {
            if (nameIter->second == objectID)
            {
                nameIter = m_nameToIdMap.erase(nameIter);
                break;
            }
            ++nameIter;
        }
    }

    return sbk::ok();
}

auto sbk::core::database::try_find_database_object(sbk_id objectID) const -> std::weak_ptr<sbk::core::database_object>
{
    std::weak_ptr<sbk::core::database_object> result;

    if (auto iter = m_idToPointerMap.find(objectID); iter != m_idToPointerMap.end())
    {
        result = iter->second;
    }

    return result;
}

auto sbk::core::database::try_find_database_object(const database_name& name) const -> std::weak_ptr<sbk::core::database_object>
{
    std::weak_ptr<sbk::core::database_object> result;

    if (auto iter = m_nameToIdMap.find(name); iter != m_nameToIdMap.end())
    {
        result = try_find_database_object(iter->second);
    }

    return result;
}

auto sbk::core::database::get_all_database_objects() const -> std::vector<std::weak_ptr<sbk::core::database_object>>
{
    std::vector<std::weak_ptr<sbk::core::database_object>> result;
    result.reserve(m_idToPointerMap.size());

    for (const auto& i : m_idToPointerMap)
    {
        result.push_back(i.second);
    }

    return result;
}

auto sbk::core::database::get_all_database_names() const -> std::vector<database_name>
{
    std::vector<database_name> result;
    result.reserve(m_nameToIdMap.size());

    for (const auto& iter : m_nameToIdMap)
    {
        result.push_back(iter.first);
    }

    return result;
}

auto sbk::core::database::get_database_object_count() const -> size_t
{
    return m_idToPointerMap.size();
}

auto sbk::core::database::get_database_object_at(size_t index) const -> std::weak_ptr<database_object>
{
    auto iter = m_idToPointerMap.cbegin();
    if (iter != m_idToPointerMap.cend() && index < m_idToPointerMap.size())
    {
        std::advance(iter, index);
    }
    return iter != m_idToPointerMap.cend() ? iter->second : std::weak_ptr<database_object>();
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
    const std::string typeName = type.is_valid() ? sbk::util::type_helper::get_display_name_from_type(type).data() : "Object";

    return fmt::format("{}_{}", typeName, serialNumberGenerator.fetch_add(1));
}

auto sbk::core::database::work_till_name_is_unique(const std::shared_ptr<database_object>& object) -> sbk::result<void>
{
    database_name objectName = object->get_database_name();

    SBK_CHECK_MSG(objectName.valid(), SBK_ERR_BAKERY, "Database names must be valid");

    auto iter = m_nameToIdMap.find(objectName);

    while (iter != m_nameToIdMap.end())
    {
        object->m_objectName = create_new_name(object->get_type());
        objectName           = object->get_database_name();
        iter                 = m_nameToIdMap.find(objectName);
    }

    return sbk::ok();
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
            (void)remove_object_from_database(databaseObject->get_database_id());
        }
    }
}
#include "database.h"

#include "sound_bakery/system.h"
#include "sound_bakery/util/type_helper.h"

auto sbk::core::database::add_object_to_database(std::shared_ptr<database_object> object) -> concurrencpp::result<void>
{
    if (!object)
    {
        SBK_ERROR("Cannot add object to database. Object was null");
        co_return;
    }

    co_await concurrencpp::resume_on(sbk::engine::system::get()->get_database_executor());

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
        co_return;
    }

    auto iter = m_nameToIdMap.find(objectName);

    while (iter != m_nameToIdMap.end())
    {
        object->m_objectName = create_new_name(object->get_type());
        objectName           = object->get_database_name();
        iter = m_nameToIdMap.find(objectName);
    }

    m_idToPointerMap[objectID] = object;
    m_nameToIdMap[objectName]  = objectID;

    SBK_INFO(fmt::format("Adding {} to database", objectName.databaseName).c_str());

    object->get_on_destroy().AddRaw(this, &sbk::core::database::on_object_destroyed);
    object->get_on_update_id().AddRaw(this, &sbk::core::database::update_id);
    object->get_on_update_database_name().AddRaw(this, &sbk::core::database::update_database_name);
}

auto sbk::core::database::add_object_to_database(sbk_id id, database_name name) -> concurrencpp::result<void>
{
    if (id == SBK_INVALID_ID)
    {
        SBK_ERROR("Cannot add invalid ID to database");
        co_return;
    }

    if (!name.valid())
    {
        SBK_ERROR("Cannot add invalid name to database");
        co_return;
    }

    co_await concurrencpp::resume_on(sbk::engine::system::get()->get_database_executor());

    if (const auto iter = m_idToPointerMap.find(id); iter != m_idToPointerMap.end())
    {
        SBK_WARN("Cannot add object to database. Object ID already mapped");
        co_return;
    }

    if (const auto iter = m_nameToIdMap.find(name); iter != m_nameToIdMap.end())
    {
        SBK_WARN("Cannot add object to database. Object name already mapped");
        m_idToPointerMap.erase(id);
        co_return;
    }

    m_nameToIdMap[name] = id;
}

auto sbk::core::database::remove_object_from_database(sbk_id objectID) -> concurrencpp::result<void>
{
    if (objectID == SBK_INVALID_ID)
    {
        SBK_ERROR("Cannot remove object from database. Object ID is 0!");
        co_return;
    }

    co_await concurrencpp::resume_on(sbk::engine::system::get()->get_database_executor());

    if (const auto idIter = m_idToPointerMap.find(objectID); idIter != m_idToPointerMap.end())
    {
        if (const std::shared_ptr<sbk::core::database_object> object = idIter->second.lock())
        {
            if (const auto nameIter = m_nameToIdMap.find(object->get_database_name());
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

auto sbk::core::database::try_find_database_object(sbk_id objectID) const -> concurrencpp::result<std::weak_ptr<sbk::core::database_object>>
{
    co_await concurrencpp::resume_on(sbk::engine::system::get()->get_database_executor());

    std::weak_ptr<sbk::core::database_object> result;

    if (const auto iter = m_idToPointerMap.find(objectID); iter != m_idToPointerMap.end())
    {
        result = iter->second;
    }

    co_return result;
}

auto sbk::core::database::try_find_database_object(database_name name) const -> concurrencpp::result<std::weak_ptr<sbk::core::database_object>>
{
    co_await concurrencpp::resume_on(sbk::engine::system::get()->get_database_executor());

    std::weak_ptr<sbk::core::database_object> result;

    if (const auto iter = m_nameToIdMap.find(name); iter != m_nameToIdMap.end())
    {
        result = co_await try_find_database_object(iter->second);
    }

    co_return result;
}

auto sbk::core::database::get_all_database_objects() const -> concurrencpp::result<std::vector<std::weak_ptr<sbk::core::database_object>>>
{
    co_await concurrencpp::resume_on(sbk::engine::system::get()->get_database_executor());

    std::vector<std::weak_ptr<sbk::core::database_object>> result;
    result.reserve(m_idToPointerMap.size());

    for (const auto& iter : m_idToPointerMap)
    {
        result.push_back(iter.second);
    }

    co_return result;
}

auto sbk::core::database::get_all_database_names() const -> concurrencpp::result<std::vector<database_name>>
{
    co_await concurrencpp::resume_on(sbk::engine::system::get()->get_database_executor());

    std::vector<database_name> result;
    result.reserve(m_nameToIdMap.size());

    for (const auto& key : m_nameToIdMap | std::views::keys)
    {
        result.push_back(key);
    }

    co_return result;
}

auto sbk::core::database::get_database_object_count() const -> concurrencpp::result<size_t>
{
    co_await concurrencpp::resume_on(sbk::engine::system::get()->get_database_executor());

    co_return std::max<std::size_t>(m_idToPointerMap.size(), m_nameToIdMap.size());
}

auto sbk::core::database::get_database_object_at(size_t index) const -> concurrencpp::result<std::weak_ptr<database_object>>
{
    co_await concurrencpp::resume_on(sbk::engine::system::get()->get_database_executor());

    auto iter = m_idToPointerMap.cbegin();
    if (iter != m_idToPointerMap.cend() && index < m_idToPointerMap.size())
    {
        std::advance(iter, index);
    }
    co_return iter != m_idToPointerMap.cend() ? iter->second : std::weak_ptr<database_object>();
}

auto sbk::core::database::clear_database() -> concurrencpp::result<void>
{
    co_await concurrencpp::resume_on(sbk::engine::system::get()->get_database_executor());

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
    static std::atomic<int> s_serialNumberGenerator = 0;

    BOOST_ASSERT(type.is_valid());
    const std::string typeName =
        type.is_valid() ? sbk::util::type_helper::get_display_name_from_type(type).data() : "Object";

    return fmt::format("{}_{}", typeName, s_serialNumberGenerator.fetch_add(1));
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

    sbk::engine::system::get()->get_database_executor()->submit([this, oldID, newID]() -> void
        {
            if (const auto iter = m_idToPointerMap.find(oldID); iter != m_idToPointerMap.end())
            {
                const std::weak_ptr<sbk::core::database_object>& object = iter->second;

                m_idToPointerMap.erase(iter);
                m_idToPointerMap[newID] = object;
            }
        });
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

    sbk::engine::system::get()->get_database_executor()->submit([this, oldName = oldName, newName = newName]() -> void
        {
            if (const auto iter = m_nameToIdMap.find(oldName); iter != m_nameToIdMap.end())
            {
                const sbk_id id = iter->second;
                m_nameToIdMap.erase(iter);
                m_nameToIdMap[newName] = id;

                SBK_INFO(fmt::format("Renamed {} to {}", oldName.databaseName, newName.databaseName).c_str());
            }
        });
}

auto sbk::core::database::on_object_destroyed(object* object) -> void
{
    if (object != nullptr)
    {
        if (const database_object* const databaseObject = object->try_convert_object<database_object>())
        {
            sbk::engine::system::get()->get_database_executor()->submit([this, objectID = databaseObject->get_database_id()]() -> void
                {
                    remove_object_from_database(objectID);
                });
        }
    }
}
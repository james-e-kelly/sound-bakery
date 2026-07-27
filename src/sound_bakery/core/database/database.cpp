#include "database.h"

#include "sound_bakery/core/thread_domain.h"
#include "sound_bakery/util/type_helper.h"

auto sbk::core::database::add_object_to_database(const std::shared_ptr<database_object>& object) -> sbk::result<void>
{
    SBK_EXPECT_STUDIO_THREAD();
    SBK_CHECK_MSG(object, SBK_ERR_INVALID_PARAMETER, "Cannot add a null object to the database");

    sbk_id objectID = object->get_database_id();

    if (objectID == SBK_INVALID_ID)
    {
        objectID           = create_new_id();
        object->m_objectID = objectID;  // calling the function would trigger the callbacks so set directly
    }

    const auto idIter = m_idToPointerMap.find(objectID);
    SBK_CHECK_MSG(idIter == m_idToPointerMap.end(), SBK_ERR_BAKERY_OBJECT_EXISTS, "Adding an object to the database should only happen once. There is already an object with this ID");

    if (object->has_flag(object_flags::default_name))
    {
        object->set_object_name(create_new_name(object->get_type()));
    }

    m_idToPointerMap[objectID] = object;

    object->get_on_update_id().AddRaw(this, &sbk::core::database::update_id);

    return sbk::ok();
}

auto sbk::core::database::assign_name_to_id(sbk_id id, const database_name& name) -> sbk::result<void>
{
    SBK_EXPECT_STUDIO_THREAD();
    SBK_CHECK(id != SBK_INVALID_ID, SBK_ERR_INVALID_PARAMETER);
    SBK_CHECK(name.valid(), SBK_ERR_INVALID_PARAMETER);

    auto nameIter = m_nameToIdMap.find(name);
    SBK_CHECK_MSG(nameIter == m_nameToIdMap.end(), SBK_ERR_BAKERY_OBJECT_EXISTS, "Assigning a name to an ID should only happen once");

    m_nameToIdMap[name] = id;

    return sbk::ok();
}

auto sbk::core::database::remove_object_from_database(sbk_id objectID) -> sbk::result<void>
{
    SBK_EXPECT_STUDIO_THREAD();
    SBK_CHECK(objectID != SBK_INVALID_ID, SBK_ERR_INVALID_PARAMETER);

    if (const auto idIter = m_idToPointerMap.find(objectID); idIter != m_idToPointerMap.end())
    {
        if (const std::shared_ptr<sbk::core::database_object> object = idIter->second.lock())
        {
            object->get_on_update_id().RemoveObject(this);
        }

        m_idToPointerMap.erase(idIter);
    }

    // m_nameToIdMap is a decoupled bank artifact (see header) - not touched per-object here.
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
    if (const auto iter = m_nameToIdMap.find(name); iter != m_nameToIdMap.end())
    {
        return try_find_database_object(iter->second);
    }

    // Editor fallback
    return resolve_name_in_graph(name);
}

auto sbk::core::database::resolve_name_in_graph(const database_name& name) const -> std::weak_ptr<sbk::core::database_object>
{
    for (const auto& [id, weakObject] : m_idToPointerMap)
    {
        if (const std::shared_ptr<sbk::core::database_object> object = weakObject.lock())
        {
            if (object->get_database_name() == name)
            {
                return object;
            }
        }
    }

    return {};
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
    // Names are derived, so build the list from the live objects rather than the (bank-only) index.
    std::vector<database_name> result;
    result.reserve(m_idToPointerMap.size());

    for (const auto& [id, weakObject] : m_idToPointerMap)
    {
        if (const std::shared_ptr<sbk::core::database_object> object = weakObject.lock())
        {
            result.push_back(object->get_database_name());
        }
    }

    return result;
}

auto sbk::core::database::get_database_object_name(sbk_id objectID) const -> database_name
{
    if (const auto iter = m_idToPointerMap.find(objectID); iter != m_idToPointerMap.end())
    {
        if (const std::shared_ptr<sbk::core::database_object> object = iter->second.lock())
        {
            return object->get_database_name();
        }
    }

    return database_name{};
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
    SBK_EXPECT_STUDIO_THREAD();
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

auto sbk::core::database::on_object_destroyed(object* object) -> void
{
    if (object != nullptr)
    {
        if (auto* databaseObject = object->try_convert_object<database_object>())
        {
            (void)remove_object_from_database(databaseObject->get_database_id());
        }
    }
}
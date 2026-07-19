#include "database_object.h"

#include "sound_bakery/core/database/database.h"
#include "sound_bakery/editor/editor_defines.h"
#include "sound_bakery/node/node.h"
#include "sound_bakery/system.h"

DEFINE_REFLECTION(sbk::core::database_object)

auto sbk::core::database_name_comparator::operator()(const database_name& lhs, const database_name& rhs) const -> bool
{
    return std::strcmp(lhs, rhs) < 0;
}

sbk::core::parsed_database_name::parsed_database_name(const database_name* databaseName)
{
    if (databaseName)
    {
        std::vector<std::string> typeSplit;
        boost::algorithm::split_regex(typeSplit, databaseName->databaseName, boost::regex(":/"));

        BOOST_ASSERT_MSG(
            typeSplit.size() == 2,
            "After parsing the string, we should be left with the type and then the remaining path and name");

        if (typeSplit.size() == 2)
        {
            objectType = typeSplit[0];

            const std::string pathAndName      = typeSplit[1];
            const std::size_t lastForwardSlash = pathAndName.find_last_of('/');

            if (lastForwardSlash != std::string::npos)
            {
                objectPath = pathAndName.substr(0, lastForwardSlash);
                objectName = pathAndName.substr(lastForwardSlash, pathAndName.size() - lastForwardSlash);
            }
        }
    }
}

auto sbk::core::database_name::parse() const -> parsed_database_name
{
    return parsed_database_name(this);
}

auto sbk::core::database_name::valid() const -> bool { return !databaseName.empty(); }

sbk::core::database_object::database_object()
{
    m_onUpdateNameHandle = get_on_update_name().AddRaw(this, &database_object::on_update_name);
}

sbk::core::database_object::~database_object() {
    get_on_update_name().Remove(m_onUpdateNameHandle);
}

auto sbk::core::database_object::get_database_id() const -> sbk_id { return m_objectID; }

auto sbk::core::database_object::get_asset_name() const -> std::string
{
    return std::to_string(get_database_id());
}

auto sbk::core::database_object::get_database_path(std::string& path) const -> void
{
    if (const sbk::engine::node_base* const nodeThis = try_convert_object<sbk::engine::node>())
    {
        if (const auto parent = nodeThis->get_parent())
        {
            parent->get_database_path(path);
        }
    }
    path.append(fmt::format("/{}", get_object_name()));
}

auto sbk::core::database_object::get_database_name() const -> database_name
{ 
    std::string path;
    get_database_path(path);

    return database_name(fmt::format("{}:{}", get_object_type().get_name().data(), path));
}

auto sbk::core::database_object::set_database_id(sbk_id id) -> void
{
    //BOOST_ASSERT_MSG(m_objectID == 0, "Shouldn't update an object's ID at runtime");

    if (id != 0)
    {
        m_onUpdateID.Broadcast(m_objectID, id);
        m_objectID = id;
    }
}

auto sbk::core::database_object::get_editor_hidden() const -> bool { return editorHidden; }

auto sbk::core::database_object::get_is_export() const -> bool
{
    return !has_flag(object_flags::loading);
}

auto sbk::core::database_object::set_editor_hidden(bool hidden) -> void { editorHidden = hidden; }

auto sbk::core::database_object::get_on_update_id() -> MulticastDelegate<sbk_id, sbk_id>& { return m_onUpdateID; }

auto sbk::core::database_object::get_on_update_database_name() -> update_database_name_delegate& { return m_onUpdateDatabaseName; }

namespace
{
    struct cached_synced_property
    {
        std::uint32_t propertyID;
        rttr::property property;
    };

    /**
     * @brief Synced properties per rttr type, discovered once.
     *
     * Metadata filtering and name hashing are too slow to repeat per object
     * per poll at 50k+ objects. Game-thread only, like the database.
     */
    auto get_synced_properties_for_type(const rttr::type& type) -> const std::vector<cached_synced_property>&
    {
        static std::unordered_map<rttr::type::type_id, std::vector<cached_synced_property>> cache;

        const auto found = cache.find(type.get_id());

        if (found != cache.end())
        {
            return found->second;
        }

        std::vector<cached_synced_property>& properties = cache[type.get_id()];

        for (const rttr::property& property : type.get_properties())
        {
            if (!property.get_metadata(sbk::editor::metadata_key::synced).to_bool())
            {
                continue;
            }

            const rttr::string_view name = property.get_name();
            properties.push_back(
                {sbk::core::synced_property_id(std::string_view(name.data(), name.size())), property});
        }

        return properties;
    }
}  // namespace

auto sbk::core::database_object::get_synced_property_values(std::vector<synced_property_value>& outValues) -> void
{
    for (const cached_synced_property& cached : get_synced_properties_for_type(get_object_type()))
    {
        const rttr::variant value = cached.property.get_value(*this);

        if (value.is_type<float_property>())
        {
            outValues.push_back({cached.propertyID, value.get_value<float_property>().get()});
        }
    }
}

auto sbk::core::database_object::set_synced_property(const std::uint32_t propertyID, const float value) -> bool
{
    for (const cached_synced_property& cached : get_synced_properties_for_type(get_object_type()))
    {
        if (cached.propertyID != propertyID)
        {
            continue;
        }

        const rttr::variant variant = cached.property.get_value(*this);

        if (!variant.is_type<float_property>())
        {
            return false;
        }

        // Same flow as the editor's property drawer: copy out, set() so range
        // clamping and change delegates behave normally, write back.
        float_property floatProperty = variant.get_value<float_property>();

        if (!floatProperty.set(value))
        {
            return false;
        }

        return cached.property.set_value(*this, floatProperty);
    }

    return false;
}

auto sbk::core::database_object::on_update_name(std::string_view oldName, std::string_view newName) -> void
{
    const database_name oldDatabaseName = get_database_name();
    
    parsed_database_name parsedDatabaseName = oldDatabaseName.parse();
    parsedDatabaseName.objectName           = newName;

    const database_name newDatabaseName = database_name(parsedDatabaseName);

    m_onUpdateDatabaseName.Broadcast(oldDatabaseName, newDatabaseName);
}
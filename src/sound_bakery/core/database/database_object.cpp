#include "database_object.h"

#include "sound_bakery/core/database/database.h"
#include "sound_bakery/node/node.h"
#include "sound_bakery/system.h"

DEFINE_REFLECTION(sbk::core::database_object)

bool sbk::core::database_name_comparator::operator()(const database_name& lhs, const database_name& rhs) const
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
        if (const sbk::engine::node_base* const parent = nodeThis->get_parent())
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

auto sbk::core::database_object::set_editor_hidden(bool hidden) -> void { editorHidden = hidden; }

auto sbk::core::database_object::get_on_update_id() -> MulticastDelegate<sbk_id, sbk_id>& { return m_onUpdateID; }

auto sbk::core::database_object::get_on_update_database_name() -> update_database_name_delegate& { return m_onUpdateDatabaseName; }

auto sbk::core::database_object::on_update_name(std::string_view oldName, std::string_view newName) -> void
{
    const database_name oldDatabaseName = get_database_name();
    
    parsed_database_name parsedDatabaseName = oldDatabaseName.parse();
    parsedDatabaseName.objectName           = newName;

    const database_name newDatabaseName = database_name(parsedDatabaseName);

    m_onUpdateDatabaseName.Broadcast(oldDatabaseName, newDatabaseName);
}
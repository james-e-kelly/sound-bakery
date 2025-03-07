#include "database_object.h"

#include "sound_bakery/core/database/database.h"
#include "sound_bakery/system.h"

using namespace sbk::core;

DEFINE_REFLECTION(sbk::core::database_object)

auto sbk::core::database_object::get_database_id() const -> sbk_id { return m_objectID; }

auto sbk::core::database_object::get_database_name() const -> std::string_view { return m_objectName; }

auto sbk::core::database_object::set_database_id(sbk_id id) -> void
{
    //BOOST_ASSERT_MSG(m_objectID == 0, "Shouldn't update an object's ID at runtime");

    if (id != 0)
    {
        m_onUpdateID.Broadcast(m_objectID, id);
        m_objectID = id;
    }
}

auto sbk::core::database_object::set_database_name(std::string_view name) -> void
{
    BOOST_ASSERT_MSG(name.length() > 0, "Database names must have a length");
    BOOST_ASSERT_MSG(name[0] != '\0', "Names can't start with closing tags");
    m_onUpdateName.Broadcast(m_objectName, name);
    m_objectName = name;
}

auto sbk::core::database_object::get_editor_hidden() const -> bool { return editorHidden; }

auto sbk::core::database_object::set_editor_hidden(bool hidden) -> void { editorHidden = hidden; }

auto sbk::core::database_object::get_on_update_id() -> MulticastDelegate<sbk_id, sbk_id>& { return m_onUpdateID; }

auto sbk::core::database_object::get_on_update_name() -> MulticastDelegate<std::string_view, std::string_view>&
{
    return m_onUpdateName;
}
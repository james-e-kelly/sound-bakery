#include "database_object.h"

#include "sound_bakery/core/database/database.h"
#include "sound_bakery/system.h"

using namespace sbk::core;

DEFINE_REFLECTION(sbk::core::database_object)

sbk_id sbk::core::database_object::get_database_id() const { return m_objectID; }

std::string_view sbk::core::database_object::get_database_name() const { return m_objectName; }

void sbk::core::database_object::set_database_id(sbk_id id)
{
    assert(m_objectID == 0 && "Shouldn't update an object's ID at runtime");

    if (id != 0)
    {
        m_onUpdateID.Broadcast(m_objectID, id);
        m_objectID = id;
    }
}

void sbk::core::database_object::set_database_name(std::string_view name)
{
    m_onUpdateName.Broadcast(m_objectName, name);
    m_objectName = name;
}

MulticastDelegate<sbk_id, sbk_id>& sbk::core::database_object::get_on_update_id() { return m_onUpdateID; }

MulticastDelegate<std::string_view, std::string_view>& sbk::core::database_object::get_on_update_name()
{
    return m_onUpdateName;
}